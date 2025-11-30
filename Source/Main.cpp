#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include "../Header/Renderer2D.h"
#include "../Header/State.h"

#include <array>
#include <fstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

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

        handlePowerToggle(appState, mouseX, mouseY, mouseDown, lamp);
        updateVent(appState, deltaTime);

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

        renderer.drawFrame(bowlOutline, bowlThickness);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
