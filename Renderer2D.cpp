#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

static const char* vShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;
out vec4 vColor;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* fShaderSource = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)";

Renderer2D::Renderer2D() {}

Renderer2D::~Renderer2D() {
    Cleanup();
}

void Renderer2D::Init() {
    // Compile shaders
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vShaderSource, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fShaderSource, nullptr);
    glCompileShader(fs);

    m_Shader = glCreateProgram();
    glAttachShader(m_Shader, vs);
    glAttachShader(m_Shader, fs);
    glLinkProgram(m_Shader);

    glDeleteShader(vs);
    glDeleteShader(fs);

    m_ProjLoc = glGetUniformLocation(m_Shader, "uProjection");

    // Setup buffers
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, position));
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer2D::Begin(int screenWidth, int screenHeight) {
    m_Vertices.clear();
    m_Indices.clear();
    m_IndexOffset = 0;

    glUseProgram(m_Shader);
    glm::mat4 proj = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(m_ProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
}

void Renderer2D::Flush() {
    if (m_Indices.empty()) return;

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex2D), m_Vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, (GLsizei)m_Indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    m_Vertices.clear();
    m_Indices.clear();
    m_IndexOffset = 0;
}

void Renderer2D::End() {
    Flush();
}

void Renderer2D::Cleanup() {
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
    if (m_EBO) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
    if (m_Shader) { glDeleteProgram(m_Shader); m_Shader = 0; }
}

void Renderer2D::DrawQuad(float x, float y, float w, float h, const glm::vec4& color) {
    DrawQuadVertices(
        glm::vec2(x, y),
        glm::vec2(x + w, y),
        glm::vec2(x + w, y + h),
        glm::vec2(x, y + h),
        color
    );
}

void Renderer2D::DrawQuadVertices(const glm::vec2& v0, const glm::vec2& v1, 
                                  const glm::vec2& v2, const glm::vec2& v3, 
                                  const glm::vec4& color) {
    unsigned int startIdx = m_IndexOffset;

    m_Vertices.push_back({ v0, color });
    m_Vertices.push_back({ v1, color });
    m_Vertices.push_back({ v2, color });
    m_Vertices.push_back({ v3, color });

    m_Indices.push_back(startIdx + 0);
    m_Indices.push_back(startIdx + 1);
    m_Indices.push_back(startIdx + 2);

    m_Indices.push_back(startIdx + 0);
    m_Indices.push_back(startIdx + 2);
    m_Indices.push_back(startIdx + 3);

    m_IndexOffset += 4;
}

void Renderer2D::DrawRectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color) {
    // Top
    DrawQuad(x, y, w, thickness, color);
    // Bottom
    DrawQuad(x, y + h - thickness, w, thickness, color);
    // Left
    DrawQuad(x, y + thickness, thickness, h - 2.0f * thickness, color);
    // Right
    DrawQuad(x + w - thickness, y + thickness, thickness, h - 2.0f * thickness, color);
}

void Renderer2D::DrawLine(const glm::vec2& p1, const glm::vec2& p2, float thickness, const glm::vec4& color) {
    glm::vec2 d = p2 - p1;
    float len = glm::length(d);
    if (len < 0.0001f) return;
    glm::vec2 norm = glm::vec2(-d.y, d.x) / len * (thickness * 0.5f);

    glm::vec2 v0 = p1 + norm;
    glm::vec2 v1 = p2 + norm;
    glm::vec2 v2 = p2 - norm;
    glm::vec2 v3 = p1 - norm;

    DrawQuadVertices(v0, v1, v2, v3, color);
}

void Renderer2D::DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments) {
    if (segments < 6) segments = 6;
    unsigned int centerIdx = m_IndexOffset;
    m_Vertices.push_back({ center, color });
    m_IndexOffset++;

    float angleStep = 6.2831853f / (float)segments;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        glm::vec2 p = center + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
        m_Vertices.push_back({ p, color });
    }

    for (int i = 0; i < segments; ++i) {
        m_Indices.push_back(centerIdx);
        m_Indices.push_back(centerIdx + 1 + i);
        m_Indices.push_back(centerIdx + 2 + i);
    }
    m_IndexOffset += (segments + 1);
}

