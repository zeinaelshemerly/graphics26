#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include "GameUI.h"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>

void expect(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
void press(GameUI& ui, int key) { ui.Key(key, GLFW_PRESS); }
int main() {
    GameUI ui;
    ui.Init();
    expect(ui.GetScreen() == Screen::Main, "Launch must show main menu");
    press(ui, GLFW_KEY_ENTER);
    expect(ui.GetScreen() == Screen::Intro, "Start must enter intro");
    ui.Click(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    expect(ui.GetScreen() == Screen::Intro, "Intro must not accept menu clicks");
    for (int i=0; i<149; ++i) ui.Update(.1f);
    expect(ui.GetScreen() == Screen::Intro, "Intro must last 15 seconds");
    for (int i=0; i<2; ++i) ui.Update(.1f);
    expect(ui.GetScreen() == Screen::Playing, "Intro must transition to gameplay");
    press(ui, GLFW_KEY_ESCAPE);
    expect(ui.GetScreen() == Screen::Paused && !ui.ShouldQuit(), "Escape must pause without quitting");
    for (int i=0; i<3000; ++i) ui.Update(.1f);
    expect(ui.GetScreen() == Screen::Paused, "Paused screen must persist");
    // A stationary pointer must not override keyboard focus every frame.
    ui.Mouse(500,350);
    press(ui, GLFW_KEY_DOWN);
    press(ui, GLFW_KEY_DOWN);
    ui.Mouse(500,350);
    press(ui, GLFW_KEY_ENTER);
    expect(ui.GetScreen() == Screen::Main, "Keyboard navigation must work with a stationary pointer");
    press(ui, GLFW_KEY_ENTER);
    press(ui, GLFW_KEY_SPACE);
    expect(ui.GetScreen() == Screen::Playing, "Space must skip intro");
    ui.FocusLost();
    expect(ui.GetScreen() == Screen::Paused, "Focus loss must pause gameplay");
    press(ui, GLFW_KEY_ESCAPE);
    expect(ui.GetScreen() == Screen::Playing, "Escape must resume");
    press(ui, GLFW_KEY_ESCAPE);
    ui.Mouse(500,430);
    ui.Click(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    expect(ui.GetScreen() == Screen::Playing, "Pause restart button must restart gameplay");
    press(ui, GLFW_KEY_ESCAPE);
    ui.Mouse(500,505);
    ui.Click(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    expect(ui.GetScreen() == Screen::Main, "Pause main-menu button must return home");
    ui.Mouse(500,430);
    ui.Click(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    expect(ui.ShouldQuit(), "Exit button must request application close");
    GameUI defeat;
    defeat.Init(); press(defeat, GLFW_KEY_ENTER); press(defeat, GLFW_KEY_SPACE);
    for (int i=0; i<10000 && defeat.GetScreen() == Screen::Playing; ++i) defeat.Update(.1f);
    expect(defeat.GetScreen() == Screen::Lost, "Enemies reaching fridge must show lose menu");
    defeat.Mouse(500,350); defeat.Click(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    expect(defeat.GetScreen() == Screen::Playing, "Lose menu must allow restart");
    std::cout << "Menu and intro flow checks passed\n";
}
