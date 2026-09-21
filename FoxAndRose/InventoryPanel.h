#pragma once
#include "Renderer.h"
#include "RpgProgression.h"
#include "StoryMapPanel.h"
#include <algorithm>
#include <array>
#include <string>

struct InventoryStatus
{
    int level = 1;
    int experience = 0;
    int attack = 18;
    int maxHp = 100;
    int defense = 2;
    std::wstring effects = L"없음";
    std::array<bool, 4> skillsUnlocked{{true, false, false, false}};
    bool showSampleEffect = true;
    // Stones, medicine, scrap, preserved food, household supplies.
    std::array<int, 5> items{};
    unsigned mapPieces = 0;
};

// Keep colors separate from layout so future levels can supply their own palette.
struct InventoryTheme
{
    Color background{0.19f, 0.22f, 0.34f, 1};
    Color border{0.59f, 0.63f, 0.79f, 1};
    Color slot{0.085f, 0.10f, 0.17f, 1};
    Color highlight{0.9f, 0.76f, 0.4f, 1};
};

inline void DrawInventoryPanel(Renderer& r,
                               int width,
                               int height,
                               const InventoryStatus& status,
                               int mouseX,
                               int mouseY,
                               const InventoryViewState& view,
                               const InventoryTheme& theme = InventoryTheme())
{
    float w = (std::min)(900.0f, width - 48.0f), h = (std::min)(610.0f, height - 48.0f);
    float left = (width - w) * 0.5f, top = (height - h) * 0.5f;
    r.Rect(
        0, 0, static_cast<float>(width), static_cast<float>(height), {0.01f, 0.02f, 0.015f, 0.8f});
    r.Rect(left - 2, top - 2, w + 4, h + 4, theme.border);
    r.Rect(left, top, w, h, theme.background);
    r.Text(left + 28, top + 18, L"스테이터스", {0.97f, 0.84f, 0.55f});
    r.Text(left + 28, top + h - 38, L"E 닫기", {0.7f, 0.78f, 0.68f});
    float cell = (std::min)((w * 0.55f - 32) / 5, (h - 130 - 5 * 8 - 20) / 6);
    float gridLeft = left + w - 28 - cell * 5 - 32, gridTop = top + 74;
    r.Text(gridLeft, top + 18, L"인벤토리", {0.78f, 0.83f, 0.72f});
    int hovered = -1;
    static const wchar_t* names[] = {L"돌", L"회복약", L"고철", L"보존식", L"생필품"};
    for (int slot = 0; slot < 30; ++slot)
    {
        // First twenty slots are ordinary items; the last ten are story-only.
        int i = slot < 3 ? slot : (slot >= 20 && slot < 22 ? slot - 17 : -1);
        float x = gridLeft + (slot % 5) * (cell + 8);
        float y = gridTop + (slot / 5) * (cell + 8) + (slot >= 20 ? 20 : 0);
        bool occupied = i >= 0 && i < static_cast<int>(status.items.size()) && status.items[i] > 0;
        bool hover =
            occupied && mouseX >= x && mouseX < x + cell && mouseY >= y && mouseY < y + cell;
        if (hover)
            hovered = i;
        r.Rect(x, y, cell, cell, hover ? theme.highlight : theme.border);
        r.Rect(x + 1, y + 1, cell - 2, cell - 2, theme.slot);
        if (!occupied)
            continue;
        float cx = x + cell * 0.5f, cy = y + cell * 0.42f, s = cell / 64;
        if (i == 0)
        {
            r.Ellipse({cx - 6 * s, cy + 3 * s}, 11 * s, 8 * s, {0.43f, 0.47f, 0.44f});
            r.Ellipse({cx + 6 * s, cy - 4 * s}, 10 * s, 7 * s, {0.65f, 0.67f, 0.58f});
        }
        else if (i == 1)
        {
            r.Rect(cx - 10 * s, cy - 10 * s, 20 * s, 25 * s, {0.75f, 0.8f, 0.7f});
            r.Rect(cx - 7 * s, cy - 16 * s, 14 * s, 6 * s, {0.43f, 0.22f, 0.14f});
            r.Rect(cx - 6 * s, cy, 12 * s, 3 * s, {0.75f, 0.17f, 0.13f});
            r.Rect(cx - 1.5f * s, cy - 4.5f * s, 3 * s, 12 * s, {0.75f, 0.17f, 0.13f});
        }
        else if (i == 2)
        {
            r.Line({cx - 14 * s, cy + 10 * s},
                   {cx + 11 * s, cy - 10 * s},
                   7 * s,
                   {0.54f, 0.37f, 0.21f});
            r.Line({cx - 10 * s, cy - 10 * s},
                   {cx + 14 * s, cy + 6 * s},
                   5 * s,
                   {0.47f, 0.55f, 0.53f});
        }
        else if (i == 3)
        {
            r.Rect(cx - 12 * s, cy - 10 * s, 24 * s, 24 * s, {0.65f, 0.53f, 0.25f});
            r.Ellipse({cx, cy - 10 * s}, 12 * s, 4 * s, {0.79f, 0.79f, 0.64f});
        }
        else if (i == 4)
        {
            r.Rect(cx - 14 * s, cy - 6 * s, 28 * s, 20 * s, {0.65f, 0.79f, 0.71f});
            r.Rect(cx - 3 * s, cy - 6 * s, 6 * s, 20 * s, {0.36f, 0.48f, 0.4f});
        }
        else
        {
            r.Rect(cx - 13 * s, cy - 15 * s, 26 * s, 30 * s, {0.78f, 0.71f, 0.53f});
            r.Line({cx - 9 * s, cy + 8 * s}, {cx, cy - 3 * s}, 2 * s, {0.36f, 0.32f, 0.23f});
            r.Line({cx, cy - 3 * s}, {cx + 8 * s, cy - 9 * s}, 2 * s, {0.36f, 0.32f, 0.23f});
        }
        std::wstring count = std::to_wstring(status.items[i]);
        r.Text(x + cell - 6 - count.size() * 11, y + cell - 27, count, {1, 0.93f, 0.7f});
    }
    float cx = (left + 28 + gridLeft - 24) * 0.5f;
    r.Character({cx, top + 220}, 1.65f, 0, 0, false);
    r.Text(left + 28, top + 250, L"LV " + std::to_wstring(status.level), {1, 0.9f, 0.6f});
    float barX = left + 100, barY = top + 252;
    float barWidth = gridLeft - 28 - barX;
    int baseline = RpgProgression::Threshold(status.level);
    int next = RpgProgression::Threshold(status.level + 1);
    bool maxLevel = status.level >= RpgProgression::MaxLevel;
    float progress =
        maxLevel ? 1.0f : (status.experience - baseline) / static_cast<float>(next - baseline);
    progress = (std::max)(0.0f, (std::min)(1.0f, progress));
    r.Rect(barX, barY, barWidth, 24, theme.slot);
    r.Rect(barX, barY, barWidth * progress, 24, {0.55f, 0.65f, 0.86f});
    std::wstring xp = maxLevel ? L"MAX"
                               : std::to_wstring(status.experience - baseline) + L" / " +
                                     std::to_wstring(next - baseline);
    r.Text(barX + 6, barY - 2, xp, {1, 1, 1});
    float statusWidth = gridLeft - 24 - (left + 28);
    r.Text(left + 28, top + 302, L"MHP " + std::to_wstring(status.maxHp));
    r.Text(left + 28 + statusWidth / 3, top + 302, L"ATK " + std::to_wstring(status.attack));
    r.Text(left + 28 + statusWidth * 2 / 3, top + 302, L"DFS " + std::to_wstring(status.defense));

    std::wstring tooltip = hovered >= 0 ? names[hovered] : L"";
    std::wstring detail;
    auto contains = [&](float x, float y, float size)
    {
        return mouseX >= x && mouseX < x + size && mouseY >= y && mouseY < y + size;
    };
    const float iconSize = (std::min)(48.0f, (statusWidth - 40) / 5);
    for (int skill = 0; skill < 4; ++skill)
    {
        float x = left + 28 + skill * (iconSize + 10), y = top + 353;
        bool unlocked = status.skillsUnlocked[skill];
        bool hover = contains(x, y, iconSize);
        r.Rect(x, y, iconSize, iconSize, hover ? theme.highlight : theme.border);
        r.Rect(x + 2, y + 2, iconSize - 4, iconSize - 4, theme.slot);
        float centerX = x + iconSize * 0.5f, centerY = y + iconSize * 0.5f;
        Color ink = unlocked ? Color(0.98f, 0.68f, 0.25f) : Color(0.26f, 0.29f, 0.34f);
        if (skill == 0)
        {
            // A lightning-shaped sample icon; these slots do not activate gameplay skills.
            r.Triangle(
                {centerX + 7, y + 7}, {centerX - 10, centerY + 2}, {centerX + 4, centerY + 2}, ink);
            r.Triangle({centerX - 7, y + iconSize - 7},
                       {centerX + 10, centerY - 2},
                       {centerX - 4, centerY - 2},
                       ink);
        }
        else if (skill == 1)
        {
            r.Quad({centerX - 12, y + 10},
                   {centerX + 12, y + 10},
                   {centerX + 10, centerY + 8},
                   {centerX - 10, centerY + 8},
                   ink);
            r.Triangle({centerX - 10, centerY + 8},
                       {centerX + 10, centerY + 8},
                       {centerX, y + iconSize - 7},
                       ink);
        }
        else if (skill == 2)
        {
            r.Ellipse({centerX, centerY}, 14, 8, ink);
            r.Ellipse({centerX, centerY}, 4, 6, theme.slot);
        }
        else
        {
            r.Line({centerX - 11, centerY}, {centerX + 11, centerY}, 6, ink);
            r.Line({centerX, centerY - 11}, {centerX, centerY + 11}, 6, ink);
        }
        if (!unlocked)
        {
            // A separate lock silhouette makes the state clear without relying on color alone.
            float lockX = x + iconSize - 11, lockY = y + iconSize - 12;
            r.Ellipse({lockX, lockY - 4}, 5, 6, {0.64f, 0.66f, 0.7f});
            r.Ellipse({lockX, lockY - 4}, 3, 4, theme.slot);
            r.Rect(lockX - 6, lockY - 2, 12, 9, {0.64f, 0.66f, 0.7f});
        }
        if (hover)
        {
            tooltip = L"스킬" + std::to_wstring(skill + 1);
            detail = unlocked ? L"" : L"잠김";
        }
    }

    if (status.mapPieces)
    {
        float skillsRight = left + 28 + 4 * iconSize + 3 * 10;
        float x = (skillsRight + gridLeft - 24 - iconSize) * 0.5f, y = top + 353;
        bool hover = contains(x, y, iconSize);
        r.Rect(x, y, iconSize, iconSize, hover ? theme.highlight : theme.border);
        r.Rect(x + 2, y + 2, iconSize - 4, iconSize - 4, theme.slot);
        r.Quad({x + 9, y + 10},
               {x + iconSize - 10, y + 7},
               {x + iconSize - 7, y + iconSize - 10},
               {x + 12, y + iconSize - 7},
               {0.82f, 0.75f, 0.54f});
        r.Line({x + 14, y + iconSize - 15}, {x + iconSize - 14, y + 15}, 2, {0.36f, 0.33f, 0.23f});
        if (hover)
        {
            tooltip = StoryMapName(status.mapPieces);
            detail.clear();
        }
    }
    int effectSlot = 0;
    auto drawEffect = [&](const std::wstring& time, const std::wstring& description, bool sample)
    {
        float x = left + 28 + effectSlot++ * (iconSize + 10), y = top + 430;
        bool hover = contains(x, y, iconSize);
        r.Rect(x, y, iconSize, iconSize, hover ? theme.highlight : theme.border);
        r.Rect(x + 2, y + 2, iconSize - 4, iconSize - 4, {0.16f, 0.3f, 0.36f});
        float cx = x + iconSize * 0.5f, cy = y + iconSize * 0.5f;
        if (sample)
        {
            r.Rect(cx - 13, cy - 10, 26, 21, {0.87f, 0.81f, 0.58f});
            r.Line({cx, cy - 10}, {cx, cy + 11}, 2, theme.slot);
            r.Line({cx - 9, cy - 4}, {cx - 3, cy - 4}, 1, theme.slot);
            r.Line({cx + 3, cy - 4}, {cx + 9, cy - 4}, 1, theme.slot);
        }
        else
        {
            r.Ellipse({cx, cy}, 11, 13, {0.52f, 0.81f, 0.92f});
            r.Ellipse({cx, cy}, 6, 8, theme.slot);
        }
        if (hover)
        {
            tooltip = time.empty() ? description : time;
            detail = time.empty() ? L"" : description;
        }
    };
    if (status.showSampleEffect)
    {
        drawEffect(L"10:00", L"튜토리얼 중", true);
    }
    if (!status.effects.empty() && status.effects != L"없음")
    {
        drawEffect(L"", status.effects, false);
    }

    if (!tooltip.empty())
    {
        float tooltipWidth = 206, tooltipHeight = detail.empty() ? 38.0f : 68.0f;
        float tx = (std::min)(mouseX + 16.0f, width - tooltipWidth - 8);
        float ty = (std::min)(mouseY + 20.0f, height - tooltipHeight - 8);
        r.Rect(tx, ty, tooltipWidth, tooltipHeight, theme.highlight);
        r.Rect(tx + 1, ty + 1, tooltipWidth - 2, tooltipHeight - 2, {0.04f, 0.045f, 0.08f, 1});
        r.Text(tx + 12, ty + 5, tooltip, {1, 0.94f, 0.77f});
        if (!detail.empty())
            r.Text(tx + 12, ty + 34, detail);
    }
    if (view.mapOpen)
    {
        DrawStoryMapPanel(r, width, height, status.mapPieces, view, mouseX, mouseY);
    }
    else if (view.selectedItem >= 3 && view.selectedItem <= 4)
    {
        float panelWidth = (std::min)(540.0f, width - 64.0f);
        float x = (width - panelWidth) * 0.5f, y = height * 0.5f - 100;
        r.Rect(0, 0, static_cast<float>(width), static_cast<float>(height), {0, 0, 0, 0.7f});
        r.Rect(x - 2, y - 2, panelWidth + 4, 204, theme.border);
        r.Rect(x, y, panelWidth, 200, theme.background);
        r.Text(x + 22, y + 16, names[view.selectedItem], theme.highlight);
        r.Text(x + panelWidth - 70, y + 16, L"닫기", theme.highlight, 0.8f);
        DrawWrappedInfo(r,
                        x + 22,
                        y + 60,
                        panelWidth - 44,
                        view.selectedItem == 3
                            ? L"거점에서 먹을 식료품이다. 거점의 보관함에 내려놓자."
                            : L"여우와 장미의 생활에 필요한 생필품이다. 거점의 보관함에 내려놓자.");
        r.Text(x + 22, y + 160, L"닫기 버튼 · E 인벤토리 닫기", {0.74f, 0.8f, 0.76f}, 0.8f);
    }
}

