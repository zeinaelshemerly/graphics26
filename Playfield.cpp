// ============================================================================
// Playfield.cpp — Person 2: Draw the grid, lanes, home base & environment
// ============================================================================

#include "Playfield.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

Playfield::Playfield()
{
    m_GridStartX = 245.0f;
    m_GridStartY =  95.0f;
    m_GridWidth  = 1010.0f;
    m_GridHeight =  550.0f;
    m_LaneHeight = m_GridHeight / (float)NUM_LANES;
    m_CellWidth  = m_GridWidth  / (float)NUM_COLS;
}

void Playfield::Init()
{
    for (int i = 0; i < NUM_LANES; ++i)
        m_SelectedLanes[i] = false;

    m_GameWorld.Init(m_GridStartX, m_GridStartY,
                     m_GridWidth,  m_GridHeight, m_LaneHeight);
}

void Playfield::Update(float deltaTime)
{
    m_AnimTimer += deltaTime;
    m_GameWorld.Update(deltaTime);
}

float Playfield::GetLaneY(int lane) const
{
    if (lane < 0)        lane = 0;
    if (lane >= NUM_LANES) lane = NUM_LANES - 1;
    return m_GridStartY + (float)lane * m_LaneHeight;
}

// ---------------------------------------------------------------------------
// Mouse / Keyboard input
// ---------------------------------------------------------------------------

void Playfield::UpdateMouse(float virtualMouseX, float virtualMouseY)
{
    m_MouseX = virtualMouseX;
    m_MouseY = virtualMouseY;

    if (m_MouseX >= m_GridStartX && m_MouseX <= (m_GridStartX + m_GridWidth) &&
        m_MouseY >= m_GridStartY && m_MouseY <= (m_GridStartY + m_GridHeight))
    {
        m_HoveredLane = (int)((m_MouseY - m_GridStartY) / m_LaneHeight);
        if (m_HoveredLane < 0)        m_HoveredLane = 0;
        if (m_HoveredLane >= NUM_LANES) m_HoveredLane = NUM_LANES - 1;
    } else {
        m_HoveredLane = -1;
    }
}

void Playfield::OnKey(int key, int action)
{
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_R) {
        // Reset entire game world
        m_GameWorld.Reset();
        for (int i = 0; i < NUM_LANES; ++i) m_SelectedLanes[i] = false;
    }
}

void Playfield::OnMouseClick(int button, int action)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    // 1. Check card clicks in the top header
    const float cardStartX = 400.0f;
    const float cardW      =  90.0f;
    const float cardH      =  65.0f;
    const float cardGap    =  16.0f;
    const float cardY      =   6.0f;

    P2PlayerType cardTypes[3] = { P2_PLAYER_MELEE, P2_PLAYER_RANGED, P2_PLAYER_DEFENSE };

    for (int i = 0; i < 3; ++i) {
        float cx = cardStartX + (float)i * (cardW + cardGap);
        if (m_MouseX >= cx && m_MouseX <= cx + cardW &&
            m_MouseY >= cardY && m_MouseY <= cardY + cardH)
        {
            if (m_GameWorld.IsCharacterUnlocked(cardTypes[i]))
                m_GameWorld.SelectCard(cardTypes[i]);
            return;
        }
    }

    // 2. Click collectibles (try to collect dropped coins)
    m_GameWorld.TryCollectAt(m_MouseX, m_MouseY);

    // 3. Click lane — deploy selected card OR toggle lane selection
    if (m_HoveredLane >= 0 && m_HoveredLane < NUM_LANES) {
        if (m_GameWorld.GetSelectedCard() != P2_PLAYER_NONE) {
            m_GameWorld.PurchaseAndDeploy(m_GameWorld.GetSelectedCard(), m_HoveredLane);
        } else {
            m_SelectedLanes[m_HoveredLane] = !m_SelectedLanes[m_HoveredLane];
        }
    }
}

// ---------------------------------------------------------------------------
// Top-level render call
// ---------------------------------------------------------------------------