// 7-segment / stroke vector text renderer
float Renderer2D::GetTextWidth(const std::string& text, float scale) {
    return (float)text.length() * (6.0f * scale);
}

void Renderer2D::DrawText(const std::string& text, float x, float y, float scale, const glm::vec4& color, float thickness) {
    float curX = x;
    for (char c : text) {
        if (c == ' ') {
            curX += 5.0f * scale;
            continue;
        }
        DrawChar(c, curX, y, scale, color, thickness);
        curX += 6.5f * scale;
    }
}

void Renderer2D::DrawChar(char c, float x, float y, float scale, const glm::vec4& color, float thickness) {
    // 5x7 grid points normalized 0..5, 0..7
    auto P = [x, y, scale](float px, float py) {
        return glm::vec2(x + px * scale, y + py * scale);
    };

    char ch = (char)toupper(c);

    // Common line draw helper
    auto L = [this, &P, thickness, &color](float x1, float y1, float x2, float y2) {
        this->DrawLine(P(x1, y1), P(x2, y2), thickness, color);
    };

    switch (ch) {
        case 'A': L(0,7, 0,2); L(0,2, 2.5f,0); L(2.5f,0, 5,2); L(5,2, 5,7); L(0,4, 5,4); break;
        case 'B': L(0,0, 0,7); L(0,0, 3.5f,0); L(3.5f,0, 5,1.75f); L(5,1.75f, 3.5f,3.5f); L(0,3.5f, 3.5f,3.5f); L(3.5f,3.5f, 5,5.25f); L(5,5.25f, 3.5f,7); L(3.5f,7, 0,7); break;
        case 'C': L(5,0, 0,0); L(0,0, 0,7); L(0,7, 5,7); break;
        case 'D': L(0,0, 0,7); L(0,0, 3.5f,0); L(3.5f,0, 5,3.5f); L(5,3.5f, 3.5f,7); L(3.5f,7, 0,7); break;
        case 'E': L(5,0, 0,0); L(0,0, 0,7); L(0,7, 5,7); L(0,3.5f, 4,3.5f); break;
        case 'F': L(5,0, 0,0); L(0,0, 0,7); L(0,3.5f, 4,3.5f); break;
        case 'G': L(5,1, 1,0); L(1,0, 0,2); L(0,2, 0,5); L(0,5, 1,7); L(1,7, 5,7); L(5,7, 5,3.5f); L(5,3.5f, 2.5f,3.5f); break;
        case 'H': L(0,0, 0,7); L(5,0, 5,7); L(0,3.5f, 5,3.5f); break;
        case 'I': L(1,0, 4,0); L(2.5f,0, 2.5f,7); L(1,7, 4,7); break;
        case 'J': L(5,0, 5,5.5f); L(5,5.5f, 3.5f,7); L(3.5f,7, 1,7); L(1,7, 0,5.5f); break;
        case 'K': L(0,0, 0,7); L(5,0, 0,3.5f); L(0,3.5f, 5,7); break;
        case 'L': L(0,0, 0,7); L(0,7, 5,7); break;
        case 'M': L(0,7, 0,0); L(0,0, 2.5f,3.5f); L(2.5f,3.5f, 5,0); L(5,0, 5,7); break;
        case 'N': L(0,7, 0,0); L(0,0, 5,7); L(5,7, 5,0); break;
        case 'O': L(1,0, 4,0); L(4,0, 5,2); L(5,2, 5,5); L(5,5, 4,7); L(4,7, 1,7); L(1,7, 0,5); L(0,5, 0,2); L(0,2, 1,0); break;
        case 'P': L(0,0, 0,7); L(0,0, 4,0); L(4,0, 5,1.75f); L(5,1.75f, 4,3.5f); L(4,3.5f, 0,3.5f); break;
        case 'Q': L(1,0, 4,0); L(4,0, 5,2); L(5,2, 5,5); L(5,5, 4,7); L(4,7, 1,7); L(1,7, 0,5); L(0,5, 0,2); L(0,2, 1,0); L(3,5, 5,7); break;
        case 'R': L(0,0, 0,7); L(0,0, 4,0); L(4,0, 5,1.75f); L(5,1.75f, 4,3.5f); L(4,3.5f, 0,3.5f); L(2.5f,3.5f, 5,7); break;
        case 'S': L(5,0.5f, 1,0); L(1,0, 0,1.5f); L(0,1.5f, 1,3.5f); L(1,3.5f, 4,3.5f); L(4,3.5f, 5,5.5f); L(5,5.5f, 4,7); L(4,7, 0,6.5f); break;
        case 'T': L(0,0, 5,0); L(2.5f,0, 2.5f,7); break;
        case 'U': L(0,0, 0,5.5f); L(0,5.5f, 1.5f,7); L(1.5f,7, 3.5f,7); L(3.5f,7, 5,5.5f); L(5,5.5f, 5,0); break;
        case 'V': L(0,0, 2.5f,7); L(2.5f,7, 5,0); break;
        case 'W': L(0,0, 1.25f,7); L(1.25f,7, 2.5f,3.5f); L(2.5f,3.5f, 3.75f,7); L(3.75f,7, 5,0); break;
        case 'X': L(0,0, 5,7); L(5,0, 0,7); break;
        case 'Y': L(0,0, 2.5f,3.5f); L(5,0, 2.5f,3.5f); L(2.5f,3.5f, 2.5f,7); break;
        case 'Z': L(0,0, 5,0); L(5,0, 0,7); L(0,7, 5,7); break;
        case '0': L(0,0, 5,0); L(5,0, 5,7); L(5,7, 0,7); L(0,7, 0,0); L(5,0, 0,7); break;
        case '1': L(1,2, 2.5f,0); L(2.5f,0, 2.5f,7); L(0.5f,7, 4.5f,7); break;
        case '2': L(0,1.5f, 1.5f,0); L(1.5f,0, 4,0); L(4,0, 5,2); L(5,2, 0,7); L(0,7, 5,7); break;
        case '3': L(0,0, 5,0); L(5,0, 2.5f,3.5f); L(2.5f,3.5f, 4.5f,4); L(4.5f,4, 5,5.5f); L(5,5.5f, 3.5f,7); L(3.5f,7, 0,7); break;
        case '4': L(4,7, 4,0); L(4,0, 0,4.5f); L(0,4.5f, 5,4.5f); break;
        case '5': L(5,0, 0,0); L(0,0, 0,3); L(0,3, 4.5f,3); L(4.5f,3, 5,5); L(5,5, 3.5f,7); L(3.5f,7, 0,6.5f); break;
        case '6': L(4,0, 1,0); L(1,0, 0,2); L(0,2, 0,6); L(0,6, 1,7); L(1,7, 4,7); L(4,7, 5,5.5f); L(5,5.5f, 4,4); L(4,4, 0,4); break;
        case '7': L(0,0, 5,0); L(5,0, 2,7); break;
        case '8': L(1,0, 4,0); L(4,0, 5,1.5f); L(5,1.5f, 4,3.5f); L(4,3.5f, 1,3.5f); L(1,3.5f, 0,1.5f); L(0,1.5f, 1,0);
                  L(4,3.5f, 5,5.5f); L(5,5.5f, 4,7); L(4,7, 1,7); L(1,7, 0,5.5f); L(0,5.5f, 1,3.5f); break;
        case '9': L(5,3, 1,3); L(1,3, 0,1.5f); L(0,1.5f, 1,0); L(1,0, 4,0); L(4,0, 5,1.5f); L(5,1.5f, 5,5.5f); L(5,5.5f, 4,7); L(4,7, 1,7); break;
        case ':': DrawCircle(P(2.5f, 2.0f), scale * 0.6f, color, 8); DrawCircle(P(2.5f, 5.0f), scale * 0.6f, color, 8); break;
        case '/': L(0,7, 5,0); break;
        case '-': L(1,3.5f, 4,3.5f); break;
        case '+': L(1,3.5f, 4,3.5f); L(2.5f,1.5f, 2.5f,5.5f); break;
        case '%': L(0,7, 5,0); DrawCircle(P(1.5f, 1.8f), scale * 0.6f, color, 8); DrawCircle(P(3.5f, 5.2f), scale * 0.6f, color, 8); break;
        case '.': DrawCircle(P(2.5f, 6.5f), scale * 0.6f, color, 8); break;
        default: break;
    }
}
