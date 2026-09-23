#include "stdafx.h"
#include "BookMenu.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace
{
constexpr float PageWidth = 360;
constexpr float PageHeight = 500;
constexpr float Top = 70;
constexpr float Spine = 500;
constexpr float Pi = 3.14159265f;
constexpr int SpreadCount = 4;
constexpr float FoldSize = 54;
const Color Ink(0.20f, 0.24f, 0.23f);
const Color Gold(0.76f, 0.62f, 0.36f);

struct Layout
{
    float scale;
    Vec2 origin;

    Layout(int width, int height)
    {
        scale = (std::min)(width / 1000.0f, height / 640.0f);
        origin = {(width - 1000 * scale) * 0.5f, (height - 640 * scale) * 0.5f};
    }

    Vec2 Local(int x, int y) const
    {
        return {(x - origin.x) / scale, (y - origin.y) / scale};
    }

    void Apply(Renderer& r, float x = 0, float y = 0, float sx = 1, float shear = 0) const
    {
        r.SetUiTransform(
            {origin.x + x * scale, origin.y + y * scale}, scale * sx, scale, scale * shear);
    }
};

bool Inside(Vec2 p, float x, float y, float w, float h)
{
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
}

// Match the triangular paper fold, not an invisible rectangle around it.
bool FoldHit(Vec2 p, bool next)
{
    float edge = next ? Spine + PageWidth : Spine - PageWidth;
    float dx = next ? edge - p.x : p.x - edge;
    float dy = Top + PageHeight - p.y;
    return dx >= 0 && dy >= 0 && dx <= FoldSize && dy <= FoldSize && dx + dy >= FoldSize;
}

struct Chapter
{
    const wchar_t* title;
    const wchar_t* lines[3];
};

const Chapter Chapters[] = {
    {L"모험의 시작", {L"평범한 일상을 보내던 여우는 어느 날,", L"특이한 존재와 만나게 된다.", L""}},
    {L"왕",
     {L"거점을 벗어나 펼쳐진 폐허의 길.",
      L"물자를 모으고 위험에 맞서며",
      L"더 먼 곳으로 나아갈 힘을 기른다."}},
    {L"허영심 많은 사람",
     {L"여행길에서 허영심 많은 이를 만난다.", L"그와 나눌 이야기는", L"아직 쓰이지 않았다."}},
    {L"술꾼", {L"다음 길목에서 술꾼과 마주한다.", L"그가 품은 사연은", L"아직 쓰이지 않았다."}},
    {L"사업가",
     {L"여우의 여정은 사업가에게 닿는다.", L"이 만남의 이야기는", L"아직 쓰이지 않았다."}},
    {L"가로등지기",
     {L"길 위에서 가로등지기를 만난다.", L"그와 함께할 시간은", L"아직 쓰이지 않았다."}},
    {L"지리학자",
     {L"종착지를 앞두고 지리학자를 만난다.",
      L"마지막 길로 이어지는 이야기는",
      L"아직 쓰이지 않았다."}},
    {L"장미에게로",
     {L"수많은 장미, 그리고 왕자와의 만남.",
      L"함께 쌓아 온 시간의 의미를 깨닫고,",
      L"여우는 자신의 장미에게 돌아간다."}}};

static_assert(sizeof(Chapters) / sizeof(Chapters[0]) == SpreadCount * 2,
              "Each book spread must contain two chapter pages.");

void Border(Renderer& r, float x, float y, float w, float h, Color c)
{
    r.Rect(x, y, w, 1, c);
    r.Rect(x, y + h, w, 1, c);
    r.Rect(x, y, 1, h, c);
    r.Rect(x + w, y, 1, h, c);
}

void Star(Renderer& r, float x, float y, float size, Color c)
{
    r.Quad({x, y - size}, {x + size * 0.3f, y}, {x, y + size}, {x - size * 0.3f, y}, c);
    r.Line({x - size * 0.7f, y}, {x + size * 0.7f, y}, 1, c);
}

void Plant(Renderer& r, float x, float y, float size, Color color)
{
    r.Line({x, y}, {x + size * 0.2f, y - size}, 2, color);
    for (int i = 0; i < 4; ++i)
    {
        float py = y - size * (0.2f + i * 0.19f);
        float px = x + (y - py) * 0.2f;
        float sign = i % 2 == 0 ? -1.0f : 1.0f;
        r.Quad({px, py},
               {px + sign * 13, py - 3},
               {px + sign * 18, py - 15},
               {px + sign * 5, py - 12},
               color);
    }
}

// Cover symbols are independent of the in-game character models.
void RedFox(Renderer& r, float x, float y)
{
    const Color fur(0.78f, 0.30f, 0.12f);
    const Color light(0.94f, 0.48f, 0.20f);
    const Color dark(0.12f, 0.10f, 0.10f);
    // A curved, tapered tail with a pointed dark tip, behind the seated body.
    auto tailEdge = [&](float t, float side)
    {
        float u = 1 - t;
        float cx = u * u * -12 + 2 * u * t * -57 + t * t * -65;
        float cy = u * u * -12 + 2 * u * t * -6 + t * t * -48;
        float dx = 2 * u * -45 + 2 * t * -8;
        float dy = 2 * u * 6 + 2 * t * -42;
        float length = std::sqrt(dx * dx + dy * dy);
        float radius = (1 - t) * 5 + std::sin(Pi * t) * 11;
        return Vec2{x + cx - dy / length * radius * side, y + cy + dx / length * radius * side};
    };
    for (int i = 0; i < 20; ++i)
    {
        float t0 = i / 20.0f;
        float t1 = (i + 1) / 20.0f;
        r.Quad(tailEdge(t0, -1),
               tailEdge(t1, -1),
               tailEdge(t1, 1),
               tailEdge(t0, 1),
               i >= 16 ? dark : fur);
    }

    const Color farFur(0.63f, 0.25f, 0.12f);
    const Color scarf(0.76f, 0.16f, 0.18f);

    // Draw the far leg first so that its shoulder is hidden by the torso.
    r.Quad({x + 17, y - 34}, {x + 26, y - 34}, {x + 22, y}, {x + 13, y}, farFur);
    r.Ellipse({x + 21.5f, y - 34}, 4.5f, 5, farFur);
    r.Ellipse({x + 19, y}, 6, 3, farFur);

    // A single rectangular scarf end hangs diagonally to the left.
    r.Line({x - 18, y - 52}, {x - 32, y - 29}, 10, scarf);
    r.Ellipse({x, y - 30}, 23, 34, fur);
    r.Ellipse({x + 7, y - 38}, 12, 25, {0.91f, 0.77f, 0.55f});

    // The near foreleg slopes inward; the two paws finish with a small gap.
    r.Quad({x - 18, y - 32}, {x - 6, y - 32}, {x + 11, y + 1}, {x - 1, y + 1}, light);
    r.Ellipse({x - 12, y - 32}, 6, 8, light);
    r.Ellipse({x + 5, y + 1}, 7, 3, light);

    // The far ear and its dark tip must be behind the face.
    r.Triangle({x + 5, y - 80}, {x + 14, y - 101}, {x + 20, y - 72}, farFur);
    r.Triangle({x + 14, y - 101}, {x + 10, y - 92}, {x + 17, y - 90}, dark);
    r.Ellipse({x + 5, y - 67}, 22, 19, light);
    r.Triangle({x - 13, y - 75}, {x - 13, y - 105}, {x + 2, y - 81}, light);
    r.Triangle({x - 13, y - 105}, {x - 13, y - 92}, {x - 6, y - 94}, dark);
    r.Triangle({x + 12, y - 75}, {x + 39, y - 63}, {x + 11, y - 52}, light);
    r.Triangle({x + 12, y - 63}, {x + 39, y - 63}, {x + 11, y - 52}, {0.94f, 0.82f, 0.62f});
    r.Ellipse({x + 36, y - 63}, 3, 2, dark);
    r.Ellipse({x + 15, y - 72}, 2, 2, dark);

    // Preserve the rounded wrap, with a darker lower face beneath the red upper fold.
    const Color scarfShade(0.49f, 0.055f, 0.095f);
    r.Rect(x - 14, y - 57, 33, 13, scarfShade);
    r.Ellipse({x - 14, y - 50.5f}, 7, 6.5f, scarfShade);
    r.Ellipse({x + 19, y - 50.5f}, 7, 6.5f, scarfShade);
    r.Rect(x - 14, y - 57, 33, 9, scarf);
    r.Ellipse({x - 14, y - 52.5f}, 7, 4.5f, scarf);
    r.Ellipse({x + 19, y - 52.5f}, 7, 4.5f, scarf);
}

void Grass(Renderer& r, float x, float y)
{
    const Color color(0.35f, 0.46f, 0.33f);
    r.Triangle({x - 7, y}, {x - 16, y - 13}, {x - 2, y}, color);
    r.Triangle({x - 3, y}, {x - 5, y - 20}, {x + 3, y}, color);
    r.Triangle({x + 1, y}, {x + 12, y - 16}, {x + 6, y}, color);
    r.Triangle({x + 5, y}, {x + 20, y - 7}, {x + 10, y}, color);
}

void BlackRose(Renderer& r, float x, float y)
{
    const Color stem(0.38f, 0.48f, 0.32f);
    r.Line({x - 5, y + 88}, {x, y + 12}, 3, stem);
    r.Quad({x - 2, y + 55}, {x - 27, y + 48}, {x - 35, y + 27}, {x - 10, y + 36}, stem);
    r.Quad({x - 1, y + 39}, {x + 24, y + 34}, {x + 32, y + 14}, {x + 9, y + 24}, stem);
    r.Triangle({x - 3, y + 65}, {x + 5, y + 59}, {x - 2, y + 57}, stem);
    // Muted silver edges separate the black petals from the dark cover.
    for (int ring = 0; ring < 3; ++ring)
    {
        float radius = 17.0f - ring * 6;
        float petal = 13.0f - ring * 3;
        for (int i = 0; i < 5; ++i)
        {
            float angle = i * 2 * Pi / 5 + ring * 0.65f;
            Vec2 center{x + std::cos(angle) * radius, y + std::sin(angle) * radius};
            r.Ellipse(center, petal, petal * 0.85f, {0.42f, 0.43f, 0.43f});
            r.Ellipse({center.x, center.y + 1},
                      petal - 1.5f,
                      petal * 0.85f - 1.5f,
                      {0.055f + ring * 0.018f, 0.065f + ring * 0.018f, 0.08f + ring * 0.018f});
        }
    }
    r.Ellipse({x, y}, 4, 3, {0.025f, 0.03f, 0.04f});
}

// Original primitive illustration: a red fox beside a black rose in overgrown ruins.
void Cover(Renderer& r, bool hover, bool exitHover)
{
    r.Rect(0, 0, PageWidth, PageHeight, {0.10f, 0.19f, 0.22f});
    r.Rect(0, 0, 15, PageHeight, {0.065f, 0.12f, 0.15f});
    Border(r, 23, 17, 319, 465, hover ? Color(0.92f, 0.77f, 0.48f) : Gold);
    Border(r, 28, 22, 309, 455, {0.35f, 0.39f, 0.32f});
    const Vec2 stars[] = {
        {61, 171}, {116, 194}, {169, 164}, {295, 177}, {76, 243}, {162, 230}, {302, 224}};
    for (const Vec2& star : stars)
    {
        Star(r, star.x, star.y, 3.5f, {0.76f, 0.71f, 0.49f, 0.8f});
    }
    // Keep the former top edge at 166 while raising the lower edge away from the ruins.
    r.Ellipse({224, 199}, 33, 33, {0.69f, 0.68f, 0.48f});
    r.Ellipse({234.5f, 193.5f}, 29, 30, {0.10f, 0.19f, 0.22f});
    for (int i = 0; i < 5; ++i)
    {
        float x = 45.0f + i * 55;
        float h = 45.0f + (i * 31 % 65);
        r.Rect(x, 351 - h, 40, h + 26, {0.16f, 0.27f, 0.28f});
        r.Triangle({x, 351 - h}, {x + 17, 342 - h}, {x + 40, 351 - h}, {0.16f, 0.27f, 0.28f});
        for (int j = 0; j < 3; ++j)
        {
            r.Rect(x + 8, 361 - h + j * 17, 7, 8, {0.09f, 0.17f, 0.20f});
        }
    }
    r.Quad({32, 361}, {204, 337}, {330, 365}, {330, 402}, {0.19f, 0.32f, 0.29f});
    r.Triangle({32, 361}, {330, 402}, {32, 402}, {0.19f, 0.32f, 0.29f});
    RedFox(r, 148, 382);
    BlackRose(r, 230, 294);
    Grass(r, 63, 393);
    Grass(r, 296, 398);
    r.Text(185, 66, L"여우와 장미", {0.94f, 0.84f, 0.59f}, 1.65f, true);
    r.Line({103, 136}, {267, 136}, 1, Gold);
    r.Ellipse({185, 136}, 2, 2, Gold);
    r.Rect(110, 431, 150, 34, exitHover ? Color(0.30f, 0.36f, 0.33f) : Color(0.10f, 0.19f, 0.22f));
    Border(r, 110, 431, 150, 34, Gold);
    r.Text(185, 448, L"게임 종료", {0.88f, 0.80f, 0.62f}, 0.9f, true, true);
}

void Page(Renderer& r, int chapter, bool hover)
{
    r.Rect(0, 0, PageWidth, PageHeight, {0.89f, 0.85f, 0.73f});
    r.Rect(7, 5, PageWidth - 14, PageHeight - 10, {0.95f, 0.91f, 0.80f});
    for (int i = 0; i < 18; ++i)
    {
        r.Line({18, 18.0f + i * 26}, {342, 17.0f + i * 26}, 0.6f, {0.63f, 0.52f, 0.36f, 0.065f});
    }
    bool left = chapter % 2 == 0;
    for (int i = 0; i < 18; ++i)
    {
        r.Rect(left ? PageWidth - 18 + i : static_cast<float>(i),
               0,
               1,
               PageHeight,
               {0.23f, 0.18f, 0.13f, (left ? i : 17 - i) * 0.009f});
    }
    const Chapter& data = Chapters[chapter];
    r.Text(180, 42, L"CHAPTER " + std::to_wstring(chapter + 1), {0.49f, 0.38f, 0.23f}, 0.85f, true);
    r.Text(180, 83, data.title, Ink, 1.25f, true);
    r.Line({115, 137}, {245, 137}, 1, Gold);
    Star(r, 180, 137, 5, Gold);
    for (int i = 0; i < 3; ++i)
    {
        r.Text(180, 177.0f + 31 * i, data.lines[i], Ink, 0.78f, true);
    }
    r.Line({91, 344}, {269, 344}, 1, {0.52f, 0.58f, 0.44f});
    Plant(r, 122, 344, 55, {0.39f, 0.48f, 0.34f});
    Plant(r, 242, 344, 38, {0.39f, 0.48f, 0.34f});
    Star(r, 184, 306, 11, Gold);
    bool playable = chapter < 2;
    r.Rect(84,
           386,
           192,
           43,
           playable ? (hover ? Color(0.25f, 0.38f, 0.32f) : Color(0.16f, 0.28f, 0.26f))
                    : Color(0.83f, 0.80f, 0.70f));
    Border(r, 84, 386, 192, 43, playable ? Gold : Color(0.65f, 0.62f, 0.53f));
    r.Text(180,
           407.5f,
           playable ? L"챕터 시작" : L"준비 중",
           playable ? Color(0.96f, 0.90f, 0.74f) : Color(0.46f, 0.44f, 0.38f),
           0.9f,
           true,
           true);
    r.Text(180, 461, std::to_wstring(chapter + 1), {0.52f, 0.45f, 0.32f}, 0.8f, true);
}

void Fold(Renderer& r, bool next, bool hover)
{
    float x = next ? Spine + PageWidth : Spine - PageWidth;
    float y = Top + PageHeight;
    float sign = next ? -1.0f : 1.0f;
    const Vec2 edgeTop{x, y - FoldSize};
    const Vec2 edgeBottom{x + sign * FoldSize, y};
    const Vec2 foldedTip{x + sign * FoldSize, y - FoldSize};
    // The corner folds inward; the exposed underlying sheet is not a button.
    r.Triangle(edgeTop, edgeBottom, {x, y}, {0.68f, 0.62f, 0.46f});
    r.Triangle({edgeTop.x + sign * 2, edgeTop.y + 3},
               {edgeBottom.x + sign * 3, edgeBottom.y - 1},
               {foldedTip.x + sign * 4, foldedTip.y + 3},
               {0.30f, 0.25f, 0.16f, 0.20f});
    r.Triangle(edgeTop,
               foldedTip,
               edgeBottom,
               hover ? Color(1.0f, 0.95f, 0.78f) : Color(0.94f, 0.88f, 0.71f));
    r.Line(edgeTop, foldedTip, 1, {0.99f, 0.95f, 0.82f});
    r.Line(foldedTip, edgeBottom, 1, Gold);
    r.Line(edgeTop, edgeBottom, 1, {0.58f, 0.51f, 0.36f});
}
} // namespace

