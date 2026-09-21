#include "stdafx.h"
#include "Chapter2.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <iostream>

namespace
{
float Distance(Vec2 a, Vec2 b)
{
    float x = a.x - b.x, y = a.y - b.y;
    return std::sqrt(x * x + y * y);
}

Vec2 Direction(Vec2 from, Vec2 to)
{
    float length = Distance(from, to);
    return length > 0.001f ? Vec2{(to.x - from.x) / length, (to.y - from.y) / length} : Vec2{1, 0};
}
}

Chapter2::Chapter2() : m_Seed(std::random_device{}()), m_Random(m_Seed)
{
    GenerateMap();
    SpawnWave();
    std::cout << "Level 1 seed: " << m_Seed << "\n";
}

void Chapter2::BuildDistances(int start)
{
    m_Distance.fill(-1);
    if (start < 0 || start >= Size * Size || m_Map[start] == Tile::Wall)
    {
        return;
    }
    std::queue<int> pending;
    pending.push(start);
    m_Distance[start] = 0;
    while (!pending.empty())
    {
        int cell = pending.front();
        pending.pop();
        int x = cell % Size, y = cell / Size;
        for (Vec2 offset : {Vec2{1, 0}, Vec2{-1, 0}, Vec2{0, 1}, Vec2{0, -1}})
        {
            int nx = x + static_cast<int>(offset.x), ny = y + static_cast<int>(offset.y);
            if (nx < 1 || ny < 1 || nx >= Size - 1 || ny >= Size - 1)
            {
                continue;
            }
            int next = ny * Size + nx;
            if (m_Map[next] != Tile::Wall && m_Distance[next] < 0)
            {
                m_Distance[next] = m_Distance[cell] + 1;
                pending.push(next);
            }
        }
    }
}

void Chapter2::GenerateMap()
{
    m_Map.fill(Tile::Ground);
    for (int y = 0; y < Size; ++y)
    {
        for (int x = 0; x < Size; ++x)
        {
            if (!x || !y || x == Size - 1 || y == Size - 1)
            {
                m_Map[y * Size + x] = Tile::Wall;
            }
        }
    }

    // Random ruined wall segments, never covering the starting clearing.
    std::uniform_int_distribution<int> coordinate(7, Size - 4);
    for (int wall = 0; wall < 105; ++wall)
    {
        int x = coordinate(m_Random), y = coordinate(m_Random);
        bool horizontal = (m_Random() % 2) == 0;
        int length = 2 + m_Random() % 4;
        for (int i = 0; i < length; ++i)
        {
            int nx = x + (horizontal ? i : 0), ny = y + (horizontal ? 0 : i);
            if (nx < Size - 1 && ny < Size - 1)
            {
                m_Map[ny * Size + nx] = Tile::Wall;
            }
        }
    }

    // Repair every disconnected floor component by carving to the start.
    const int start = 3 * Size + 3;
    BuildDistances(start);
    for (int y = 1; y < Size - 1; ++y)
    {
        for (int x = 1; x < Size - 1; ++x)
        {
            if (m_Map[y * Size + x] == Tile::Wall || m_Distance[y * Size + x] >= 0)
            {
                continue;
            }
            int cx = x, cy = y;
            while (cx != 3)
            {
                m_Map[cy * Size + cx] = Tile::Ground;
                cx += cx > 3 ? -1 : 1;
            }
            while (cy != 3)
            {
                m_Map[cy * Size + cx] = Tile::Ground;
                cy += cy > 3 ? -1 : 1;
            }
            BuildDistances(start);
        }
    }

    BuildDistances(start);
    std::vector<int> floor;
    for (int cell = 0; cell < Size * Size; ++cell)
    {
        if (m_Distance[cell] >= 0 && m_Map[cell] == Tile::Ground)
        {
            floor.push_back(cell);
        }
    }
    std::shuffle(floor.begin(), floor.end(), m_Random);
    for (size_t i = 0; i < floor.size() && i < 45; ++i)
    {
        int cell = floor[i];
        m_Loot.push_back({{cell % Size + 0.5f, cell / Size + 0.5f}, false, i % 3 == 0, 4, 15});
    }
}