inline void ClickInventoryPanel(int width,
                                int height,
                                int mouseX,
                                int mouseY,
                                const InventoryStatus& status,
                                InventoryViewState& view)
{
    if (view.mapOpen)
    {
        ClickStoryMap(width, height, mouseX, mouseY, status.mapPieces, view);
        return;
    }
    if (view.selectedItem >= 0)
    {
        float panelWidth = (std::min)(540.0f, width - 64.0f);
        float x = (width - panelWidth) * 0.5f, y = height * 0.5f - 100;
        if (mouseX >= x + panelWidth - 80 && mouseX <= x + panelWidth && mouseY >= y &&
            mouseY <= y + 48)
        {
            view.Back();
        }
        return;
    }
    float w = (std::min)(900.0f, width - 48.0f), h = (std::min)(610.0f, height - 48.0f);
    float left = (width - w) * 0.5f, top = (height - h) * 0.5f;
    float cell = (std::min)((w * 0.55f - 32) / 5, (h - 130 - 5 * 8 - 20) / 6);
    float gridLeft = left + w - 28 - cell * 5 - 32, gridTop = top + 74;
    float statusWidth = gridLeft - 24 - (left + 28);
    float iconSize = (std::min)(48.0f, (statusWidth - 40) / 5);
    float skillsRight = left + 28 + 4 * iconSize + 3 * 10;
    float mx = (skillsRight + gridLeft - 24 - iconSize) * 0.5f, my = top + 353;
    if (status.mapPieces && mouseX >= mx && mouseX < mx + iconSize && mouseY >= my &&
        mouseY < my + iconSize)
    {
        view.mapOpen = true;
        view.selectedPiece = -1;
        return;
    }
    for (int i = 3; i <= 4; ++i)
    {
        int slot = i + 17;
        float x = gridLeft + (slot % 5) * (cell + 8);
        float y = gridTop + (slot / 5) * (cell + 8) + 20;
        if (status.items[i] > 0 && mouseX >= x && mouseX < x + cell && mouseY >= y &&
            mouseY < y + cell)
        {
            view.selectedItem = i;
        }
    }
}