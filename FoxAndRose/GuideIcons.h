#pragma once
#include "Renderer.h"
#include <algorithm>
#include <cmath>

enum class GoalIcon
{
    Food,
    Supplies,
    Store,
    Sound,
    Talk,
    Depart
};

inline void DrawKeyIcon(Renderer& r, Vec2 p, const wchar_t* key)
{
    r.Rect(p.x - 15, p.y - 16, 30, 32, {0.65f, 0.68f, 0.69f});
    r.Rect(p.x - 12, p.y - 13, 24, 24, {1, 1, 1});
    r.Text(p.x, p.y - 1, key, {0.08f, 0.10f, 0.09f}, 0.9f, true, true);
}

inline void DrawMouseGuide(Renderer& r, Vec2 p)
{
    Color shell{0.85f, 0.85f, 0.74f}, dark{0.09f, 0.13f, 0.12f};
    r.Ellipse(p, 15, 22, shell);
    r.Ellipse(p, 12, 19, dark);
    // A depressed left button; the right button remains unlit.
    r.Quad({p.x - 10, p.y - 12},
           {p.x - 2, p.y - 16},
           {p.x - 2, p.y - 2},
           {p.x - 11, p.y - 2},
           {1, 0.76f, 0.28f});
    r.Line({p.x, p.y - 17}, {p.x, p.y - 1}, 1.5f, shell);
    r.Line({p.x - 11, p.y}, {p.x + 11, p.y}, 1.5f, shell);
    r.Rect(p.x - 2, p.y - 10, 4, 6, shell);
}

inline void DrawGoalIcon(Renderer& r, Vec2 p, GoalIcon icon)
{
    const Color gold{1, 0.84f, 0.35f};
    if (icon == GoalIcon::Sound)
    {
        r.Line({p.x + 3, p.y - 13}, {p.x + 3, p.y + 9}, 3, gold);
        r.Line({p.x + 3, p.y - 13}, {p.x + 13, p.y - 9}, 4, gold);
        r.Ellipse({p.x - 2, p.y + 9}, 7, 5, gold);
    }
    else if (icon == GoalIcon::Talk)
    {
        r.Ellipse({p.x, p.y - 3}, 16, 11, gold);
        r.Triangle({p.x - 10, p.y + 3}, {p.x - 12, p.y + 15}, {p.x + 1, p.y + 5}, gold);
        for (int i = -1; i <= 1; ++i)
        {
            r.Ellipse({p.x + i * 7, p.y - 3}, 1.5f, 1.5f, {0.16f, 0.18f, 0.12f});
        }
    }
    else if (icon == GoalIcon::Depart)
    {
        r.Line({p.x - 12, p.y + 15}, {p.x - 12, p.y - 15}, 3, gold);
        r.Line({p.x - 12, p.y - 15}, {p.x + 3, p.y - 15}, 3, gold);
        r.Line({p.x - 5, p.y}, {p.x + 13, p.y}, 3, gold);
        r.Triangle({p.x + 16, p.y}, {p.x + 8, p.y - 7}, {p.x + 8, p.y + 7}, gold);
    }
    else if (icon == GoalIcon::Food || icon == GoalIcon::Supplies)
    {
        // Open carton: visible opening, two outward flaps and folded side faces.
        r.Quad({p.x - 11, p.y - 5},
               {p.x, p.y - 10},
               {p.x + 11, p.y - 5},
               {p.x, p.y + 1},
               {0.43f, 0.32f, 0.12f});
        r.Quad({p.x - 11, p.y - 5}, {p.x, p.y + 1}, {p.x, p.y + 15}, {p.x - 11, p.y + 9}, gold);
        r.Quad({p.x, p.y + 1},
               {p.x + 11, p.y - 5},
               {p.x + 11, p.y + 9},
               {p.x, p.y + 15},
               {0.83f, 0.61f, 0.23f});
        r.Quad(
            {p.x - 11, p.y - 5}, {p.x, p.y - 10}, {p.x - 6, p.y - 16}, {p.x - 18, p.y - 10}, gold);
        r.Quad(
            {p.x, p.y - 10}, {p.x + 11, p.y - 5}, {p.x + 18, p.y - 10}, {p.x + 6, p.y - 16}, gold);
    }
    else if (icon == GoalIcon::Store)
    {
        r.Triangle({p.x - 17, p.y - 2}, {p.x, p.y - 17}, {p.x + 17, p.y - 2}, gold);
        r.Rect(p.x - 12, p.y - 2, 24, 17, gold);
        r.Rect(p.x - 4, p.y + 4, 8, 11, {0.22f, 0.24f, 0.15f});
    }
}

