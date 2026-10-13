#pragma once

#include "Renderer2D.h"
#include <vector>

class Playfield {
public:
    static constexpr int NUM_LANES = 5;
    static constexpr int NUM_COLS = 8;

    Playfield();

    void Init();
    void Update(float deltaTime);
    void UpdateMouse(float virtualMouseX, float virtualMouseY);
    void OnMouseClick(int button, int action);

    void Render(Renderer2D& renderer);

    // Grid queries
    int GetHoveredLane() const { return m_HoveredLane; }
    bool IsLaneSelected(int lane) const {
        if (lane >= 0 && lane < NUM_LANES) return m_SelectedLanes[lane];
        return false;
    }

    // Geometry helpers for future character/enemy placement
    float GetLaneY(int lane) const;
    float GetLaneHeight() const { return m_LaneHeight; }
    float GetGridStartX() const { return m_GridStartX; }
    float GetGridEndX() const { return m_GridStartX + m_GridWidth; }
    float GetCellWidth() const { return m_CellWidth; }

private:
    void RenderEnvironment(Renderer2D& renderer);
    void RenderHomeFridge(Renderer2D& renderer);
    void RenderCountertopGrid(Renderer2D& renderer);
    void RenderLaneHighlights(Renderer2D& renderer);
    void RenderTopAndBottomUI(Renderer2D& renderer);

    // Layout coordinates (in virtual 1280x720 space)
    float m_GridStartX = 250.0f;
    float m_GridStartY = 95.0f;
    float m_GridWidth  = 1000.0f;
    float m_GridHeight = 550.0f;
    float m_LaneHeight = 110.0f;
    float m_CellWidth  = 125.0f;

    // Home Base (Fridge) coordinates
    float m_FridgeX = 20.0f;
    float m_FridgeY = 85.0f;
    float m_FridgeW = 210.0f;
    float m_FridgeH = 570.0f;

    // Interaction state
    int m_HoveredLane = -1;
    bool m_SelectedLanes[NUM_LANES] = { false };
    float m_MouseX = -1.0f;
    float m_MouseY = -1.0f;
    float m_AnimTimer = 0.0f;
};
