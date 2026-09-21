#include "stdafx.h"
#include "Chapter1.h"
#include <algorithm>
#include <cmath>

bool Chapter1::ReturnBlocked(Vec2 p) const
{
    return (m_Quest == Quest::FollowingSound || m_Quest == Quest::InspectingMap) && p.x > 10.0f &&
           p.x < 11.5f && p.y > 32.7f && p.y < 35.8f;
}

void Chapter1::DrawObjectiveMarkers(Renderer& r, bool labels)
{
    if (DialogueActive() || m_EntranceTime < 1.2f)
    {
        return;
    }

    struct Goal
    {
        Vec2 position;
        const wchar_t* label;
    };

    std::vector<Goal> goals;
    switch (m_Quest)
    {
    case Quest::Collecting:
        if (!m_HasFood)
        {
            goals.push_back({m_Food, L"식료품 얻기"});
        }
        if (!m_HasSupplies)
        {
            goals.push_back({m_Supplies, L"생필품 얻기"});
        }
        break;
    case Quest::Returning:
        goals.push_back({m_Storage, L"물자 보관하기"});
        break;
    case Quest::FollowingSound:
    case Quest::InspectingMap:
        goals.push_back({m_Speaker, L"이상한 소리 따라가기"});
        break;
    case Quest::ReturningHome:
        goals.push_back({m_Deposited ? m_Rose : m_Storage,
                         m_Deposited ? L"장미에게 말 걸기" : L"물자 보관하기"});
        break;
    case Quest::Leaving:
        goals.push_back({m_Exit, L"모험 떠나기"});
        break;
    default:
        break;
    }
    std::vector<Vec2> placed;
    for (const Goal& goal : goals)
    {
        Vec2 p = Project(goal.position.x, goal.position.y);
        bool visible = p.x >= 0 && p.x < m_Width && p.y >= 0 && p.y < m_Height;
        if (visible)
        {
            if (!labels)
            {
                bool storage = goal.position.x == m_Storage.x && goal.position.y == m_Storage.y;
                float scale = m_Scale / 30;
                float rx = (storage ? 42.0f : 22.0f) * scale;
                float ry = (storage ? 20.0f : 9.0f) * scale;
                r.Ellipse(p, rx, ry, {1, 0.81f, 0.37f, 0.18f});
                for (int i = 0; i < 40; ++i)
                {
                    float a = i * 6.2831853f / 40, b = (i + 1) * 6.2831853f / 40;
                    r.Line({p.x + std::cos(a) * rx, p.y + std::sin(a) * ry},
                           {p.x + std::cos(b) * rx, p.y + std::sin(b) * ry},
                           2 * scale,
                           {1, 0.83f, 0.28f, 0.85f});
                }
            }
            continue;
        }
        if (!labels)
        {
            continue;
        }
        Vec2 center{m_Width * 0.5f, m_Height * 0.5f};
        float dx = p.x - center.x, dy = p.y - center.y;
        float tx = std::abs(dx) > 0.001f ? (center.x - 104) / std::abs(dx) : 1e6f;
        float ty = std::abs(dy) > 0.001f ? (center.y - 30) / std::abs(dy) : 1e6f;
        float t = (std::min)(tx, ty);
        Vec2 label{center.x + dx * t, center.y + dy * t};
        // Keep edge labels clear of the minimap and each other.
        if (label.x < 338 && label.y < 250)
        {
            if (ty <= tx)
            {
                label.x = 338;
            }
            else
            {
                label.y = 250;
            }
        }
        for (Vec2 previous : placed)
        {
            if (std::abs(label.x - previous.x) < 194 && std::abs(label.y - previous.y) < 38)
            {
                label.y += label.y + 40 <= m_Height - 30 ? 40 : -40;
            }
        }
        placed.push_back(label);
        r.Text(label.x, label.y - 12, goal.label, {1, 0.86f, 0.43f}, 0.85f, true);
    }
}

void Chapter1::DrawMinimap(Renderer& r)
{
    const float x = 18, y = 24;
    r.Rect(x, y, 220, 204, {0.035f, 0.07f, 0.065f, 0.95f});
    r.Text(x + 10, y + 7, m_MapName, Color(), 0.85f);
    auto map = [&](Vec2 p)
    {
        return ProjectMinimap(p, {x, y});
    };
    auto area = [&](float bx, float by, float w, float d, Color color)
    {
        r.Quad(map({bx, by}), map({bx + w, by}), map({bx + w, by + d}), map({bx, by + d}), color);
    };
    area(1, 1, 40, 38, {0.22f, 0.29f, 0.21f});
    area(8, 5, 3, 30, {0.35f, 0.37f, 0.28f});
    area(8, 17, 32, 3, {0.35f, 0.37f, 0.28f});
    area(1.4f, 29.4f, 9.2f, 9.2f, {0.44f, 0.39f, 0.28f});
    area(31.4f, 1.4f, 9.2f, 27.2f, {0.32f, 0.37f, 0.31f});
    for (size_t i = 0; i < m_Plants.size(); i += 12)
    {
        r.Ellipse(map({m_Plants[i].x, m_Plants[i].y}), 1.1f, 0.7f, {0.33f, 0.47f, 0.26f});
    }
    for (const Box& box : m_Boxes)
    {
        Color color = box.kind == 8    ? Color(0.12f, 0.16f, 0.17f)
                      : box.kind == 7  ? Color(0.62f, 0.75f, 0.77f)
                      : box.kind == 9  ? Color(0.55f, 0.41f, 0.25f)
                      : box.kind == 13 ? Color(0.61f, 0.48f, 0.31f)
                                       : Color(0.48f, 0.51f, 0.43f);
        area(box.x, box.y, box.w, box.d, color);
    }
    auto marker = [&](Vec2 p, bool active)
    {
        Vec2 v = map(p);
        Color c = active ? Color(1, 0.79f, 0.22f) : Color(0.6f, 0.66f, 0.6f);
        r.Quad({v.x, v.y - 4}, {v.x + 4, v.y}, {v.x, v.y + 4}, {v.x - 4, v.y}, c);
    };
    if (!m_HasFood)
    {
        marker(m_Food, true);
    }
    if (!m_HasSupplies)
    {
        marker(m_Supplies, true);
    }
    marker(m_Storage, m_Quest == Quest::ReturningHome && !m_Deposited);
    marker(m_Rose, m_Quest == Quest::ReturningHome && m_Deposited);
    if (m_Quest == Quest::FollowingSound || m_Quest == Quest::InspectingMap)
    {
        marker(m_Speaker, true);
    }
    if (m_Quest == Quest::Leaving)
    {
        marker(m_Exit, true);
    }
    for (const Creature& creature : m_Creatures)
    {
        r.Ellipse(map(creature.position), 2, 2, {0.8f, 0.34f, 0.27f});
    }
    r.Ellipse(map(m_Player), 3, 3, {1, 1, 1});
}