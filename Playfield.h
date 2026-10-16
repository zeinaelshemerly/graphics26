#pragma once

// ============================================================================
// Playfield.h — Person 2: Grid, Lanes, Home Base & Environment
// ============================================================================

#include "Renderer2D.h"
#include "GameWorld.h"
#include <vector>

class Playfield {
public:
    static constexpr int NUM_LANES = 5;
    static constexpr int NUM_COLS  = 8;

    Playfield();

    void Init();
    void Update(float deltaTime);
    void UpdateMouse(float virtualMouseX, float virtualMouseY);
    void OnMouseClick(int button, int action);
    void OnKey(int key, int action);

    void Render(Renderer2D& renderer);

    // Grid geometry queries
    int   GetHoveredLane() const { return m_HoveredLane; }
    bool  IsLaneSelected(int lane) const {
        return (lane >= 0 && lane < NUM_LANES) ? m_SelectedLanes[lane] : false;
    }
    float GetLaneY(int lane)    const;
    float GetLaneHeight()       const { return m_LaneHeight; }
    float GetGridStartX()       const { return m_GridStartX; }
    float GetGridEndX()         const { return m_GridStartX + m_GridWidth; }
    float GetCellWidth()        const { return m_CellWidth; }

    GameWorld& GetGameWorld() { return m_GameWorld; }

private:
    // --- Person 2: Draw the grid, lanes, home base, background ---
    void RenderEnvironment(Renderer2D& renderer);
    void RenderHomeFridge(Renderer2D& renderer);
    void RenderCountertopGrid(Renderer2D& renderer);
    void RenderLaneHighlights(Renderer2D& renderer);
    void RenderTopAndBottomUI(Renderer2D& renderer);

    // Layout (virtual 1280×720 space)
    float m_GridStartX = 245.0f;
    float m_GridStartY =  95.0f;
    float m_GridWidth  = 1010.0f;
    float m_GridHeight =  550.0f;
    float m_LaneHeight =  110.0f;
    float m_CellWidth  =  126.25f;

    // Home Base (Fridge)
    float m_FridgeX =  20.0f;
    float m_FridgeY =  85.0f;
    float m_FridgeW = 210.0f;
    float m_FridgeH = 570.0f;

    // Interaction state
    int   m_HoveredLane = -1;
    bool  m_SelectedLanes[NUM_LANES] = { false };
    float m_MouseX   = -1.0f;
    float m_MouseY   = -1.0f;
    float m_AnimTimer = 0.0f;

    // Person 2 game-world engine
    GameWorld m_GameWorld;
};
