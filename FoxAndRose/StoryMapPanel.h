#pragma once
#include "Renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

inline int StoryMapCount(unsigned pieces)
{
    int count = 0;
    for (int i = 0; i < 8; ++i)
    {
        count += (pieces >> i) & 1u;
    }
    return count;
}

inline const wchar_t* StoryMapName(unsigned pieces)
{
    int count = StoryMapCount(pieces);
    return count == 8 ? L"지도" : count > 1 ? L"지도 조각들" : L"지도 조각";
}

// Shared torn edges form eight contiguous pieces. No unrecovered route is drawn.
inline std::vector<Vec2> StoryMapPolygon(int piece)
{
    static const int cells[] = {0, 4, 5, 7, 6, 3, 2, 1};
    int column = cells[piece] % 4, row = cells[piece] / 4;
    auto vertex = [](int x, int y)
    {
        float px = x / 16.0f, py = y / 8.0f;
        if (x > 0 && x < 16 && x % 4 == 0 && y % 4 != 0)
        {
            px += (y % 2 ? 0.012f : -0.014f);
        }
        if (y == 4 && x % 4 != 0)
        {
            py += (x % 2 ? 0.022f : -0.025f);
        }
        return Vec2{px, py};
    };
    std::vector<Vec2> polygon;
    for (int i = 0; i < 4; ++i)
    {
        polygon.push_back(vertex(column * 4 + i, row * 4));
    }
    for (int i = 0; i < 4; ++i)
    {
        polygon.push_back(vertex(column * 4 + 4, row * 4 + i));
    }
    for (int i = 0; i < 4; ++i)
    {
        polygon.push_back(vertex(column * 4 + 4 - i, row * 4 + 4));
    }
    for (int i = 0; i < 4; ++i)
    {
        polygon.push_back(vertex(column * 4, row * 4 + 4 - i));
    }
    return polygon;
}

