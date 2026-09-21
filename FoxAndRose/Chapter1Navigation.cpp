#include "stdafx.h"
#include "Chapter1.h"
#include <algorithm>
#include <cmath>
#include <queue>

bool Chapter1::ReturnBlocked(Vec2 p) const
{
    return (m_Quest == Quest::FollowingSound || m_Quest == Quest::InspectingMap) && p.x > 10.0f &&
           p.x < 11.5f && p.y > 32.7f && p.y < 35.8f;
}

void Chapter1::BuildNavigation()
{
    m_NavWalkable.resize(NavWidth * NavHeight);
    for (int y = 0; y < NavHeight; ++y)
    {
        for (int x = 0; x < NavWidth; ++x)
        {
            m_NavWalkable[y * NavWidth + x] = !Blocked({x * 0.5f, y * 0.5f});
        }
    }
}

bool Chapter1::NavigationGoal(Vec2& goal) const
{
    switch (m_Quest)
    {
    case Quest::Collecting:
        if (!m_HasFood && !m_HasSupplies)
        {
            auto distance = [&](Vec2 p)
            {
                float x = p.x - m_Player.x, y = p.y - m_Player.y;
                return x * x + y * y;
            };
            goal = distance(m_Food) < distance(m_Supplies) ? m_Food : m_Supplies;
        }
        else
        {
            goal = m_HasFood ? m_Supplies : m_Food;
        }
        return true;
    case Quest::Returning:
        goal = {30, 18};
        return true;
    case Quest::FollowingSound:
    case Quest::InspectingMap:
        goal = m_Speaker;
        return true;
    case Quest::ReturningHome:
        goal = m_Deposited ? m_Rose : m_Storage;
        return true;
    case Quest::Leaving:
        goal = m_Exit;
        return true;
    default:
        return false;
    }
}

void Chapter1::DrawDirection(Renderer& r)
{
    Vec2 goal;
    if (DialogueActive() || m_EntranceTime < 1.2f || !NavigationGoal(goal))
    {
        return;
    }
    int goalKey = static_cast<int>(std::round(goal.y * 2)) * NavWidth +
                  static_cast<int>(std::round(goal.x * 2));
    static const int dx[] = {1, -1, 0, 0};
    static const int dy[] = {0, 0, 1, -1};
    if (goalKey != m_NavGoal)
    {
        m_NavGoal = goalKey;
        m_NavDistance.assign(NavWidth * NavHeight, -1);
        std::queue<int> pending;
        for (int y = (std::max)(0, static_cast<int>(goal.y * 2) - 3);
             y <= (std::min)(NavHeight - 1, static_cast<int>(goal.y * 2) + 3);
             ++y)
        {
            for (int x = (std::max)(0, static_cast<int>(goal.x * 2) - 3);
                 x <= (std::min)(NavWidth - 1, static_cast<int>(goal.x * 2) + 3);
                 ++x)
            {
                int index = y * NavWidth + x;
                Vec2 p{x * 0.5f, y * 0.5f};
                float gx = p.x - goal.x, gy = p.y - goal.y;
                if (m_NavWalkable[index] && gx * gx + gy * gy < 2.0f && ClearLine(p, goal))
                {
                    m_NavDistance[index] = 0;
                    pending.push(index);
                }
            }
        }
        while (!pending.empty())
        {
            int current = pending.front();
            pending.pop();
            for (int direction = 0; direction < 4; ++direction)
            {
                int x = current % NavWidth + dx[direction], y = current / NavWidth + dy[direction];
                if (x < 0 || x >= NavWidth || y < 0 || y >= NavHeight)
                {
                    continue;
                }
                int next = y * NavWidth + x;
                if (m_NavWalkable[next] && m_NavDistance[next] < 0)
                {
                    m_NavDistance[next] = m_NavDistance[current] + 1;
                    pending.push(next);
                }
            }
        }
    }
    int px = static_cast<int>(std::round(m_Player.x * 2));
    int py = static_cast<int>(std::round(m_Player.y * 2));
    int best = -1;
    float bestScore = 1e9f;
    for (int y = (std::max)(0, py - 2); y <= (std::min)(NavHeight - 1, py + 2); ++y)
    {
        for (int x = (std::max)(0, px - 2); x <= (std::min)(NavWidth - 1, px + 2); ++x)
        {
            int index = y * NavWidth + x;
            float distance = std::hypot(x * 0.5f - m_Player.x, y * 0.5f - m_Player.y);
            float score = m_NavDistance[index] + distance * 2;
            if (m_NavDistance[index] >= 0 && score < bestScore &&
                ClearLine(m_Player, {x * 0.5f, y * 0.5f}))
            {
                best = index;
                bestScore = score;
            }
        }
    }
    if (best < 0)
    {
        return;
    }
    Vec2 waypoint = goal;
    if (m_NavDistance[best] > 0)
    {
        // Look a short distance ahead without crossing walls.
        int current = best;
        for (int step = 0; step < 4; ++step)
        {
            int next = current;
            for (int d = 0; d < 4; ++d)
            {
                int x = current % NavWidth + dx[d], y = current / NavWidth + dy[d];
                if (x < 0 || x >= NavWidth || y < 0 || y >= NavHeight)
                {
                    continue;
                }
                int index = y * NavWidth + x;
                if (m_NavDistance[index] >= 0 && m_NavDistance[index] < m_NavDistance[next] &&
                    ClearLine(m_Player, {x * 0.5f, y * 0.5f}))
                {
                    next = index;
                }
            }
            current = next;
        }
        waypoint = {current % NavWidth * 0.5f, current / NavWidth * 0.5f};
    }
    Vec2 feet = Project(m_Player.x, m_Player.y);
    Vec2 target = Project(waypoint.x, waypoint.y);
    float length = std::hypot(target.x - feet.x, target.y - feet.y);
    if (length < 2)
    {
        return;
    }
    Vec2 direction{(target.x - feet.x) / length, (target.y - feet.y) / length};
    static const Vec2 facing[] = {{0, 1}, {-1, 0}, {1, 0}, {0, -1}};
    float scale = m_Scale / 30;
    Vec2 center{feet.x + facing[m_Facing].x * 28 * scale,
                feet.y + 12 * scale + facing[m_Facing].y * 18 * scale};
    r.Triangle({center.x + direction.x * 12 * scale, center.y + direction.y * 12 * scale},
               {center.x - direction.x * 7 * scale - direction.y * 7 * scale,
                center.y - direction.y * 7 * scale + direction.x * 7 * scale},
               {center.x - direction.x * 7 * scale + direction.y * 7 * scale,
                center.y - direction.y * 7 * scale - direction.x * 7 * scale},
               {1.0f, 0.83f, 0.25f, 0.9f});
}

void Chapter1::DrawMinimap(Renderer& r)
{
    const float x = 18, y = 24;
    r.Rect(x, y, 220, 204, {0.035f, 0.07f, 0.065f, 0.95f});
    r.Text(x + 10, y + 7, m_MapName, Color(), 0.85f);
    auto map = [&](Vec2 p)
    {
        return Vec2{x + 109 + (p.x - p.y) * 2, y + 38 + (p.x + p.y) * 1.65f};
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
    r.Text(x + 10, y + 181, L"흰색: 여우  금색: 주요 대상", {0.88f, 0.84f, 0.65f}, 0.65f);
}