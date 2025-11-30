#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"

#include <array>
#include <cmath>
#include <fstream>
#include <vector>

// Osnovni 2D pipeline: kolor sejder, VAO/VBO za pravougaonik i helper koji radi sa koordinatama u pikselima
// (poreklo u gornjem levom uglu, y raste na dole)

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

struct Color
{
    float r, g, b, a;
};

struct RectShape
{
    float x, y, w, h;
    Color color;
};

struct CircleShape
{
    float x, y, radius;
    Color color;
};

void fillRectVertices(float x, float y, float w, float h, float* outVertices)
{
    // Konverzija iz piksel koordinata (0,0 u gornjem levom uglu) u NDC (-1..1)
    float x0 = 2.0f * x / WINDOW_WIDTH - 1.0f;
    float x1 = 2.0f * (x + w) / WINDOW_WIDTH - 1.0f;
    float y0 = 1.0f - 2.0f * y / WINDOW_HEIGHT;
    float y1 = 1.0f - 2.0f * (y + h) / WINDOW_HEIGHT;

    // Dva trougla (6 temena) za pravougaonik
    outVertices[0] = x0; outVertices[1] = y0;
    outVertices[2] = x1; outVertices[3] = y0;
    outVertices[4] = x1; outVertices[5] = y1;

    outVertices[6] = x0; outVertices[7] = y0;
    outVertices[8] = x1; outVertices[9] = y1;
    outVertices[10] = x0; outVertices[11] = y1;
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

    // Shader program za bojene 2D pravougaonike
    unsigned int colorProgram = createShader("Shaders/basic.vert", "Shaders/basic.frag");
    int uColorLocation = glGetUniformLocation(colorProgram, "uColor");

    // Geometrija: staticki oblik (6 temena), pozicije se menjaju po crtanju
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float initialVertices[12] = { 0.0f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(initialVertices), initialVertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    auto drawRect = [&](float x, float y, float w, float h, const Color& color)
    {
        float vertices[12];
        fillRectVertices(x, y, w, h, vertices);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glUseProgram(colorProgram);
        glUniform4f(uColorLocation, color.r, color.g, color.b, color.a);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    };

    auto drawCircle = [&](float cx, float cy, float radius, const Color& color, int segments = 48)
    {
        std::vector<float> vertices;
        vertices.reserve((segments + 2) * 2);

        float centerX = 2.0f * cx / WINDOW_WIDTH - 1.0f;
        float centerY = 1.0f - 2.0f * cy / WINDOW_HEIGHT;
        vertices.push_back(centerX);
        vertices.push_back(centerY);

        const float twoPi = 6.28318530718f;
        for (int i = 0; i <= segments; ++i)
        {
            float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);
            float px = cx + std::cos(angle) * radius;
            float py = cy + std::sin(angle) * radius;

            float ndcX = 2.0f * px / WINDOW_WIDTH - 1.0f;
            float ndcY = 1.0f - 2.0f * py / WINDOW_HEIGHT;

            vertices.push_back(ndcX);
            vertices.push_back(ndcY);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        glUseProgram(colorProgram);
        glUniform4f(uColorLocation, color.r, color.g, color.b, color.a);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
        glBindVertexArray(0);
    };

    auto drawFrame = [&](const RectShape& rect, float thickness)
    {
        drawRect(rect.x, rect.y, rect.w, thickness, rect.color); // top
        drawRect(rect.x, rect.y + rect.h - thickness, rect.w, thickness, rect.color); // bottom
        drawRect(rect.x, rect.y, thickness, rect.h, rect.color); // left
        drawRect(rect.x + rect.w - thickness, rect.y, thickness, rect.h, rect.color); // right
    };

    const Color bodyColor{ 0.90f, 0.93f, 0.95f, 1.0f };
    const Color ventColor{ 0.32f, 0.36f, 0.45f, 1.0f };
    const Color lampOffColor{ 0.22f, 0.18f, 0.20f, 1.0f };
    const Color screenOffColor{ 0.08f, 0.10f, 0.12f, 1.0f };
    const Color bowlColor{ 0.78f, 0.82f, 0.88f, 1.0f };

    const float acWidth = 480.0f;
    const float acHeight = 200.0f;
    const float acX = (WINDOW_WIDTH - acWidth) * 0.5f;
    const float acY = 140.0f;

    RectShape acBody{ acX, acY, acWidth, acHeight, bodyColor };
    RectShape ventBar{ acX + 24.0f, acY + acHeight - 64.0f, acWidth - 48.0f, 16.0f, ventColor };
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
        std::ifstream in(cursorPath, std::ios::binary);
        if (in.good())
        {
            GLFWcursor* cursor = loadImageToCursor(cursorPath);
            if (cursor != nullptr)
            {
                glfwSetCursor(window, cursor);
            }
        }
    };

    setCustomCursorIfPresent();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        drawRect(acBody.x, acBody.y, acBody.w, acBody.h, acBody.color);
        drawRect(ventBar.x, ventBar.y, ventBar.w, ventBar.h, ventBar.color);
        drawCircle(lamp.x, lamp.y, lamp.radius, lamp.color);

        for (const auto& screen : screens)
        {
            drawRect(screen.x, screen.y, screen.w, screen.h, screen.color);
        }

        drawFrame(bowlOutline, bowlThickness);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
