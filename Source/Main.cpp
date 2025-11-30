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
#include <chrono>
#include <string>
#include <cstdio>
#include <thread>

// Entry point: fullscreen AC simulator with timed logic and on-screen UI.
const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

// Pointers handed to the framebuffer-size callback so we can update renderers on resize.
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

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary); // fullscreen mode descriptor

    int windowWidth = mode ? mode->width : 800;
    int windowHeight = mode ? mode->height : 800;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "AC Simulator", primary, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

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
    GLuint overlayProgram = createShader("Shaders/overlay.vert", "Shaders/overlay.frag");
    GLint overlayWindowSizeLoc = glGetUniformLocation(overlayProgram, "uWindowSize");
    GLint overlayTintLoc = glGetUniformLocation(overlayProgram, "uTint");
    GLint overlayTextureLoc = glGetUniformLocation(overlayProgram, "uTexture");

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
    const Color nameplateBg{ 0.08f, 0.08f, 0.10f, 0.45f };
    const Color nameplateText{ 0.96f, 0.98f, 1.0f, 0.95f };

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

    GLuint nameplateTexture = 0;
    int nameplateW = 0;
    int nameplateH = 0;
    textRenderer.createTextTexture("Vuk Vicentic, SV45/2022", nameplateText, nameplateBg, 10, 42, nameplateTexture, nameplateW, nameplateH);

    GLuint overlayVao = 0;
    GLuint overlayVbo = 0;
    glGenVertexArrays(1, &overlayVao);
    glGenBuffers(1, &overlayVbo);
    glBindVertexArray(overlayVao);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Create and set a simple remote-shaped cursor (hotspot at laser dot top-left).
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
    std::string frameStats = "FPS --";
    double logAccumulator = 0.0;
    int logFrames = 0;

    auto lastTime = std::chrono::steady_clock::now(); // main clock source

    while (!glfwWindowShouldClose(window))
    {
        auto frameStartTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(frameStartTime - lastTime).count(); // seconds since last frame
        lastTime = frameStartTime;
        logAccumulator += deltaTime;
        ++logFrames;
        if (logAccumulator >= 1.0)
        {
            double avgDelta = logAccumulator / static_cast<double>(logFrames);
            double avgFps = avgDelta > 0.0 ? 1.0 / avgDelta : 0.0;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "FPS %.1f", avgFps); // once per second
            frameStats = buf;
            logAccumulator = 0.0;
            logFrames = 0;
        }

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
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

        if (!frameStats.empty())
        {
            float statsScale = 0.6f;
            float margin = 16.0f;
            textRenderer.drawText(frameStats, margin, margin, statsScale, digitColor);
        }

        if (nameplateTexture != 0)
        {
            float margin = 20.0f;
            float overlayX = static_cast<float>(windowWidth) - static_cast<float>(nameplateW) - margin;
            float overlayY = static_cast<float>(windowHeight) - static_cast<float>(nameplateH) - margin;

            float vertices[6][4] = {
                { overlayX,                          overlayY + nameplateH, 0.0f, 0.0f },
                { overlayX,                          overlayY,               0.0f, 1.0f },
                { overlayX + nameplateW,             overlayY,               1.0f, 1.0f },

                { overlayX,                          overlayY + nameplateH, 0.0f, 0.0f },
                { overlayX + nameplateW,             overlayY,               1.0f, 1.0f },
                { overlayX + nameplateW,             overlayY + nameplateH, 1.0f, 0.0f },
            };

            glUseProgram(overlayProgram);
            glUniform2f(overlayWindowSizeLoc, static_cast<float>(windowWidth), static_cast<float>(windowHeight));
            glUniform4f(overlayTintLoc, 1.0f, 1.0f, 1.0f, 1.0f);
            glUniform1i(overlayTextureLoc, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, nameplateTexture);

            glBindVertexArray(overlayVao);
            glBindBuffer(GL_ARRAY_BUFFER, overlayVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto targetTime = frameStartTime + std::chrono::duration<double>(TARGET_FRAME_TIME);
        auto now = std::chrono::steady_clock::now();
        if (now < targetTime)
        {
            std::this_thread::sleep_until(targetTime); // coarse frame limiter to ~75 FPS
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
