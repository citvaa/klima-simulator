#include "../Header/TextRenderer.h"

#include "../Header/Util.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <iostream>

namespace
{
    constexpr const char* kDefaultFontPath = "C:\\Windows\\Fonts\\arial.ttf";
    constexpr const char* kTextVertexShader = "Shaders/text.vert";
    constexpr const char* kTextFragmentShader = "Shaders/text.frag";
}

TextRenderer::TextRenderer(int windowWidth, int windowHeight)
    : m_windowWidth(static_cast<float>(windowWidth))
    , m_windowHeight(static_cast<float>(windowHeight))
{
    m_program = createShader(kTextVertexShader, kTextFragmentShader);
    m_uTextColor = glGetUniformLocation(m_program, "uTextColor");
    m_uWindowSize = glGetUniformLocation(m_program, "uWindowSize");
    m_uTexture = glGetUniformLocation(m_program, "uTexture");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Attempt to load a default Windows font so the UI is usable out of the box.
    loadFont(kDefaultFontPath, 48);
}

TextRenderer::~TextRenderer()
{
    cleanup();
}

void TextRenderer::cleanup()
{
    destroyGlyphTextures();

    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_program != 0) glDeleteProgram(m_program);

    m_vbo = 0;
    m_vao = 0;
    m_program = 0;
}

void TextRenderer::destroyGlyphTextures()
{
    for (auto& kv : m_glyphs)
    {
        if (kv.second.texture != 0)
        {
            glDeleteTextures(1, &kv.second.texture);
        }
    }
    m_glyphs.clear();
}

bool TextRenderer::loadFont(const std::string& fontPath, unsigned int pixelHeight)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "FreeType init failed.\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
        std::cout << "Failed to load font: " << fontPath << "\n";
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixelHeight);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    destroyGlyphTextures();
    m_fontPixelHeight = pixelHeight;

    const std::string charset = "-0123456789C ";
    for (char c : charset)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load glyph: " << c << "\n";
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Glyph glyph;
        glyph.texture = texture;
        glyph.width = face->glyph->bitmap.width;
        glyph.height = face->glyph->bitmap.rows;
        glyph.bearingX = face->glyph->bitmap_left;
        glyph.bearingY = face->glyph->bitmap_top;
        glyph.advance = static_cast<unsigned int>(face->glyph->advance.x);

        m_glyphs[c] = glyph;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return !m_glyphs.empty();
}

void TextRenderer::setWindowSize(float width, float height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

TextMetrics TextRenderer::measure(const std::string& text, float scale) const
{
    float width = 0.0f;
    float maxAscent = 0.0f;
    float maxDescent = 0.0f;

    for (char c : text)
    {
        auto it = m_glyphs.find(c);
        if (it == m_glyphs.end()) continue;
        const Glyph& g = it->second;

        width += (g.advance >> 6) * scale;
        maxAscent = std::max(maxAscent, static_cast<float>(g.bearingY) * scale);
        float descent = static_cast<float>(g.height - g.bearingY) * scale;
        maxDescent = std::max(maxDescent, descent);
    }

    // Fallback so height is non-zero even if glyph set is incomplete.
    if (maxAscent + maxDescent <= 0.0f && m_fontPixelHeight > 0)
    {
        maxAscent = static_cast<float>(m_fontPixelHeight) * scale;
    }

    TextMetrics metrics;
    metrics.width = width;
    metrics.ascent = maxAscent;
    metrics.height = maxAscent + maxDescent;
    return metrics;
}

void TextRenderer::drawText(const std::string& text, float x, float y, float scale, const Color& color)
{
    if (m_glyphs.empty()) return;

    TextMetrics m = measure(text, scale);
    float baselineY = y + m.ascent;

    glUseProgram(m_program);
    glUniform4f(m_uTextColor, color.r, color.g, color.b, color.a);
    glUniform2f(m_uWindowSize, m_windowWidth, m_windowHeight);
    glUniform1i(m_uTexture, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    float cursorX = x;
    for (char c : text)
    {
        auto it = m_glyphs.find(c);
        if (it == m_glyphs.end()) continue;
        const Glyph& g = it->second;

        float xpos = cursorX + static_cast<float>(g.bearingX) * scale;
        float ypos = baselineY - static_cast<float>(g.bearingY) * scale;

        float w = static_cast<float>(g.width) * scale;
        float h = static_cast<float>(g.height) * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, g.texture);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cursorX += (g.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
