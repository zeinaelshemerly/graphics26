#pragma once
#include "Playfield.h"

// Presentation state is separate from the combat engine's win/lose state.
enum class Screen { Main, Intro, Playing, Paused, Won, Lost };

class GameUI {
public:
    void Init();
    void Update(float dt);
    void Render(Renderer2D& renderer);
    void Mouse(float x, float y);
    void Click(int button, int action);
    void Key(int key, int action);
    void FocusLost();
    Screen GetScreen() const { return m_Screen; }
    bool ShouldQuit() const { return m_Quit; }
private:
    void Enter(Screen screen);
    void Restart();
    void Activate(int index);
    int ButtonCount() const;
    int HoveredButton() const;
    void CenterText(Renderer2D& r, const std::string& text, float y, float scale, const glm::vec4& color);
    void RenderIntro(Renderer2D& r);
    void RenderControls(Renderer2D& r);
    void RenderResultScene(Renderer2D& r, bool won);
    Playfield m_Field;
    Screen m_Screen = Screen::Main;
    float m_MouseX = -1, m_MouseY = -1;
    float m_Time = 0, m_Fade = 0;
    int m_Focused = 0;
    bool m_Quit = false;
};
