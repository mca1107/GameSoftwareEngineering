#include "stdafx.h"
#include "Renderer.h"
#include <utility>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace
{
std::string ReadShader(const wchar_t* name)
{
    // Load beside the executable, independently of the debugger's working directory.
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    if (!length || length >= 32768)
        return {};
    std::wstring full(path, length);
    full = full.substr(0, full.find_last_of(L"\\/") + 1) + L"Shaders\\" + name;
    std::ifstream file(full.c_str(), std::ios::binary);
    if (!file)
    {
        std::wcerr << L"Cannot read shader: " << full << L"\n";
        return {};
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

GLuint Shader(GLenum type, const std::string& source)
{
    if (source.empty())
        return 0;
    GLuint shader = glCreateShader(type);
    if (!shader)
        return 0;
    const char* data = source.c_str();
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
} // namespace

Renderer::Renderer(int width, int height)
{
    Resize(width, height);
    m_Program = CompileShaders();
    if (!m_Program)
        return;
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));
    m_Viewport = glGetUniformLocation(m_Program, "u_Viewport");
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,
                          1,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, material)));
    m_Textured = glGetUniformLocation(m_Program, "u_Textured");
    glUseProgram(m_Program);
    glUniform1i(glGetUniformLocation(m_Program, "u_Texture"), 0);
    m_Initialized = m_VAO != 0 && m_VBO != 0;
    m_Vertices.reserve(60000);
    m_PostProgram = CompileShaders(L"PostProcess.vs", L"PostProcess.fs");
    CreateTargets();
    CreateCharacterAtlas();
    CreateModels();
}

Renderer::~Renderer()
{
    DeleteTargets();
    if (m_PostProgram)
        glDeleteProgram(m_PostProgram);
    if (m_CharacterAtlas)
        glDeleteTextures(1, &m_CharacterAtlas);
    for (const auto& entry : m_Text)
        glDeleteTextures(1, &entry.second.texture);
    if (m_VBO)
        glDeleteBuffers(1, &m_VBO);
    if (m_VAO)
        glDeleteVertexArrays(1, &m_VAO);
    if (m_Program)
        glDeleteProgram(m_Program);
}

