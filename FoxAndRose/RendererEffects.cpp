#include "stdafx.h"
#include "Renderer.h"
#include "AssetCache.h"
#include <algorithm>
#include <cmath>
#include <iostream>

void Renderer::DeleteTargets()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(4, m_Targets);
    glDeleteTextures(4, m_Images);
    for (int i = 0; i < 4; ++i)
    {
        m_Targets[i] = 0;
        m_Images[i] = 0;
    }
    m_EffectsReady = false;
}

void Renderer::CreateTargets()
{
    DeleteTargets();
    if (!m_PostProgram)
        return;
    glGenFramebuffers(4, m_Targets);
    glGenTextures(4, m_Images);
    for (int i = 0; i < 4; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_Images[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Targets[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Images[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Post-processing target unavailable; using direct rendering.\n";
            DeleteTargets();
            return;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_EffectsReady = true;
}

void Renderer::PostPass(GLuint target, GLuint source, int mode, float dx, float dy, float time)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glViewport(0, 0, m_Width, m_Height);
    glUseProgram(m_PostProgram);
    glBindVertexArray(m_VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mode == 3 ? m_Images[3] : 0);
    glUniform1i(glGetUniformLocation(m_PostProgram, "u_Source"), 0);
    glUniform1i(glGetUniformLocation(m_PostProgram, "u_Bloom"), 1);
    glUniform1i(glGetUniformLocation(m_PostProgram, "u_Mode"), mode);
    glUniform2f(glGetUniformLocation(m_PostProgram, "u_Direction"), dx, dy);
    glUniform1f(glGetUniformLocation(m_PostProgram, "u_Time"), time);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glActiveTexture(GL_TEXTURE0);
}

bool Renderer::BeginShadows()
{
    Flush();
    if (!m_EffectsReady)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, m_Targets[1]);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Overlapping projected silhouettes form a union, instead of becoming progressively black.
    glBlendEquation(GL_MAX);
    return true;
}

void Renderer::EndShadows()
{
    if (!m_EffectsReady)
        return;
    Flush();
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_BLEND);
    PostPass(m_Targets[2], m_Images[1], 0, 1.6f / m_Width, 0);
    PostPass(m_Targets[3], m_Images[2], 0, 0, 1.6f / m_Height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    PostPass(m_Targets[0], m_Images[3], 1);
}

void Renderer::FinishWorld(float time)
{
    Flush();
    if (m_EffectsReady)
    {
        glDisable(GL_BLEND);
        PostPass(m_Targets[1], m_Images[0], 2);
        PostPass(m_Targets[2], m_Images[1], 0, 3.0f / m_Width, 0);
        PostPass(m_Targets[3], m_Images[2], 0, 0, 3.0f / m_Height);
        PostPass(0, m_Images[0], 3, 0, 0, time);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::Surface(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color t, int material, float u, float v)
{
    const float m = static_cast<float>(material);
    Vertex vertices[] = {{a.x, a.y, 0, 0, t.r, t.g, t.b, t.a, m},
                         {b.x, b.y, u, 0, t.r, t.g, t.b, t.a, m},
                         {c.x, c.y, u, v, t.r, t.g, t.b, t.a, m},
                         {a.x, a.y, 0, 0, t.r, t.g, t.b, t.a, m},
                         {c.x, c.y, u, v, t.r, t.g, t.b, t.a, m},
                         {d.x, d.y, 0, v, t.r, t.g, t.b, t.a, m}};
    m_Vertices.insert(m_Vertices.end(), vertices, vertices + 6);
}

void Renderer::CreateCharacterAtlas()
{
    // Bake a reusable RGBA sprite sheet once: 8 frames x 4 directions x 2 outfits.
    // This remains replaceable by authored art without changing the animation state machine.
    const int oldWidth = m_Width, oldHeight = m_Height;
    m_Width = 512;
    m_Height = 768;
    constexpr unsigned CharacterVersion = 7;
    std::vector<unsigned char> cachedPixels;
    if (AssetCache::Load(L"fox-atlas.bin", CharacterVersion, 512 * 768 * 4, cachedPixels))
    {
        glGenTextures(1, &m_CharacterAtlas);
        glBindTexture(GL_TEXTURE_2D, m_CharacterAtlas);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     512,
                     768,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     cachedPixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_Width = oldWidth;
        m_Height = oldHeight;
        return;
    }
    // Increment the version when procedural character art or atlas dimensions change.
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &m_CharacterAtlas);
    glBindTexture(GL_TEXTURE_2D, m_CharacterAtlas);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_CharacterAtlas, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        glViewport(0, 0, m_Width, m_Height);
        glDisable(GL_BLEND);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        for (int outfit = 0; outfit < 2; ++outfit)
            for (int direction = 0; direction < 4; ++direction)
                for (int frame = 0; frame < 8; ++frame)
                {
                    float phase = frame == 0 ? 0 : (frame - 1) * 6.2831853f / 7;
                    float stride = frame == 0 ? 0 : std::sin(phase) * 6;
                    float x = frame * 64.0f + 32, y = (outfit * 4 + direction) * 96.0f + 85;
                    float bob = frame == 0 ? 0 : std::abs(std::cos(phase)) * 1.5f;
                    Color dark{0.09f, 0.13f, 0.15f}, skin{0.64f, 0.51f, 0.39f};
                    Color coat = outfit ? Color(0.32f, 0.49f, 0.48f) : Color(0.62f, 0.76f, 0.35f);
                    const Color fur{0.76f, 0.32f, 0.12f}, furLight{0.94f, 0.48f, 0.19f};
                    const Color tip{0.12f, 0.09f, 0.085f}, scarf{0.73f, 0.12f, 0.12f};
                    const bool side = direction == 1 || direction == 2;
                    float facing = direction == 1 ? -1.0f : 1.0f;
                    auto drawTail = [&](bool back)
                    {
                        // A curved, tapered oval stays within the atlas cell in every walk frame.
                        float tailSide = side ? -facing : 1.0f;
                        float sway = stride * 0.25f;
                        auto tailPoint = [&](float t, float edge)
                        {
                            float curve = std::sin(t * 3.14159265f);
                            float radius = (back ? 6.0f : 8.0f) * curve;
                            if (back)
                            {
                                return Vec2{x + sway * t + edge * radius, y - 29 + 25 * t};
                            }
                            return Vec2{x + tailSide * (5 + 21 * t + edge * radius * 0.55f),
                                        y - bob - 29 - 14 * t + (7 + sway) * curve +
                                            edge * radius * 0.83f};
                        };

                        for (int segment = 0; segment < 24; ++segment)
                        {
                            float start = segment / 24.0f;
                            float end = (segment + 1) / 24.0f;
                            Color shade = segment >= 21 ? tip : fur;
                            Quad(tailPoint(start, -1),
                                 tailPoint(end, -1),
                                 tailPoint(end, 1),
                                 tailPoint(start, 1),
                                 shade);

                            if (segment < 21)
                            {
                                Quad(tailPoint(start, -0.65f),
                                     tailPoint(end, -0.65f),
                                     tailPoint(end, 0),
                                     tailPoint(start, 0),
                                     furLight);
                            }
                        }
                    };

                    if (!outfit && side)
                    {
                        drawTail(false);
                    }

                    if (!outfit && side)
                    {
                        // Profile legs overlap at the hip and stride along the facing axis.
                        for (int leg = 0; leg < 2; ++leg)
                        {
                            float sign = leg == 0 ? -1.0f : 1.0f;
                            float swing = stride * sign * facing;
                            Vec2 hip{x + sign * 2 * facing, y - 26};
                            Vec2 knee{x + swing * 0.55f, y - 13};
                            Vec2 foot{x + swing, y - 2};
                            Color trousers =
                                leg == 0 ? Color(0.18f, 0.23f, 0.16f) : Color(0.29f, 0.36f, 0.21f);
                            Line(hip, knee, 7, trousers);
                            Line(knee, foot, 6, trousers);
                            Ellipse({foot.x + facing * 2, foot.y}, 5, 2.5f, dark);
                        }
                        y -= bob;
                        // Far sleeve, narrow torso, then near sleeve establish side depth.
                        Line({x + facing * 2, y - 49},
                             {x + facing * (5 - stride * 0.5f), y - 31},
                             6,
                             {0.42f, 0.54f, 0.24f});
                        Quad({x - facing * 6, y - 53},
                             {x + facing * 5, y - 51},
                             {x + facing * 8, y - 27},
                             {x - facing * 7, y - 27},
                             dark);
                        Quad({x - facing * 5, y - 52},
                             {x + facing * 4, y - 50},
                             {x + facing * 7, y - 29},
                             {x - facing * 6, y - 29},
                             coat);
                        Line({x - facing * 4, y - 30},
                             {x + facing * 6, y - 30},
                             3,
                             {0.39f, 0.49f, 0.23f});
                        Vec2 elbow{x - facing * 2 + facing * stride * 0.5f, y - 39};
                        Vec2 hand{x + facing * stride * 0.7f, y - 30};
                        Line({x - facing * 2, y - 49}, elbow, 8, coat);
                        Line(elbow, hand, 7, coat);
                        Ellipse(hand, 3, 4, dark);
                        Ellipse({x - facing * 2, y - 49}, 3, 3, {0.75f, 0.85f, 0.47f});
                    }
                    else
                    {
                        for (int leg = 0; leg < 2; ++leg)
                        {
                            float sign = leg == 0 ? -1.0f : 1.0f;
                            Vec2 hip{x + sign * 5, y - 26},
                                knee{x + sign * 5 + stride * sign * 0.4f, y - 13};
                            Vec2 foot{x + sign * 5 + stride * sign, y - 2};
                            Line(hip, knee, 7, dark);
                            Line(knee, foot, 6, {0.18f, 0.22f, 0.22f});
                            Rect(foot.x - 4, foot.y - 2, 9, 4, {0.07f, 0.09f, 0.1f});
                            Rect(knee.x - 3, knee.y - 2, 6, 4, {0.31f, 0.34f, 0.3f});
                        }
                        y -= bob;
                        Quad({x - 10, y - 53},
                             {x + 10, y - 53},
                             {x + 12, y - 26},
                             {x - 11, y - 26},
                             dark);
                        Quad({x - 8, y - 52},
                             {x + 8, y - 52},
                             {x + 10, y - 29},
                             {x - 9, y - 29},
                             coat);
                        Rect(x - 8, y - 32, 17, 4, {0.19f, 0.22f, 0.18f});
                        for (int arm = 0; arm < 2; ++arm)
                        {
                            float sign = arm == 0 ? -1.0f : 1.0f;
                            Vec2 shoulder{x + sign * 10, y - 49},
                                elbow{x + sign * 13, y - 39 - stride * sign * 0.4f};
                            Vec2 hand{x + sign * 12, y - 30 - stride * sign * 0.6f};
                            Line(shoulder, elbow, outfit ? 6.0f : 9.0f, coat);
                            Line(elbow, hand, outfit ? 5.0f : 8.0f, coat);
                            Ellipse(hand, 3, 4, dark);
                            Rect(shoulder.x - 3, shoulder.y - 2, 6, 5, {0.4f, 0.43f, 0.37f});
                        }
                    }
                    Ellipse({x, y - 61}, 8, 10, outfit ? skin : fur);
                    if (outfit)
                    {
                        Ellipse({x, y - 67}, 9, 5, {0.19f, 0.23f, 0.22f});
                    }
                    if (!outfit)
                    {
                        auto drawEar = [&](float center,
                                           float halfWidth,
                                           float height,
                                           float lean,
                                           bool inner,
                                           Color color)
                        {
                            Vec2 left{x + center - halfWidth, y - 64};
                            Vec2 right{x + center + halfWidth, y - 64};
                            Vec2 peak{x + center + lean, y - 64 - height};
                            Triangle(left, right, peak, color);

                            if (inner)
                            {
                                Triangle({x + center - halfWidth * 0.45f, y - 66},
                                         {x + center + halfWidth * 0.45f, y - 66},
                                         {peak.x, peak.y + 4},
                                         {0.9f, 0.58f, 0.4f});
                            }

                            // Follow the ear edges so the dark tip remains pointed in each view.
                            Triangle(peak,
                                     {peak.x + (left.x - peak.x) * 0.24f, peak.y + height * 0.24f},
                                     {peak.x + (right.x - peak.x) * 0.24f, peak.y + height * 0.24f},
                                     tip);
                        };

                        if (side)
                        {
                            // The far ear is smaller and darker; the near ear defines the profile.
                            drawEar(
                                facing * 4, 2.8f, 11, facing * 1.5f, false, {0.49f, 0.23f, 0.12f});
                            drawEar(-facing * 3, 4.5f, 14, facing * 2, true, fur);
                        }
                        else
                        {
                            for (int ear = -1; ear <= 1; ear += 2)
                            {
                                // The back view shows outer fur, never the pink inner surface.
                                drawEar(ear * 6.5f,
                                        4.5f,
                                        14,
                                        ear * 2.0f,
                                        direction != 3,
                                        direction == 3 ? Color(0.62f, 0.28f, 0.12f) : fur);
                            }
                        }

                        // Uncovered fox head; hair remains hidden in every direction.
                    }
                    if (!outfit && direction != 3)
                    {
                        const Color muzzle(0.95f, 0.89f, 0.73f);
                        if (side)
                        {
                            Ellipse({x + facing * 7, y - 57}, 5, 3.5f, muzzle);
                            Triangle({x + facing * 8, y - 60},
                                     {x + facing * 16, y - 57},
                                     {x + facing * 8, y - 54},
                                     muzzle);
                            Ellipse({x + facing * 15, y - 57.5f}, 2, 1.6f, tip);
                        }
                        else
                        {
                            Ellipse({x - 3, y - 56.5f}, 4.5f, 3.5f, muzzle);
                            Ellipse({x + 3, y - 56.5f}, 4.5f, 3.5f, muzzle);
                            Ellipse({x, y - 57.5f}, 2.2f, 1.6f, tip);
                        }
                    }
                    if (direction != 3)
                    {
                        float eyeX = x + (side ? facing * 5 : 0);
                        if (outfit)
                        {
                            Rect(eyeX - 6, y - 63, side ? 7.0f : 12.0f, 4, dark);
                            Rect(eyeX - 4, y - 62, side ? 4.0f : 8.0f, 1, {0.65f, 0.85f, 0.8f});
                            Rect(x - 4 + (side ? facing * 4 : 0),
                                 y - 57,
                                 8,
                                 5,
                                 {0.28f, 0.34f, 0.32f});
                        }
                        else
                        {
                            auto drawEye = [&](float eyeX)
                            {
                                Ellipse({eyeX, y - 63}, 2.3f, 1.8f, {0.91f, 0.64f, 0.23f});
                                Ellipse({eyeX, y - 63}, 0.65f, 1.6f, tip);
                                Ellipse({eyeX - 0.6f, y - 63.6f}, 0.6f, 0.6f, {1, 0.95f, 0.8f});
                            };
                            drawEye(x + (side ? facing * 3 : -3.5f));
                            if (!side)
                            {
                                drawEye(x + 3.5f);
                            }
                        }
                    }
                    if (!outfit && side)
                    {
                        Line({x - facing * 4, y - 51},
                             {x + facing * 4, y - 46},
                             4,
                             {0.78f, 0.63f, 0.3f});
                    }
                    else
                    {
                        Line({x - 8, y - 51}, {x + 7, y - 46}, 4, {0.78f, 0.63f, 0.3f});
                    }
                    if (direction == 3)
                    {
                        Rect(x - 8, y - 51, 16, 21, {0.25f, 0.31f, 0.24f});
                        Rect(x - 6, y - 47, 12, 5, {0.37f, 0.42f, 0.31f});
                        Rect(x - 5, y - 37, 10, 5, {0.19f, 0.24f, 0.2f});
                    }
                    else
                    {
                        float packX = x + (side ? -facing * 10 : -11);
                        Rect(packX - 3, y - 48, 6, 16, {0.25f, 0.31f, 0.24f});
                        Rect(x + (!outfit && side ? facing * 5 - 1 : 3),
                             y - 42,
                             !outfit && side ? 2.0f : 4.0f,
                             5,
                             outfit ? Color(0.4f, 0.43f, 0.37f) : Color(0.72f, 0.85f, 0.46f));
                        Line({x + (!outfit && side ? facing * 6 : -1), y - 47},
                             {x + (!outfit && side ? facing * 6 : -1), y - 33},
                             1,
                             {0.77f, 0.64f, 0.44f});
                    }
                    if (!outfit)
                    {
                        // Hair remains part of the character setting, but is hidden in game.
                        float trailing = side ? -facing : -1.0f;
                        float scarfWidth = side ? 6.0f : 9.0f;
                        Quad({x - scarfWidth, y - 54},
                             {x + scarfWidth, y - 54},
                             {x + scarfWidth - 1, y - 48},
                             {x - scarfWidth + 1, y - 49},
                             scarf);
                        Line({x + trailing * 7, y - 50},
                             {x + trailing * 17 + stride * 0.3f, y - 39},
                             5,
                             scarf);
                    }

                    if (!outfit && direction == 3)
                    {
                        // Rear tail grows from the lower back, over the legs and below the pack.
                        drawTail(true);
                    }
                }
        Flush();
        cachedPixels.resize(512 * 768 * 4);
        glReadPixels(0, 0, 512, 768, GL_RGBA, GL_UNSIGNED_BYTE, cachedPixels.data());
        AssetCache::Save(
            L"fox-atlas.bin", CharacterVersion, cachedPixels.data(), cachedPixels.size());
    }
    else
    {
        std::cerr << "Character sprite atlas allocation failed.\n";
        m_Initialized = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    m_Width = oldWidth;
    m_Height = oldHeight;
    glViewport(0, 0, m_Width, m_Height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::Character(
    Vec2 feet, float scale, int direction, int frame, bool caretaker, Color tint)
{
    Flush();
    direction = (std::max)(0, (std::min)(3, direction));
    frame = (std::max)(0, (std::min)(7, frame));
    float x = feet.x - 32 * scale, y = feet.y - 85 * scale, w = 64 * scale, h = 96 * scale;
    float u = frame / 8.0f, uu = (frame + 1) / 8.0f;
    int row = (caretaker ? 4 : 0) + direction;
    float v = 1 - row / 8.0f, vv = 1 - (row + 1) / 8.0f;
    Vertex vertices[] = {{x, y, u, v, 1, 1, 1, 1},
                         {x + w, y, uu, v, 1, 1, 1, 1},
                         {x + w, y + h, uu, vv, 1, 1, 1, 1},
                         {x, y, u, v, 1, 1, 1, 1},
                         {x + w, y + h, uu, vv, 1, 1, 1, 1},
                         {x, y + h, u, vv, 1, 1, 1, 1}};
    for (Vertex& vertex : vertices)
    {
        vertex.r = tint.r;
        vertex.g = tint.g;
        vertex.b = tint.b;
        vertex.a = tint.a;
    }
    Submit(vertices, 6, m_CharacterAtlas, 2);
}
