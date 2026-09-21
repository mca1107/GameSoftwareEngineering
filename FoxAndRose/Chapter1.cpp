#include "stdafx.h"
#include "Chapter1.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <utility>
#include <queue>
#include <functional>
#include <numeric>
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <cstring>
#pragma comment(lib, "winmm.lib")

namespace
{
Color Shade(Color c, float scale)
{
    return {c.r * scale, c.g * scale, c.b * scale, c.a};
}

float Distance(Vec2 a, Vec2 b)
{
    float x = a.x - b.x, y = a.y - b.y;
    return std::sqrt(x * x + y * y);
}

} // namespace

Chapter1::~Chapter1()
{
    StopSound();
}

void Chapter1::StopSound()
{
    PlaySoundW(nullptr, nullptr, 0);
}

void Chapter1::PlaySoundCue()
{
    // A short, quiet two-part rasp. Static storage outlives asynchronous playback.
    static const std::vector<unsigned char> wave = []
    {
        const int rate = 22050, count = rate;
        std::vector<unsigned char> data(44 + count * 2, 0);
        auto put16 = [&](int at, unsigned value)
        {
            data[at] = static_cast<unsigned char>(value);
            data[at + 1] = static_cast<unsigned char>(value >> 8);
        };
        auto put32 = [&](int at, unsigned value)
        {
            for (int b = 0; b < 4; ++b)
                data[at + b] = static_cast<unsigned char>(value >> (b * 8));
        };
        std::memcpy(data.data(), "RIFF", 4);
        put32(4, 36 + count * 2);
        std::memcpy(data.data() + 8, "WAVEfmt ", 8);
        put32(16, 16);
        put16(20, 1);
        put16(22, 1);
        put32(24, rate);
        put32(28, rate * 2);
        put16(32, 2);
        put16(34, 16);
        std::memcpy(data.data() + 36, "data", 4);
        put32(40, count * 2);
        for (int i = 0; i < count; ++i)
        {
            float t = i / static_cast<float>(rate), local = t < 0.48f ? t : t - 0.52f;
            float envelope = local > 0 && local < 0.4f ? std::sin(local / 0.4f * 3.14159265f) : 0;
            float tone = std::sin(6.2831853f * (170 * t + 12 * std::sin(t * 9)));
            float rasp = std::sin(6.2831853f * 391 * t) * std::sin(6.2831853f * 53 * t);
            auto sample = static_cast<std::int16_t>(envelope * (tone * 2200 + rasp * 900));
            put16(44 + i * 2, static_cast<std::uint16_t>(sample));
        }
        return data;
    }();
    PlaySoundW(
        reinterpret_cast<LPCWSTR>(wave.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

void Chapter1::AddBox(float x, float y, float w, float d, float h, Color c, bool solid, int kind)
{
    // Split long walls into contiguous panels for local occlusion ordering.
    if (kind == 0 && ((w > 1 && d <= 0.4f) || (d > 1 && w <= 0.4f)))
    {
        bool horizontal = w > d;
        float length = horizontal ? w : d;
        for (float offset = 0; offset < length; offset += 1)
        {
            float segment = (std::min)(1.0f, length - offset);
            m_Boxes.push_back({x + (horizontal ? offset : 0),
                               y + (horizontal ? 0 : offset),
                               horizontal ? segment : w,
                               horizontal ? d : segment,
                               h,
                               c,
                               solid,
                               kind});
        }
        return;
    }
    m_Boxes.push_back({x, y, w, d, h, c, solid, kind});
}

void Chapter1::Building(float x, float y, float w, float d)
{
    const Color wall{0.43f, 0.47f, 0.41f};
    // Open roof and a wide doorway in the near wall. Short segments sort correctly.
    for (float offset = 0; offset < w; offset += 1)
    {
        AddBox(x + offset, y, 0.94f, 0.4f, 2.3f + 0.35f * std::sin(offset * 2), wall);
        if (offset < w * 0.5f - 1.5f || offset > w * 0.5f + 0.5f)
            AddBox(x + offset, y + d - 0.4f, 0.94f, 0.4f, 0.65f, wall);
    }
    for (float offset = 1; offset < d - 1; offset += 1)
    {
        AddBox(x, y + offset, 0.4f, 0.94f, 1.6f, wall);
        AddBox(x + w - 0.4f, y + offset, 0.4f, 0.94f, 1.15f, wall);
    }
    for (float offset = 0; offset < w; offset += 1.25f)
    {
        m_Plants.push_back({x + offset, y + 0.2f, 0.65f});
        m_Plants.push_back({x + offset, y + d - 0.2f, 0.35f});
    }
}

Chapter1::Chapter1()
{
    const Color concrete{0.36f, 0.40f, 0.34f};
    const Color crates{0.43f, 0.34f, 0.23f};
    // Solid footprints represent buildings with no accessible interior.
    AddBox(1, 14, 7, 9, 5.4f, concrete, true, 8);
    AddBox(11, 8, 6, 9, 6.6f, concrete, true, 8);
    AddBox(24, 1, 7, 8, 7.2f, concrete, true, 8);
    AddBox(24, 20, 7, 9, 4.8f, concrete, true, 8);
    // Shelter in the southwest, with an east-facing doorway.
    AddBox(1, 29, 10, 0.4f, 2.0f, concrete);
    AddBox(1, 38.6f, 10, 0.4f, 0.9f, concrete);
    AddBox(1, 29.4f, 0.4f, 9.2f, 2.0f, concrete);
    AddBox(10.6f, 29.4f, 0.4f, 3.6f, 2.0f, concrete);
    AddBox(10.6f, 35.5f, 0.4f, 3.1f, 0.9f, concrete);
    AddBox(2, 30, 3, 0.8f, 0.55f, crates);
    AddBox(8, 30, 1, 2, 0.8f, crates);
    // Eastern food store: west entrance and a southeast exit toward the speaker.
    AddBox(31, 1, 10, 0.4f, 2.2f, concrete);
    AddBox(40.6f, 1.4f, 0.4f, 27.6f, 2.2f, concrete);
    AddBox(31, 1.4f, 0.4f, 14.6f, 2.2f, concrete);
    AddBox(31, 20, 0.4f, 8.6f, 1.1f, concrete);
    AddBox(31, 28.6f, 7, 0.4f, 1.1f, concrete);
    auto pile = [&](float x, float y, int columns, int rows, float minimumHeight)
    {
        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                int seed = row * 7 + column * 11;
                float h = minimumHeight + (seed % 4) * 0.28f;
                Color shade = seed % 3 == 0 ? Color(0.31f, 0.39f, 0.34f)
                                            : Color(0.46f, 0.34f + (seed % 3) * 0.03f, 0.23f);
                AddBox(x + column + (seed % 2) * 0.04f,
                       y + row,
                       0.90f + (seed % 2) * 0.06f,
                       0.94f,
                       h,
                       shade,
                       true,
                       9);
            }
        }
    };
    pile(34, 7, 3, 6, 0.8f);
    pile(35, 20, 5, 2, 0.7f);
    pile(32, 26, 4, 2, 0.8f);
    pile(1.5f, 2, 1, 8, 0.9f);
    pile(32, 30, 2, 9, 3.0f);
    pile(34, 35, 3, 4, 2.8f);
    // Furniture is kept along the walls, leaving the east doorway and quest targets clear.
    AddBox(2, 36.3f, 2.6f, 1.1f, 0.5f, crates, true, 10);
    AddBox(5, 36.3f, 2.6f, 1.1f, 0.5f, crates, true, 10);
    AddBox(7.8f, 31.8f, 1.7f, 1.0f, 0.85f, crates, true, 11);
    AddBox(8.3f, 33.2f, 0.55f, 0.55f, 0.48f, crates, true, 11);
    AddBox(2, 32, 1, 1, 0.9f, crates, true, 9);
    AddBox(6, 30, 1.2f, 0.8f, 0.4f, crates, true, 14);
    AddBox(8.4f, 36.5f, 0.9f, 0.9f, 0.15f, crates, true, 12);
    AddBox(12, 29, 0.35f, 2.5f, 1.1f, crates, true, 13);
    AddBox(2, 27.5f, 3, 0.35f, 1.0f, crates, true, 13);
    AddBox(6.2f, 27.5f, 2.4f, 0.35f, 0.9f, crates, true, 13);
    AddBox(12, 36.5f, 0.35f, 2, 1.2f, crates, true, 13);
    AddBox(13.5f, 32, 2.5f, 0.35f, 1.0f, crates, true, 13);
    // Leave a 1.35 m opening between the two entrance barricades.
    AddBox(10.6f, 33, 0.4f, 0.55f, 0.9f, crates, true, 13);
    AddBox(10.6f, 34.9f, 0.4f, 0.6f, 1.0f, crates, true, 13);
    m_Creatures.push_back({{19, 4}, {19, 4}, 0, 0, false});
    m_Creatures.push_back({{22, 4}, {22, 4}, 2, 0, false});
    BuildEnclosures();
    BuildNavigation();
    std::mt19937 rng(17);
    std::uniform_real_distribution<float> pos(1, 41), size(0.25f, 0.75f);
    for (int i = 0; i < 850; ++i)
    {
        float x = pos(rng), y = pos(rng);
        // Leave circulation corridors sparse. Decorative foliage has no collision.
        bool corridor = (x > 8 && x < 11) || (y > 17 && y < 20) ||
                        (x > 18 && x < 24 && y > 8 && y < 12) || (x > 37 && y > 13);
        bool nearTarget = Distance({x, y}, m_Storage) < 2 || Distance({x, y}, m_Food) < 2 ||
                          Distance({x, y}, m_Supplies) < 2 || Distance({x, y}, m_Rose) < 2 ||
                          Distance({x, y}, m_Speaker) < 2;
        bool pen = x >= m_Observation.left && x <= m_Observation.right && y >= m_Observation.top &&
                   y <= m_Observation.bottom;
        if (!nearTarget && !pen && (!corridor || i % 7 == 0))
            m_Plants.push_back({x, y, corridor ? 0.18f : size(rng) * 1.15f});
    }
    for (int i = 0; i < 12; ++i)
        m_Plants.push_back({2.0f + (i % 4) * 1.6f, 29.4f + (i / 4) * 0.3f, 0.45f});
}

bool Chapter1::Blocked(Vec2 p, float radius, bool enclosedActor) const
{
    if (p.x < 1 || p.y < 1 || p.x > 41 || p.y > 39)
        return true;
    if (!enclosedActor && ReturnBlocked(p))
    {
        return true;
    }
    // Entire pens are inaccessible to the player, including corners and fence seams.
    if (!enclosedActor)
    {
        float dx =
            (std::max)(m_Observation.left - p.x, (std::max)(0.0f, p.x - m_Observation.right));
        float dy =
            (std::max)(m_Observation.top - p.y, (std::max)(0.0f, p.y - m_Observation.bottom));
        if (dx * dx + dy * dy <= radius * radius)
            return true;
    }
    // Supply crates and the base storage remain solid.
    for (Vec2 target : {m_Storage, m_Food, m_Supplies, m_Rose})
        if (Distance(p, target) < radius + 0.37f)
            return true;
    for (const Box& b : m_Boxes)
    {
        if (!b.solid)
            continue;
        float nearX = (std::max)(b.x, (std::min)(p.x, b.x + b.w));
        float nearY = (std::max)(b.y, (std::min)(p.y, b.y + b.d));
        float dx = p.x - nearX, dy = p.y - nearY;
        if (dx * dx + dy * dy < radius * radius)
            return true;
    }
    return false;
}

bool Chapter1::ClearLine(Vec2 from, Vec2 to) const
{
    // Exact segment/AABB slab intersection prevents interaction through even thin walls.
    for (const Box& box : m_Boxes)
    {
        if (!box.solid)
            continue;
        float low = 0, high = 1;
        const float origin[] = {from.x, from.y}, delta[] = {to.x - from.x, to.y - from.y};
        const float minimum[] = {box.x, box.y}, maximum[] = {box.x + box.w, box.y + box.d};
        bool intersects = true;
        for (int axis = 0; axis < 2; ++axis)
        {
            if (std::abs(delta[axis]) < 0.00001f)
            {
                if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                {
                    intersects = false;
                    break;
                }
            }
            else
            {
                float a = (minimum[axis] - origin[axis]) / delta[axis];
                float b = (maximum[axis] - origin[axis]) / delta[axis];
                if (a > b)
                    std::swap(a, b);
                low = (std::max)(low, a);
                high = (std::min)(high, b);
                if (low > high)
                {
                    intersects = false;
                    break;
                }
            }
        }
        if (intersects)
            return false;
    }
    return true;
}

void Chapter1::Notify(const std::wstring& message)
{
    m_Toast = message;
    m_ToastTime = 6;
    m_Idle = 0;
}

void Chapter1::Update(float dt, const bool* keys)
{
    m_Time += dt;
    if (m_EntranceTime < 1.2f)
    {
        m_EntranceTime += (std::max)(0.0f, dt);
        m_Moving = false;
        return;
    }
    m_Moving = false;
    if (m_Quest != Quest::Complete)
        m_Elapsed += dt;
    m_ToastTime = (std::max)(0.0f, m_ToastTime - dt);
    m_Idle += dt;
    // Departure is a timed transition, not a physics step. Do not stretch it at low FPS.
    if (m_Quest == Quest::Complete)
    {
        m_DepartureTime += (std::max)(0.0f, dt);
        return;
    }
    dt = (std::max)(0.0f, (std::min)(dt, 0.1f));
    if (DialogueActive())
    {
        // These are the separate, living observation-room creatures.
        UpdateCreatures(dt);
        return;
    }
    float sx = (keys['d'] ? 1.0f : 0.0f) - (keys['a'] ? 1.0f : 0.0f);
    float sy = (keys['s'] ? 1.0f : 0.0f) - (keys['w'] ? 1.0f : 0.0f);
    // Inverse of screen x=(worldX-worldY), y=(worldX+worldY)/2.
    float dx = sx + 2 * sy, dy = -sx + 2 * sy;
    float length = std::sqrt(dx * dx + dy * dy);
    if (length > 0)
    {
        dx = dx / length * (2.5f / GroundScale) * dt;
        dy = dy / length * (2.5f / GroundScale) * dt;
        if (m_Player.x > 14)
        {
            m_ReturnReminderShown = false;
        }
        if (dx < 0 && ReturnBlocked({m_Player.x + dx - 0.15f, m_Player.y}) &&
            !m_ReturnReminderShown)
        {
            m_ReturnReminderShown = true;
            StartDialogue(Dialogue::ReturnReminder);
            return;
        }
        const Vec2 before = m_Player;
        // Substeps prevent crossing narrow colliders on a delayed frame; slide along each axis.
        int steps = (std::max)(1, static_cast<int>(std::ceil(std::sqrt(dx * dx + dy * dy) / 0.1f)));
        for (int i = 0; i < steps; ++i)
        {
            Vec2 next{m_Player.x + dx / steps, m_Player.y};
            if (!Blocked(next))
                m_Player = next;
            next = {m_Player.x, m_Player.y + dy / steps};
            if (!Blocked(next))
                m_Player = next;
        }
        if (Distance(before, m_Player) > 0.001f)
        {
            m_Moved = true;
            if (m_MoveHintActive)
            {
                m_MovePracticeDistance += Distance(before, m_Player) * GroundScale;
                if (m_MovePracticeDistance >= 4.0f)
                {
                    m_MoveHintActive = false;
                }
            }
            m_Moving = true;
            m_Walk += dt * 9;
            m_Facing = std::abs(sy) > std::abs(sx) ? (sy < 0 ? 3 : 0) : (sx < 0 ? 1 : 2);
        }
    }
    UpdateCreatures(dt);
    UpdateStory(dt);
    if (!m_InteractionLearned)
    {
        int target = Target();
        if (target == 1 || target == 2)
        {
            m_InteractionHintActive = true;
        }
    }
    float blend = 1 - std::exp(-5 * dt);
    m_Camera.x += (m_Player.x - m_Camera.x) * blend;
    m_Camera.y += (m_Player.y - m_Camera.y) * blend;
}

Vec2 Chapter1::Project(float x, float y, float z) const
{
    x = (x - m_Camera.x) * GroundScale;
    y = (y - m_Camera.y) * GroundScale;
    return {m_Width * 0.5f + (x - y) * m_Scale,
            m_Height * 0.53f + (x + y) * m_Scale * 0.5f - z * m_Scale};
}

void Chapter1::Ground(Renderer& r, float x, float y, float w, float d, Color c)
{
    r.Surface(
        Project(x, y), Project(x + w, y), Project(x + w, y + d), Project(x, y + d), c, 3, w, d);
}

void Chapter1::DrawBox(Renderer& r, const Box& original)
{
    Box b = original;
    if (b.kind == 0 || b.kind == 8)
    {
        float weather = 0.94f + 0.06f * std::sin(b.x * 2.7f + b.y * 1.9f);
        b.color = Shade(b.color, weather);
    }
    auto projectBox = [&](float x, float y, float height = 0.0f)
    {
        if (b.kind == 9)
        {
            float angle = std::sin(b.x * 3.1f + b.y * 1.7f) * 0.055f;
            float cx = b.x + b.w * 0.5f, cy = b.y + b.d * 0.5f;
            float dx = (x - cx) * 0.92f, dy = (y - cy) * 0.92f;
            x = cx + dx * std::cos(angle) - dy * std::sin(angle);
            y = cy + dx * std::sin(angle) + dy * std::cos(angle);
        }
        return Project(x, y, height);
    };
    float z = b.kind == 2 ? 3.1f : 0;
    Vec2 center = projectBox(b.x + b.w * 0.5f, b.y + b.d * 0.5f);
    if (center.x < -600 || center.x > m_Width + 600 || center.y < -600 || center.y > m_Height + 600)
        return;
    if (b.kind == 7)
    {
        // See-through welded mesh, with continuous collision along all four sides.
        bool horizontal = b.w > b.d;
        float ex = b.x + (horizontal ? b.w : 0), ey = b.y + (horizontal ? 0 : b.d);
        Color steel{0.38f, 0.48f, 0.46f, 0.9f};
        r.Line(projectBox(b.x, b.y), projectBox(b.x, b.y, b.h), 3, steel);
        r.Line(projectBox(ex, ey), projectBox(ex, ey, b.h), 3, steel);
        for (int rail = 1; rail <= 3; ++rail)
        {
            float z = rail * b.h / 3;
            r.Line(projectBox(b.x, b.y, z), projectBox(ex, ey, z), 1.5f, steel);
        }
        for (int wire = 1; wire < 5; ++wire)
        {
            float t = wire / 5.0f;
            float x = b.x + (ex - b.x) * t, y = b.y + (ey - b.y) * t;
            r.Line(
                projectBox(x, y, 0.08f), projectBox(x, y, b.h), 0.7f, {0.57f, 0.66f, 0.61f, 0.5f});
        }
        return;
    }
    if (b.kind >= 10 && b.kind <= 14)
    {
        Color wood{0.32f, 0.24f, 0.16f};
        float right = b.x + b.w, front = b.y + b.d;
        if (b.kind == 12)
        {
            for (int i = 0; i < 8; ++i)
            {
                float angle = i * 0.785398f;
                Vec2 stone = projectBox(b.x + b.w * 0.5f + std::cos(angle) * 0.42f,
                                        b.y + b.d * 0.5f + std::sin(angle) * 0.42f);
                r.Ellipse(stone, 5, 3, {0.32f, 0.33f, 0.29f});
            }
            Vec2 fire = projectBox(b.x + b.w * 0.5f, b.y + b.d * 0.5f);
            r.Line({fire.x - 9, fire.y + 2}, {fire.x + 8, fire.y - 2}, 4, wood);
            r.Surface({fire.x - 13, fire.y - 32},
                      {fire.x + 13, fire.y - 32},
                      {fire.x + 13, fire.y + 2},
                      {fire.x - 13, fire.y + 2},
                      {1, 1, 1},
                      8,
                      1,
                      1);
        }
        else if (b.kind == 13)
        {
            for (int i = 0; i < 4; ++i)
            {
                float t = i / 3.0f;
                float x = b.x + b.w * t, y = b.y + b.d * t;
                r.Line(projectBox(x, y),
                       projectBox(x + 0.08f, y, b.h + (i % 2) * 0.15f),
                       5,
                       i % 2 == 0 ? wood : Color(0.32f, 0.38f, 0.37f));
            }
            r.Line(projectBox(b.x, b.y, 0.3f), projectBox(right, front, b.h), 6, wood);
            r.Line(projectBox(b.x, b.y, b.h * 0.75f),
                   projectBox(right, front, 0.45f),
                   4,
                   {0.39f, 0.44f, 0.41f});
        }
        else if (b.kind == 14)
        {
            for (int i = 0; i < 4; ++i)
            {
                Vec2 base = projectBox(b.x + i * 0.25f, front);
                Vec2 tip = projectBox(b.x + i * 0.25f + 0.25f, b.y, 0.8f + i * 0.08f);
                r.Line(base, tip, 3, wood);
                r.Line({tip.x - 5, tip.y}, {tip.x + 5, tip.y - 2}, 4, {0.44f, 0.48f, 0.46f});
            }
        }
        else
        {
            for (Vec2 foot :
                 {Vec2{b.x, b.y}, Vec2{right, b.y}, Vec2{right, front}, Vec2{b.x, front}})
            {
                r.Line(projectBox(foot.x, foot.y), projectBox(foot.x, foot.y, b.h), 4, wood);
            }
            r.Surface(projectBox(b.x, b.y, b.h),
                      projectBox(right, b.y, b.h),
                      projectBox(right, front, b.h),
                      projectBox(b.x, front, b.h),
                      wood,
                      4,
                      b.w,
                      b.d);
            if (b.kind == 10)
            {
                r.Surface(projectBox(b.x + 0.1f, b.y + 0.1f, b.h + 0.08f),
                          projectBox(right - 0.1f, b.y + 0.1f, b.h + 0.08f),
                          projectBox(right - 0.1f, front - 0.1f, b.h + 0.08f),
                          projectBox(b.x + 0.1f, front - 0.1f, b.h + 0.08f),
                          {0.40f, 0.45f, 0.32f},
                          5,
                          2,
                          1);
                Vec2 pillow = projectBox(b.x + 0.4f, b.y + b.d * 0.5f, b.h + 0.14f);
                r.Ellipse(pillow, 10, 5, {0.65f, 0.61f, 0.47f});
            }
            else if (b.w < 1)
            {
                r.Line(projectBox(b.x, b.y, b.h), projectBox(b.x, b.y, b.h + 0.5f), 4, wood);
                r.Line(projectBox(right, b.y, b.h), projectBox(right, b.y, b.h + 0.5f), 4, wood);
                r.Line(
                    projectBox(b.x, b.y, b.h + 0.5f), projectBox(right, b.y, b.h + 0.5f), 5, wood);
            }
            else
            {
                Vec2 cup = projectBox(b.x + 0.5f, b.y + 0.5f, b.h);
                r.Rect(cup.x, cup.y - 6, 5, 6, {0.55f, 0.53f, 0.42f});
            }
        }
        return;
    }
    Vec2 a = projectBox(b.x, b.y, z), bb = projectBox(b.x + b.w, b.y, z);
    Vec2 c = projectBox(b.x + b.w, b.y + b.d, z), d = projectBox(b.x, b.y + b.d, z);
    Vec2 at = projectBox(b.x, b.y, z + b.h), bt = projectBox(b.x + b.w, b.y, z + b.h);
    Vec2 ct = projectBox(b.x + b.w, b.y + b.d, z + b.h), dt = projectBox(b.x, b.y + b.d, z + b.h);
    int material = b.kind == 4 || b.kind >= 9 ? 4 : (b.kind == 8 ? 1 : (b.kind >= 2 ? 2 : 1));
    r.Surface(bb, c, ct, bt, Shade(b.color, 0.70f), material, b.d, b.h);
    r.Surface(c, d, dt, ct, Shade(b.color, 0.9f), material, b.w, b.h);
    r.Surface(at, bt, ct, dt, Shade(b.color, 1.16f), material, b.w, b.d);
    if (b.kind == 0 && b.h > 1.8f && b.w > 0.8f && b.d < 0.5f)
    {
        // Broken glazing inset into modular wall panels hints at the former future city.
        r.Surface(projectBox(b.x + 0.12f, b.y + b.d + 0.01f, 0.85f),
                  projectBox(b.x + b.w - 0.12f, b.y + b.d + 0.01f, 0.85f),
                  projectBox(b.x + b.w - 0.12f, b.y + b.d + 0.01f, 1.75f),
                  projectBox(b.x + 0.12f, b.y + b.d + 0.01f, 1.75f),
                  {0.12f, 0.23f, 0.24f, b.color.a},
                  6,
                  1,
                  1);
    }
    if (b.kind == 8)
    {
        for (int face = 0; face < 2; ++face)
        {
            float length = face == 0 ? b.w : b.d;
            auto wallPoint = [&](float along, float height)
            {
                return face == 0 ? projectBox(b.x + along, b.y + b.d + 0.01f, height)
                                 : projectBox(b.x + b.w + 0.01f, b.y + along, height);
            };
            for (float offset = 0.8f; offset < length - 1; offset += 1.9f)
            {
                float bottom = b.h * 0.42f;
                Vec2 a = wallPoint(offset, bottom), c = wallPoint(offset + 0.9f, bottom + 1.2f);
                Vec2 bb = wallPoint(offset + 0.9f, bottom), d = wallPoint(offset, bottom + 1.2f);
                r.Quad(a, bb, c, d, {0.07f, 0.13f, 0.13f});
                r.Triangle(a, bb, wallPoint(offset + 0.6f, bottom + 0.55f), {0.32f, 0.44f, 0.42f});
                r.Line(d, c, 2, {0.46f, 0.49f, 0.42f});
                r.Line(wallPoint(offset + 0.25f, bottom + 1.2f),
                       wallPoint(offset + 0.35f, bottom + 0.8f),
                       1,
                       {0.68f, 0.72f, 0.63f});
                for (int leaf = 0; leaf < 6; ++leaf)
                {
                    Vec2 vine = wallPoint(offset + 1.1f + std::sin(leaf * 1.3f) * 0.08f,
                                          bottom + 1.3f - leaf * 0.25f);
                    r.Ellipse(vine, 4, 2.5f, {0.20f, 0.34f, 0.17f});
                }
            }
        }
    }
    if (b.kind == 9)
    {
        for (float tier = 0.7f; tier < b.h; tier += 0.75f)
        {
            r.Line(projectBox(b.x, b.y + b.d, tier),
                   projectBox(b.x + b.w, b.y + b.d, tier),
                   2,
                   {0.19f, 0.19f, 0.14f});
            r.Line(projectBox(b.x + b.w, b.y, tier),
                   projectBox(b.x + b.w, b.y + b.d, tier),
                   2,
                   {0.19f, 0.19f, 0.14f});
        }
        r.Line(projectBox(b.x + 0.1f, b.y + b.d, 0.15f),
               projectBox(b.x + b.w - 0.1f, b.y + b.d, b.h - 0.1f),
               3,
               {0.57f, 0.45f, 0.29f});
        Vec2 label = projectBox(b.x + b.w * 0.4f, b.y + b.d + 0.01f, b.h * 0.6f);
        r.Quad(label,
               {label.x + 7, label.y + 2},
               {label.x + 6, label.y + 7},
               {label.x - 1, label.y + 5},
               {0.62f, 0.58f, 0.42f});
    }
    if (b.kind == 3)
    {
        r.Surface(projectBox(b.x + 0.1f, b.y + b.d + 0.01f, 0.7f),
                  projectBox(b.x + b.w - 0.1f, b.y + b.d + 0.01f, 0.7f),
                  projectBox(b.x + b.w - 0.1f, b.y + b.d + 0.01f, 1.5f),
                  projectBox(b.x + 0.1f, b.y + b.d + 0.01f, 1.5f),
                  {0.08f, 0.22f, 0.23f, b.color.a},
                  2,
                  1,
                  1);
        Vec2 led = projectBox(b.x + 0.3f, b.y + b.d + 0.02f, 1.25f);
        r.Line(led, {led.x + 9, led.y + 4}, 2, {0.46f, 1.05f, 0.89f, b.color.a});
        for (int vent = 0; vent < 4; ++vent)
        {
            Vec2 p = projectBox(b.x + 0.1f, b.y + b.d + 0.02f, 0.2f + vent * 0.1f);
            r.Line(p, {p.x + 10, p.y + 5}, 1, {0.1f, 0.14f, 0.14f, b.color.a});
        }
    }
    r.Line(dt, ct, 1, {0.7f, 0.73f, 0.6f, b.color.a * 0.3f});
    if (b.h > 1 && (b.kind == 0 || b.kind == 8))
    {
        Vec2 crack{ct.x - 3, ct.y + 12};
        r.Line(crack, {crack.x - 5, crack.y + 13}, 1, Shade(b.color, 0.55f));
        // Hanging vines on broken masonry.
        for (int i = 0; i < 3; ++i)
        {
            float t = (i + 1) * 0.24f;
            Vec2 vine{dt.x + (ct.x - dt.x) * t, dt.y + (ct.y - dt.y) * t};
            r.Line(vine, {vine.x - 3, vine.y + 16 + i * 4}, 2, {0.18f, 0.32f, 0.19f, b.color.a});
            r.Ellipse({vine.x - 3, vine.y + 10}, 4, 2, {0.28f, 0.43f, 0.24f, b.color.a});
        }
    }
    if (b.kind == 1)
    {
        Vec2 light = projectBox(b.x, b.y, b.h);
        r.Ellipse(light, 18, 18, {1, 0.74f, 0.32f, 0.07f});
        r.Rect(light.x - 3, light.y - 5, 6, 10, {1.9f, 1.4f, 0.65f});
    }
}

void Chapter1::DrawPlant(Renderer& r, const Plant& plant)
{
    Vec2 p = Project(plant.x, plant.y);
    if (p.x < -70 || p.x > m_Width + 70 || p.y < -70 || p.y > m_Height + 70)
        return;
    float s = plant.size * m_Scale;
    Vec2 avatar = Project(m_Player.x, m_Player.y, 0.6f);
    float alpha =
        (p.y > avatar.y && std::abs(p.x - avatar.x) < 24 && p.y - avatar.y < 45) ? 0.25f : 1.0f;
    r.Ellipse(p, s * 0.7f, s * 0.22f, {0.04f, 0.1f, 0.06f, 0.18f});
    for (int i = 0; i < 5; ++i)
    {
        float angle = i * 1.3f + plant.x;
        float dx =
            std::cos(angle) * s * 0.65f + std::sin(m_Time * 1.4f + plant.x + plant.y) * s * 0.13f;
        float dy = -s * (0.55f + 0.25f * std::sin(angle));
        r.Triangle({p.x - 2, p.y},
                   {p.x + dx, p.y + dy},
                   {p.x + 3, p.y - 4},
                   {0.18f + i * 0.018f, 0.31f + i * 0.025f, 0.17f + i * 0.012f, alpha});
    }
}

void Chapter1::Person(Renderer& r, Vec2 position, bool player)
{
    Vec2 p = Project(position.x, position.y);
    const float scale = m_Scale / 30;
    r.Ellipse({p.x + 1, p.y + 2}, 9 * scale, 3.5f * scale, {0.015f, 0.025f, 0.02f, 0.28f});
    r.Character(p,
                scale * 0.76f,
                player ? m_Facing : 0,
                player && m_Moving ? 1 + static_cast<int>(m_Walk) % 7 : 0,
                !player);
}

void Chapter1::Draw(Renderer& r, int width, int height)
{
    m_Width = width;
    m_Height = height;
    m_Scale = 30 * (std::min)(1.35f, (std::max)(0.75f, height / 800.0f));
    r.Begin(m_Time);
    // Muted paving, moss seams and floor slabs establish the abandoned garden district.
    for (int y = 0; y < 40; ++y)
        for (int x = 0; x < 42; ++x)
        {
            Vec2 p = Project(static_cast<float>(x), static_cast<float>(y));
            if (p.x < -80 || p.x > width + 80 || p.y < -80 || p.y > height + 100)
                continue;
            float variation = ((x * 17 + y * 31) % 11) * 0.006f;
            bool path = (x >= 8 && x <= 10 && y >= 5 && y <= 35) ||
                        (x >= 8 && x <= 40 && y >= 17 && y <= 19);
            int debrisSeed = (x * 37 + y * 19 + x * y * 3) % 101;
            path = (path && debrisSeed % 7 != 0) || (!path && debrisSeed % 19 == 0);
            Color c = path ? Color(0.34f + variation, 0.37f + variation, 0.31f + variation)
                           : Color(0.20f + variation, 0.27f + variation, 0.20f + variation);
            if (x >= 2 && x <= 9 && y >= 30 && y <= 37)
            {
                c = {0.39f + variation, 0.36f + variation, 0.27f};
            }
            else if (x >= 32 && x <= 39 && y >= 2 && y <= 27)
            {
                c = {0.31f + variation, 0.36f + variation, 0.31f};
            }
            // Cover the whole tile so clear-color gaps cannot form dark outlines.
            Ground(r, static_cast<float>(x), static_cast<float>(y), 1, 1, c);
            if (debrisSeed % 9 == 0)
            {
                r.Quad(Project(x + 0.18f, y + 0.25f),
                       Project(x + 0.7f, y + 0.15f),
                       Project(x + 0.85f, y + 0.65f),
                       Project(x + 0.35f, y + 0.8f),
                       {0.22f, 0.23f, 0.17f, 0.25f});
            }
            if (debrisSeed % 11 == 0)
            {
                r.Line(Project(x + 0.1f, y + 0.25f),
                       Project(x + 0.5f, y + 0.6f),
                       1,
                       {0.10f, 0.14f, 0.12f});
                r.Line(Project(x + 0.5f, y + 0.6f),
                       Project(x + 0.8f, y + 0.55f),
                       1,
                       {0.10f, 0.14f, 0.12f});
            }
            if (debrisSeed % 13 == 0)
            {
                Vec2 scrap = Project(x + 0.6f, y + 0.4f);
                r.Ellipse(scrap, m_Scale * 0.12f, m_Scale * 0.07f, {0.38f, 0.37f, 0.30f});
                r.Line(
                    {scrap.x + 3, scrap.y}, {scrap.x + 8, scrap.y - 3}, 2, {0.48f, 0.40f, 0.29f});
            }
        }
    Vec2 glow = Project(4, 32);
    DrawShadows(r);
    r.Ellipse(glow, 120, 55, {1, 0.77f, 0.32f, 0.055f});

    // All upright objects share one painter ordering; the background is not a flat mockup.
    struct Item
    {
        float depth;
        int type, index;
    };

    std::vector<Item> items;
    items.reserve(m_Boxes.size() + m_Plants.size() + m_Creatures.size() + 6);
    for (size_t i = 0; i < m_Boxes.size(); ++i)
    {
        const Box& b = m_Boxes[i];
        items.push_back({b.x + b.y + (b.w + b.d) * 0.5f, 0, static_cast<int>(i)});
    }
    for (size_t i = 0; i < m_Plants.size(); ++i)
        items.push_back({m_Plants[i].x + m_Plants[i].y, 1, static_cast<int>(i)});
    items.push_back({m_Player.x + m_Player.y, 2, 0});
    items.push_back({m_Rose.x + m_Rose.y, 7, 0});
    if (m_Quest >= Quest::FollowingSound && m_Player.x > 37 && m_Player.y > 29 &&
        ClearLine(m_Player, m_Speaker))
    {
        items.push_back({m_Speaker.x + m_Speaker.y, 8, 0});
    }
    items.push_back({m_Storage.x + m_Storage.y, 3, 0});
    items.push_back({m_Food.x + m_Food.y, 4, 0});
    items.push_back({m_Supplies.x + m_Supplies.y, 5, 0});
    for (size_t i = 0; i < m_Creatures.size(); ++i)
        items.push_back(
            {m_Creatures[i].position.x + m_Creatures[i].position.y, 6, static_cast<int>(i)});
    std::stable_sort(items.begin(),
                     items.end(),
                     [](const Item& a, const Item& b)
                     {
                         return a.depth < b.depth;
                     });

    // A footprint, not its center, decides whether an actor is behind a building.
    struct Bounds
    {
        float left, top, right, bottom;
        float screenLeft, screenTop, screenRight, screenBottom;
    };

    std::vector<Bounds> bounds;
    bounds.reserve(items.size());
    for (const Item& item : items)
    {
        Vec2 p;
        if (item.type == 0)
        {
            const Box& box = m_Boxes[item.index];
            Vec2 left = Project(box.x, box.y + box.d);
            Vec2 right = Project(box.x + box.w, box.y);
            Vec2 top = Project(box.x, box.y, box.h);
            Vec2 bottom = Project(box.x + box.w, box.y + box.d);
            bounds.push_back({box.x,
                              box.y,
                              box.x + box.w,
                              box.y + box.d,
                              left.x - 6,
                              top.y - 16,
                              right.x + 6,
                              bottom.y + 6});
            continue;
        }
        switch (item.type)
        {
        case 1:
            p = {m_Plants[item.index].x, m_Plants[item.index].y};
            break;
        case 2:
            p = m_Player;
            break;
        case 3:
            p = m_Storage;
            break;
        case 4:
            p = m_Food;
            break;
        case 5:
            p = m_Supplies;
            break;
        case 6:
            p = m_Creatures[item.index].position;
            break;
        case 7:
            p = m_Rose;
            break;
        default:
            p = m_Speaker;
            break;
        }
        Vec2 screen = Project(p.x, p.y);
        bounds.push_back({p.x,
                          p.y,
                          p.x,
                          p.y,
                          screen.x - 40 * m_Scale / 30,
                          screen.y - 100 * m_Scale / 30,
                          screen.x + 40 * m_Scale / 30,
                          screen.y + 12});
    }
    // Remove offscreen geometry before building the occlusion graph.
    size_t visibleCount = 0;
    for (size_t i = 0; i < items.size(); ++i)
    {
        const Bounds& b = bounds[i];
        if (b.screenRight < -64 || b.screenLeft > width + 64 || b.screenBottom < -64 ||
            b.screenTop > height + 64)
        {
            continue;
        }
        items[visibleCount] = items[i];
        bounds[visibleCount] = bounds[i];
        ++visibleCount;
    }
    items.resize(visibleCount);
    bounds.resize(visibleCount);

    // Sweep screen X: disjoint objects cannot constrain one another.
    std::vector<size_t> sweep(items.size());
    std::iota(sweep.begin(), sweep.end(), size_t{0});
    std::sort(sweep.begin(),
              sweep.end(),
              [&](size_t a, size_t b)
              {
                  return bounds[a].screenLeft < bounds[b].screenLeft;
              });
    std::vector<std::vector<size_t>> after(items.size());
    std::vector<int> incoming(items.size(), 0);
    for (size_t u = 0; u < sweep.size(); ++u)
    {
        size_t i = sweep[u];
        const Bounds& a = bounds[i];
        for (size_t v = u + 1; v < sweep.size(); ++v)
        {
            size_t j = sweep[v];
            const Bounds& b = bounds[j];
            if (b.screenLeft > a.screenRight)
            {
                break;
            }
            if ((items[i].type != 0 && items[j].type != 0) || a.screenBottom < b.screenTop ||
                b.screenBottom < a.screenTop)
            {
                continue;
            }
            bool aBehind = a.right <= b.left || a.bottom <= b.top;
            bool bBehind = b.right <= a.left || b.bottom <= a.top;
            if (aBehind == bBehind)
            {
                continue;
            }
            size_t first = aBehind ? i : j, second = aBehind ? j : i;
            after[first].push_back(second);
            ++incoming[second];
        }
    }
    std::vector<Item> ordered;
    ordered.reserve(items.size());
    std::vector<bool> drawn(items.size(), false);
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> ready;
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (incoming[i] == 0)
        {
            ready.push(i);
        }
    }
    size_t fallback = 0;
    while (ordered.size() < items.size())
    {
        while (!ready.empty() && drawn[ready.top()])
        {
            ready.pop();
        }
        size_t next;
        if (!ready.empty())
        {
            next = ready.top();
            ready.pop();
        }
        else
        {
            // Preserve the original deterministic cycle fallback.
            while (drawn[fallback])
            {
                ++fallback;
            }
            next = fallback;
        }
        drawn[next] = true;
        ordered.push_back(items[next]);
        for (size_t later : after[next])
        {
            if (--incoming[later] == 0 && !drawn[later])
            {
                ready.push(later);
            }
        }
    }
    items.swap(ordered);
    for (const Item& item : items)
    {
        if (item.type == 0)
            DrawBox(r, m_Boxes[item.index]);
        else if (item.type == 1)
            DrawPlant(r, m_Plants[item.index]);
        else if (item.type == 2)
            Person(r, m_Player, true);
        else if (item.type == 3)
            DrawBox(r,
                    {m_Storage.x - 0.4f,
                     m_Storage.y - 0.35f,
                     0.8f,
                     0.7f,
                     0.7f,
                     {0.53f, 0.43f, 0.24f},
                     false,
                     4});
        else if (item.type == 7)
        {
            DrawRose(r);
        }
        else if (item.type == 8)
        {
            DrawSpeaker(r);
        }
        else if (item.type == 6)
            DrawCreature(r, m_Creatures[item.index]);
        else
        {
            bool food = item.type == 4, empty = food ? m_HasFood : m_HasSupplies;
            Vec2 p = food ? m_Food : m_Supplies;
            Color crate = food ? Color(0.61f, 0.43f, 0.21f) : Color(0.32f, 0.5f, 0.48f);
            DrawBox(r, {p.x - 0.4f, p.y - 0.35f, 0.8f, 0.7f, 0.5f, crate, false, 4});
            if (!empty)
            {
                for (int i = 0; i < 3; ++i)
                {
                    Vec2 can = Project(p.x - 0.22f + i * 0.22f, p.y, 0.65f);
                    r.Rect(can.x - 3,
                           can.y - 5,
                           6,
                           8,
                           food ? Color(0.87f, 0.7f, 0.38f) : Color(0.79f, 0.85f, 0.73f));
                }
                Vec2 marker = Project(p.x, p.y, 1.2f);
                r.Ellipse(marker, 5, 5, {0.99f, 0.83f, 0.4f, 0.85f});
            }
        }
    }
    DrawDirection(r);
    int target = Target();
    if (target >= 0)
    {
        Vec2 p = Project(TargetPosition(target).x, TargetPosition(target).y);
        r.Ellipse(p, 22, 9, {1, 0.81f, 0.37f, 0.25f});
    }
    // Soft edge shade leaves the central gameplay area readable.
    for (int i = 0; i < 12; ++i)
    {
        float f = static_cast<float>(i);
        r.Rect(f * 3, 0, 3, static_cast<float>(height), {0.02f, 0.06f, 0.04f, 0.18f - f * 0.012f});
        r.Rect(width - (f + 1) * 3,
               0,
               3,
               static_cast<float>(height),
               {0.02f, 0.06f, 0.04f, 0.18f - f * 0.012f});
    }
    DrawAtmosphere(r);
    if (m_Quest == Quest::FollowingSound && !m_CreatureDead)
    {
        Vec2 source = Project(m_Speaker.x, m_Speaker.y, 0.4f);
        float pulse = std::fmod(m_Time, 1.8f) / 1.8f;
        r.Ellipse(
            source, 8 + 18 * pulse, 4 + 9 * pulse, {0.85f, 0.89f, 0.64f, (1 - pulse) * 0.45f});
    }
    r.FinishWorld(m_Time);
    DrawUI(r);
    if (m_EntranceTime < 1.2f)
    {
        float alpha = 1 - m_EntranceTime / 1.2f;
        r.Rect(0, 0, static_cast<float>(width), static_cast<float>(height), {0, 0, 0, alpha});
    }
    r.Flush();
}

void Chapter1::DrawUI(Renderer& r)
{
    if (DrawStoryUI(r))
    {
        return;
    }
    DrawMinimap(r);
    const wchar_t* controlHint = nullptr;
    if (m_InventoryUnlocked && !m_InventoryLearned)
    {
        controlHint = L"E로 인벤토리 열기";
    }
    else if (m_InteractionHintActive && !m_InteractionLearned)
    {
        controlHint = L"F로 상호작용 하기";
    }
    else if (m_MoveHintActive)
    {
        controlHint = L"WASD로 이동하기";
    }
    if (controlHint)
    {
        DrawControlHint(r, controlHint, m_Height - 204.0f);
    }
}

void Chapter1::DrawControlHint(Renderer& r, const wchar_t* text, float dialogueTop)
{
    const float top = dialogueTop - 58;
    r.Rect(m_Width * 0.5f - 180, top, 360, 46, {0.035f, 0.07f, 0.065f, 0.95f});
    r.Text(m_Width * 0.5f, top + 9, text, {1, 0.87f, 0.52f}, 1.0f, true);
}