void Playfield::Render(Renderer2D& renderer)
{
    // Person 2: background & environmental objects
    RenderEnvironment(renderer);

    // Person 2: home base (fridge)
    RenderHomeFridge(renderer);

    // Person 2: countertop grid & lanes
    RenderCountertopGrid(renderer);

    // Person 2: interactive lane highlights
    RenderLaneHighlights(renderer);

    // Person 2: enemies, players (simple shapes), collectibles, banners
    m_GameWorld.RenderEntities(renderer);

    // Person 2: HUD top bar + bottom level-progress bar
    RenderTopAndBottomUI(renderer);
}

// ---------------------------------------------------------------------------
// Person 2 — Draw the background and environmental objects
// ---------------------------------------------------------------------------

void Playfield::RenderEnvironment(Renderer2D& renderer)
{
    // 1. Kitchen backsplash wall with ceramic tile grid
    renderer.DrawQuad(0.0f, 0.0f, 1280.0f, 720.0f,
                      glm::vec4(0.88f, 0.86f, 0.83f, 1.0f));

    const float tileSize = 45.0f;
    glm::vec4 grout(0.80f, 0.77f, 0.74f, 1.0f);
    for (float x = 0.0f; x < 1280.0f; x += tileSize)
        renderer.DrawLine(glm::vec2(x, 0.0f), glm::vec2(x, 720.0f), 1.0f, grout);
    for (float y = 0.0f; y < 720.0f; y += tileSize)
        renderer.DrawLine(glm::vec2(0.0f, y), glm::vec2(1280.0f, y), 1.0f, grout);

    // 2. Under-counter cabinetry base
    float cabY = m_GridStartY + m_GridHeight + 18.0f;
    float cabH = 720.0f - cabY;
    renderer.DrawQuad(m_GridStartX - 15.0f, cabY,
                      1280.0f - (m_GridStartX - 15.0f), cabH,
                      glm::vec4(0.76f, 0.74f, 0.71f, 1.0f));
    renderer.DrawQuad(m_GridStartX - 15.0f, cabY,
                      1280.0f - (m_GridStartX - 15.0f), 6.0f,
                      glm::vec4(0.55f, 0.53f, 0.50f, 1.0f)); // shadow strip

    // Drawer seams & brushed-steel handles
    for (float cx = m_GridStartX + 120.0f; cx < 1280.0f; cx += 260.0f) {
        renderer.DrawQuad(cx, cabY + 6.0f, 2.0f, cabH - 6.0f,
                          glm::vec4(0.58f, 0.56f, 0.53f, 1.0f));
        renderer.DrawQuad(cx + 80.0f, cabY + 16.0f, 90.0f, 8.0f,
                          glm::vec4(0.40f, 0.40f, 0.42f, 1.0f));
        renderer.DrawQuad(cx + 82.0f, cabY + 17.0f, 86.0f, 3.0f,
                          glm::vec4(0.85f, 0.85f, 0.88f, 1.0f));
    }

    // Dish towel hanging on the first handle
    float towelX = m_GridStartX + 205.0f;
    renderer.DrawQuad(towelX, cabY + 22.0f, 40.0f, 38.0f,
                      glm::vec4(0.92f, 0.40f, 0.40f, 1.0f));
    renderer.DrawRectOutline(towelX, cabY + 22.0f, 40.0f, 38.0f,
                             1.5f, glm::vec4(0.35f, 0.15f, 0.15f, 1.0f));
    renderer.DrawLine(glm::vec2(towelX + 10.0f, cabY + 22.0f),
                      glm::vec2(towelX + 10.0f, cabY + 60.0f),
                      1.5f, glm::vec4(0.98f, 0.98f, 0.98f, 1.0f));
    renderer.DrawLine(glm::vec2(towelX + 25.0f, cabY + 22.0f),
                      glm::vec2(towelX + 25.0f, cabY + 60.0f),
                      1.5f, glm::vec4(0.98f, 0.98f, 0.98f, 1.0f));

    // Floor toe-kick recess
    renderer.DrawQuad(0.0f, 706.0f, 1280.0f, 14.0f,
                      glm::vec4(0.32f, 0.31f, 0.30f, 1.0f));

    // 3. Wall-mounted condiment shelf (olive oil, vinegar, pepper mill)
    float shelfX = 720.0f, shelfY = 82.0f;
    renderer.DrawQuad(shelfX, shelfY, 150.0f, 6.0f,
                      glm::vec4(0.45f, 0.28f, 0.15f, 1.0f)); // wood plank
    // Olive oil bottle
    renderer.DrawQuad(shelfX + 15.0f, shelfY - 32.0f, 16.0f, 32.0f,
                      glm::vec4(0.40f, 0.65f, 0.30f, 0.9f));
    renderer.DrawQuad(shelfX + 18.0f, shelfY - 40.0f, 10.0f, 8.0f,
                      glm::vec4(0.55f, 0.40f, 0.25f, 1.0f));
    // Vinegar bottle
    renderer.DrawQuad(shelfX + 45.0f, shelfY - 28.0f, 15.0f, 28.0f,
                      glm::vec4(0.68f, 0.25f, 0.25f, 0.9f));
    renderer.DrawQuad(shelfX + 48.0f, shelfY - 35.0f, 9.0f, 7.0f,
                      glm::vec4(0.55f, 0.40f, 0.25f, 1.0f));
    // Pepper mill
    renderer.DrawQuad(shelfX + 75.0f, shelfY - 38.0f, 18.0f, 38.0f,
                      glm::vec4(0.35f, 0.20f, 0.10f, 1.0f));
    renderer.DrawCircle(glm::vec2(shelfX + 84.0f, shelfY - 42.0f), 5.0f,
                        glm::vec4(0.85f, 0.70f, 0.20f, 1.0f));

    // 4. Stainless-steel hanging utensil rail
    float railX = 920.0f, railY = 80.0f;
    renderer.DrawQuad(railX, railY, 260.0f, 4.0f,
                      glm::vec4(0.50f, 0.52f, 0.55f, 1.0f));
    renderer.DrawCircle(glm::vec2(railX + 6.0f,   railY + 2.0f), 4.0f,
                        glm::vec4(0.30f, 0.32f, 0.35f, 1.0f));
    renderer.DrawCircle(glm::vec2(railX + 254.0f, railY + 2.0f), 4.0f,
                        glm::vec4(0.30f, 0.32f, 0.35f, 1.0f));
    // Soup ladle
    renderer.DrawLine(glm::vec2(railX + 40.0f, railY + 4.0f),
                      glm::vec2(railX + 40.0f, railY + 32.0f),
                      2.0f, glm::vec4(0.65f, 0.68f, 0.70f, 1.0f));
    renderer.DrawCircle(glm::vec2(railX + 40.0f, railY + 36.0f), 7.0f,
                        glm::vec4(0.65f, 0.68f, 0.70f, 1.0f));
    // Slotted spatula
    renderer.DrawLine(glm::vec2(railX + 90.0f, railY + 4.0f),
                      glm::vec2(railX + 90.0f, railY + 28.0f),
                      2.0f, glm::vec4(0.65f, 0.68f, 0.70f, 1.0f));
    renderer.DrawQuad(railX + 84.0f, railY + 28.0f, 12.0f, 16.0f,
                      glm::vec4(0.65f, 0.68f, 0.70f, 1.0f));
    // Chef's knife
    renderer.DrawLine(glm::vec2(railX + 145.0f, railY + 4.0f),
                      glm::vec2(railX + 145.0f, railY + 16.0f),
                      3.0f, glm::vec4(0.35f, 0.20f, 0.10f, 1.0f));
    renderer.DrawQuadVertices(
        glm::vec2(railX + 143.0f, railY + 16.0f),
        glm::vec2(railX + 148.0f, railY + 16.0f),
        glm::vec2(railX + 148.0f, railY + 44.0f),
        glm::vec2(railX + 140.0f, railY + 38.0f),
        glm::vec4(0.85f, 0.88f, 0.90f, 1.0f)
    );
    // Wire whisk
    renderer.DrawLine(glm::vec2(railX + 205.0f, railY + 4.0f),
                      glm::vec2(railX + 205.0f, railY + 20.0f),
                      2.5f, glm::vec4(0.50f, 0.52f, 0.55f, 1.0f));
    renderer.DrawCircle(glm::vec2(railX + 205.0f, railY + 30.0f), 9.0f,
                        glm::vec4(0.65f, 0.68f, 0.70f, 0.6f));
}