void Chapter2::SpawnWave()
{
    m_Enemies.clear();
    std::vector<int> candidates;
    for (int y = 1; y < Size - 1; ++y)
    {
        for (int x = 1; x < Size - 1; ++x)
        {
            Vec2 position{x + 0.5f, y + 0.5f};
            if (m_Map[y * Size + x] != Tile::Wall && Distance(position, m_Player) > 7 &&
                Distance(position, {3.5f, 3.5f}) > 7)
            {
                candidates.push_back(y * Size + x);
            }
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), m_Random);
    for (size_t i = 0; i < candidates.size() && i < 14; ++i)
    {
        Vec2 position{candidates[i] % Size + 0.5f, candidates[i] / Size + 0.5f};
        int hp = 45 + 8 * (std::min)(m_Wave - 1, 10);
        m_Enemies.push_back({position, position, hp, hp, 0, 0});
    }
}

bool Chapter2::Blocked(Vec2 p, float radius) const
{
    if (p.x < 1 + radius || p.y < 1 + radius || p.x > Size - 1 - radius || p.y > Size - 1 - radius)
    {
        return true;
    }
    for (int y = static_cast<int>(p.y - radius); y <= static_cast<int>(p.y + radius); ++y)
    {
        for (int x = static_cast<int>(p.x - radius); x <= static_cast<int>(p.x + radius); ++x)
        {
            if (m_Map[y * Size + x] != Tile::Wall)
                continue;
            float nearX = (std::max)(static_cast<float>(x), (std::min)(p.x, x + 1.0f));
            float nearY = (std::max)(static_cast<float>(y), (std::min)(p.y, y + 1.0f));
            if (Distance(p, {nearX, nearY}) <= radius)
                return true;
        }
    }
    return false;
}

bool Chapter2::ClearLine(Vec2 from, Vec2 to, float radius) const
{
    if (Blocked(from, radius) || Blocked(to, radius))
    {
        return false;
    }

    // Segment against expanded wall boxes: do not skip thin corner intersections.
    int left = (std::max)(0, static_cast<int>((std::min)(from.x, to.x) - radius));
    int right = (std::min)(Size - 1, static_cast<int>((std::max)(from.x, to.x) + radius));
    int top = (std::max)(0, static_cast<int>((std::min)(from.y, to.y) - radius));
    int bottom = (std::min)(Size - 1, static_cast<int>((std::max)(from.y, to.y) + radius));
    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            if (m_Map[y * Size + x] != Tile::Wall)
            {
                continue;
            }
            float low = 0, high = 1;
            const float origin[] = {from.x, from.y};
            const float delta[] = {to.x - from.x, to.y - from.y};
            const float minimum[] = {x - radius, y - radius};
            const float maximum[] = {x + 1 + radius, y + 1 + radius};
            bool intersects = true;
            for (int axis = 0; axis < 2; ++axis)
            {
                if (std::abs(delta[axis]) < 0.00001f)
                {
                    if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                        intersects = false;
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
                        intersects = false;
                }
            }
            if (intersects)
                return false;
        }
    }
    return true;
}

void Chapter2::Move(Vec2& position, Vec2 delta)
{
    int steps = (std::max)(1, static_cast<int>(std::ceil(Distance({}, delta) / 0.08f)));
    for (int i = 0; i < steps; ++i)
    {
        Vec2 next{position.x + delta.x / steps, position.y};
        if (!Blocked(next))
            position = next;
        next = {position.x, position.y + delta.y / steps};
        if (!Blocked(next))
            position = next;
    }
}

void Chapter2::Notify(const std::wstring& text)
{
    m_Notice = text;
    m_NoticeTime = 4;
}

void Chapter2::GainExperience(int amount)
{
    if (m_Stats.Gain(amount))
    {
        Notify(L"레벨 상승! Lv." + std::to_wstring(m_Stats.level) +
               L" · 최대 HP / 공격력 / 방어력 증가");
    }
}

