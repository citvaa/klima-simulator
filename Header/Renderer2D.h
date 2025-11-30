#pragma once

#include <GL/glew.h>

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

class Renderer2D
{
public:
    Renderer2D(int windowWidth, int windowHeight, const char* vertexShaderPath, const char* fragmentShaderPath);
    ~Renderer2D();

    void drawRect(float x, float y, float w, float h, const Color& color) const;
    void drawCircle(float cx, float cy, float radius, const Color& color, int segments = 48) const;
    void drawFrame(const RectShape& rect, float thickness) const;

private:
    float m_windowWidth;
    float m_windowHeight;
    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLint m_uColorLocation = -1;
};