void BookMenu::Reset()
{
    m_Open = false;
    m_Spread = 0;
    m_Motion = Motion::Still;
    m_Progress = 0;
}

void BookMenu::Update(float dt)
{
    if (m_Motion == Motion::Still)
    {
        return;
    }
    float duration = m_Motion == Motion::Opening || m_Motion == Motion::Closing ? 0.85f : 0.65f;
    // Animation duration follows elapsed time, including frames slower than 20 FPS.
    m_Progress = (std::min)(1.0f, m_Progress + (std::max)(0.0f, dt) / duration);
    if (m_Progress < 1)
    {
        return;
    }
    if (m_Motion == Motion::Opening)
    {
        m_Open = true;
    }
    else if (m_Motion == Motion::Closing)
    {
        m_Open = false;
    }
    else
    {
        m_Spread += m_Motion == Motion::Next ? 1 : -1;
    }
    m_Motion = Motion::Still;
    m_Progress = 0;
}

BookMenu::Action BookMenu::Click(int x, int y, int width, int height)
{
    if (m_Motion != Motion::Still)
    {
        return Action::None;
    }
    Vec2 p = Layout(width, height).Local(x, y);
    if (!m_Open)
    {
        if (Inside(p, 430, Top + 431, 150, 34))
        {
            return Action::Exit;
        }
        if (Inside(p, 320, Top, PageWidth, PageHeight))
        {
            m_Motion = Motion::Opening;
        }
    }
    else if (FoldHit(p, false))
    {
        m_Motion = m_Spread == 0 ? Motion::Closing : Motion::Previous;
    }
    else if (m_Spread < SpreadCount - 1 && FoldHit(p, true))
    {
        m_Motion = Motion::Next;
    }
    else if (m_Spread == 0)
    {
        if (Inside(p, Spine - PageWidth + 84, Top + 386, 192, 43))
        {
            return Action::Chapter1;
        }
        if (Inside(p, Spine + 84, Top + 386, 192, 43))
        {
            return Action::Chapter2;
        }
    }
    return Action::None;
}

