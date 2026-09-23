#include "stdafx.h"
#include "GuideIcons.h"
#include "Chapter2.h"
#include <algorithm>
#include <cmath>

Vec2 Chapter2::Project(Vec2 position, float height) const
{
    float x = position.x - m_Camera.x, y = position.y - m_Camera.y;
    return {m_Width * 0.5f + (x - y) * m_Scale,
            m_Height * 0.53f + (x + y) * m_Scale * 0.5f - height * m_Scale};
}

void Chapter2::Draw(Renderer& renderer, int width, int height)
{
    m_Width = width;
    m_Height = height;
    m_Scale = 28 * (std::max)(0.8f, (std::min)(1.4f, height / 800.0f));
    float scale = m_Scale / 28;
    renderer.Begin(m_Time);

    struct Object
    {
        Vec2 position;
        int kind;
        int index;
    };

    std::vector<Object> objects;
    for (int y = 0; y < Size; ++y)
    {
        for (int x = 0; x < Size; ++x)
        {
            Vec2 center{x + 0.5f, y + 0.5f};
            Vec2 screen = Project(center);
            if (screen.x < -90 || screen.x > width + 90 || screen.y < -60 ||
                screen.y > height + 100)
                continue;
            Tile tile = m_Map[y * Size + x];
            float shade = ((x * 31 + y * 17) % 9) * 0.009f;
            renderer.Surface(Project({static_cast<float>(x), static_cast<float>(y)}),
                             Project({x + 1.0f, static_cast<float>(y)}),
                             Project({x + 1.0f, y + 1.0f}),
                             Project({static_cast<float>(x), y + 1.0f}),
                             {0.25f + shade, 0.32f + shade, 0.23f + shade},
                             3,
                             1,
                             1);
            if (tile == Tile::Wall)
                objects.push_back({center, 0, 0});

            else if (tile == Tile::Ground && (x * 11 + y * 7) % 9 == 0)
                objects.push_back({center, 5, 0});
        }
    }
    for (size_t i = 0; i < m_Loot.size(); ++i)
        if (!m_Loot[i].taken)
            objects.push_back({m_Loot[i].position, 1, static_cast<int>(i)});
    for (size_t i = 0; i < m_Enemies.size(); ++i)
        if (m_Enemies[i].hp > 0)
            objects.push_back({m_Enemies[i].position, 2, static_cast<int>(i)});
    objects.push_back({m_Player, 3, 0});
    for (size_t i = 0; i < m_Stones.size(); ++i)
        objects.push_back({m_Stones[i].position, 6, static_cast<int>(i)});

    // Cull before shadow generation and sorting, with room for tall models and shadows.
    objects.erase(std::remove_if(objects.begin(),
                                 objects.end(),
                                 [&](const Object& object)
                                 {
                                     Vec2 p = Project(object.position);
                                     float margin = (std::max)(120.0f, 110 * scale);
                                     return p.x < -margin || p.x > width + margin ||
                                            p.y < -margin || p.y > height + margin;
                                 }),
                  objects.end());

    if (renderer.BeginShadows())
    {
        for (const Object& object : objects)
        {
            Vec2 p = Project(object.position);
            if (object.kind == 0)
                renderer.Quad({p.x - 25 * scale, p.y},
                              {p.x + 25 * scale, p.y},
                              {p.x + 50 * scale, p.y + 17 * scale},
                              {p.x, p.y + 26 * scale},
                              {1, 1, 1, 1});
            else if (object.kind == 2 || object.kind == 3)
                renderer.Ellipse(
                    {p.x + 12 * scale, p.y + 5 * scale}, 20 * scale, 6 * scale, {1, 1, 1, 1});
        }
        renderer.EndShadows();
    }

    std::stable_sort(objects.begin(),
                     objects.end(),
                     [](const Object& a, const Object& b)
                     {
                         return a.position.x + a.position.y < b.position.x + b.position.y;
                     });
    for (const Object& object : objects)
    {
        Vec2 p = Project(object.position);
        const float margin = (std::max)(100.0f, 90 * scale);
        if (p.x < -margin || p.x > width + margin || p.y < -margin || p.y > height + margin)
            continue;
        switch (object.kind)
        {
        case 0:
            renderer.DrawModel(Renderer::Model::Wall, p, scale);
            break;
        case 1:
            renderer.DrawModel(Renderer::Model::Crate, p, scale);
            renderer.Ellipse({p.x, p.y - 24 * scale}, 3, 3, {1.3f, 0.95f, 0.35f});
            break;
        case 2:
        {
            const Enemy& enemy = m_Enemies[object.index];
            Vec2 target = Project(m_Player);
            renderer.AggressiveCreature(
                p,
                scale,
                enemy.upright,
                enemy.walkPhase,
                target.x < p.x,
                (m_Player.x - enemy.position.x) * (m_Player.x - enemy.position.x) +
                        (m_Player.y - enemy.position.y) * (m_Player.y - enemy.position.y) <
                    81);
            renderer.Rect(p.x - 15 * scale, p.y - 84 * scale, 30 * scale, 4, {0.1f, 0.06f, 0.05f});
            renderer.Rect(p.x - 15 * scale,
                          p.y - 84 * scale,
                          30 * scale * enemy.hp / enemy.maxHp,
                          4,
                          {0.8f, 0.22f, 0.12f});
            if (enemy.flash > 0)
                renderer.Ellipse(
                    {p.x, p.y - 34 * scale}, 24 * scale, 20 * scale, {1, 0.8f, 0.5f, 0.4f});
            break;
        }
        case 3:
        {
            renderer.Character(
                p, scale * 0.76f, m_Facing, m_Moving ? 1 + static_cast<int>(m_Walk) % 7 : 0, false);
            if (m_Invulnerable > 0)
                renderer.Ellipse(p, 16 * scale, 6 * scale, {1, 0.3f, 0.17f, 0.4f});
            float aim = std::atan2((m_Aim.x + m_Aim.y) * 0.5f, m_Aim.x - m_Aim.y);
            float swing = m_Swing > 0 ? (0.5f - m_Swing / 0.25f) * SwingArc : 0.8f;
            if (m_Swing > 0)
            {
                float elapsed = 0.25f - m_Swing;
                for (int ghost = 4; ghost >= 1; --ghost)
                {
                    float lag = ghost * 0.018f;
                    if (elapsed < lag)
                        continue;
                    renderer.DrawModel(Renderer::Model::Pipe,
                                       {p.x + 5 * scale, p.y - 22 * scale},
                                       scale,
                                       aim + swing - lag / 0.25f * SwingArc,
                                       0.24f - ghost * 0.035f);
                }
            }
            renderer.DrawModel(
                Renderer::Model::Pipe, {p.x + 5 * scale, p.y - 22 * scale}, scale, aim + swing);
            if (m_Swing > 0)
            {
                for (int i = 0; i < 10; ++i)
                {
                    float angle = aim + swing - i * 0.09f;
                    renderer.Line({p.x + std::cos(angle) * 43 * scale,
                                   p.y - 20 * scale + std::sin(angle) * 30 * scale},
                                  {p.x + std::cos(angle - 0.09f) * 43 * scale,
                                   p.y - 20 * scale + std::sin(angle - 0.09f) * 30 * scale},
                                  2,
                                  {0.95f, 0.9f, 0.66f, 0.6f - i * 0.05f});
                }
            }
            break;
        }
        case 5:
            renderer.DrawModel(Renderer::Model::Plant, p, scale * 0.65f);
            break;
        case 6:
        {
            float progress = 1 - m_Stones[object.index].life / 1.2f;
            renderer.DrawModel(Renderer::Model::Stone,
                               {p.x, p.y - 16 * scale - std::sin(progress * 3.14159f) * 18 * scale},
                               scale);
            break;
        }
        }
    }
    for (const Impact& impact : m_Impacts)
    {
        Vec2 p = Project(impact.position, 0.55f);
        float progress = 1 - impact.life / 0.22f;
        for (int ray = 0; ray < 8; ++ray)
        {
            float angle = ray * 0.785398f + 0.2f;
            float radius = (5 + progress * 22) * scale;
            renderer.Line(
                {p.x + std::cos(angle) * radius * 0.5f, p.y + std::sin(angle) * radius * 0.5f},
                {p.x + std::cos(angle) * radius, p.y + std::sin(angle) * radius},
                2 * scale,
                {1.5f, 0.95f, 0.35f, 1 - progress});
        }
    }
    renderer.FinishWorld(m_Time);
    if (m_Stats.hp > 0)
    {
        for (const Loot& loot : m_Loot)
        {
            if (!loot.taken &&
                std::hypot(m_Player.x - loot.position.x, m_Player.y - loot.position.y) < 1.4f &&
                ClearLine(m_Player, loot.position))
            {
                Vec2 p = Project(loot.position);
                DrawKeyIcon(renderer, {p.x, p.y - 48 * scale}, L"F");
            }
        }
    }
    DrawHud(renderer);
    renderer.Flush();
}