// ---------------------------------------------------------------------------
// Person 2 — Draw the home base
// ---------------------------------------------------------------------------

void Playfield::RenderHomeFridge(Renderer2D& renderer)
{
    glm::vec4 fridgeBody (0.48f, 0.50f, 0.52f, 1.0f);
    glm::vec4 fridgeTop  (0.60f, 0.62f, 0.65f, 1.0f);
    glm::vec4 doorFill   (0.53f, 0.55f, 0.58f, 1.0f);
    glm::vec4 outline    (0.12f, 0.12f, 0.14f, 1.0f);
    glm::vec4 textColor  (0.12f, 0.12f, 0.14f, 1.0f);

    // 3-D perspective top roof (angled trapezoid)
    renderer.DrawQuadVertices(
        glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
        glm::vec2(m_FridgeX + m_FridgeW, m_FridgeY - 8.0f),
        glm::vec2(m_FridgeX + m_FridgeW, m_FridgeY),
        glm::vec2(m_FridgeX,             m_FridgeY),
        fridgeTop
    );
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
                      glm::vec2(m_FridgeX + m_FridgeW, m_FridgeY - 8.0f), 2.5f, outline);
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
                      glm::vec2(m_FridgeX, m_FridgeY), 2.5f, outline);

    // Left perspective side wall
    renderer.DrawQuadVertices(
        glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
        glm::vec2(m_FridgeX,         m_FridgeY),
        glm::vec2(m_FridgeX,         m_FridgeY + m_FridgeH),
        glm::vec2(m_FridgeX - 12.0f, m_FridgeY + m_FridgeH - 10.0f),
        fridgeBody
    );
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
                      glm::vec2(m_FridgeX - 12.0f, m_FridgeY + m_FridgeH - 10.0f), 2.5f, outline);
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY + m_FridgeH - 10.0f),
                      glm::vec2(m_FridgeX,          m_FridgeY + m_FridgeH), 2.5f, outline);

    // Main body
    renderer.DrawQuad(m_FridgeX, m_FridgeY, m_FridgeW, m_FridgeH, fridgeBody);
    renderer.DrawRectOutline(m_FridgeX, m_FridgeY, m_FridgeW, m_FridgeH, 3.0f, outline);

    // Single unified door
    const float dm = 10.0f;
    float doorW = m_FridgeW - dm * 2.0f;
    float doorH = m_FridgeH - dm * 2.0f;
    float doorX = m_FridgeX + dm;
    float doorY = m_FridgeY + dm;
    renderer.DrawQuad(doorX, doorY, doorW, doorH, doorFill);
    renderer.DrawRectOutline(doorX, doorY, doorW, doorH, 3.0f, outline);

    // "HOME FRIDGE" label centred on door
    float tw1 = renderer.GetTextWidth("HOME",   2.6f);
    float tw2 = renderer.GetTextWidth("FRIDGE", 2.6f);
    renderer.DrawText("HOME",   doorX + (doorW - tw1) * 0.5f, doorY + doorH * 0.5f - 24.0f,
                      2.6f, textColor, 2.4f);
    renderer.DrawText("FRIDGE", doorX + (doorW - tw2) * 0.5f, doorY + doorH * 0.5f + 8.0f,
                      2.6f, textColor, 2.4f);
}

