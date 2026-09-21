/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#include "Renderer.h"
#include "BookMenu.h"
#include "Chapter1.h"
#include "Chapter2.h"
#include "Dependencies/freeglut.h"
#include <windows.h>
#include <memory>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <iostream>

namespace
{
enum class Screen
{
    Main,
    Playing,
    Paused,
    Inventory
};
Screen screen = Screen::Main;
BookMenu bookMenu;
InventoryViewState inventoryView;
std::unique_ptr<Renderer> renderer;
std::unique_ptr<Chapter1> chapter1;
std::unique_ptr<Chapter2> chapter2;
bool keys[256] = {};
int width = 1280, height = 800;
int mouseX = -1, mouseY = -1;
HWND gameWindow = nullptr;
auto previous = std::chrono::steady_clock::now();

struct Button
{
    float x, y, w, h;

    bool Contains(int px, int py) const
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

Button MainButton()
{
    return {width * 0.5f - 140, height * 0.5f + 28, 280, 54};
}

void ResetInput()
{
    std::fill(std::begin(keys), std::end(keys), false);
    previous = std::chrono::steady_clock::now();
}

unsigned char Normalize(unsigned char key)
{
    return key >= 'A' && key <= 'Z' ? static_cast<unsigned char>(key - 'A' + 'a') : key;
}

void Close()
{
    chapter1.reset();
    chapter2.reset();
    renderer.reset();
}

void DrawButton(Button b, const wchar_t* label)
{
    bool hover = b.Contains(mouseX, mouseY);
    renderer->Rect(
        b.x, b.y, b.w, b.h, hover ? Color(0.28f, 0.36f, 0.27f) : Color(0.13f, 0.2f, 0.17f));
    renderer->Rect(b.x, b.y, 3, b.h, {0.8f, 0.67f, 0.4f});
    renderer->Text(b.x + 76, b.y + 13, label, {0.94f, 0.89f, 0.73f});
}

void Display()
{
    if (!renderer)
        return;
    if (screen == Screen::Main)
    {
        bookMenu.Draw(*renderer, width, height, mouseX, mouseY);
    }
    else if (chapter1 || chapter2)
    {
        if (chapter2)
            chapter2->Draw(*renderer, width, height);
        else
            chapter1->Draw(*renderer, width, height);
        if (screen == Screen::Inventory)
        {
            DrawInventoryPanel(*renderer,
                               width,
                               height,
                               chapter2 ? chapter2->Status() : chapter1->Status(),
                               mouseX,
                               mouseY,
                               inventoryView);
        }
        if (screen == Screen::Paused)
        {
            renderer->Rect(0,
                           0,
                           static_cast<float>(width),
                           static_cast<float>(height),
                           {0.01f, 0.025f, 0.02f, 0.72f});
            renderer->Rect(
                width * 0.5f - 220, height * 0.5f - 115, 440, 245, {0.055f, 0.09f, 0.075f, 0.98f});
            renderer->Text(
                width * 0.5f - 40, height * 0.5f - 85, L"일시정지", {0.95f, 0.85f, 0.6f});
            renderer->Text(
                width * 0.5f - 126, height * 0.5f - 42, L"ESC를 다시 누르면 계속합니다.");
            DrawButton(MainButton(), L"메인화면으로");
            renderer->Text(width * 0.5f - 163,
                           height * 0.5f + 92,
                           L"메인화면으로 가면 진행이 초기화됩니다.",
                           {0.65f, 0.73f, 0.65f});
        }
    }
    renderer->Flush();
    glutSwapBuffers();
}

void Resize(int w, int h)
{
    width = (std::max)(800, w);
    height = (std::max)(600, h);
    if (w < 800 || h < 600)
        glutReshapeWindow(width, height);
    if (renderer)
        renderer->Resize(width, height);
}

void KeyDown(unsigned char key, int, int)
{
    key = Normalize(key);
    if (keys[key])
        return;
    keys[key] = true;
    if (key == 'e' && (screen == Screen::Playing || screen == Screen::Inventory))
    {
        if (screen == Screen::Playing && chapter1)
        {
            if (!chapter1->CanOpenInventory())
            {
                return;
            }
            chapter1->OnInventoryOpened();
        }
        if (screen == Screen::Inventory && inventoryView.mapOpen)
        {
            inventoryView = {};
        }
        else
        {
            inventoryView = {};
            screen = screen == Screen::Inventory ? Screen::Playing : Screen::Inventory;
        }
        if (chapter1)
            chapter1->StopSound();
        ResetInput();
        keys['e'] = true;
        return;
    }
    if (key == 27)
    {
        if (screen == Screen::Inventory)
        {
            // Inventory and its map remain open on ESC.
            return;
        }
        if (screen == Screen::Playing || screen == Screen::Paused)
        {
            screen = screen == Screen::Playing ? Screen::Paused : Screen::Playing;
            if (chapter1 && screen == Screen::Paused)
                chapter1->StopSound();
            ResetInput();
            keys[27] = true; // Holding ESC must not toggle repeatedly.
        }
        return;
    }
    if (screen == Screen::Playing && chapter1 && key == 'f')
        chapter1->Interact();
    if (screen == Screen::Playing && chapter2)
    {
        if (key == 'f')
            chapter2->Interact();
        if (key == 'h')
            chapter2->Heal();
    }
}

void KeyUp(unsigned char key, int, int)
{
    keys[Normalize(key)] = false;
}

void Mouse(int button, int state, int x, int y)
{
    mouseX = x;
    mouseY = y;
    if (state != GLUT_DOWN)
        return;
    if (screen == Screen::Inventory)
    {
        if (button == GLUT_LEFT_BUTTON)
        {
            ClickInventoryPanel(width,
                                height,
                                x,
                                y,
                                chapter2 ? chapter2->Status() : chapter1->Status(),
                                inventoryView);
        }
        return;
    }
    if (screen == Screen::Playing && chapter2)
    {
        if (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON)
            chapter2->Click(button == GLUT_RIGHT_BUTTON, x, y);
        return;
    }
    if (button != GLUT_LEFT_BUTTON)
        return;
    if (screen == Screen::Main)
    {
        BookMenu::Action action = bookMenu.Click(x, y, width, height);
        if (action == BookMenu::Action::Chapter1 || action == BookMenu::Action::Chapter2)
        {
            chapter1.reset();
            chapter2.reset();
            if (action == BookMenu::Action::Chapter1)
            {
                chapter1.reset(new Chapter1());
            }
            else
            {
                chapter2.reset(new Chapter2());
            }
            screen = Screen::Playing;
            ResetInput();
        }
        else if (action == BookMenu::Action::Exit)
        {
            Close();
            glutLeaveMainLoop();
        }
        return; // Starting the game cannot also advance its first monologue page.
    }
    if (screen == Screen::Paused)
    {
        if (MainButton().Contains(x, y))
        {
            chapter1.reset();
            chapter2.reset();
            screen = Screen::Main;
            bookMenu.Reset();
            ResetInput();
        }
        return;
    }
    if (chapter1 && chapter1->DialogueActive())
    {
        chapter1->AdvanceDialogue();
        ResetInput();
    }
}

void MouseMove(int x, int y)
{
    mouseX = x;
    mouseY = y;
    glutPostRedisplay();
}

void Tick(int)
{
    if (!renderer)
        return;
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - previous).count();
    previous = now;
    if (!gameWindow || GetForegroundWindow() == gameWindow)
    {
        if (screen == Screen::Main)
        {
            bookMenu.Update(dt);
        }
        if (screen == Screen::Playing && chapter1)
            chapter1->Update(dt, keys);
        if (screen == Screen::Playing && chapter2)
            chapter2->Update(dt, keys);
        if (screen == Screen::Playing && chapter1 && chapter1->ReadyForChapter2())
        {
            unsigned mapPieces = chapter1->Status().mapPieces;
            chapter1.reset();
            chapter2.reset(new Chapter2());
            chapter2->CarryStoryMap(mapPieces);
            ResetInput();
        }
    }
    else
    {
        std::fill(std::begin(keys), std::end(keys), false);
        if (chapter1)
            chapter1->StopSound();
    }
    glutPostRedisplay();
    glutTimerFunc(16, Tick, 0);
}
} // namespace

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(width, height);
    glutCreateWindow("Fox And Rose");
    gameWindow = GetActiveWindow();
    if (gameWindow)
    {
        SetWindowTextW(gameWindow, L"Fox And Rose");
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glewExperimental = GL_TRUE;
    GLenum status = glewInit();
    if (status != GLEW_OK || !GLEW_VERSION_3_3)
    {
        MessageBoxW(nullptr,
                    L"OpenGL 3.3 초기화에 실패했습니다. 그래픽 드라이버를 확인하세요.",
                    L"렌더러 초기화 실패",
                    MB_OK | MB_ICONERROR);
        glutDestroyWindow(glutGetWindow());
        return 1;
    }
    // GLEW may leave GL_INVALID_ENUM after querying a core context.
    while (glGetError() != GL_NO_ERROR)
    {
    }
    renderer.reset(new Renderer(width, height));
    if (!renderer->IsInitialized())
    {
        MessageBoxW(
            nullptr,
            L"셰이더를 초기화하지 못했습니다. 실행 파일 옆 Shaders 폴더와 콘솔 로그를 확인하세요.",
            L"렌더러 초기화 실패",
            MB_OK | MB_ICONERROR);
        renderer.reset();
        glutDestroyWindow(glutGetWindow());
        return 1;
    }
    screen = Screen::Main;
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutMouseFunc(Mouse);
    glutPassiveMotionFunc(MouseMove);
    glutMotionFunc(MouseMove);
    glutCloseFunc(Close);
    glutIgnoreKeyRepeat(1);
    previous = std::chrono::steady_clock::now();
    glutTimerFunc(16, Tick, 0);
    glutMainLoop();
    return 0;
}
