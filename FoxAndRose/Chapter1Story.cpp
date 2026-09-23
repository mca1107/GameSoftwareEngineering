#include "stdafx.h"
#include "GuideIcons.h"
#include "Chapter1.h"
#include <algorithm>
#include <cmath>

namespace
{
float StoryDistance(Vec2 a, Vec2 b)
{
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
}

const std::vector<Chapter1::DialogueLine>& Chapter1::DialogueLines() const
{
    static const std::vector<DialogueLine> intro = {
        {L"장미", L"바리게이트 다 고쳤어. 먹을 건 남아 있어?"},
        {L"여우",
         L"아직 조금 남아 있긴 한데, 거의 다 떨어졌어. 생필품도. 오늘은 물자를 구하러 가야 할 것 "
         L"같아."},
        {L"장미", L"알았어, 그럼 난 무기 손질하고 청소 좀 하고 있을게. 조심히 다녀와."},
        {L"여우",
         L"저번에 바로 앞 건물은 다 찾아봤었으니까, 오늘은 다른 곳을 찾아봐야겠다.",
         false,
         true}};
    static const std::vector<DialogueLine> observation = {
        {L"여우",
         L"으, 크리처네... 저 짐승들은 정말 난폭하다니까. 갇혀 있어서 다행이지... 너무 가까이 "
         L"다가가진 말자.",
         false,
         true}};
    static const std::vector<DialogueLine> supplies = {
        {L"여우", L"이걸로 당분간은 버틸 수 있겠네. 이제 돌아가야겠다."}};
    static const std::vector<DialogueLine> sound = {
        {L"", L"거점으로 돌아가는 길에 이상한 소리가 들린다."},
        {L"여우", L"이게 무슨 소리지? 뭔가, 웅얼거리는 듯한 소리인데..."}};
    static const std::vector<DialogueLine> encounter = {
        {L"여우", L"이건... 크리처? 다행히 공격하진 않을 것 같은데..."},
        {L"???", L"그... 어, 어..."},
        {L"여우", L"심하게 다쳤네, 곧 죽을 것 같아."},
        {L"여우", L"굳이 신경 쓸 필요는 없을 것 같은데, 이만 가자."},
        {L"???", L"아... 파..."},
        {L"여우", L"?!"},
        {L"여우", L"뭐야...? 잘못 들은 건가?"},
        {L"???", L"죽... 고... 싶지... 않..."},
        {L"???", L"집, 에..."},
        {L"???", L"...", true},
        {L"여우", L"잘못 들은 게 아니야! 크리처가 말을 할 줄 안다고...?"},
        {L"여우", L"이 크리처... 대체 뭐야?"}};
    static const std::vector<DialogueLine> map = {
        {L"", L"죽은 크리처를 살핀다."},
        {L"여우", L"이 종이... 뭐지? 뭔가 그려져 있는데."},
        {L"", L"종이를 자세히 관찰한다."},
        {L"여우", L"지도...인가? 그리고 이건 글씨...?"},
        {L"여우", L"「우리의 집」...이라. 크리처가 왜 이런 걸...?"},
        {L"여우", L"..."},
        {L"여우", L"일단... 돌아가자."}};
    static const std::vector<DialogueLine> argument = {
        {L"장미", L"왔구나, 다친 덴 없어?"},
        {L"여우", L"응. 그런데..."},
        {L"", L"여우는 말을 하는 크리처를 만난 일에 대해 이야기한다."},
        {L"여우", L"그리고 이런 걸 갖고 있었어. 있지, 이 지도를 따라가면 뭐가 있을지 궁금한데..."},
        {L"장미", L"...꿈도 꾸지 마."},
        {L"여우", L"뭐?"},
        {L"장미", L"못 들었어? 꿈도 꾸지 말라고."},
        {L"장미",
         L"정체 모를 크리처가 갖고 있던 종이 쪼가리 하나 믿고 어딜 가겠다는 거야? 무슨 위험이 "
         L"있을지도 모르는데?"},
        {L"장미", L"우리가 지금 이렇게 평화롭게 살 수 있는 것도 기적이야, 잊은 건 아니지?"},
        {L"장미", L"괜한 위험을 무릅쓸 필요는 없어. 이 종이는 그냥 버리고, 우리 그냥 이대로..."},
        {L"여우", L"...언제까지 이대로 살아야 해? 일 년? 십 년? 평생?"},
        {L"여우",
         L"오늘은 내일 먹을 걸 구하고, 내일은 그다음 날에 필요한 걸 얻고, 그다음 날엔 또 그다음을 "
         L"위한 걸 찾아다니고..."},
        {L"여우",
         L"도대체 얼마나 이런 반복되는 지루한 일상을 보내야 해? ...난 이제 지쳤어, 죽을 때까지 "
         L"이렇게 살 순 없다고."},
        {L"장미", L"..."},
        {L"장미", L"...그래, 알았어. 그렇게까지 이곳을... 나를 떠나고 싶다면 말리진 않을게."},
        {L"장미",
         L"가, 가서 네가 원하는 즐거운 모험이나 해. 난 여기서 재미없는 일상이나 보내고 있을 "
         L"거니까."},
        {L"여우", L"...하, 그래."}};
    static const std::vector<DialogueLine> reminder = {
        {L"여우",
         L"이상한 소리의 정체를 확인해 보자. 식료품점 안쪽에서 들리는 것 같아.",
         false,
         true}};
    static const std::vector<DialogueLine> empty;
    switch (m_Dialogue)
    {
    case Dialogue::Intro:
        return intro;
    case Dialogue::Observation:
        return observation;
    case Dialogue::Supplies:
        return supplies;
    case Dialogue::Sound:
        return sound;
    case Dialogue::Encounter:
        return encounter;
    case Dialogue::Map:
        return map;
    case Dialogue::ReturnReminder:
        return reminder;
    case Dialogue::Argument:
        return argument;
    default:
        return empty;
    }
}

void Chapter1::StartDialogue(Dialogue dialogue)
{
    m_Dialogue = dialogue;
    m_DialoguePage = 0;
    m_Moving = false;
    StopSound();
}

Vec2 Chapter1::TargetPosition(int target) const
{
    switch (target)
    {
    case 0:
        return m_Storage;
    case 1:
        return m_Food;
    case 2:
        return m_Supplies;
    case 3:
        return m_Rose;
    case 4:
        return m_Speaker;
    default:
        return m_Exit;
    }
}

int Chapter1::Target() const
{
    if (DialogueActive() || m_Quest == Quest::Complete)
    {
        return -1;
    }
    int result = -1;
    float nearest = 1e9f;
    for (int target = 0; target < 6; ++target)
    {
        bool enabled = (target == 0 && m_Quest == Quest::ReturningHome && !m_Deposited) ||
                       (target == 1 && m_Quest == Quest::Collecting && !m_HasFood) ||
                       (target == 2 && m_Quest == Quest::Collecting && !m_HasSupplies) ||
                       (target == 3 && m_Quest == Quest::ReturningHome && m_Deposited) ||
                       (target == 4 &&
                        (m_Quest == Quest::FollowingSound || m_Quest == Quest::InspectingMap)) ||
                       (target == 5 && m_Quest == Quest::Leaving);
        if (!enabled)
        {
            continue;
        }
        Vec2 p = TargetPosition(target);
        float distance = StoryDistance(m_Player, p);
        Vec2 center = Project(p.x, p.y), player = Project(m_Player.x, m_Player.y);
        Vec2 radius = TargetRadii(target);
        float nx = (player.x - center.x) / radius.x, ny = (player.y - center.y) / radius.y;
        // Use the maximum pulse extent so availability does not flicker with animation.
        if (nx * nx + ny * ny <= 1 && distance < nearest && ClearLine(m_Player, p))
        {
            nearest = distance;
            result = target;
        }
    }
    return result;
}

const wchar_t* Chapter1::Objective() const
{
    switch (m_Quest)
    {
    case Quest::Collecting:
        return L"WASD 이동 · 서북쪽 생필품과 동쪽 건물의 식료품을 F로 회수";
    case Quest::Returning:
        return L"물자를 챙겼다. 식료품점 출입구 쪽을 지나 거점으로 돌아가자.";
    case Quest::FollowingSound:
        return L"통로에서 들려오는 소리를 따라가 F로 크리처를 살펴보세요.";
    case Quest::InspectingMap:
        return L"F로 죽은 크리처의 품에 있는 종이를 확인하세요.";
    case Quest::ReturningHome:
        return L"남서쪽 거점 안의 장미에게 F로 말을 거세요.";
    case Quest::Leaving:
        return L"거점을 나와 바로 위 건물과 거점 사이 골목 안쪽의 표식에서 F로 모험을 떠나세요.";
    default:
        return L"장미와 함께 지내는 거점";
    }
}

void Chapter1::Interact()
{
    if (DialogueActive() || m_Quest == Quest::Complete)
    {
        return;
    }
    switch (Target())
    {
    case 0:
        m_Deposited = true;
        Notify(L"물자를 보관했습니다. 장미에게 발견한 일을 이야기하세요.");
        break;
    case 1:
        m_HasFood = true;
        m_InventoryUnlocked = true;
        Notify(L"보존식 묶음을 챙겼습니다.");
        break;
    case 2:
        m_HasSupplies = true;
        m_InventoryUnlocked = true;
        Notify(L"생필품 묶음을 챙겼습니다.");
        break;
    case 3:
        // Storage interaction must be completed before this target is enabled.
        StartDialogue(Dialogue::Argument);
        break;
    case 4:
        StartDialogue(m_Quest == Quest::FollowingSound ? Dialogue::Encounter : Dialogue::Map);
        break;
    case 5:
        m_Quest = Quest::Complete;
        m_DepartureTime = 0;
        m_Moving = false;
        StopSound();
        break;
    default:
        // The quest state is tracked without repeating control instructions.
        break;
    }
    if (m_Quest == Quest::Collecting && m_HasFood && m_HasSupplies)
    {
        m_Quest = Quest::Returning;
        StartDialogue(Dialogue::Supplies);
        // The quest state is tracked without repeating control instructions.
    }
}

void Chapter1::AdvanceDialogue()
{
    if (!DialogueActive() || m_EntranceTime < 1.2f)
    {
        return;
    }
    ++m_DialoguePage;
    if (m_DialoguePage < static_cast<int>(DialogueLines().size()) &&
        DialogueLines()[m_DialoguePage].creatureDies)
    {
        m_CreatureDead = true;
    }
    if (m_DialoguePage < static_cast<int>(DialogueLines().size()))
    {
        return;
    }
    Dialogue finished = m_Dialogue;
    m_Dialogue = Dialogue::None;
    m_DialoguePage = 0;
    switch (finished)
    {
    case Dialogue::Intro:
        m_Quest = Quest::Collecting;
        m_MoveHintActive = true;
        break;
    case Dialogue::Supplies:
        break;
    case Dialogue::Sound:
        m_SpeakerSpawned = true;
        m_SoundTimer = 0;
        break;
    case Dialogue::Encounter:
        m_Quest = Quest::InspectingMap;
        break;
    case Dialogue::Map:
        m_HasMap = true;
        m_Quest = Quest::ReturningHome;
        break;
    case Dialogue::Argument:
        m_Quest = Quest::Leaving;
        break;
    default:
        break;
    }
    // Control hints are drawn separately and only until learned.
}

void Chapter1::UpdateStory(float dt)
{
    // Both exits belong to the food store; collecting the western supplies last
    // requires returning through this area rather than firing the scene remotely.
    bool nearWestExit =
        m_Player.x > 29.5f && m_Player.x < 32.5f && m_Player.y > 16 && m_Player.y < 20;
    bool nearSouthExit =
        m_Player.x > 37.5f && m_Player.x < 40.5f && m_Player.y > 27.5f && m_Player.y < 30.5f;
    if (!DialogueActive() && m_Quest == Quest::Returning && (nearWestExit || nearSouthExit))
    {
        m_Quest = Quest::FollowingSound;
        StartDialogue(Dialogue::Sound);
        PlaySoundCue();
    }
    if (m_Quest == Quest::FollowingSound && !DialogueActive())
    {
        m_SoundTimer -= dt;
        if (m_SoundTimer <= 0)
        {
            PlaySoundCue();
            m_SoundTimer = 5;
        }
    }
}

void Chapter1::DrawRose(Renderer& r)
{
    Vec2 p = Project(m_Rose.x, m_Rose.y);
    float s = m_Scale / 30;
    r.Ellipse({p.x + s, p.y + 2 * s}, 10 * s, 4 * s, {0.01f, 0.02f, 0.02f, 0.3f});
    r.DrawModel(Renderer::Model::Rose, p, s * 0.76f);
    r.Text(p.x, p.y - 76 * s, L"장미", {0.93f, 0.78f, 0.78f}, 0.8f, true);
}

void Chapter1::DrawSpeaker(Renderer& r)
{
    Vec2 p = Project(m_Speaker.x, m_Speaker.y);
    float s = m_Scale / 30;
    float breath = m_CreatureDead ? 0 : std::sin(m_Time * 6) * 1.2f * s;
    r.Ellipse(p, 42 * s, 12 * s, {0.02f, 0.03f, 0.025f, 0.3f});
    // Reuse the evolved ape model in a collapsed side-lying pose.
    r.DrawModel(
        Renderer::Model::Creature, {p.x - 26 * s, p.y - 6 * s + breath}, s * 0.9f, 1.5707963f);
    if (m_CreatureDead && !m_HasMap)
    {
        r.Quad({p.x, p.y - 8 * s},
               {p.x + 8 * s, p.y - 10 * s},
               {p.x + 10 * s, p.y - 2 * s},
               {p.x + 2 * s, p.y},
               {0.77f, 0.71f, 0.53f});
    }
}

bool Chapter1::DrawStoryUI(Renderer& r)
{
    if (m_Quest == Quest::Complete)
    {
        float alpha = (std::min)(1.0f, m_DepartureTime / 0.6f);
        r.Rect(0,
               0,
               static_cast<float>(m_Width),
               static_cast<float>(m_Height),
               {0.02f, 0.03f, 0.03f, alpha});
        r.Text(m_Width * 0.5f,
               m_Height * 0.5f - 30,
               L"여우는 장미와 함께 살던 거점을 떠나 모험을 시작한다.",
               {0.94f, 0.86f, 0.68f},
               1,
               true);
        r.Text(m_Width * 0.5f,
               m_Height * 0.5f + 15,
               L"챕터 2로 이어집니다",
               {0.73f, 0.78f, 0.71f},
               0.9f,
               true);
        return true;
    }
    if (!DialogueActive() || m_EntranceTime < 1.2f)
    {
        return false;
    }
    const DialogueLine& line = DialogueLines()[m_DialoguePage];
    // Conservative character width keeps long Korean dialogue inside the panel.
    const size_t columns = static_cast<size_t>((std::max)(12, (m_Width - 96) / 21));
    const bool narration = line.speaker[0] == L'\0';
    const bool thought = line.thought;
    const std::wstring quote = thought ? L"'" : L"\"";
    const std::wstring text = narration ? std::wstring(line.text) : quote + line.text + quote;
    std::vector<std::wstring> rows;
    for (size_t start = 0; start < text.size();)
    {
        size_t count = (std::min)(columns, text.size() - start);
        if (start + count < text.size())
        {
            size_t space = text.rfind(L' ', start + count - 1);
            if (space != std::wstring::npos && space > start + count / 2)
            {
                count = space - start + 1;
            }
        }
        rows.push_back(text.substr(start, count));
        start += count;
    }
    float panelHeight = 118.0f + static_cast<float>(rows.size()) * 28;
    float y = m_Height - panelHeight - 30;
    if (m_Dialogue == Dialogue::Map && m_DialoguePage >= 2)
    {
        float availableHeight = (std::max)(80.0f, y - 64);
        float mapWidth =
            (std::min)(720.0f, (std::min)(m_Width - 64.0f, availableHeight * 400 / 210));
        float mapHeight = mapWidth * 210 / 400;
        float mapX = (m_Width - mapWidth) * 0.5f;
        float mapY = (std::max)(24.0f, (y - mapHeight) * 0.5f);
        DrawStoryMapPiece(r, 0, mapX, mapY, mapWidth, mapHeight);
    }
    r.Rect(24, y, static_cast<float>(m_Width - 48), panelHeight, {0.025f, 0.05f, 0.045f, 0.97f});
    r.Rect(24, y, 4, panelHeight, {0.86f, 0.68f, 0.36f});
    if (!narration)
    {
        r.Text(44, y + 14, line.speaker, {0.94f, 0.8f, 0.5f}, 1.1f);
    }
    for (size_t row = 0; row < rows.size(); ++row)
    {
        r.Text(44, y + 58 + static_cast<float>(row) * 28, rows[row], Color(), 0.95f);
    }
    if (m_Dialogue == Dialogue::Intro && m_DialoguePage == 0)
    {
        // Right inset: 62 - 24 - 15 = 23; bottom inset: 45 - 22 = 23.
        DrawMouseGuide(r, {m_Width - 62.0f, y + panelHeight - 45});
    }
    return true;
}
