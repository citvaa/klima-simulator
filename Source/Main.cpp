#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include "../Header/Renderer2D.h"
#include "../Header/State.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

namespace
{
    constexpr float kDigitWidth = 18.0f;
    constexpr float kDigitHeight = 32.0f;
    constexpr float kDigitThickness = 3.0f;
    constexpr float kDigitSpacing = 6.0f;

    // Segment order: A, B, C, D, E, F, G
    const uint8_t kDigitMasks[10] = {
        0b00111111, // 0
        0b00000110, // 1
        0b01011011, // 2
        0b01001111, // 3
        0b01100110, // 4
        0b01101101, // 5
        0b01111101, // 6
        0b00000111, // 7
        0b01111111, // 8
        0b01101111  // 9
    };

    void drawDigit(Renderer2D& renderer, float x, float y, int digit, const Color& color)
    {
        if (digit < 0 || digit > 9) return;

        struct Segment { float x, y, w, h; };
        static const Segment segments[7] = {
            { kDigitThickness, 0.0f, kDigitWidth - 2.0f * kDigitThickness, kDigitThickness }, // A
            { kDigitWidth - kDigitThickness, kDigitThickness, kDigitThickness, kDigitHeight * 0.5f - kDigitThickness }, // B
            { kDigitWidth - kDigitThickness, kDigitHeight * 0.5f, kDigitThickness, kDigitHeight * 0.5f - kDigitThickness }, // C
            { kDigitThickness, kDigitHeight - kDigitThickness, kDigitWidth - 2.0f * kDigitThickness, kDigitThickness }, // D
            { 0.0f, kDigitHeight * 0.5f, kDigitThickness, kDigitHeight * 0.5f - kDigitThickness }, // E
            { 0.0f, kDigitThickness, kDigitThickness, kDigitHeight * 0.5f - kDigitThickness }, // F
            { kDigitThickness, kDigitHeight * 0.5f - kDigitThickness * 0.5f, kDigitWidth - 2.0f * kDigitThickness, kDigitThickness } // G
        };

        uint8_t mask = kDigitMasks[digit];
        for (int i = 0; i < 7; ++i)
        {
            if (mask & (1 << i))
            {
                const auto& s = segments[i];
                renderer.drawRect(x + s.x, y + s.y, s.w, s.h, color);
            }
        }
    }

    void drawMinus(Renderer2D& renderer, float x, float y, const Color& color)
    {
        float lineY = y + (kDigitHeight * 0.5f - kDigitThickness * 0.5f);
        renderer.drawRect(x + kDigitThickness, lineY, kDigitWidth - 2.0f * kDigitThickness, kDigitThickness, color);
    }

    void drawTemperatureValue(Renderer2D& renderer, float value, const RectShape& screen, const Color& color)
    {
        int rounded = static_cast<int>(std::round(value));
        rounded = std::clamp(rounded, -99, 99);
        bool negative = rounded < 0;
        int absVal = std::abs(rounded);
        int tens = absVal / 10;
        int ones = absVal % 10;
        bool hasTens = absVal >= 10;

        int digitCount = hasTens ? 2 : 1;
        float totalWidth = digitCount * kDigitWidth + (digitCount - 1) * kDigitSpacing;
        if (negative)
        {
            totalWidth += kDigitWidth + kDigitSpacing;
        }

        float startX = screen.x + (screen.w - totalWidth) * 0.5f;
        float startY = screen.y + (screen.h - kDigitHeight) * 0.5f;
        float cursorX = startX;

        if (negative)
        {
            drawMinus(renderer, cursorX, startY, color);
            cursorX += kDigitWidth + kDigitSpacing;
        }

        if (hasTens)
        {
            drawDigit(renderer, cursorX, startY, tens, color);
            cursorX += kDigitWidth + kDigitSpacing;
        }

        drawDigit(renderer, cursorX, startY, ones, color);
    }

    void drawHeatIcon(Renderer2D& renderer, float cx, float cy, float radius, const Color& outer, const Color& inner)
    {
        renderer.drawCircle(cx, cy + radius * 0.25f, radius, outer);
        renderer.drawCircle(cx, cy, radius * 0.65f, inner);
        renderer.drawRect(cx - radius * 0.25f, cy + radius * 0.4f, radius * 0.5f, radius * 0.5f, outer);
    }

    void drawSnowIcon(Renderer2D& renderer, float cx, float cy, float size, const Color& color)
    {
        float arm = size * 0.45f;
        float thickness = size * 0.12f;
        renderer.drawRect(cx - thickness * 0.5f, cy - arm, thickness, arm * 2.0f, color);
        renderer.drawRect(cx - arm, cy - thickness * 0.5f, arm * 2.0f, thickness, color);

        float offset = arm * 0.7f;
        float small = thickness;
        renderer.drawRect(cx - offset - small, cy - small, small, small * 2.0f, color);
        renderer.drawRect(cx + offset, cy - small, small, small * 2.0f, color);
        renderer.drawRect(cx - small, cy - offset - small, small * 2.0f, small, color);
        renderer.drawRect(cx - small, cy + offset, small * 2.0f, small, color);
    }

    void drawCheckIcon(Renderer2D& renderer, float cx, float cy, float size, const Color& color)
    {
        float dot = size * 0.1f;
        float startX = cx - size * 0.35f;
        float startY = cy + size * 0.05f;

        for (int i = 0; i < 4; ++i)
        {
            float step = static_cast<float>(i) * dot * 1.1f;
            renderer.drawRect(startX + step, startY + step, dot, dot, color);
        }

        float midX = startX + 3.0f * dot * 1.1f;
        float midY = startY + 3.0f * dot * 1.1f;
        for (int i = 0; i < 6; ++i)
        {
            float step = static_cast<float>(i) * dot * 1.1f;
            renderer.drawRect(midX + step, midY - step, dot, dot, color);
        }
    }