// ---------------------------------------------------------------------------
// Person 2 — Draw the grid and lanes
// ---------------------------------------------------------------------------

void Playfield::RenderCountertopGrid(Renderer2D& renderer)
{
    float tableX = m_GridStartX, tableY = m_GridStartY;
    float tableW = m_GridWidth,  tableH = m_GridHeight;

    // Drop shadow
    renderer.DrawQuad(tableX + 4.0f, tableY + 6.0f, tableW, tableH + 18.0f,
                      glm::vec4(0.0f, 0.0f, 0.0f, 0.20f));

    // Left bevel edge
    renderer.DrawQuadVertices(
        glm::vec2(tableX - 16.0f, tableY + 8.0f),
        glm::vec2(tableX,         tableY),
        glm::vec2(tableX,         tableY + tableH + 18.0f),
        glm::vec2(tableX - 16.0f, tableY + tableH + 12.0f),
        glm::vec4(0.70f, 0.54f, 0.38f, 1.0f)
    );
    renderer.DrawLine(glm::vec2(tableX - 16.0f, tableY + 8.0f),
                      glm::vec2(tableX - 16.0f, tableY + tableH + 12.0f),
                      3.0f, glm::vec4(0.32f, 0.20f, 0.12f, 1.0f));

    // Front apron rim
    renderer.DrawQuad(tableX, tableY + tableH, tableW, 18.0f,
                      glm::vec4(0.82f, 0.68f, 0.50f, 1.0f));
    renderer.DrawQuad(tableX, tableY + tableH + 15.0f, tableW, 3.0f,
                      glm::vec4(0.45f, 0.30f, 0.18f, 1.0f));

    // Cutting-board surface (warm cream wood)
    renderer.DrawQuad(tableX, tableY, tableW, tableH,
                      glm::vec4(0.96f, 0.93f, 0.86f, 1.0f));
    renderer.DrawRectOutline(tableX, tableY, tableW, tableH,
                             4.0f, glm::vec4(0.35f, 0.22f, 0.14f, 1.0f));

    // Wood-grain lines
    glm::vec4 grain(0.92f, 0.88f, 0.80f, 0.6f);
    for (float gy = tableY + 12.0f; gy < tableY + tableH; gy += 24.0f)
        renderer.DrawLine(glm::vec2(tableX + 4.0f, gy),
                          glm::vec2(tableX + tableW - 4.0f, gy), 1.0f, grain);

    // 5 horizontal lane dividers
    glm::vec4 laneColor(0.72f, 0.66f, 0.58f, 1.0f);
    for (int i = 1; i < NUM_LANES; ++i) {
        float ly = tableY + (float)i * m_LaneHeight;
        renderer.DrawLine(glm::vec2(tableX, ly), glm::vec2(tableX + tableW, ly),
                          2.5f, laneColor);
    }

    // 8 vertical column dividers
    for (int j = 1; j < NUM_COLS; ++j) {
        float cx = tableX + (float)j * m_CellWidth;
        renderer.DrawLine(glm::vec2(cx, tableY), glm::vec2(cx, tableY + tableH),
                          1.5f, laneColor);
    }

    // Safe-zone marker (left edge — player spawn side)
    renderer.DrawLine(glm::vec2(tableX + 2.0f, tableY),
                      glm::vec2(tableX + 2.0f, tableY + tableH),
                      4.0f, glm::vec4(0.40f, 0.65f, 0.40f, 0.8f));

    // Enemy spawn marker (right edge)
    renderer.DrawLine(glm::vec2(tableX + tableW - 3.0f, tableY),
                      glm::vec2(tableX + tableW - 3.0f, tableY + tableH),
                      4.0f, glm::vec4(0.85f, 0.35f, 0.35f, 0.8f));
}

