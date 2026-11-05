# Kitchen Defense

A C++17 / OpenGL 3.3 kitchen defense game using GLFW, GLAD, GLM and CMake.

## Person 3 branch

`person3-ui-cutscene` starts from `origin/integration-gamelogic-review`.
`GameUI.h/.cpp` own the main menu, 15-second animated introduction, pause menu,
and win/lose menus. Combat remains in `GameLogic`; environment and HUD rendering
remain in `Playfield`. Gameplay updates and input are gated by the current UI screen.

- Main menu: Start Game plays the intro; Exit closes the application.
- Intro: Space or Enter skips to gameplay; Escape returns to the main menu.
- Gameplay: click lanes to select them, then click an unlocked card to deploy.
  W / Up jumps; S / Down ducks; R restarts; Escape pauses.
- Pause: Continue, Restart, or Main Menu. Escape resumes. Losing window focus pauses.
- Win/lose: Restart or Main Menu. Restart begins fresh gameplay without replaying the intro.
- Menus support mouse clicks or Up/Down and Enter/Space.
- Locked cards ignore interaction; unlocked cards have mouse-hover feedback.

## Build and run

With CMake, Ninja and a C++17 compiler available in the terminal:

```powershell
cmake -S . -B build/person3 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/person3 -j 4
.\build\person3\OpenGLStarter.exe
```

Use a different build directory when changing compiler or CMake generator.

## Verification

```powershell
ctest --test-dir build/person3 --output-on-failure
```

The headless flow test covers launch, intro timing/skip, pause/resume, focus loss,
restart, return to main menu, mouse and keyboard navigation, exit, and the lose flow.
It does not check rendered appearance or the victory path. To verify visuals, run the
application and check the intro, button feedback, resized-window clicks, pause freeze,
and both end menus. Configure with `-DBUILD_TESTING=OFF` to omit the test executable.