inline bool StoryMapContains(const std::vector<Vec2>& polygon, Vec2 p)
{
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
    {
        Vec2 a = polygon[i], b = polygon[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
        {
            inside = !inside;
        }
    }
    return inside;
}

inline void DrawStoryMapFragment(
    Renderer& r, int piece, float x, float y, float w, float h, bool selected = false)
{
    auto polygon = StoryMapPolygon(piece);
    auto project = [&](Vec2 p)
    {
        return Vec2{x + p.x * w, y + p.y * h};
    };
    Vec2 center{};
    for (Vec2 p : polygon)
    {
        center.x += p.x / static_cast<float>(polygon.size());
        center.y += p.y / static_cast<float>(polygon.size());
    }
    Color paper = selected ? Color(0.9f, 0.82f, 0.62f) : Color(0.77f, 0.71f, 0.54f);
    for (size_t i = 0; i < polygon.size(); ++i)
    {
        r.Triangle(project(center),
                   project(polygon[i]),
                   project(polygon[(i + 1) % polygon.size()]),
                   paper);
    }
    Color ink{0.34f, 0.28f, 0.18f, 0.48f};
    auto stroke = [&](Vec2 a, Vec2 b, float thickness, Color color)
    {
        int steps = (std::max)(1,
                               (std::min)(48,
                                          static_cast<int>(std::ceil(
                                              std::hypot((b.x - a.x) * w, (b.y - a.y) * h) / 3))));
        for (int step = 0; step < steps; ++step)
        {
            float t = step / static_cast<float>(steps), u = (step + 1) / static_cast<float>(steps);
            Vec2 from{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
            Vec2 to{a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u};
            if (StoryMapContains(polygon, from) && StoryMapContains(polygon, to))
            {
                r.Line(project(from), project(to), thickness, color);
            }
        }
    };
    // Soft, uneven road beds and worn edges share the parchment's warm pigments.
    auto road = [&](const std::vector<Vec2>& points, float size)
    {
        for (size_t i = 1; i < points.size(); ++i)
        {
            Vec2 a = points[i - 1], b = points[i];
            stroke(a, b, w * size * 1.4f, {0.56f, 0.43f, 0.26f, 0.10f});
            stroke(a, b, w * size, {0.61f, 0.49f, 0.31f, 0.22f});
            float dx = b.x - a.x, dy = b.y - a.y;
            float length = std::hypot(dx, dy);
            if (length > 0)
            {
                Vec2 edge{-dy / length * size * 0.42f, dx / length * size * 0.42f};
                stroke({a.x + edge.x, a.y + edge.y},
                       {b.x + edge.x, b.y + edge.y},
                       0.8f,
                       {0.37f, 0.31f, 0.21f, 0.3f});
                stroke({a.x - edge.x, a.y - edge.y},
                       {b.x - edge.x, b.y - edge.y},
                       0.8f,
                       {0.37f, 0.31f, 0.21f, 0.22f});
            }
        }
    };
    // One continuous painted route; no rectangular image or destination badge is pasted on.
    road({{0.029f, 0.208f},
          {0.012f, 0.224f},
          {0.027f, 0.30f},
          {0.074f, 0.40f},
          {0.12f, 0.55f},
          {0.12f, 0.76f}},
         0.008f);
    road({{0.12f, 0.76f},
          {0.25f, 0.80f},
          {0.37f, 0.79f},
          {0.62f, 0.80f},
          {0.88f, 0.78f},
          {0.75f, 0.70f},
          {0.64f, 0.63f},
          {0.79f, 0.42f},
          {0.88f, 0.23f},
          {0.73f, 0.18f},
          {0.63f, 0.22f},
          {0.43f, 0.30f},
          {0.235f, 0.40f}},
         0.007f);
    if (piece == 0)
    {
        // Chapter 1's real top-down footprints, drawn directly into the parchment.
        auto terrain = [](float px, float py)
        {
            return Vec2{0.022f + px * 0.0044f, 0.055f + py * 0.006f};
        };
        auto wall = [&](float ax, float ay, float bx, float by)
        {
            stroke(terrain(ax, ay), terrain(bx, by), 1.2f, ink);
        };
        auto ruin = [&](float px, float py, float width, float depth)
        {
            Vec2 a = terrain(px, py), b = terrain(px + width, py + depth);
            r.Quad(project(a),
                   project({b.x, a.y}),
                   project(b),
                   project({a.x, b.y}),
                   {0.36f, 0.31f, 0.20f, 0.12f});
            wall(px, py, px + width, py);
            wall(px + width, py, px + width, py + depth);
            wall(px + width, py + depth, px, py + depth);
            wall(px, py + depth, px, py);
            wall(px + 1, py + 1, px + width - 1, py + depth - 1);
            wall(px + 1, py + depth - 1, px + width - 1, py + 1);
        };
        road({terrain(9, 5), terrain(9, 19), terrain(9, 25.5f), terrain(1.6f, 25.5f)}, 0.006f);
        road({terrain(9, 34), terrain(13, 34), terrain(13, 25.5f), terrain(9, 25.5f)}, 0.006f);
        road({terrain(9, 19), terrain(22, 18), terrain(39, 18), terrain(39, 5)}, 0.006f);
        road({terrain(39, 18), terrain(39, 30), terrain(39, 36)}, 0.005f);
        ruin(1, 14, 7, 9);
        ruin(11, 8, 6, 9);
        ruin(24, 1, 7, 8);
        ruin(24, 20, 7, 9);
        // Open shelter outline: no generic house marker at the first destination.
        wall(1, 29, 11, 29);
        wall(1, 29, 1, 39);
        wall(1, 39, 11, 39);
        wall(11, 29, 11, 33);
        wall(11, 35.5f, 11, 39);
        wall(31, 1, 41, 1);
        wall(41, 1, 41, 29);
        wall(31, 1, 31, 16);
        wall(31, 20, 31, 29);
        wall(31, 29, 38, 29);
        // Fences, supplies and beds distinguish the sketch from an abstract route diagram.
        wall(17, 1, 17, 8);
        wall(17, 8, 24, 8);
        wall(31, 29, 31, 39);
        for (int i = 0; i < 7; ++i)
        {
            wall(16.5f, 1.0f + i, 17.5f, 1.0f + i);
            wall(17.0f + i, 7.5f, 17.0f + i, 8.5f);
        }
        ruin(2, 36, 2.5f, 1);
        ruin(5, 36, 2.5f, 1);
        ruin(34, 7, 3, 6);
        ruin(35, 20, 5, 2);
        ruin(32, 30, 2, 9);
        ruin(34, 35, 3, 4);
        ruin(2, 2, 1.5f, 7);
        for (int i = 0; i < 24; ++i)
        {
            float px = 2.0f + (i * 13 % 39), py = 2.0f + (i * 17 % 36);
            Vec2 v = terrain(px, py);
            stroke({v.x - 0.002f, v.y + 0.003f},
                   {v.x, v.y - 0.002f},
                   0.8f,
                   {0.34f, 0.38f, 0.22f, 0.28f});
            stroke({v.x, v.y - 0.002f},
                   {v.x + 0.002f, v.y + 0.003f},
                   0.8f,
                   {0.34f, 0.38f, 0.22f, 0.28f});
        }
    }
    // The final destination straddles the tear shared by the first and eighth pieces.
    // Only its recovered contour is visible; its future detailed map is not invented here.
    const Vec2 home{0.235f, 0.40f};
    for (int i = 0; i < 64; ++i)
    {
        float a = i * 6.2831853f / 64, b = (i + 1) * 6.2831853f / 64;
        stroke({home.x + std::cos(a) * 0.047f, home.y + std::sin(a) * 0.068f},
               {home.x + std::cos(b) * 0.047f, home.y + std::sin(b) * 0.068f},
               1.0f,
               {0.34f, 0.28f, 0.18f, 0.4f});
    }
    if (piece == 0)
    {
        r.Text(x + w * 0.167f, y + h * 0.27f, L"우리의 집", ink, 0.7f);
    }
    // Sparse paper fibers unify the blank paper and illustrated terrain.
    for (int i = 0; i < 36; ++i)
    {
        Vec2 v{center.x + ((i * 17 % 29) - 14) * 0.007f, center.y + ((i * 11 % 31) - 15) * 0.013f};
        stroke(v, {v.x + 0.005f, v.y + 0.001f}, 0.6f, {0.43f, 0.35f, 0.22f, 0.1f});
    }
    for (size_t i = 0; i < polygon.size(); ++i)
    {
        r.Line(project(polygon[i]),
               project(polygon[(i + 1) % polygon.size()]),
               1.5f,
               selected ? Color(0.98f, 0.8f, 0.38f) : Color(0.43f, 0.37f, 0.25f));
    }
}

inline void DrawStoryMapPiece(Renderer& r, int piece, float x, float y, float w, float h)
{
    // Chapter 1 discovery shows only the first torn portion, never the whole route.
    DrawStoryMapFragment(r, piece, x + 12, y + 10, (w - 24) * 4, (h - 20) * 2);
}

struct InventoryViewState
{
    bool mapOpen = false;
    int selectedPiece = -1;
    int selectedItem = -1;

    bool Back()
    {
        if (selectedPiece >= 0 || selectedItem >= 0)
        {
            selectedPiece = selectedItem = -1;
            return true;
        }
        if (mapOpen)
        {
            mapOpen = false;
            return true;
        }
        return false;
    }
};

inline void DrawWrappedInfo(Renderer& r, float x, float y, float width, const std::wstring& text)
{
    const auto rows = r.WrapText(text, width, 0.8f);
    for (size_t row = 0; row < rows.size(); ++row)
    {
        r.Text(x, y + static_cast<float>(row) * 25, rows[row], {0.94f, 0.9f, 0.78f}, 0.8f);
    }
}

struct StoryMapLayout
{
    float x, y, w, h;

    explicit StoryMapLayout(int width, int height)
    {
        w = (std::min)(960.0f, width - 48.0f);
        h = (std::min)(640.0f, height - 48.0f);
        x = (width - w) * 0.5f;
        y = (height - h) * 0.5f;
    }
};

inline void DrawStoryMapPanel(Renderer& r,
                              int width,
                              int height,
                              unsigned pieces,
                              const InventoryViewState& state,
                              int mouseX,
                              int mouseY)
{
    StoryMapLayout p(width, height);
    r.Rect(0, 0, static_cast<float>(width), static_cast<float>(height), {0, 0, 0, 0.85f});
    r.Rect(p.x - 2, p.y - 2, p.w + 4, p.h + 4, {0.65f, 0.61f, 0.46f});
    r.Rect(p.x, p.y, p.w, p.h, {0.12f, 0.17f, 0.2f});
    r.Text(p.x + 24, p.y + 16, StoryMapName(pieces));
    float mx = p.x + 32, my = p.y + 68, mw = p.w - 64, mh = p.h - 240;
    r.Rect(mx, my, mw, mh, {0.075f, 0.10f, 0.12f});
    Vec2 cursor{(mouseX - mx) / mw, (mouseY - my) / mh};
    for (int i = 0; i < 8; ++i)
    {
        if (pieces & (1u << i))
        {
            DrawStoryMapFragment(r,
                                 i,
                                 mx,
                                 my,
                                 mw,
                                 mh,
                                 state.selectedPiece == i ||
                                     StoryMapContains(StoryMapPolygon(i), cursor));
        }
    }
    if (state.selectedPiece == 0)
    {
        DrawWrappedInfo(r,
                        mx,
                        my + mh + 18,
                        mw,
                        L"이상한 크리처가 가지고 있던 지도 조각이다. 「우리의 집」이라는 글자와 "
                        L"어딘가로 향하는 "
                        L"길이 그려져 있다. 이 길을 따라가면 뭐가 나올까?");
    }
    else if (state.selectedPiece >= 0)
    {
        DrawWrappedInfo(r, mx, my + mh + 18, mw, L"모험 중 모은 지도 조각이다.");
    }
    else
    {
        r.Text(mx,
               my + mh + 18,
               L"모은 지도 조각을 클릭하면 자세히 살펴볼 수 있습니다.",
               {0.73f, 0.77f, 0.72f},
               0.8f);
    }
    r.Text(mx, p.y + p.h - 34, L"E 인벤토리로 돌아가기", {0.7f, 0.76f, 0.74f}, 0.8f);
}

inline void ClickStoryMap(
    int width, int height, int x, int y, unsigned pieces, InventoryViewState& state)
{
    StoryMapLayout p(width, height);
    Vec2 cursor{(x - p.x - 32) / (p.w - 64), (y - p.y - 68) / (p.h - 240)};
    for (int i = 0; i < 8; ++i)
    {
        if ((pieces & (1u << i)) && StoryMapContains(StoryMapPolygon(i), cursor))
        {
            state.selectedPiece = i;
            return;
        }
    }
}