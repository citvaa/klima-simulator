#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"

// Osnovni 2D pipeline: kolor sejder, VAO/VBO za pravougaonik i helper koji radi sa koordinatama u pikselima
// (poreklo u gornjem levom uglu, y raste na dole)

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

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

    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);

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

    auto drawRect = [&](float x, float y, float w, float h, float r, float g, float b, float a)
    {
        float vertices[12];
        fillRectVertices(x, y, w, h, vertices);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glUseProgram(colorProgram);
        glUniform4f(uColorLocation, r, g, b, a);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    };

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // Test pravougaonik za potvrdu pipeline-a
        drawRect(100.0f, 100.0f, 200.0f, 150.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
