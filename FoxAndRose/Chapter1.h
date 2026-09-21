#pragma once
#include "Renderer.h"
#include "InventoryPanel.h"
#include <vector>
#include <string>

class Chapter1
{
public:
    Chapter1();
    ~Chapter1();
    void StopSound();
    void Update(float dt, const bool* keys);
    void Interact();
    void AdvanceDialogue();

    bool CanOpenInventory() const
    {
        return m_InventoryUnlocked && !DialogueActive() && m_Quest != Quest::Complete;
    }

    void OnInventoryOpened()
    {
        m_InventoryLearned = true;
    }

    InventoryStatus Status() const
    {
        InventoryStatus result;
        result.attack = result.defense = 0;
        if (!m_Deposited)
        {
            result.items[3] = m_HasFood ? 1 : 0;
            result.items[4] = m_HasSupplies ? 1 : 0;
        }
        result.mapPieces = m_HasMap ? 1u : 0u;
        return result;
    }

    bool DialogueActive() const
    {
        return m_Dialogue != Dialogue::None;
    }

    void Draw(Renderer& renderer, int width, int height);

    bool ReadyForChapter2() const
    {
        return m_Quest == Quest::Complete && m_DepartureTime >= 1.2f;
    }

private:
    enum class Dialogue
    {
        None,
        Intro,
        Observation,
        Supplies,
        Sound,
        Encounter,
        Map,
        ReturnReminder,
        Argument
    };

    struct DialogueLine
    {
        const wchar_t* speaker;
        const wchar_t* text;
        bool creatureDies = false;
        bool thought = false;
    };

    Dialogue m_Dialogue = Dialogue::Intro;
    int m_DialoguePage = 0;
    bool m_CreatureDead = false, m_HasMap = false, m_Deposited = false;
    float m_DepartureTime = 0;
    float m_EntranceTime = 0;
    float m_MovePracticeDistance = 0;
    bool m_MoveHintActive = false;
    bool m_InteractionHintActive = false;
    bool m_InteractionLearned = false;
    bool m_InventoryUnlocked = false;
    bool m_InventoryLearned = false;
    const std::wstring m_MapName = L"여우와 장미의 거점";
    static constexpr float GroundScale = 1.2f;
    Vec2 m_Rose{6, 33}, m_Speaker{39, 36}, m_Exit{1.6f, 25.5f};
    const std::vector<DialogueLine>& DialogueLines() const;
    void StartDialogue(Dialogue dialogue);
    void UpdateStory(float dt);
    bool ReturnBlocked(Vec2 p) const;
    bool m_ReturnReminderShown = false;
    void DrawMinimap(Renderer& r);
    void DrawDirection(Renderer& r);
    bool NavigationGoal(Vec2& goal) const;
    static constexpr int NavWidth = 85, NavHeight = 81;
    std::vector<bool> m_NavWalkable;
    std::vector<int> m_NavDistance;
    int m_NavGoal = -1;
    void BuildNavigation();
    bool DrawStoryUI(Renderer& r);
    void DrawRose(Renderer& r);
    void DrawSpeaker(Renderer& r);
    const wchar_t* Objective() const;
    float m_SoundTimer = 0;
    void PlaySoundCue();

    // One observation room beside the only northbound passage.
    struct RoomBounds
    {
        float left, top, right, bottom;
    };

    RoomBounds m_Observation{17, 1, 24, 8};

    struct Box
    {
        float x, y, w, d, h;
        Color color;
        bool solid;
        int kind;
    };

    struct Plant
    {
        float x, y, size;
    };

    struct Creature
    {
        Vec2 position, home;
        float phase = 0, heading = 0;
        bool alert = false;
        bool returning = false;
        float upright = 1;
    };

    std::vector<Creature> m_Creatures;
    float m_Time = 0;
    int m_Facing = 0;
    bool m_Moving = false;
    void UpdateCreatures(float dt);
    void DrawCreature(Renderer& r, const Creature& creature);
    void DrawShadows(Renderer& r);
    void DrawAtmosphere(Renderer& r);
    enum class Quest
    {
        NotAccepted,
        Collecting,
        Returning,
        FollowingSound,
        InspectingMap,
        ReturningHome,
        Leaving,
        Complete
    };
    std::vector<Box> m_Boxes;
    std::vector<Plant> m_Plants;
    Vec2 m_Player{7, 35}, m_Camera{7, 35};
    Vec2 m_Storage{4, 34}, m_Food{38, 5}, m_Supplies{4, 5};
    Quest m_Quest = Quest::NotAccepted;
    bool m_HasFood = false, m_HasSupplies = false, m_Moved = false;
    bool m_EncounterSeen = false;
    float m_Elapsed = 0, m_ToastTime = 8, m_Idle = 0, m_Walk = 0;
    std::wstring m_Toast = L"장미와 함께 지내는 거점 · 오늘의 물자를 준비하세요.";
    int m_Width = 1280, m_Height = 800;
    float m_Scale = 30;
    void AddBox(
        float x, float y, float w, float d, float h, Color color, bool solid = true, int kind = 0);
    void Building(float x, float y, float w, float d);
    bool Blocked(Vec2 point, float radius = 0.28f, bool enclosedActor = false) const;
    void BuildEnclosures();
    bool ClearLine(Vec2 from, Vec2 to) const;
    int Target() const;
    Vec2 TargetPosition(int target) const;
    void Notify(const std::wstring& text);
    Vec2 Project(float x, float y, float z = 0) const;
    void Ground(Renderer& r, float x, float y, float w, float d, Color c);
    void DrawBox(Renderer& r, const Box& b);
    void DrawPlant(Renderer& r, const Plant& plant);
    void Person(Renderer& r, Vec2 p, bool player);
    void DrawUI(Renderer& r);
    void DrawControlHint(Renderer& r, const wchar_t* text, float dialogueTop);
};