void Chapter2::Hit(Enemy& enemy, int damage, Vec2 direction)
{
    if (enemy.hp <= 0)
        return;
    enemy.hp = (std::max)(0, enemy.hp - damage);
    enemy.flash = 0.18f;
    enemy.knockback = {direction.x * 4.5f, direction.y * 4.5f};
    enemy.stagger = 0.2f;
    m_Impacts.push_back({enemy.position, 0.22f});
    if (enemy.hp == 0)
    {
        GainExperience(40);
        m_Loot.push_back({enemy.position, false, m_Random() % 3 == 0, 3, 0});
    }
}

Vec2 Chapter2::Unproject(int x, int y) const
{
    float sx = (x - m_Width * 0.5f) / m_Scale;
    float sy = (y - m_Height * 0.53f) / m_Scale;
    return {m_Camera.x + sy + sx * 0.5f, m_Camera.y + sy - sx * 0.5f};
}

void Chapter2::Click(bool ranged, int x, int y)
{
    if (m_Stats.hp <= 0)
    {
        if (!ranged && x >= m_Width / 2 - 140 && x <= m_Width / 2 + 140 && y >= m_Height / 2 + 15 &&
            y <= m_Height / 2 + 65)
            Respawn();
        return;
    }
    if (m_AttackTimer > 0)
        return;
    Vec2 target = Unproject(x, y);
    float nearest = 1000;
    // Clicking the visible torso should aim at that actor, not the ground behind it.
    for (const Enemy& enemy : m_Enemies)
    {
        if (enemy.hp <= 0)
            continue;
        Vec2 screen = Project(enemy.position);
        float scale = m_Scale / 28;
        float distance = Distance({static_cast<float>(x), static_cast<float>(y)},
                                  {screen.x, screen.y - 15 * scale});
        if (std::abs(x - screen.x) <= 20 * scale && y >= screen.y - 36 * scale &&
            y <= screen.y + 4 * scale && distance < nearest)
        {
            nearest = distance;
            target = enemy.position;
        }
    }
    m_Aim = Direction(m_Player, target);
    float sx = m_Aim.x - m_Aim.y, sy = (m_Aim.x + m_Aim.y) * 0.5f;
    m_Facing = std::abs(sy) > std::abs(sx) ? (sy < 0 ? 3 : 0) : (sx < 0 ? 1 : 2);
    if (ranged)
    {
        if (!m_Ammo)
        {
            Notify(L"돌이 없습니다. F로 물자를 회수하거나 쇠파이프를 사용하세요.");
            return;
        }
        --m_Ammo;
        m_AttackTimer = 0.6f;
        m_Stones.push_back({m_Player, {m_Aim.x * 9, m_Aim.y * 9}, 1.2f, m_Stats.RangedDamage()});
    }
    else
    {
        m_AttackTimer = 0.45f;
        m_Swing = 0.25f;
        for (Enemy& enemy : m_Enemies)
        {
            Vec2 direction = Direction(m_Player, enemy.position);
            if (enemy.hp > 0 && Distance(m_Player, enemy.position) <= MeleeRange &&
                direction.x * m_Aim.x + direction.y * m_Aim.y >= 0 &&
                ClearLine(m_Player, enemy.position))
                Hit(enemy, m_Stats.MeleeDamage(), m_Aim);
        }
    }
}

void Chapter2::Interact()
{
    if (m_Stats.hp <= 0)
        return;
    bool found = false;
    int xp = 0;
    for (Loot& loot : m_Loot)
    {
        if (!loot.taken && Distance(m_Player, loot.position) < 1.4f &&
            ClearLine(m_Player, loot.position))
        {
            loot.taken = true;
            m_Ammo += loot.stones;
            m_Medicine += loot.medicine ? 1 : 0;
            ++m_Scrap;
            xp += loot.experience;
            found = true;
        }
    }
    Notify(found ? L"물자 획득 · 돌 / 고철 / 회복약을 확인하세요."
                 : L"가까운 물자 옆에서 F를 누르세요.");
    GainExperience(xp);
    m_Loot.erase(std::remove_if(m_Loot.begin(),
                                m_Loot.end(),
                                [](const Loot& loot)
                                {
                                    return loot.taken;
                                }),
                 m_Loot.end());
}