    void drawStatusIcon(Renderer2D& renderer, const RectShape& screen, float desired, float current)
    {
        float cx = screen.x + screen.w * 0.5f;
        float cy = screen.y + screen.h * 0.5f;
        float size = std::min(screen.w, screen.h) * 0.35f;

        const float tolerance = 0.25f;
        float diff = desired - current;

        Color heatOuter{ 0.96f, 0.46f, 0.28f, 1.0f };
        Color heatInner{ 0.99f, 0.66f, 0.32f, 1.0f };
        Color snowColor{ 0.66f, 0.85f, 0.98f, 1.0f };
        Color checkColor{ 0.38f, 0.92f, 0.58f, 1.0f };

        if (diff > tolerance)
        {
            drawHeatIcon(renderer, cx, cy, size, heatOuter, heatInner);
        }
        else if (diff < -tolerance)
        {
            drawSnowIcon(renderer, cx, cy, size, snowColor);
        }
        else
        {
            drawCheckIcon(renderer, cx, cy, size, checkColor);
        }
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Kostur", NULL, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    const Color backgroundColor{ 0.10f, 0.12f, 0.16f, 1.0f };
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

    // Shader program and basic geometry
    Renderer2D renderer(WINDOW_WIDTH, WINDOW_HEIGHT, "Shaders/basic.vert", "Shaders/basic.frag");

    const Color bodyColor{ 0.90f, 0.93f, 0.95f, 1.0f };
    const Color ventColor{ 0.32f, 0.36f, 0.45f, 1.0f };
    const Color lampOffColor{ 0.22f, 0.18f, 0.20f, 1.0f };
    const Color lampOnColor{ 0.93f, 0.22f, 0.20f, 1.0f };
    const Color screenOffColor{ 0.08f, 0.10f, 0.12f, 1.0f };
    const Color screenOnColor{ 0.18f, 0.68f, 0.72f, 1.0f };
    const Color bowlColor{ 0.78f, 0.82f, 0.88f, 1.0f };
    const Color digitColor{ 0.96f, 0.98f, 1.0f, 1.0f };

    const float acWidth = 480.0f;
    const float acHeight = 200.0f;
    const float acX = (WINDOW_WIDTH - acWidth) * 0.5f;
    const float acY = 140.0f;

    RectShape acBody{ acX, acY, acWidth, acHeight, bodyColor };

    const float ventClosedHeight = 4.0f;
    const float ventOpenHeight = 18.0f;
    RectShape ventBar{ acX + 24.0f, acY + acHeight - 64.0f, acWidth - 48.0f, ventClosedHeight, ventColor };
    CircleShape lamp{ acX + acWidth - 44.0f, acY + acHeight - 26.0f, 14.0f, lampOffColor };

    const float screenWidth = 94.0f;
    const float screenHeight = 54.0f;
    const float screenSpacing = 22.0f;
    const float screenStartX = acX + 70.0f;
    const float screenY = acY + 52.0f;
    std::array<RectShape, 3> screens{};
    for (size_t i = 0; i < screens.size(); ++i)
    {
        screens[i] = RectShape{
            screenStartX + static_cast<float>(i) * (screenWidth + screenSpacing),
            screenY,
            screenWidth,
            screenHeight,
            screenOffColor
        };
    }

    const float bowlWidth = 260.0f;
    const float bowlHeight = 140.0f;
    const float bowlThickness = 10.0f;
    const float bowlX = (WINDOW_WIDTH - bowlWidth) * 0.5f;
    const float bowlY = acY + acHeight + 120.0f;
    RectShape bowlOutline{ bowlX, bowlY, bowlWidth, bowlHeight, bowlColor };

    auto setCustomCursorIfPresent = [&]()
    {
        const char* cursorPath = "Resources/cursor.png";
        GLFWcursor* cursor = nullptr;

        std::ifstream in(cursorPath, std::ios::binary);
        if (in.good())
        {
            cursor = loadImageToCursor(cursorPath);
        }

        if (cursor == nullptr)
        {
            cursor = createProceduralRemoteCursor();
        }

        if (cursor != nullptr)
        {
            glfwSetCursor(window, cursor);
        }
    };

    setCustomCursorIfPresent();

    AppState appState{};

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;

        handlePowerToggle(appState, mouseX, mouseY, mouseDown, lamp);
        handleTemperatureInput(appState, upPressed, downPressed);
        updateVent(appState, deltaTime);
        updateTemperature(appState, deltaTime);

        lamp.color = appState.isOn ? lampOnColor : lampOffColor;
        float ventHeight = ventClosedHeight + (ventOpenHeight - ventClosedHeight) * appState.ventOpenness;
        ventBar.h = ventHeight;

        Color screenColor = appState.isOn ? screenOnColor : screenOffColor;

        glClear(GL_COLOR_BUFFER_BIT);

        renderer.drawRect(acBody.x, acBody.y, acBody.w, acBody.h, acBody.color);
        renderer.drawRect(ventBar.x, ventBar.y, ventBar.w, ventBar.h, ventBar.color);
        renderer.drawCircle(lamp.x, lamp.y, lamp.radius, lamp.color);

        for (const auto& screen : screens)
        {
            renderer.drawRect(screen.x, screen.y, screen.w, screen.h, screenColor);
        }

        if (appState.isOn)
        {
            drawTemperatureValue(renderer, appState.desiredTemp, screens[0], digitColor);
            drawTemperatureValue(renderer, appState.currentTemp, screens[1], digitColor);
            drawStatusIcon(renderer, screens[2], appState.desiredTemp, appState.currentTemp);
        }

        renderer.drawFrame(bowlOutline, bowlThickness);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
