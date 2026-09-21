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
    road({{0.1342f, 0.2974f}, {0.13f, 0.36f}, {0.12f, 0.46f}, {0.12f, 0.55f}, {0.12f, 0.76f}},
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
        // Rotate the chapter footprint 90 degrees counterclockwise before painting.
        auto terrain = [](float px, float py)
        {
            return Vec2{0.022f + py * 0.0044f, 0.055f + (42 - px) * 0.006f};
        };
        auto wash = [&](Vec2 p, float rx, float ry, Color color)
        {
            // Translucent overlapping brush dabs leave the parchment visible.
            Vec2 v = project(p);
            r.Ellipse(v, w * rx, h * ry, Color(color.r, color.g, color.b, color.a * 0.35f));
            r.Ellipse(v, w * rx * 0.76f, h * ry * 0.8f, color);
        };
        for (int i = 0; i < 38; ++i)
        {
            Vec2 p = terrain(3.0f + (i * 13 % 36), 4.0f + (i * 17 % 32));
            wash(p,
                 0.011f,
                 0.018f,
                 i % 3 ? Color(0.36f, 0.46f, 0.27f, 0.12f) : Color(0.68f, 0.53f, 0.32f, 0.14f));
        }
        road({terrain(9, 5), terrain(9, 19), terrain(9, 25.5f), terrain(1.6f, 25.5f)}, 0.007f);
        road({terrain(9, 34), terrain(13, 34), terrain(13, 25.5f), terrain(9, 25.5f)}, 0.007f);
        road({terrain(9, 19), terrain(22, 18), terrain(39, 18), terrain(39, 5)}, 0.007f);
        road({terrain(39, 18), terrain(39, 30), terrain(39, 36)}, 0.005f);
        auto building = [&](float px, float py, float bw, float bd, float elevation, Color roof)
        {
            Vec2 a = project(terrain(px + bw, py));
            Vec2 b = project(terrain(px + bw, py + bd));
            Vec2 c = project(terrain(px, py + bd));
            Vec2 d = project(terrain(px, py));
            float rise = h * elevation;
            Vec2 at{a.x, a.y - rise}, bt{b.x, b.y - rise};
            Vec2 ct{c.x, c.y - rise * 0.83f}, dt{d.x, d.y - rise};
            r.Ellipse({(c.x + d.x) * 0.5f + 3, c.y + 2},
                      std::abs(c.x - d.x) * 0.64f,
                      h * 0.012f,
                      {0.24f, 0.28f, 0.18f, 0.16f});
            r.Quad(d, c, ct, dt, {0.42f, 0.43f, 0.31f, 0.76f});
            r.Quad(b, c, ct, bt, {0.29f, 0.34f, 0.27f, 0.66f});
            r.Quad(at, bt, ct, dt, roof);
            // Broken roof lip, dark windows and hanging vegetation give small ruins volume.
            r.Line(at, bt, 1, {0.72f, 0.66f, 0.45f, 0.65f});
            for (int j = 1; j <= 3; ++j)
            {
                float t = j / 4.0f;
                float wx = d.x + (c.x - d.x) * t, wy = d.y - rise * 0.55f;
                r.Quad({wx - w * 0.0015f, wy},
                       {wx + w * 0.0015f, wy - 1},
                       {wx + w * 0.0015f, wy + rise * 0.28f},
                       {wx - w * 0.0015f, wy + rise * 0.24f},
                       {0.19f, 0.27f, 0.23f, 0.8f});
                r.Line({wx + 2, wy - rise * 0.4f},
                       {wx + 1, wy + rise * 0.3f},
                       1.5f,
                       {0.34f, 0.46f, 0.22f, 0.7f});
            }
        };
        auto wall = [&](float ax, float ay, float bx, float by)
        {
            Vec2 a = project(terrain(ax, ay)), b = project(terrain(bx, by));
            float rise = h * 0.009f;
            r.Quad(a, b, {b.x, b.y - rise}, {a.x, a.y - rise}, {0.45f, 0.45f, 0.31f, 0.7f});
            r.Line({a.x, a.y - rise}, {b.x, b.y - rise}, 1, {0.68f, 0.64f, 0.45f, 0.6f});
        };
        // Warehouse, then distant-to-near ruins in the rotated composition.
        wall(31, 1, 41, 1);
        wall(41, 1, 41, 29);
        wall(31, 1, 31, 16);
        wall(31, 20, 31, 29);
        wall(31, 29, 38, 29);
        building(34, 7, 3, 6, 0.012f, {0.64f, 0.44f, 0.25f, 0.8f});
        building(35, 20, 5, 2, 0.010f, {0.59f, 0.42f, 0.24f, 0.8f});
        building(34, 35, 3, 4, 0.019f, {0.58f, 0.41f, 0.26f, 0.8f});
        building(32, 30, 2, 9, 0.021f, {0.50f, 0.42f, 0.27f, 0.8f});
        building(24, 1, 7, 8, 0.034f, {0.52f, 0.58f, 0.40f, 0.85f});
        building(24, 20, 7, 9, 0.024f, {0.49f, 0.55f, 0.39f, 0.85f});
        wall(17, 1, 17, 8);
        wall(17, 8, 24, 8);
        building(11, 8, 6, 9, 0.030f, {0.55f, 0.58f, 0.40f, 0.85f});
        building(1, 14, 7, 9, 0.025f, {0.55f, 0.52f, 0.36f, 0.85f});
        wall(1, 29, 11, 29);
        wall(1, 29, 1, 39);
        wall(1, 39, 11, 39);
        wall(11, 29, 11, 33);
        wall(11, 35.5f, 11, 39);
        building(2, 36.3f, 1.9f, 1.1f, 0.005f, {0.42f, 0.51f, 0.33f, 0.8f});
        building(5, 36.3f, 1.9f, 1.1f, 0.005f, {0.42f, 0.51f, 0.33f, 0.8f});
        // Small green clusters and a warm campfire provide scenery, without a pasted background.
        for (int i = 0; i < 18; ++i)
        {
            Vec2 p = terrain(2.0f + (i * 19 % 38), 3.0f + (i * 11 % 33));
            wash(p, 0.0035f, 0.006f, {0.31f, 0.44f, 0.23f, 0.45f});
            wash({p.x - 0.001f, p.y - 0.002f}, 0.002f, 0.004f, {0.47f, 0.57f, 0.29f, 0.35f});
        }
        Vec2 fire = terrain(8.5f, 36.5f);
        wash(fire, 0.003f, 0.005f, {0.86f, 0.45f, 0.16f, 0.55f});
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

// Captions are a final layer, after every recovered piece and its illustration.
inline void DrawStoryMapCaption(Renderer& r, float x, float y, float w, float h)
{
    float tx = x + w * 0.167f, ty = y + h * 0.27f;
    for (Vec2 offset : {Vec2{-1, 0}, Vec2{1, 0}, Vec2{0, -1}, Vec2{0, 1}})
    {
        r.Text(tx + offset.x, ty + offset.y, L"우리의 집", {0.94f, 0.87f, 0.68f}, 0.7f);
    }
    r.Text(tx, ty, L"우리의 집", {0.24f, 0.18f, 0.10f}, 0.7f);
}

inline void DrawStoryMapPiece(Renderer& r, int piece, float x, float y, float w, float h)
{
    // Chapter 1 discovery shows only the first torn portion, never the whole route.
    DrawStoryMapFragment(r, piece, x + 12, y + 10, (w - 24) * 4, (h - 20) * 2);
    if (piece == 0)
    {
        DrawStoryMapCaption(r, x + 12, y + 10, (w - 24) * 4, (h - 20) * 2);
    }
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
    if (pieces & 1u)
    {
        DrawStoryMapCaption(r, mx, my, mw, mh);
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