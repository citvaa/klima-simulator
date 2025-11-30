#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include "../Header/Renderer2D.h"
#include "../Header/State.h"
#include "../Header/TemperatureUI.h"
#include "../Header/Controls.h"
#include "../Header/TextRenderer.h"

#include <array>
#include <algorithm>
#include <cmath>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

struct ResizeContext
{
    Renderer2D* renderer = nullptr;
    TextRenderer* textRenderer = nullptr;
    int* windowWidth = nullptr;
    int* windowHeight = nullptr;
};

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "AC Simulator", NULL, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    const Color backgroundColor{ 0.10f, 0.12f, 0.16f, 1.0f };
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

    // Shader program and basic geometry
    Renderer2D renderer(fbWidth, fbHeight, "Shaders/basic.vert", "Shaders/basic.frag");
    TextRenderer textRenderer(fbWidth, fbHeight);

    ResizeContext resizeCtx;
    resizeCtx.renderer = &renderer;
    resizeCtx.textRenderer = &textRenderer;
    resizeCtx.windowWidth = &windowWidth;
    resizeCtx.windowHeight = &windowHeight;
    glfwSetWindowUserPointer(window, &resizeCtx);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h)
    {
        auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(win));
        if (!ctx) return;
        glViewport(0, 0, w, h);
        if (ctx->windowWidth) *ctx->windowWidth = w;
        if (ctx->windowHeight) *ctx->windowHeight = h;
        if (ctx->renderer) ctx->renderer->setWindowSize(static_cast<float>(w), static_cast<float>(h));
        if (ctx->textRenderer) ctx->textRenderer->setWindowSize(static_cast<float>(w), static_cast<float>(h));
    });

    const Color bodyColor{ 0.90f, 0.93f, 0.95f, 1.0f };
    const Color ventColor{ 0.32f, 0.36f, 0.45f, 1.0f };
    const Color lampOffColor{ 0.22f, 0.18f, 0.20f, 1.0f };
    const Color lampOnColor{ 0.93f, 0.22f, 0.20f, 1.0f };
    const Color screenOffColor{ 0.08f, 0.10f, 0.12f, 1.0f };
    const Color screenOnColor{ 0.18f, 0.68f, 0.72f, 1.0f };
    const Color bowlColor{ 0.78f, 0.82f, 0.88f, 1.0f };
    const Color digitColor{ 0.96f, 0.98f, 1.0f, 1.0f };
    const Color arrowBg{ 0.15f, 0.18f, 0.22f, 1.0f };
    const Color arrowColor{ 0.90f, 0.96f, 1.0f, 1.0f };
    const Color waterColor{ 0.50f, 0.78f, 0.94f, 0.9f };

    const float acWidth = 480.0f;
    const float acHeight = 200.0f;
    const float acY = 0.0f;

    RectShape acBody{ 0.0f, acY, acWidth, acHeight, bodyColor };

    const float ventClosedHeight = 4.0f;
    const float ventOpenHeight = 18.0f;
    RectShape ventBar{ 24.0f, acY + acHeight - 64.0f, acWidth - 48.0f, ventClosedHeight, ventColor };
    CircleShape lamp{ acWidth - 44.0f, acY + acHeight - 26.0f, 14.0f, lampOffColor };

    const float screenWidth = 94.0f;
    const float screenHeight = 54.0f;
    const float screenSpacing = 22.0f;
    const float screenStartX = 70.0f;
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

    const float arrowWidth = 40.0f;
    RectShape tempArrowButton{ screenStartX - arrowWidth - 12.0f, screenY, arrowWidth, screenHeight, arrowBg };

    const float bowlWidth = 260.0f;
    const float bowlHeight = 140.0f;
    const float bowlThickness = 10.0f;
    const float bowlX = (acWidth - bowlWidth) * 0.5f;
    const float bowlY = acY + acHeight + 120.0f;
    RectShape bowlOutline{ bowlX, bowlY, bowlWidth, bowlHeight, bowlColor };

    auto setProceduralCursor = [&]()
    {
        GLFWcursor* cursor = createProceduralRemoteCursor();

        if (cursor != nullptr)
        {
            glfwSetCursor(window, cursor);
        }
    };

    setProceduralCursor();

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
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool clickStarted = mouseDown && !appState.prevMouseDown;

        float sceneMinX = std::min({ acBody.x, tempArrowButton.x, bowlOutline.x });
        float sceneMaxX = std::max({ acBody.x + acBody.w, tempArrowButton.x + tempArrowButton.w, bowlOutline.x + bowlOutline.w });
        float sceneMinY = std::min({ acBody.y, tempArrowButton.y, bowlOutline.y });
        float sceneMaxY = std::max({ acBody.y + acBody.h, tempArrowButton.y + tempArrowButton.h, bowlOutline.y + bowlOutline.h });
        float sceneW = sceneMaxX - sceneMinX;
        float sceneH = sceneMaxY - sceneMinY;
        float offsetX = (static_cast<float>(windowWidth) - sceneW) * 0.5f - sceneMinX;
        float offsetY = (static_cast<float>(windowHeight) - sceneH) * 0.5f - sceneMinY;

        auto shiftRect = [&](const RectShape& r)
        {
            RectShape out = r;
            out.x += offsetX;
            out.y += offsetY;
            return out;
        };
        auto shiftCircle = [&](const CircleShape& c)
        {
            CircleShape out = c;
            out.x += offsetX;
            out.y += offsetY;
            return out;
        };

        RectShape acBodyDraw = shiftRect(acBody);
        RectShape ventBarDraw = shiftRect(ventBar);
        CircleShape lampDraw = shiftCircle(lamp);

        std::array<RectShape, 3> screensDraw = screens;
        for (auto& s : screensDraw)
        {
            s = shiftRect(s);
        }

        RectShape tempArrowDraw = shiftRect(tempArrowButton);
        RectShape bowlDraw = shiftRect(bowlOutline);
        float bowlInnerX = bowlDraw.x + bowlThickness;
        float bowlInnerY = bowlDraw.y + bowlThickness;
        float bowlInnerW = bowlDraw.w - 2.0f * bowlThickness;
        float bowlInnerH = bowlDraw.h - 2.0f * bowlThickness;

        if (clickStarted && !appState.lockedByFullBowl)
        {
            if (pointInRect(mouseX, mouseY, tempArrowDraw))
            {
                float midY = tempArrowDraw.y + tempArrowDraw.h * 0.5f;
                if (mouseY < midY)
                {
                    appState.desiredTemp += appState.tempChangeStep;
                }
                else
                {
                    appState.desiredTemp -= appState.tempChangeStep;
                }
            }

            if (appState.desiredTemp < -10.0f) appState.desiredTemp = -10.0f;
            if (appState.desiredTemp > 40.0f) appState.desiredTemp = 40.0f;
        }

        handlePowerToggle(appState, mouseX, mouseY, mouseDown, lampDraw);
        handleTemperatureInput(appState, upPressed, downPressed);
        updateVent(appState, deltaTime);
        updateTemperature(appState, deltaTime);
        updateWater(appState, deltaTime, spacePressed);

        lampDraw.color = appState.isOn ? lampOnColor : lampOffColor;
        float ventHeight = ventClosedHeight + (ventOpenHeight - ventClosedHeight) * appState.ventOpenness;
        ventBarDraw.h = ventHeight;

        Color screenColor = appState.isOn ? screenOnColor : screenOffColor;

        glClear(GL_COLOR_BUFFER_BIT);

        renderer.drawRect(acBodyDraw.x, acBodyDraw.y, acBodyDraw.w, acBodyDraw.h, acBodyDraw.color);
        renderer.drawRect(ventBarDraw.x, ventBarDraw.y, ventBarDraw.w, ventBarDraw.h, ventBarDraw.color);
        renderer.drawCircle(lampDraw.x, lampDraw.y, lampDraw.radius, lampDraw.color);

        for (const auto& screen : screensDraw)
        {
            renderer.drawRect(screen.x, screen.y, screen.w, screen.h, screenColor);
        }

        if (appState.isOn)
        {
            drawTemperatureValue(textRenderer, appState.desiredTemp, screensDraw[0], digitColor);
            drawTemperatureValue(textRenderer, appState.currentTemp, screensDraw[1], digitColor);
            drawStatusIcon(renderer, screensDraw[2], appState.desiredTemp, appState.currentTemp);
        }

        if (appState.waterLevel > 0.0f)
        {
            float waterHeight = bowlInnerH * appState.waterLevel;
            float waterY = bowlInnerY + bowlInnerH - waterHeight;
            renderer.drawRect(bowlInnerX, waterY, bowlInnerW, waterHeight, waterColor);
        }
        renderer.drawFrame(bowlDraw, bowlThickness);
        RectShape arrowTop{ tempArrowDraw.x, tempArrowDraw.y, tempArrowDraw.w, tempArrowDraw.h * 0.5f, arrowBg };
        RectShape arrowBottom{ tempArrowDraw.x, tempArrowDraw.y + tempArrowDraw.h * 0.5f, tempArrowDraw.w, tempArrowDraw.h * 0.5f, arrowBg };
        drawHalfArrow(renderer, arrowTop, true, arrowColor, arrowBg);
        drawHalfArrow(renderer, arrowBottom, false, arrowColor, arrowBg);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