void Chapter2::DrawHud(Renderer& renderer)
{
    renderer.Rect(m_Width - 346.0f, 16, 330, 58, {0.025f, 0.055f, 0.04f, 0.95f});
    renderer.Text(m_Width - 334.0f,
                  21,
                  L"HP " + std::to_wstring(m_Stats.hp) + L" / " + std::to_wstring(m_Stats.maxHp));
    renderer.Rect(m_Width - 334.0f, 52, 300, 10, {0.17f, 0.1f, 0.08f});
    renderer.Rect(
        m_Width - 334.0f, 52, 300.0f * m_Stats.hp / m_Stats.maxHp, 10, {0.64f, 0.22f, 0.15f});
    auto map = [](Vec2 position)
    {
        return ProjectMinimap(position, {18, 24});
    };
    renderer.Rect(18, 24, 220, 204, {0.035f, 0.055f, 0.04f, 0.93f});
    renderer.Text(30, 29, L"챕터 2", Color(), 0.85f);
    for (int y = 0; y < Size; ++y)
        for (int x = 0; x < Size; ++x)
        {
            Tile tile = m_Map[y * Size + x];
            Color color =
                tile == Tile::Wall ? Color(0.13f, 0.17f, 0.14f) : Color(0.37f, 0.43f, 0.31f);
            float px = static_cast<float>(x), py = static_cast<float>(y);
            renderer.Quad(
                map({px, py}), map({px + 1, py}), map({px + 1, py + 1}), map({px, py + 1}), color);
        }
    for (const Loot& loot : m_Loot)
    {
        if (!loot.taken)
        {
            Vec2 p = map(loot.position);
            renderer.Quad(
                {p.x, p.y - 3}, {p.x + 3, p.y}, {p.x, p.y + 3}, {p.x - 3, p.y}, {1, 0.79f, 0.22f});
        }
    }
    for (const Enemy& enemy : m_Enemies)
    {
        if (enemy.hp > 0)
        {
            renderer.Ellipse(map(enemy.position), 2, 2, {0.85f, 0.3f, 0.22f});
        }
    }
    renderer.Ellipse(map(m_Player), 3, 3, {1, 1, 1});
    if (m_NoticeTime > 0)
    {
        renderer.Rect(16, m_Height - 112.0f, m_Width - 32.0f, 36, {0.035f, 0.055f, 0.04f, 0.95f});
        renderer.Text(28, m_Height - 107.0f, m_Notice, {0.94f, 0.85f, 0.57f});
    }
    if (m_Stats.hp <= 0)
    {
        renderer.Rect(0,
                      0,
                      static_cast<float>(m_Width),
                      static_cast<float>(m_Height),
                      {0.02f, 0.01f, 0.01f, 0.75f});
        renderer.Text(m_Width * 0.5f - 90, m_Height * 0.5f - 50, L"쓰러졌습니다", {1, 0.65f, 0.4f});
        renderer.Rect(m_Width * 0.5f - 140, m_Height * 0.5f + 15, 280, 50, {0.23f, 0.31f, 0.22f});
        renderer.Text(m_Width * 0.5f,
                      m_Height * 0.5f + 40,
                      L"시작 지점에서 다시 일어나기",
                      Color(),
                      1,
                      true,
                      true);
    }
}
