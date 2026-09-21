#include "stdafx.h"
#include "Chapter1.h"
#include <algorithm>
#include <cmath>

namespace
{

constexpr float CreatureRadius = 0.55f;

float Length(Vec2 a, Vec2 b)
{
    float x = a.x - b.x, y = a.y - b.y;
    return std::sqrt(x * x + y * y);
}
} // namespace

void Chapter1::BuildEnclosures()
{
    const Color concrete{0.32f, 0.38f, 0.34f}, metal{0.4f, 0.5f, 0.46f};
    AddBox(17, 1, 7, 0.3f, 1.8f, concrete);
    // Transparent west and south faces match the L-shaped observation fence.
    for (int i = 0; i < 7; ++i)
    {
        AddBox(17, 1.0f + i, 0.15f, 1, 1.7f, metal, true, 7);
        AddBox(17.0f + i, 7.85f, 1, 0.15f, 1.7f, metal, true, 7);
    }
    // The adjacent solid building seals the east face.
    for (int i = 0; i < 10; ++i)
    {
        AddBox(31, 29.0f + i, 0.15f, 1, 1.7f, metal, true, 7);
    }
}

void Chapter1::UpdateCreatures(float dt)
{
    bool visible = (m_Player.x > 14 && m_Player.x < 17 && m_Player.y > 1 && m_Player.y < 8) ||
                   (m_Player.x > 17 && m_Player.x < 24 && m_Player.y >= 8 && m_Player.y < 12);
    if (visible && !m_EncounterSeen && !DialogueActive())
    {
        m_EncounterSeen = true;
        StartDialogue(Dialogue::Observation);
    }
    for (size_t index = 0; index < m_Creatures.size(); ++index)
    {
        Creature& c = m_Creatures[index];
        c.alert = visible && Length(c.position, m_Player) < 7;
        c.upright =
            c.alert ? (std::max)(0.0f, c.upright - dt * 4) : (std::min)(1.0f, c.upright + dt * 2);
        Vec2 goal{c.home.x + std::cos(m_Time * 0.35f + index * 2) * 0.65f,
                  c.home.y + std::sin(m_Time * 0.29f + index * 2) * 0.65f};
        if (c.alert)
        {
            if (m_Player.x < m_Observation.left)
            {
                goal = {m_Observation.left + 1.4f, (std::max)(2.4f, (std::min)(6.6f, m_Player.y))};
            }
            else
            {
                goal = {(std::max)(18.4f, (std::min)(22.6f, m_Player.x)),
                        m_Observation.bottom - 1.4f};
            }
        }
        float dx = goal.x - c.position.x, dy = goal.y - c.position.y;
        if (std::sqrt(dx * dx + dy * dy) < 0.08f)
            continue;
        float heading = std::atan2(dy, dx), speed = c.alert ? 1.0f : 0.55f;
        int steps = (std::max)(1, static_cast<int>(std::ceil(speed * dt / 0.08f)));
        for (int step = 0; step < steps; ++step)
        {
            Vec2 candidate{c.position.x + std::cos(heading) * speed * dt / steps,
                           c.position.y + std::sin(heading) * speed * dt / steps};
            if (candidate.x < m_Observation.left + 1.4f ||
                candidate.x > m_Observation.right - 1.35f ||
                candidate.y < m_Observation.top + 1.4f ||
                candidate.y > m_Observation.bottom - 1.4f ||
                Blocked(candidate, CreatureRadius, true))
                break;
            bool clear = true;
            for (size_t other = 0; other < m_Creatures.size(); ++other)
                if (other != index && Length(candidate, m_Creatures[other].position) < 1.4f)
                    clear = false;
            if (!clear)
                break;
            c.position = candidate;
            c.heading = heading;
            c.phase += speed * dt / steps * 7;
        }
    }
}

void Chapter1::DrawCreature(Renderer& r, const Creature& creature)
{
    Vec2 center = Project(creature.position.x, creature.position.y);
    const float margin = 90 * m_Scale / 30;
    if (center.x < -margin || center.x > m_Width + margin || center.y < -margin ||
        center.y > m_Height + margin)
        return;
    const bool faceLeft = std::cos(creature.heading) - std::sin(creature.heading) < 0;
    r.AggressiveCreature(
        center, m_Scale / 30, creature.upright, creature.phase, faceLeft, creature.alert);
}