// ---------------------------------------------------------------------------
// Lane hover / selection highlights
// ---------------------------------------------------------------------------

void Playfield::RenderLaneHighlights(Renderer2D& renderer)
{
    float pulse = 0.5f + 0.5f * std::sin(m_AnimTimer * 5.0f);

    for (int i = 0; i < NUM_LANES; ++i) {
        float ly = GetLaneY(i);
        bool  selected = m_SelectedLanes[i];
        bool  hovered  = (m_HoveredLane == i);

        if (selected) {
            float p = 0.5f + 0.5f * std::sin(m_AnimTimer * 5.0f);
            renderer.DrawQuad(m_GridStartX + 2.0f, ly + 2.0f,
                              m_GridWidth - 4.0f,  m_LaneHeight - 4.0f,
                              glm::vec4(0.20f, 0.85f, 0.35f, 0.28f + p * 0.08f));
            renderer.DrawRectOutline(m_GridStartX + 2.0f, ly + 2.0f,
                                     m_GridWidth - 4.0f,  m_LaneHeight - 4.0f,
                                     3.0f, glm::vec4(0.25f, 0.95f, 0.40f, 0.9f));
            renderer.DrawQuad(m_GridStartX + 6.0f, ly + 6.0f, 75.0f, 22.0f,
                              glm::vec4(0.15f, 0.70f, 0.30f, 0.95f));
            std::string tag = "LANE " + std::to_string(i + 1);
            renderer.DrawText(tag, m_GridStartX + 12.0f, ly + 10.0f,
                              1.4f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f);

        } else if (hovered) {
            renderer.DrawQuad(m_GridStartX + 2.0f, ly + 2.0f,
                              m_GridWidth - 4.0f,  m_LaneHeight - 4.0f,
                              glm::vec4(1.0f, 0.92f, 0.55f, 0.25f));
            renderer.DrawLine(glm::vec2(m_GridStartX, ly + 1.0f),
                              glm::vec2(m_GridStartX + m_GridWidth, ly + 1.0f),
                              2.0f, glm::vec4(1.0f, 0.85f, 0.30f, 0.8f));
            renderer.DrawLine(glm::vec2(m_GridStartX, ly + m_LaneHeight - 1.0f),
                              glm::vec2(m_GridStartX + m_GridWidth, ly + m_LaneHeight - 1.0f),
                              2.0f, glm::vec4(1.0f, 0.85f, 0.30f, 0.8f));
            renderer.DrawQuad(m_GridStartX + 6.0f, ly + 6.0f, 65.0f, 20.0f,
                              glm::vec4(0.85f, 0.70f, 0.20f, 0.85f));
            std::string tag = "LANE " + std::to_string(i + 1);
            renderer.DrawText(tag, m_GridStartX + 12.0f, ly + 10.0f,
                              1.3f, glm::vec4(0.2f, 0.15f, 0.05f, 1.0f), 1.4f);
        }
    }
}

