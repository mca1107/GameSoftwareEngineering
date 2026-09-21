#pragma once
#include <string>
#include <vector>
#include <map>
#include "Dependencies/glew.h"

struct Vec2
{
    float x, y;

    Vec2(float px = 0, float py = 0) : x(px), y(py)
    {
    }
};

struct Color
{
    float r, g, b, a;

    Color(float pr = 1, float pg = 1, float pb = 1, float pa = 1) : r(pr), g(pg), b(pb), a(pa)
    {
    }
};

// Existing shader/VBO renderer extended with batched screen-space primitives.
class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool IsInitialized() const
    {
        return m_Initialized;
    }

    void Resize(int width, int height);
    void Begin(float time = 0);
    enum class Model
    {
        Wall,
        Creature,
        Crate,
        Plant,
        Stone,
        Pipe,
        Rose,
        CreatureStride,
        CreatureUpright,
        CreatureUprightStride,
        Count
    };
    void DrawModel(Model model, Vec2 origin, float scale = 1, float angle = 0, float opacity = 1);
    void AggressiveCreature(
        Vec2 origin, float scale, float upright, float walkPhase, bool faceLeft, bool aggressive);
    bool BeginShadows();
    void EndShadows();
    void FinishWorld(float time);
    void Surface(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color, int material, float u, float v);
    void Character(
        Vec2 feet, float scale, int direction, int frame, bool caretaker, Color tint = Color());
    void Flush();
    void Triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
    void Quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color);
    void Rect(float x, float y, float width, float height, Color color);
    void Ellipse(Vec2 center, float rx, float ry, Color color);
    void Line(Vec2 a, Vec2 b, float width, Color color);
    void Text(float x,
              float y,
              const std::wstring& text,
              Color color = Color(),
              float scale = 1,
              bool centered = false);
    std::vector<std::wstring> WrapText(const std::wstring& text, float width, float scale);
    // Flush before changing the transform; menu pages use this for hinge animation.
    void SetUiTransform(Vec2 origin = Vec2(), float scaleX = 1, float scaleY = 1, float shearY = 0);
    void DrawSolidRect(float x, float y, float z, float size, float r, float g, float b, float a);

private:
    struct Vertex
    {
        float x, y, u, v, r, g, b, a;
        float material = 0;
    };

    struct TextImage
    {
        GLuint texture = 0;
        int width = 0, height = 0;
    };

    GLuint CompileShaders(const wchar_t* vertex = L"SolidRect.vs",
                          const wchar_t* fragment = L"SolidRect.fs");
    TextImage MakeText(const std::wstring& text);
    void Submit(const Vertex* vertices, size_t count, GLuint texture, int textureMode = 1);
    void CreateTargets();
    void DeleteTargets();
    void PostPass(
        GLuint target, GLuint source, int mode, float dx = 0, float dy = 0, float time = 0);
    void CreateCharacterAtlas();
    void CreateModels();
    std::vector<Vertex> m_Models[static_cast<int>(Model::Count)];
    float m_Time = 0;
    Vec2 m_UiOrigin;
    float m_UiScaleX = 1, m_UiScaleY = 1, m_UiShearY = 0;
    std::vector<Vertex> m_UiVertices;
    GLuint m_PostProgram = 0;
    GLuint m_Targets[4] = {}, m_Images[4] = {};
    GLuint m_CharacterAtlas = 0;
    bool m_EffectsReady = false;
    bool m_Initialized = false;
    int m_Width = 1, m_Height = 1;
    GLuint m_Program = 0, m_VBO = 0, m_VAO = 0;
    GLint m_Viewport = -1, m_Textured = -1;
    std::vector<Vertex> m_Vertices;
    std::map<std::wstring, TextImage> m_Text;
};