void Chapter1::DrawShadows(Renderer& r)
{
    if (!r.BeginShadows())
        return;
    const Color mask{1, 1, 1, 1};
    for (const Box& box : m_Boxes)
    {
        if (box.kind == 7)
            continue; // Mesh does not cast an opaque wall-shaped shadow.
        Vec2 center = Project(box.x, box.y);
        if (center.x < -250 || center.x > m_Width + 250 || center.y < -200 ||
            center.y > m_Height + 200)
            continue;
        Vec2 points[] = {Project(box.x, box.y),
                         Project(box.x + box.w, box.y),
                         Project(box.x + box.w, box.y + box.d),
                         Project(box.x, box.y + box.d)};
        float height = box.h + (box.kind == 2 ? 3.1f : 0);
        Vec2 offset{height * m_Scale * 0.7f, height * m_Scale * 0.28f};
        r.Quad(points[0], points[1], points[2], points[3], mask);
        for (int i = 0; i < 4; ++i)
        {
            Vec2 a = points[i], b = points[(i + 1) % 4];
            r.Quad(a, b, {b.x + offset.x, b.y + offset.y}, {a.x + offset.x, a.y + offset.y}, mask);
        }
    }
    for (Vec2 p : {m_Player, m_Rose, m_Storage, m_Food, m_Supplies})
    {
        Vec2 feet = Project(p.x, p.y);
        r.Ellipse({feet.x + 14, feet.y + 6}, 22, 7, mask);
    }
    for (const Creature& creature : m_Creatures)
    {
        Vec2 p = Project(creature.position.x, creature.position.y);
        r.Ellipse({p.x + 12, p.y + 5}, 28, 9, mask);
    }
    for (const Plant& plant : m_Plants)
    {
        Vec2 p = Project(plant.x, plant.y);
        float margin = plant.size * 32 + 16;
        if (p.x < -margin || p.x > m_Width + margin || p.y < -margin || p.y > m_Height + margin)
        {
            continue;
        }
        r.Ellipse({p.x + plant.size * 12, p.y + plant.size * 4},
                  plant.size * 18,
                  plant.size * 7,
                  {0.45f, 0.45f, 0.45f, 1});
    }
    r.EndShadows();
}

void Chapter1::DrawAtmosphere(Renderer& r)
{
    // Wind-driven pollen and falling leaves live in world coordinates, not attached to the camera.
    for (int i = 0; i < 90; ++i)
    {
        float x = std::fmod(i * 7.73f + m_Time * 0.13f, 42.0f);
        float y = std::fmod(i * 3.71f + m_Time * 0.055f, 40.0f);
        float z = 0.5f + std::fmod(i * 0.43f + m_Time * 0.08f, 2.4f);
        Vec2 p = Project(x, y, z);
        if (p.x < 0 || p.x > m_Width || p.y < 0 || p.y > m_Height)
            continue;
        float pulse = 0.4f + 0.2f * std::sin(m_Time + i);
        if (i % 5 == 0)
        {
            float sway = std::sin(m_Time * 1.7f + i) * 4;
            r.Triangle({p.x - 2, p.y},
                       {p.x + sway, p.y - 5},
                       {p.x + 3, p.y + 2},
                       {0.54f, 0.58f, 0.31f, pulse});
        }
        else
            r.Ellipse(p, 1.2f, 1.2f, {1.15f, 1.02f, 0.7f, pulse});
    }
    // Layered low-opacity mist; no opaque fullscreen fog over interaction prompts.
    for (int i = 0; i < 6; ++i)
    {
        const float index = static_cast<float>(i);
        Vec2 p = Project(4.0f + index * 6.0f + std::sin(m_Time * 0.1f + index) * 2.0f,
                         8.0f + index * 4.0f,
                         0.1f);
        for (int layer = 3; layer > 0; --layer)
            r.Ellipse(p, 55.0f + layer * 23, 9.0f + layer * 5, {0.53f, 0.65f, 0.59f, 0.009f});
    }
}