// The contour follows the screen inset and detours around occupied HUD corners.
inline Vec2 PlaceGuideIcon(
    float width, float height, Vec2 target, bool hasHp, const std::vector<Vec2>& occupied)
{
    constexpr float inset = 34;
    std::vector<Vec2> contour{{272, inset}};
    if (hasHp)
    {
        contour.push_back({width - 380, inset});
        contour.push_back({width - 380, 112});
        contour.push_back({width - inset, 112});
    }
    else
    {
        contour.push_back({width - inset, inset});
    }
    contour.push_back({width - inset, height - inset});
    contour.push_back({inset, height - inset});
    contour.push_back({inset, 262});
    contour.push_back({272, 262});
    Vec2 center{width * 0.5f, height * 0.5f};
    Vec2 direction{target.x - center.x, target.y - center.y};
    float best = 1e30f;
    Vec2 result = contour.front();
    // Solve continuous ray/edge intersections instead of snapping to 4-pixel samples.
    for (size_t i = 0; i < contour.size(); ++i)
    {
        Vec2 a = contour[i], b = contour[(i + 1) % contour.size()];
        float length = std::hypot(b.x - a.x, b.y - a.y);
        if (length < 0.001f)
        {
            continue;
        }
        Vec2 axis{(b.x - a.x) / length, (b.y - a.y) / length};
        std::vector<Vec2> intervals{{0, length}};
        for (Vec2 other : occupied)
        {
            float along = (other.x - a.x) * axis.x + (other.y - a.y) * axis.y;
            float perpendicular = (other.x - a.x) * axis.y - (other.y - a.y) * axis.x;
            if (std::abs(perpendicular) >= 42)
            {
                continue;
            }
            float half = std::sqrt(42 * 42 - perpendicular * perpendicular);
            float low = along - half, high = along + half;
            std::vector<Vec2> remaining;
            for (Vec2 interval : intervals)
            {
                if (low >= interval.y || high <= interval.x)
                {
                    remaining.push_back(interval);
                }
                else
                {
                    if (low > interval.x)
                    {
                        remaining.push_back({interval.x, low});
                    }
                    if (high < interval.y)
                    {
                        remaining.push_back({high, interval.y});
                    }
                }
            }
            intervals.swap(remaining);
        }
        float denominator = axis.x * direction.y - axis.y * direction.x;
        float intersection =
            std::abs(denominator) > 0.00001f
                ? ((center.x - a.x) * direction.y - (center.y - a.y) * direction.x) / denominator
                : 0;
        for (Vec2 interval : intervals)
        {
            float nearest = (std::max)(interval.x, (std::min)(interval.y, intersection));
            for (float distance : {interval.x, interval.y, nearest})
            {
                Vec2 p{a.x + axis.x * distance, a.y + axis.y * distance};
                float px = p.x - center.x, py = p.y - center.y;
                if (px * direction.x + py * direction.y <= 0)
                {
                    continue;
                }
                float cross = px * direction.y - py * direction.x;
                float score = cross * cross / (px * px + py * py);
                if (score < best)
                {
                    best = score;
                    result = p;
                }
            }
        }
    }
    return result;
}