GLuint Renderer::CompileShaders(const wchar_t* vertex, const wchar_t* fragment)
{
    GLuint vs = Shader(GL_VERTEX_SHADER, ReadShader(vertex));
    GLuint fs = Shader(GL_FRAGMENT_SHADER, ReadShader(fragment));
    if (!vs || !fs)
    {
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        return 0;
    }
    GLuint program = glCreateProgram();
    if (program)
    {
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!program)
        return 0;
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void Renderer::Resize(int width, int height)
{
    m_Width = (std::max)(1, width);
    m_Height = (std::max)(1, height);
    glViewport(0, 0, m_Width, m_Height);
    if (m_PostProgram)
        CreateTargets();
}

void Renderer::Begin(float time)
{
    m_Time = time;
    m_UiOrigin = Vec2();
    m_UiScaleX = m_UiScaleY = 1;
    m_UiShearY = 0;
    m_Vertices.clear();
    glBindFramebuffer(GL_FRAMEBUFFER, m_EffectsReady ? m_Targets[0] : 0);
    glViewport(0, 0, m_Width, m_Height);
    glBlendEquation(GL_FUNC_ADD);
    glClearColor(0.055f, 0.085f, 0.078f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::Submit(const Vertex* vertices, size_t count, GLuint texture, int textureMode)
{
    if (!m_Initialized || count == 0)
        return;
    if (m_UiOrigin.x != 0 || m_UiOrigin.y != 0 || m_UiScaleX != 1 || m_UiScaleY != 1 ||
        m_UiShearY != 0)
    {
        m_UiVertices.assign(vertices, vertices + count);
        for (Vertex& vertex : m_UiVertices)
        {
            float x = vertex.x;
            vertex.x = m_UiOrigin.x + x * m_UiScaleX;
            vertex.y = m_UiOrigin.y + vertex.y * m_UiScaleY + x * m_UiShearY;
        }
        vertices = m_UiVertices.data();
    }
    glUseProgram(m_Program);
    glUniform2f(m_Viewport, static_cast<float>(m_Width), static_cast<float>(m_Height));
    glUniform1f(glGetUniformLocation(m_Program, "u_Time"), m_Time);
    glUniform1i(m_Textured, texture ? textureMode : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(count));
}

void Renderer::Flush()
{
    Submit(m_Vertices.data(), m_Vertices.size(), 0);
    m_Vertices.clear();
}

void Renderer::Triangle(Vec2 a, Vec2 b, Vec2 c, Color t)
{
    for (Vec2 p : {a, b, c})
        m_Vertices.push_back({p.x, p.y, 0, 0, t.r, t.g, t.b, t.a});
}

void Renderer::Quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color)
{
    Triangle(a, b, c, color);
    Triangle(a, c, d, color);
}

void Renderer::Rect(float x, float y, float w, float h, Color c)
{
    Quad({x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, c);
}

void Renderer::Ellipse(Vec2 p, float rx, float ry, Color c)
{
    // Reuse the unit circle across plants, shadows, models and UI.
    static const std::vector<Vec2> circle = []
    {
        std::vector<Vec2> points;
        points.reserve(16);
        for (int i = 0; i < 16; ++i)
        {
            float angle = i * 6.2831853f / 16;
            points.push_back({std::cos(angle), std::sin(angle)});
        }
        return points;
    }();
    for (size_t i = 0; i < circle.size(); ++i)
    {
        const Vec2& a = circle[i];
        const Vec2& b = circle[(i + 1) % circle.size()];
        Triangle(p, {p.x + a.x * rx, p.y + a.y * ry}, {p.x + b.x * rx, p.y + b.y * ry}, c);
    }
}

void Renderer::Line(Vec2 a, Vec2 b, float w, Color c)
{
    float dx = b.x - a.x, dy = b.y - a.y, length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.001f)
        return;
    dx = dx / length * w * 0.5f;
    dy = dy / length * w * 0.5f;
    Quad({a.x - dy, a.y + dx}, {b.x - dy, b.y + dx}, {b.x + dy, b.y - dx}, {a.x + dy, a.y - dx}, c);
}

void Renderer::DrawSolidRect(
    float x, float y, float, float size, float r, float g, float b, float a)
{
    Rect(m_Width * 0.5f + x - size * 0.5f,
         m_Height * 0.5f - y - size * 0.5f,
         size,
         size,
         {r, g, b, a});
}

Renderer::TextImage Renderer::MakeText(const std::wstring& text)
{
    TextImage result;
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc)
        return result;
    HFONT font = CreateFontW(-20,
                             0,
                             0,
                             0,
                             FW_MEDIUM,
                             FALSE,
                             FALSE,
                             FALSE,
                             DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY,
                             DEFAULT_PITCH,
                             L"Malgun Gothic");
    if (!font)
    {
        DeleteDC(dc);
        return result;
    }
    HGDIOBJ oldFont = SelectObject(dc, font);
    SIZE size = {};
    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
    result.width = (std::max)(1, static_cast<int>(size.cx) + 4);
    result.height = 28;
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = result.width;
    info.bmiHeader.biHeight = -result.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap && pixels)
    {
        HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
        PatBlt(dc, 0, 0, result.width, result.height, BLACKNESS);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkColor(dc, RGB(0, 0, 0));
        TextOutW(dc, 2, 2, text.c_str(), static_cast<int>(text.size()));
        GdiFlush();
        std::vector<unsigned char> alpha(result.width * result.height);
        const unsigned char* source = static_cast<unsigned char*>(pixels);
        for (size_t i = 0; i < alpha.size(); ++i)
            alpha[i] = source[i * 4 + 1];
        glGenTextures(1, &result.texture);
        glBindTexture(GL_TEXTURE_2D, result.texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_R8,
                     result.width,
                     result.height,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     alpha.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        SelectObject(dc, oldBitmap);
    }
    if (bitmap)
        DeleteObject(bitmap);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    DeleteDC(dc);
    return result;
}

void Renderer::SetUiTransform(Vec2 origin, float scaleX, float scaleY, float shearY)
{
    Flush();
    m_UiOrigin = origin;
    m_UiScaleX = scaleX;
    m_UiScaleY = scaleY;
    m_UiShearY = shearY;
}

void Renderer::Text(float x, float y, const std::wstring& text, Color c, float scale, bool centered)
{
    if (text.empty())
        return;
    Flush();
    auto found = m_Text.find(text);
    if (found == m_Text.end())
    {
        // Bound cache size even if callers later add changing text such as a timer.
        if (m_Text.size() >= 128)
        {
            for (const auto& entry : m_Text)
                glDeleteTextures(1, &entry.second.texture);
            m_Text.clear();
        }
        found = m_Text.emplace(text, MakeText(text)).first;
    }
    const TextImage& t = found->second;
    if (!t.texture)
        return;
    float w = static_cast<float>(t.width) * scale, h = static_cast<float>(t.height) * scale;
    if (centered)
    {
        x -= w * 0.5f;
    }
    Vertex v[] = {{x, y, 0, 0, c.r, c.g, c.b, c.a},
                  {x + w, y, 1, 0, c.r, c.g, c.b, c.a},
                  {x + w, y + h, 1, 1, c.r, c.g, c.b, c.a},
                  {x, y, 0, 0, c.r, c.g, c.b, c.a},
                  {x + w, y + h, 1, 1, c.r, c.g, c.b, c.a},
                  {x, y + h, 0, 1, c.r, c.g, c.b, c.a}};
    Submit(v, 6, t.texture);
}

std::vector<std::wstring> Renderer::WrapText(const std::wstring& text, float width, float scale)
{
    // Measure using the same font as MakeText; cache layouts across frames.
    static std::map<std::pair<std::wstring, int>, std::vector<std::wstring>> layouts;
    int pixels = (std::max)(1, static_cast<int>(width / (std::max)(0.01f, scale)) - 4);
    const auto key = std::make_pair(text, pixels);
    auto found = layouts.find(key);
    if (found != layouts.end())
    {
        return found->second;
    }
    HDC dc = CreateCompatibleDC(nullptr);
    HFONT font = CreateFontW(-20,
                             0,
                             0,
                             0,
                             FW_MEDIUM,
                             FALSE,
                             FALSE,
                             FALSE,
                             DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY,
                             DEFAULT_PITCH,
                             L"Malgun Gothic");
    if (!dc || !font)
    {
        if (font)
        {
            DeleteObject(font);
        }
        if (dc)
        {
            DeleteDC(dc);
        }
        return {text};
    }
    HGDIOBJ oldFont = SelectObject(dc, font);
    std::vector<std::wstring> rows;
    for (size_t start = 0; start < text.size();)
    {
        size_t end = text.find(L'\n', start);
        if (end == std::wstring::npos)
        {
            end = text.size();
        }
        int fit = 0;
        SIZE size{};
        GetTextExtentExPointW(
            dc, text.c_str() + start, static_cast<int>(end - start), pixels, &fit, nullptr, &size);
        size_t count = (std::min)(end - start, static_cast<size_t>((std::max)(1, fit)));
        if (start + count < end)
        {
            size_t space = text.rfind(L' ', start + count);
            if (space != std::wstring::npos && space > start + count / 2)
            {
                count = space - start;
            }
        }
        rows.push_back(text.substr(start, count));
        start += count;
        if (start == end && end < text.size())
        {
            ++start;
        }
        while (start < text.size() && text[start] == L' ')
        {
            ++start;
        }
    }
    SelectObject(dc, oldFont);
    DeleteObject(font);
    DeleteDC(dc);
    if (layouts.size() >= 32)
    {
        layouts.clear();
    }
    layouts.emplace(key, rows);
    return rows;
}