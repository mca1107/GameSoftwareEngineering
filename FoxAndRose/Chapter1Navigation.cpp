#include "stdafx.h"
#include "GuideIcons.h"
#include "Chapter1.h"
#include <algorithm>
#include <cmath>

bool Chapter1::ReturnBlocked(Vec2 p) const
{
    return (m_Quest == Quest::FollowingSound || m_Quest == Quest::InspectingMap) && p.x > 10.0f &&
           p.x < 11.5f && p.y > 32.7f && p.y < 35.8f;
}

Vec2 Chapter1::TargetRadii(int target, float expansion) const
{
    Vec2 body;
    if (target <= 2)
    {
        body = {27, 13.5f};
    }
    else if (target == 3)
    {
        body = {10, 4};
    }
    else if (target == 4)
    {
        body = {42, 12};
    }
    float margin = target == 5 ? 1.0f : 2.0f / 3.0f;
    float scale = m_Scale / 30;
    return {(body.x + 22 * expansion * margin) * scale, (body.y + 9 * expansion * margin) * scale};
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
        GoalIcon icon;
    };

    std::vector<Goal> goals;
    switch (m_Quest)
    {
    case Quest::Collecting:
        if (!m_HasFood)
        {
            goals.push_back({m_Food, GoalIcon::Food});
        }
        if (!m_HasSupplies)
        {
            goals.push_back({m_Supplies, GoalIcon::Supplies});
        }
        break;
    case Quest::Returning:
        goals.push_back({m_Storage, GoalIcon::Store});
        break;
    case Quest::FollowingSound:
    case Quest::InspectingMap:
        goals.push_back({m_Speaker, GoalIcon::Sound});
        break;
    case Quest::ReturningHome:
        goals.push_back(
            {m_Deposited ? m_Rose : m_Storage, m_Deposited ? GoalIcon::Talk : GoalIcon::Store});
        break;
    case Quest::Leaving:
        goals.push_back({m_Exit, GoalIcon::Depart});
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
                int target = goal.icon == GoalIcon::Food       ? 1
                             : goal.icon == GoalIcon::Supplies ? 2
                             : goal.icon == GoalIcon::Store    ? 0
                             : goal.icon == GoalIcon::Talk     ? 3
                             : goal.icon == GoalIcon::Sound    ? 4
                                                               : 5;
                float phase = std::fmod(m_Time, 1.6f) / 1.6f;
                Vec2 radius = TargetRadii(target, 0.75f + phase * 0.65f);
                float alpha = std::sin(phase * 3.14159265f) * 0.32f;
                r.Ellipse(p, radius.x, radius.y, {1, 0.81f, 0.37f, alpha});
            }
            continue;
        }
        if (!labels)
        {
            continue;
        }
        Vec2 label = PlaceGuideIcon(m_Width, m_Height, p, false, placed);
        placed.push_back(label);
        DrawGoalIcon(r, label, goal.icon);
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