// ---------------------------------------------------------------------------
// Person 2 — HUD: score, wave, spice coins, card panel, level-progress bar
// ---------------------------------------------------------------------------

void Playfield::RenderTopAndBottomUI(Renderer2D& renderer)
{
    // --- Top header bar ---
    renderer.DrawQuad(0.0f, 0.0f, 1280.0f, 75.0f,
                      glm::vec4(0.18f, 0.18f, 0.18f, 1.0f));
    renderer.DrawLine(glm::vec2(0.0f, 75.0f), glm::vec2(1280.0f, 75.0f),
                      3.0f, glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));

    glm::vec4 statColor(0.38f, 0.88f, 0.48f, 1.0f);

    // Stats (top-left, arcade-green terminal text)
    renderer.DrawText("SCORE: " + std::to_string(m_GameWorld.GetScore()),
                      20.0f, 12.0f, 2.0f, statColor, 2.0f);
    renderer.DrawText("WAVE " + std::to_string(m_GameWorld.GetCurrentWave()) + "/3",
                      20.0f, 32.0f, 2.0f, statColor, 2.0f);
    renderer.DrawText("SPICE: " + std::to_string(m_GameWorld.GetSpiceCoins()),
                      20.0f, 52.0f, 2.0f, statColor, 2.0f);

    // Mini spice-coin icon
    float spiceIconX = 20.0f
        + renderer.GetTextWidth("SPICE: " + std::to_string(m_GameWorld.GetSpiceCoins()), 2.0f)
        + 12.0f;
    renderer.DrawCircle(glm::vec2(spiceIconX, 58.0f), 7.0f,
                        glm::vec4(0.95f, 0.80f, 0.20f, 1.0f), 12);
    renderer.DrawCircle(glm::vec2(spiceIconX, 58.0f), 4.0f,
                        glm::vec4(1.0f,  0.95f, 0.60f, 1.0f), 10);

    // "CARDS" label
    renderer.DrawText("CARDS", 300.0f, 28.0f, 2.2f, statColor, 2.0f);

    // --- 3 Character cards ---
    struct CardDef { P2PlayerType type; const char* name; int cost; };
    CardDef cards[3] = {
        { P2_PLAYER_MELEE,   "CHEF",  10 },
        { P2_PLAYER_RANGED,  "SALT",  20 },
        { P2_PLAYER_DEFENSE, "BREAD", 30 }
    };

    const float cardStartX = 400.0f;
    const float cardW      =  90.0f;
    const float cardH      =  65.0f;
    const float cardGap    =  16.0f;
    const float cardY      =   6.0f;

    for (int i = 0; i < 3; ++i) {
        float cx = cardStartX + (float)i * (cardW + cardGap);
        bool unlocked  = m_GameWorld.IsCharacterUnlocked(cards[i].type);
        bool isSelected = (m_GameWorld.GetSelectedCard() == cards[i].type);

        if (unlocked) {
            // Vibrant unlocked card
            renderer.DrawQuad(cx, cardY, cardW, cardH,
                              glm::vec4(0.32f, 0.34f, 0.38f, 1.0f));
            if (isSelected) {
                float p2 = 0.5f + 0.5f * std::sin(m_AnimTimer * 6.0f);
                renderer.DrawRectOutline(cx - 2.0f, cardY - 2.0f,
                                         cardW + 4.0f, cardH + 4.0f,
                                         3.5f, glm::vec4(0.35f, 0.95f, 0.40f, 0.8f + p2 * 0.2f));
            } else {
                renderer.DrawRectOutline(cx, cardY, cardW, cardH, 2.0f,
                                         glm::vec4(0.18f, 0.20f, 0.22f, 1.0f));
            }
            float tw = renderer.GetTextWidth(cards[i].name, 1.3f);
            renderer.DrawText(cards[i].name, cx + (cardW - tw) * 0.5f, cardY + 6.0f,
                              1.3f, glm::vec4(0.95f,0.95f,0.98f,1.0f), 1.3f);

            // Card icon
            if (cards[i].type == P2_PLAYER_MELEE) {
                renderer.DrawQuad(cx + cardW*0.5f - 12.0f, cardY + 28.0f, 10.0f, 3.0f,
                                  glm::vec4(0.20f,0.20f,0.22f,1.0f));
                renderer.DrawQuad(cx + cardW*0.5f - 2.0f,  cardY + 22.0f, 10.0f, 15.0f,
                                  glm::vec4(0.85f,0.88f,0.90f,1.0f));
            } else if (cards[i].type == P2_PLAYER_RANGED) {
                renderer.DrawQuad(cx + cardW*0.5f - 8.0f, cardY + 23.0f, 12.0f, 12.0f,
                                  glm::vec4(0.78f,0.80f,0.84f,1.0f));
                renderer.DrawQuad(cx + cardW*0.5f + 4.0f, cardY + 24.5f, 4.0f, 9.0f,
                                  glm::vec4(0.40f,0.42f,0.45f,1.0f));
            } else {
                renderer.DrawQuad(cx + cardW*0.5f - 8.0f, cardY + 24.0f, 16.0f, 12.0f,
                                  glm::vec4(0.68f,0.42f,0.20f,1.0f));
                renderer.DrawQuad(cx + cardW*0.5f - 6.0f, cardY + 25.0f, 12.0f, 10.0f,
                                  glm::vec4(0.96f,0.90f,0.78f,1.0f));
            }

            // Cost
            std::string cost = std::to_string(cards[i].cost);
            float cw = renderer.GetTextWidth(cost, 1.4f);
            renderer.DrawText(cost, cx + (cardW - cw) * 0.5f, cardY + 46.0f,
                              1.4f, statColor, 1.5f);

        } else {
            // Greyscale locked card
            renderer.DrawQuad(cx, cardY, cardW, cardH,
                              glm::vec4(0.24f, 0.24f, 0.25f, 1.0f));
            renderer.DrawRectOutline(cx, cardY, cardW, cardH, 2.0f,
                                     glm::vec4(0.14f, 0.14f, 0.15f, 1.0f));
            float tw = renderer.GetTextWidth(cards[i].name, 1.2f);
            renderer.DrawText(cards[i].name, cx + (cardW - tw) * 0.5f, cardY + 8.0f,
                              1.2f, glm::vec4(0.50f,0.50f,0.52f,1.0f), 1.2f);
            // Padlock icon
            renderer.DrawQuad(cx + cardW*0.5f - 6.0f, cardY + 26.0f, 12.0f, 10.0f,
                              glm::vec4(0.42f,0.42f,0.44f,1.0f));
            renderer.DrawCircle(glm::vec2(cx + cardW*0.5f, cardY + 25.0f),
                                4.5f, glm::vec4(0.52f,0.52f,0.55f,1.0f), 10);
            renderer.DrawCircle(glm::vec2(cx + cardW*0.5f, cardY + 25.0f),
                                2.5f, glm::vec4(0.24f,0.24f,0.25f,1.0f), 8);
            std::string cost = std::to_string(cards[i].cost);
            float cw = renderer.GetTextWidth(cost, 1.3f);
            renderer.DrawText(cost, cx + (cardW - cw) * 0.5f, cardY + 46.0f,
                              1.3f, glm::vec4(0.50f,0.50f,0.52f,1.0f), 1.2f);
        }
    }

    // Controls hint
    renderer.DrawText("SELECT CARD -> CLICK LANE TO DEPLOY",
                      750.0f, 20.0f, 1.3f, glm::vec4(0.95f,0.85f,0.35f,1.0f), 1.4f);
    renderer.DrawText("R: RESTART GAME",
                      750.0f, 42.0f, 1.3f, glm::vec4(0.75f,0.80f,0.85f,1.0f), 1.3f);

    // --- Bottom level-progress bar ---
    const float progX = 15.0f, progY = 680.0f;
    const float progW = 580.0f, progH = 26.0f;

    renderer.DrawQuad(progX, progY, progW, progH,
                      glm::vec4(0.20f, 0.22f, 0.23f, 1.0f));
    renderer.DrawRectOutline(progX, progY, progW, progH, 2.5f,
                             glm::vec4(0.10f, 0.11f, 0.12f, 1.0f));

    float progress = m_GameWorld.GetLevelProgress();
    float fillW    = (progW - 6.0f) * progress;
    renderer.DrawQuad(progX + 3.0f, progY + 3.0f, fillW, progH - 6.0f,
                      glm::vec4(0.35f, 0.85f, 0.45f, 1.0f));

    int pct = (int)(progress * 100.0f);
    renderer.DrawText("LEVEL PROGRESS: " + std::to_string(pct) + "%",
                      progX + 10.0f, progY + 6.0f,
                      1.6f, glm::vec4(0.30f,0.88f,0.40f,1.0f), 1.6f);

    // Bottom-right restart hint
    renderer.DrawText("PRESS 'R' TO RESTART",
                      960.0f, 686.0f, 1.5f, glm::vec4(0.65f,0.68f,0.72f,1.0f), 1.4f);
}
