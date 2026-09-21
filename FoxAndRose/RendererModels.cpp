#include "stdafx.h"
#include "Renderer.h"
#include "AssetCache.h"
#include <cmath>
#include <cstring>
#include <string>

void Renderer::CreateModels()
{
    constexpr unsigned ModelVersion = 6;
    // Cache local vertices, so primitive tessellation happens only on a cache miss.
    for (int index = 0; index < static_cast<int>(Model::Count); ++index)
    {
        std::wstring name = L"model-" + std::to_wstring(index) + L".bin";
        std::vector<unsigned char> bytes;
        auto& model = m_Models[index];
        if (AssetCache::Load(name.c_str(), ModelVersion, 0, bytes) &&
            bytes.size() % (3 * sizeof(Vertex)) == 0)
        {
            model.resize(bytes.size() / sizeof(Vertex));
            std::memcpy(model.data(), bytes.data(), bytes.size());
            bool valid = true;
            for (const Vertex& vertex : model)
            {
                const float values[] = {vertex.x,
                                        vertex.y,
                                        vertex.u,
                                        vertex.v,
                                        vertex.r,
                                        vertex.g,
                                        vertex.b,
                                        vertex.a,
                                        vertex.material};
                for (float value : values)
                {
                    if (!std::isfinite(value) || std::abs(value) > 1000)
                        valid = false;
                }
            }
            if (valid)
                continue;
            model.clear();
        }

        m_Vertices.clear();
        switch (static_cast<Model>(index))
        {
        case Model::Wall:
            Surface({0, 14}, {28, 0}, {28, -30}, {0, -16}, {0.28f, 0.33f, 0.3f}, 1, 1, 1);
            Surface({-28, 0}, {0, 14}, {0, -16}, {-28, -30}, {0.39f, 0.43f, 0.36f}, 1, 1, 1);
            Surface({-28, -30}, {0, -44}, {28, -30}, {0, -16}, {0.5f, 0.53f, 0.43f}, 1, 1, 1);
            break;
        case Model::Creature:
        case Model::CreatureStride:
        case Model::CreatureUpright:
        case Model::CreatureUprightStride:
        {
            const Model pose = static_cast<Model>(index);
            const bool upright =
                pose == Model::CreatureUpright || pose == Model::CreatureUprightStride;
            const float stride =
                pose == Model::CreatureStride || pose == Model::CreatureUprightStride ? 1.0f
                                                                                      : -1.0f;
            const Color skin{0.26f, 0.29f, 0.25f};
            const Color pale{0.43f, 0.45f, 0.37f};
            const Color metal{0.34f, 0.39f, 0.38f};
            const Color seam{0.10f, 0.14f, 0.13f};
            const float shoulderY = upright ? -63.0f : -42.0f;
            // Identical topology allows smooth cached-vertex interpolation.
            for (int side = -1; side <= 1; side += 2)
            {
                float step = stride * side * 4;
                Vec2 hip{side * 8.0f - 5, -25};
                Vec2 knee{side * 10.0f - 7 + step, -12};
                Vec2 foot{side * 10.0f - 4 + step, -3};
                Line(hip, knee, 12, seam);
                Line(knee, foot, 9, skin);
                Ellipse(foot, 8, 3, seam);
                if (side > 0)
                {
                    Line(knee,
                         {knee.x + (foot.x - knee.x) * 0.4f, knee.y + (foot.y - knee.y) * 0.4f},
                         4,
                         metal);
                }
            }
            Ellipse({-5, shoulderY + 16}, 17, upright ? 25.0f : 18.0f, skin);
            Ellipse({-2, shoulderY + 4}, 23, 16, skin);
            Ellipse({-7, shoulderY}, 16, 10, pale);
            for (int side = -1; side <= 1; side += 2)
            {
                float step = stride * side * 4;
                Vec2 shoulder{side * 17.0f, shoulderY + 2};
                Vec2 elbow{side * 23.0f + step, upright ? -39.0f : -23.0f};
                Vec2 hand{side * 27.0f + step, upright ? -20.0f : -4.0f};
                Line(shoulder, elbow, 15, skin);
                Line(elbow, hand, 12, pale);
                Ellipse(hand, 8, 5, seam);
                for (int finger = 0; finger < 3; ++finger)
                {
                    Line({hand.x - 4 + finger * 4, hand.y},
                         {hand.x - 3 + finger * 4, hand.y + 4},
                         2,
                         pale);
                }
                if (side < 0)
                {
                    // Only a small forearm patch remains fused with metal.
                    Line(elbow,
                         {elbow.x + (hand.x - elbow.x) * 0.45f,
                          elbow.y + (hand.y - elbow.y) * 0.45f},
                         6,
                         metal);
                    Ellipse(elbow, 4, 3, skin);
                }
            }
            Ellipse({6, shoulderY - 1}, 11, 12, skin);
            Ellipse({9, shoulderY + 4}, 9, 6, pale);
            Ellipse({0, shoulderY + 1}, 3, 4, metal);
            Line({-2, shoulderY + 4}, {2, shoulderY + 5}, 3, skin);
            Line({1, shoulderY - 4}, {14, shoulderY - 3}, 5, seam);
            Ellipse({5, shoulderY - 2}, 1.5f, 1, {0.83f, 0.48f, 0.17f});
            Ellipse({12, shoulderY - 2}, 1.5f, 1, {0.83f, 0.48f, 0.17f});
            Ellipse({13, shoulderY + 3}, 3, 2, seam);
            Line({5, shoulderY + 8}, {13, shoulderY + 8}, 2, seam);
            break;
        }
        case Model::Crate:
            Rect(-9, -15, 18, 14, {0.4f, 0.32f, 0.18f});
            Rect(-8, -17, 16, 5, {0.63f, 0.53f, 0.29f});
            Rect(-6, -14, 2, 13, {0.19f, 0.24f, 0.21f});
            Rect(4, -14, 2, 13, {0.19f, 0.24f, 0.21f});
            break;
        case Model::Plant:
            for (int leaf = 0; leaf < 5; ++leaf)
                Triangle({-2, 0},
                         {std::cos(leaf * 1.4f) * 13, -8 - std::abs(std::sin(leaf * 1.4f)) * 19},
                         {3, 0},
                         {0.19f, 0.36f + leaf * 0.02f, 0.18f});
            break;
        case Model::Stone:
            Ellipse({0, 0}, 4, 3, {0.58f, 0.6f, 0.51f});
            break;
        case Model::Pipe:
            Line({0, 0}, {39, 0}, 4, {0.24f, 0.27f, 0.26f});
            Line({4, -1}, {39, -1}, 2, {0.7f, 0.73f, 0.68f});
            Rect(1, -3, 7, 6, {0.29f, 0.18f, 0.12f});
            break;
        case Model::Rose:
        {
            const Color fur(0.055f, 0.065f, 0.085f);
            const Color furLight(0.16f, 0.18f, 0.21f);
            const Color hoodie(0.47f, 0.31f, 0.36f);
            const Color clothShadow(0.29f, 0.19f, 0.25f);
            const Color clothLight(0.61f, 0.44f, 0.47f);
            const Color pants(0.19f, 0.23f, 0.27f);
            // Curved cat tail behind the loose clothing.
            Vec2 last{-8, -17};
            for (int i = 1; i <= 18; ++i)
            {
                float t = i / 18.0f;
                Vec2 next{-8 - 17 * std::sin(t * 1.9f), -17 - 15 * t * t};
                Line(last, next, 4.5f - t, fur);
                Ellipse(next, 2.2f - t * 0.5f, 2.2f - t * 0.5f, fur);
                last = next;
            }
            // Baggy tracksuit legs, side stripes, gathered cuffs and shoes.
            for (int side = -1; side <= 1; side += 2)
            {
                float x = side * 5.5f;
                Ellipse({x, -20}, 5.5f, 10, pants);
                Rect(x - 4.5f, -22, 9, 16, pants);
                Line({x + side * 3, -24}, {x + side * 3, -9}, 1.3f, {0.56f, 0.60f, 0.59f});
                Line({x - 2, -15}, {x + 2, -17}, 1, {0.12f, 0.16f, 0.19f});
                Rect(x - 4, -8, 8, 4, {0.12f, 0.15f, 0.18f});
                Ellipse({x + side, -2}, 6, 3.5f, fur);
                Rect(x - 5, -1, 11, 2, {0.39f, 0.42f, 0.42f});
            }
            // Dropped hood behind the head leaves the ears visible.
            Ellipse({0, -53}, 14, 10, clothShadow);
            Ellipse({0, -54}, 11, 7, clothLight);
            Ellipse({0, -53}, 8, 5, clothShadow);
            Quad({-11, -51}, {11, -51}, {13, -27}, {-13, -27}, clothShadow);
            Quad({-9, -51}, {9, -51}, {11, -29}, {-11, -29}, hoodie);
            Ellipse({0, -31}, 11, 5, hoodie);
            Rect(-11, -29, 22, 4, clothShadow);
            Line({-8, -27}, {8, -27}, 1, clothLight);
            // Rounded oversized sleeves, ribbed cuffs and black paws.
            for (int side = -1; side <= 1; side += 2)
            {
                Vec2 shoulder{side * 10.0f, -47}, elbow{side * 14.0f, -39};
                Vec2 wrist{side * 13.0f, -30};
                Line(shoulder, elbow, 9, clothShadow);
                Line(elbow, wrist, 8, hoodie);
                Ellipse(shoulder, 4.5f, 5, hoodie);
                Ellipse(elbow, 4, 4.5f, hoodie);
                Line({shoulder.x - 1, -48}, {elbow.x - 1, -40}, 2, clothLight);
                Rect(wrist.x - 4, -32, 8, 4, clothShadow);
                Ellipse({wrist.x, -26}, 3, 4, fur);
            }
            // Kangaroo pocket with stitched openings and two hood drawstrings.
            Quad({-6, -36}, {6, -36}, {8, -30}, {-8, -30}, clothShadow);
            Line({-6, -36}, {-8, -31}, 1, clothLight);
            Line({6, -36}, {8, -31}, 1, clothLight);
            Line({-5, -50}, {-5.5f, -43}, 1.2f, {0.82f, 0.76f, 0.66f});
            Line({5, -50}, {5.5f, -43}, 1.2f, {0.82f, 0.76f, 0.66f});
            // A readable embroidered rose motif on the center of the hoodie.
            Line({0, -42}, {0, -37}, 1.2f, {0.34f, 0.51f, 0.33f});
            Triangle({0, -38}, {-4, -40}, {-2, -37}, {0.42f, 0.59f, 0.36f});
            for (int petal = 0; petal < 5; ++petal)
            {
                float angle = petal * 6.2831853f / 5;
                Ellipse({std::cos(angle) * 2, -43 + std::sin(angle) * 2},
                        2,
                        1.8f,
                        {0.80f, 0.24f, 0.31f});
            }
            Ellipse({0, -43}, 1.6f, 1.5f, {0.42f, 0.08f, 0.17f});
            // Cat ears, layered cheek fur and amber eyes match the fox's detail scale.
            Triangle({-10, -65}, {-10, -81}, {-1, -70}, furLight);
            Triangle({1, -70}, {10, -81}, {10, -65}, fur);
            Triangle({-8, -69}, {-8, -77}, {-4, -71}, {0.42f, 0.27f, 0.32f});
            Triangle({4, -71}, {8, -77}, {8, -69}, {0.34f, 0.22f, 0.28f});
            Ellipse({0, -62}, 10, 11, fur);
            Ellipse({-2, -63}, 7.5f, 9, {0.10f, 0.12f, 0.15f});
            Triangle({-8, -63}, {-12, -57}, {-5, -55}, furLight);
            Triangle({8, -63}, {12, -57}, {5, -55}, fur);
            for (int side = -1; side <= 1; side += 2)
            {
                Ellipse({side * 4.0f, -63}, 2.5f, 1.7f, {0.86f, 0.73f, 0.34f});
                Ellipse({side * 4.0f, -63}, 0.65f, 1.5f, fur);
                Ellipse({side * 4.0f - 0.7f, -63.5f}, 0.6f, 0.6f, {0.98f, 0.93f, 0.77f});
            }
            break;
        }
        default:
            break;
        }
        model.swap(m_Vertices);
        AssetCache::Save(name.c_str(), ModelVersion, model.data(), model.size() * sizeof(Vertex));
    }
    m_Vertices.clear();
}