void BookMenu::Draw(Renderer& r, int width, int height, int mouseX, int mouseY) const
{
    Layout layout(width, height);
    Vec2 mouse = layout.Local(mouseX, mouseY);
    bool still = m_Motion == Motion::Still;
    float t = m_Progress * m_Progress * (3 - 2 * m_Progress);
    bool coverMotion = m_Motion == Motion::Opening || m_Motion == Motion::Closing;
    float openness =
        coverMotion ? (m_Motion == Motion::Opening ? t : 1 - t) : (m_Open ? 1.0f : 0.0f);
    float spine = 320 + 180 * openness;
    float coverCosine = std::cos(Pi * openness);
    // The left half exists only after the cover has crossed the spine.
    float leftWidth = PageWidth * (std::max)(0.0f, -coverCosine);
    float left = spine - leftWidth;
    float totalWidth = PageWidth + leftWidth;

    r.Begin();
    r.Rect(0, 0, static_cast<float>(width), static_cast<float>(height), {0.035f, 0.055f, 0.07f});
    r.FinishWorld(0);
    layout.Apply(r);
    r.Ellipse({500, 366}, 461, 267, {0.11f, 0.15f, 0.16f, 0.34f});
    r.Ellipse({left + totalWidth * 0.5f + 5, 586}, totalWidth * 0.5f + 15, 25, {0, 0, 0, 0.30f});
    r.Rect(left - 8, Top - 7, totalWidth + 16, PageHeight + 24, {0.09f, 0.16f, 0.18f});
    for (int i = 6; i >= 0; --i)
    {
        r.Rect(left,
               Top + 4 + i * 1.6f,
               totalWidth,
               PageHeight,
               i % 2 == 0 ? Color(0.73f, 0.68f, 0.55f) : Color(0.87f, 0.81f, 0.66f));
    }

    auto page = [&](int chapter, float x, float sx = 1.0f, float shear = 0.0f, float y = Top)
    {
        layout.Apply(r, x, y, sx, shear);
        bool hover = still && Inside(mouse, x + 84, Top + 386, 192, 43);
        Page(r, chapter, hover);
    };

    if (!m_Open && !coverMotion)
    {
        layout.Apply(r, 320, Top);
        Cover(r,
              Inside(mouse, 320, Top, PageWidth, PageHeight),
              Inside(mouse, 430, Top + 431, 150, 34));
    }
    else if (coverMotion)
    {
        page(1, spine);
        float cosine = coverCosine;
        float lift = std::sin(Pi * openness) * 0.07f;
        if (cosine >= 0)
        {
            layout.Apply(r, spine, Top, cosine, -lift);
            Cover(r, false, false);
        }
        else
        {
            page(0, spine + PageWidth * cosine, -cosine, lift, Top - PageWidth * lift);
        }
    }
    else if (m_Motion == Motion::Next || m_Motion == Motion::Previous)
    {
        bool next = m_Motion == Motion::Next;
        int target = m_Spread + (next ? 1 : -1);
        page((next ? m_Spread : target) * 2, Spine - PageWidth);
        page((next ? target : m_Spread) * 2 + 1, Spine);
        float cosine = std::cos(Pi * t);
        float sx = std::abs(cosine);
        float lift = std::sin(Pi * t) * 0.075f;
        bool onRight = next ? cosine >= 0 : cosine < 0;
        int chapter = t < 0.5f ? m_Spread * 2 + (next ? 1 : 0) : target * 2 + (next ? 0 : 1);
        if (sx > 0.001f)
        {
            page(chapter,
                 onRight ? Spine : Spine - PageWidth * sx,
                 sx,
                 onRight ? -lift : lift,
                 onRight ? Top : Top - PageWidth * lift);
        }
    }
    else
    {
        page(m_Spread * 2, Spine - PageWidth);
        page(m_Spread * 2 + 1, Spine);
    }
    layout.Apply(r);
    if (m_Open && still)
    {
        Fold(r, false, FoldHit(mouse, false));
        if (m_Spread < SpreadCount - 1)
        {
            Fold(r, true, FoldHit(mouse, true));
        }
    }
    r.SetUiTransform();
}
