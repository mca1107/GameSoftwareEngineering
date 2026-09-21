#pragma once
#include <algorithm>

// Cumulative experience is the only source of character level and base stats.
struct RpgProgression
{
    static constexpr int MaxLevel = 10;
    int experience = 0;
    int level = 1;
    int maxHp = 100;
    int hp = 100;
    int attack = 18;
    int defense = 2;

    static int Threshold(int targetLevel)
    {
        return 50 * targetLevel * (targetLevel - 1);
    }

    int MeleeDamage() const
    {
        return attack; // 100% of the shared attack stat.
    }

    int RangedDamage() const
    {
        return attack / 2; // 50%, rounded down to whole HP.
    }

    bool Gain(int amount)
    {
        int remaining = Threshold(MaxLevel) - experience;
        experience += (std::min)(remaining, (std::max)(0, amount));
        int previousLevel = level;
        int previousHp = maxHp;
        level = 1;
        while (level < MaxLevel && experience >= Threshold(level + 1))
        {
            ++level;
        }

        maxHp = 100 + 20 * (level - 1);
        attack = 18 + 4 * (level - 1);
        defense = 2 + (level - 1);
        hp = (std::min)(maxHp, hp + maxHp - previousHp);
        return previousLevel != level;
    }
};