void Renderer::DrawModel(Model model, Vec2 origin, float scale, float angle, float opacity)
{
    int index = static_cast<int>(model);
    if (index < 0 || index >= static_cast<int>(Model::Count))
        return;
    float cosine = std::cos(angle), sine = std::sin(angle);
    for (Vertex vertex : m_Models[index])
    {
        vertex.a *= opacity;
        float x = vertex.x, y = vertex.y;
        vertex.x = origin.x + (x * cosine - y * sine) * scale;
        vertex.y = origin.y + (x * sine + y * cosine) * scale;
        m_Vertices.push_back(vertex);
    }
}

void Renderer::AggressiveCreature(
    Vec2 origin, float scale, float upright, float walkPhase, bool faceLeft, bool aggressive)
{
    float rise = (std::max)(0.0f, (std::min)(1.0f, upright));
    rise = rise * rise * (3 - 2 * rise);
    float step = 0.5f + 0.5f * std::sin(walkPhase);
    const auto& lowA = m_Models[static_cast<int>(Model::Creature)];
    const auto& lowB = m_Models[static_cast<int>(Model::CreatureStride)];
    const auto& highA = m_Models[static_cast<int>(Model::CreatureUpright)];
    const auto& highB = m_Models[static_cast<int>(Model::CreatureUprightStride)];
    Ellipse(origin, 30 * scale, 8 * scale, {0.02f, 0.03f, 0.025f, 0.3f});
    if (lowA.size() != lowB.size() || lowA.size() != highA.size() || lowA.size() != highB.size())
    {
        DrawModel(Model::Creature, origin, scale);
        return;
    }
    for (size_t i = 0; i < lowA.size(); ++i)
    {
        Vertex vertex = lowA[i];
        // The cached iris color is unique to the two eyes in these four poses.
        if (aggressive && vertex.r == 0.83f && vertex.g == 0.48f && vertex.b == 0.17f)
        {
            vertex.r = 1.5f;
            vertex.g = 0.08f;
            vertex.b = 0.035f;
        }
        float lowX = lowA[i].x + (lowB[i].x - lowA[i].x) * step;
        float lowY = lowA[i].y + (lowB[i].y - lowA[i].y) * step;
        float highX = highA[i].x + (highB[i].x - highA[i].x) * step;
        float highY = highA[i].y + (highB[i].y - highA[i].y) * step;
        vertex.x = origin.x + (lowX + (highX - lowX) * rise) * scale * (faceLeft ? -1 : 1);
        vertex.y = origin.y + (lowY + (highY - lowY) * rise) * scale;
        m_Vertices.push_back(vertex);
    }
}