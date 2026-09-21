#pragma once
#include "Renderer.h"
#include "RpgProgression.h"
#include "InventoryPanel.h"
#include <array>
#include <random>
#include <string>
#include <vector>

class Chapter2
{
public:
    Chapter2();

    void CarryStoryMap(unsigned pieces = 1u)
    {
        m_StoryMapPieces |= pieces & 0xffu;
    }

    void Update(float dt, const bool* keys);
    void Click(bool ranged, int x, int y);
    void Interact();
    void Heal();
    void Draw(Renderer& renderer, int width, int height);

    InventoryStatus Status() const
    {
        InventoryStatus result;
        result.level = m_Stats.level;
        result.experience = m_Stats.experience;
        result.attack = m_Stats.attack;
        result.maxHp = m_Stats.maxHp;
        result.defense = m_Stats.defense;
        result.effects = m_Stats.hp <= 0      ? L"행동 불가"
                         : m_Invulnerable > 0 ? L"일시 피해 보호"
                                              : L"없음";
        result.items = {{m_Ammo, m_Medicine, m_Scrap, 0, 0}};
        result.mapPieces = m_StoryMapPieces;
        return result;
    }

private:
    unsigned m_StoryMapPieces = 0;
    static constexpr int Size = 40;
    static constexpr float MeleeRange = 2.3f;
    static constexpr float SwingArc = 3.14159265f;
    enum class Tile
    {
        Ground,
        Wall
    };

    struct Enemy
    {
        Vec2 position, home;
        int hp = 45;
        int maxHp = 45;
        float cooldown = 0;
        float flash = 0;
        Vec2 knockback;
        float stagger = 0;
        float walkPhase = 0;
        float upright = 1;
    };

    struct Loot
    {
        Vec2 position;
        bool taken = false;
        bool medicine = false;
        int stones = 3;
        int experience = 10;
    };

    struct Stone
    {
        Vec2 position, velocity;
        float life = 0;
        int damage = 0;
    };

    std::array<Tile, Size * Size> m_Map{};
    std::array<int, Size * Size> m_Distance{};
    std::vector<Enemy> m_Enemies;
    std::vector<Loot> m_Loot;
    std::vector<Stone> m_Stones;

    struct Impact
    {
        Vec2 position;
        float life = 0.22f;
    };

    std::vector<Impact> m_Impacts;
    unsigned m_Seed;
    std::mt19937 m_Random;
    RpgProgression m_Stats;
    Vec2 m_Player{3.5f, 3.5f}, m_Camera{3.5f, 3.5f}, m_Aim{1, 0};
    int m_Width = 1280, m_Height = 800, m_Facing = 0;
    int m_Ammo = 12, m_Medicine = 2, m_Scrap = 0, m_Wave = 1;
    float m_Scale = 28, m_Time = 0, m_Walk = 0, m_AttackTimer = 0;
    float m_Swing = 0, m_Invulnerable = 0, m_PathTimer = 0, m_NextWave = 0;
    float m_NoticeTime = 6;
    bool m_Moving = false;
    std::wstring m_Notice = L"챕터 2 · 폐허 탐색과 전투. F로 물자를 회수하세요.";

    void GenerateMap();
    void BuildDistances(int start);
    void SpawnWave();
    bool Blocked(Vec2 position, float radius = 0.28f) const;
    bool ClearLine(Vec2 from, Vec2 to, float radius = 0.02f) const;
    void Move(Vec2& position, Vec2 delta);
    void Hit(Enemy& enemy, int damage, Vec2 direction);
    void GainExperience(int amount);
    void Respawn();
    void Notify(const std::wstring& text);
    Vec2 Project(Vec2 position, float height = 0) const;
    Vec2 Unproject(int x, int y) const;
    void DrawHud(Renderer& renderer);
};