void Chapter2::Heal()
{
    if (m_Stats.hp <= 0)
        return;
    if (!m_Medicine || m_Stats.hp == m_Stats.maxHp)
    {
        Notify(m_Medicine ? L"이미 HP가 가득 찼습니다." : L"회복약이 없습니다.");
        return;
    }
    --m_Medicine;
    m_Stats.hp = (std::min)(m_Stats.maxHp, m_Stats.hp + 45);
    Notify(L"회복약 사용 · HP +45");
}

void Chapter2::Respawn()
{
    m_Player = {3.5f, 3.5f};
    m_Camera = m_Player;
    m_Stats.hp = m_Stats.maxHp;
    m_Invulnerable = 3;
    m_AttackTimer = m_Swing = 0;
    m_Stones.clear();
    m_Impacts.clear();
    m_PathTimer = 0;
    for (Enemy& enemy : m_Enemies)
    {
        if (Distance(enemy.position, m_Player) < 7)
            enemy.position = enemy.home;
    }
    Notify(L"시작 지점에서 회복했습니다. 경험치와 소지품은 유지됩니다.");
}

void Chapter2::Update(float dt, const bool* keys)
{
    dt = (std::min)(dt, 0.1f);
    m_Time += dt;
    m_Moving = false;
    if (m_Stats.hp <= 0)
        return;
    m_NoticeTime = (std::max)(0.0f, m_NoticeTime - dt);
    m_AttackTimer = (std::max)(0.0f, m_AttackTimer - dt);
    m_Swing = (std::max)(0.0f, m_Swing - dt);
    m_Invulnerable = (std::max)(0.0f, m_Invulnerable - dt);

    float sx = (keys['d'] ? 1.0f : 0) - (keys['a'] ? 1.0f : 0);
    float sy = (keys['s'] ? 1.0f : 0) - (keys['w'] ? 1.0f : 0);
    if (sx || sy)
    {
        Vec2 direction = Direction({}, {sx + 2 * sy, -sx + 2 * sy});
        Vec2 before = m_Player;
        Move(m_Player, {direction.x * 3.4f * dt, direction.y * 3.4f * dt});
        m_Moving = Distance(before, m_Player) > 0.001f;
        if (m_Moving)
            m_Walk += dt * 9;
        if (m_Swing <= 0)
        {
            m_Facing = std::abs(sy) > std::abs(sx) ? (sy < 0 ? 3 : 0) : (sx < 0 ? 1 : 2);
        }
    }

    m_PathTimer -= dt;
    if (m_PathTimer <= 0)
    {
        BuildDistances(static_cast<int>(m_Player.y) * Size + static_cast<int>(m_Player.x));
        m_PathTimer = 0.35f;
    }
    for (Impact& impact : m_Impacts)
        impact.life -= dt;
    m_Impacts.erase(std::remove_if(m_Impacts.begin(),
                                   m_Impacts.end(),
                                   [](const Impact& impact)
                                   {
                                       return impact.life <= 0;
                                   }),
                    m_Impacts.end());
    int alive = 0;
    for (Enemy& enemy : m_Enemies)
    {
        if (enemy.hp <= 0)
            continue;
        ++alive;
        enemy.cooldown = (std::max)(0.0f, enemy.cooldown - dt);
        enemy.flash = (std::max)(0.0f, enemy.flash - dt);
        const bool aggressive = Distance(m_Player, enemy.position) < 9;
        enemy.upright = aggressive ? (std::max)(0.0f, enemy.upright - dt * 4)
                                   : (std::min)(1.0f, enemy.upright + dt * 2);
        if (enemy.stagger > 0)
        {
            Move(enemy.position, {enemy.knockback.x * dt, enemy.knockback.y * dt});
            float decay = std::exp(-10 * dt);
            enemy.knockback.x *= decay;
            enemy.knockback.y *= decay;
            enemy.stagger = (std::max)(0.0f, enemy.stagger - dt);
            continue;
        }
        float distance = Distance(m_Player, enemy.position);
        if (distance < 9 && distance > 0.65f)
        {
            Vec2 goal = m_Player;
            if (!ClearLine(enemy.position, m_Player, 0.28f))
            {
                int cell =
                    static_cast<int>(enemy.position.y) * Size + static_cast<int>(enemy.position.x);
                int best = cell;
                for (int next : {cell - 1, cell + 1, cell - Size, cell + Size})
                {
                    if (next >= 0 && next < Size * Size && m_Distance[next] >= 0 &&
                        (m_Distance[best] < 0 || m_Distance[next] < m_Distance[best]))
                        best = next;
                }
                goal = {best % Size + 0.5f, best / Size + 0.5f};
                if (!ClearLine(enemy.position, goal, 0.28f))
                {
                    // Reach this cell's center before taking a narrow right-angle turn.
                    goal = {cell % Size + 0.5f, cell / Size + 0.5f};
                }
            }
            Vec2 direction = Direction(enemy.position, goal);
            Vec2 previousPosition = enemy.position;
            Move(enemy.position, {direction.x * dt * 1.5f, direction.y * dt * 1.5f});
            enemy.walkPhase += Distance(previousPosition, enemy.position) * 7;
        }
        if (!aggressive)
        {
            Vec2 goal{enemy.home.x + std::cos(m_Time * 0.35f + enemy.home.x) * 0.8f,
                      enemy.home.y + std::sin(m_Time * 0.29f + enemy.home.y) * 0.8f};
            if (Distance(enemy.position, goal) > 0.08f)
            {
                Vec2 direction = Direction(enemy.position, goal);
                Vec2 previousPosition = enemy.position;
                Move(enemy.position, {direction.x * dt * 0.45f, direction.y * dt * 0.45f});
                enemy.walkPhase += Distance(previousPosition, enemy.position) * 7;
            }
        }
        if (Distance(enemy.position, m_Player) < 0.9f && enemy.cooldown <= 0 &&
            m_Invulnerable <= 0 && ClearLine(enemy.position, m_Player))
        {
            int damage = (std::max)(1, 12 + (std::min)(m_Wave - 1, 10) - m_Stats.defense);
            m_Stats.hp = (std::max)(0, m_Stats.hp - damage);
            enemy.cooldown = 1.2f;
            m_Invulnerable = 0.65f;
            if (!m_Stats.hp)
                break;
        }
    }
    if (!m_Stats.hp)
    {
        m_Stones.clear();
        return;
    }

    for (Stone& stone : m_Stones)
    {
        if (stone.life <= 0)
            continue;
        float travelTime = (std::min)(dt, stone.life);
        int steps = (std::max)(1, static_cast<int>(std::ceil(9 * travelTime / 0.08f)));
        for (int i = 0; i < steps && stone.life > 0; ++i)
        {
            Vec2 previous = stone.position;
            stone.position.x += stone.velocity.x * travelTime / steps;
            stone.position.y += stone.velocity.y * travelTime / steps;
            if (!ClearLine(previous, stone.position, 0.08f))
            {
                stone.life = 0;
                break;
            }
            for (Enemy& enemy : m_Enemies)
            {
                if (enemy.hp > 0 && Distance(enemy.position, stone.position) < 0.45f)
                {
                    Hit(enemy, stone.damage, Direction({}, stone.velocity));
                    stone.life = 0;
                    break;
                }
            }
        }
        stone.life = (std::max)(0.0f, stone.life - travelTime);
    }
    m_Stones.erase(std::remove_if(m_Stones.begin(),
                                  m_Stones.end(),
                                  [](const Stone& stone)
                                  {
                                      return stone.life <= 0;
                                  }),
                   m_Stones.end());
    if (!alive)
    {
        m_NextWave += dt;
        if (m_NextWave > 8)
        {
            ++m_Wave;
            SpawnWave();
            m_NextWave = 0;
            Notify(L"새 크리처 무리 출현 · 탐색과 성장을 계속할 수 있습니다.");
        }
    }
    float blend = 1 - std::exp(-7 * dt);
    m_Camera.x += (m_Player.x - m_Camera.x) * blend;
    m_Camera.y += (m_Player.y - m_Camera.y) * blend;
}
