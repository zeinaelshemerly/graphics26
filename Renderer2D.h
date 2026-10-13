#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex2D {
    glm::vec2 position;
    glm::vec4 color;
};

class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();

    void Init();
    void Begin(int screenWidth, int screenHeight);
    void Flush();
    void End();

    // Drawing primitives
    void DrawQuad(float x, float y, float w, float h, const glm::vec4& color);
    void DrawQuadVertices(const glm::vec2& v0, const glm::vec2& v1, 
                          const glm::vec2& v2, const glm::vec2& v3, 
                          const glm::vec4& color);
    void DrawRectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color);
    void DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 24);
    void DrawLine(const glm::vec2& p1, const glm::vec2& p2, float thickness, const glm::vec4& color);

    // Vector typography for titles and UI
    void DrawText(const std::string& text, float x, float y, float scale, const glm::vec4& color, float thickness = 2.0f);
    float GetTextWidth(const std::string& text, float scale);

    void Cleanup();

private:
    void DrawChar(char c, float x, float y, float scale, const glm::vec4& color, float thickness);

    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    GLuint m_Shader = 0;
    GLint m_ProjLoc = -1;

    std::vector<Vertex2D> m_Vertices;
    std::vector<unsigned int> m_Indices;
    unsigned int m_IndexOffset = 0;
};
