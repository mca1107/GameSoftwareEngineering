#pragma once
#include "Renderer.h"

class BookMenu
{
public:
    enum class Action
    {
        None,
        Exit,
        Chapter1,
        Chapter2
    };

    void Reset();
    void Update(float dt);
    void Draw(Renderer& renderer, int width, int height, int mouseX, int mouseY) const;
    Action Click(int x, int y, int width, int height);

private:
    enum class Motion
    {
        Still,
        Opening,
        Closing,
        Next,
        Previous
    };

    bool m_Open = false;
    int m_Spread = 0;
    Motion m_Motion = Motion::Still;
    float m_Progress = 0;
};
