#include "Playfield.h"
#include <GLFW/glfw3.h>
#include <cmath>

Playfield::Playfield() {
    m_GridStartX = 245.0f;
    m_GridStartY = 95.0f;
    m_GridWidth  = 1010.0f;
    m_GridHeight = 550.0f;
    m_LaneHeight = m_GridHeight / (float)NUM_LANES;
    m_CellWidth  = m_GridWidth / (float)NUM_COLS;
}

void Playfield::Init() {
    for (int i = 0; i < NUM_LANES; ++i) {
        m_SelectedLanes[i] = false;
    }
}

void Playfield::Update(float deltaTime) {
    m_AnimTimer += deltaTime;
}

float Playfield::GetLaneY(int lane) const {
    if (lane < 0) lane = 0;
    if (lane >= NUM_LANES) lane = NUM_LANES - 1;
    return m_GridStartY + (float)lane * m_LaneHeight;
}

void Playfield::UpdateMouse(float virtualMouseX, float virtualMouseY) {
    m_MouseX = virtualMouseX;
    m_MouseY = virtualMouseY;

    // Check if mouse is within the playfield horizontal grid
    if (m_MouseX >= m_GridStartX && m_MouseX <= (m_GridStartX + m_GridWidth) &&
        m_MouseY >= m_GridStartY && m_MouseY <= (m_GridStartY + m_GridHeight)) {
        m_HoveredLane = (int)((m_MouseY - m_GridStartY) / m_LaneHeight);
        if (m_HoveredLane < 0) m_HoveredLane = 0;
        if (m_HoveredLane >= NUM_LANES) m_HoveredLane = NUM_LANES - 1;
    } else {
        m_HoveredLane = -1;
    }
}

void Playfield::OnMouseClick(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (m_HoveredLane >= 0 && m_HoveredLane < NUM_LANES) {
            // Toggles selected state for the lane (multiple lanes can be selected at once)
            m_SelectedLanes[m_HoveredLane] = !m_SelectedLanes[m_HoveredLane];
        }
    }
}

void Playfield::Render(Renderer2D& renderer) {
    RenderEnvironment(renderer);
    RenderHomeFridge(renderer);
    RenderCountertopGrid(renderer);
    RenderLaneHighlights(renderer);
    RenderTopAndBottomUI(renderer);
}

void Playfield::RenderEnvironment(Renderer2D& renderer) {
    // 1. Kitchen Backsplash Wall with Ceramic Tiles
    renderer.DrawQuad(0.0f, 0.0f, 1280.0f, 720.0f, glm::vec4(0.88f, 0.86f, 0.83f, 1.0f));

    // Tile grid lines
    const float tileSize = 45.0f;
    glm::vec4 groutColor(0.80f, 0.77f, 0.74f, 1.0f);
    for (float x = 0.0f; x < 1280.0f; x += tileSize) {
        renderer.DrawLine(glm::vec2(x, 0.0f), glm::vec2(x, 720.0f), 1.0f, groutColor);
    }
    for (float y = 0.0f; y < 720.0f; y += tileSize) {
        renderer.DrawLine(glm::vec2(0.0f, y), glm::vec2(1280.0f, y), 1.0f, groutColor);
    }

    // 2. Kitchen Under-Counter Cabinetry (Base)
    float cabY = m_GridStartY + m_GridHeight + 18.0f;
    float cabH = 720.0f - cabY;
    // Cabinet face
    renderer.DrawQuad(m_GridStartX - 15.0f, cabY, 1280.0f - (m_GridStartX - 15.0f), cabH, glm::vec4(0.76f, 0.74f, 0.71f, 1.0f));
    renderer.DrawQuad(m_GridStartX - 15.0f, cabY, 1280.0f - (m_GridStartX - 15.0f), 6.0f, glm::vec4(0.55f, 0.53f, 0.50f, 1.0f)); // Shadow under counter

    // Cabinet vertical drawer seams
    for (float cx = m_GridStartX + 120.0f; cx < 1280.0f; cx += 260.0f) {
        renderer.DrawQuad(cx, cabY + 6.0f, 2.0f, cabH - 6.0f, glm::vec4(0.58f, 0.56f, 0.53f, 1.0f));
        // Brushed steel drawer handle
        renderer.DrawQuad(cx + 80.0f, cabY + 18.0f, 90.0f, 8.0f, glm::vec4(0.40f, 0.40f, 0.42f, 1.0f));
        renderer.DrawQuad(cx + 82.0f, cabY + 19.0f, 86.0f, 3.0f, glm::vec4(0.85f, 0.85f, 0.88f, 1.0f));
    }

    // Floor toe-kick recess at the very bottom
    renderer.DrawQuad(0.0f, 706.0f, 1280.0f, 14.0f, glm::vec4(0.32f, 0.31f, 0.30f, 1.0f));
}

void Playfield::RenderHomeFridge(Renderer2D& renderer) {
    // 3. Home Base: Simple Cartoon Kitchen Refrigerator (matching reference picture)
    glm::vec4 fridgeBodyColor(0.48f, 0.50f, 0.52f, 1.0f);   // Side / backing grey
    glm::vec4 fridgeTopColor(0.60f, 0.62f, 0.65f, 1.0f);    // 3D perspective top roof
    glm::vec4 doorFillColor(0.53f, 0.55f, 0.58f, 1.0f);     // Door face grey
    glm::vec4 outlineColor(0.12f, 0.12f, 0.14f, 1.0f);      // Crisp dark border outline
    glm::vec4 textColor(0.12f, 0.12f, 0.14f, 1.0f);         // Dark charcoal label text

    // 3D Perspective Top Roof (Angled trapezoid as seen in the picture)
    renderer.DrawQuadVertices(
        glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
        glm::vec2(m_FridgeX + m_FridgeW, m_FridgeY - 8.0f),
        glm::vec2(m_FridgeX + m_FridgeW, m_FridgeY),
        glm::vec2(m_FridgeX, m_FridgeY),
        fridgeTopColor
    );
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f), glm::vec2(m_FridgeX + m_FridgeW, m_FridgeY - 8.0f), 2.5f, outlineColor);
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f), glm::vec2(m_FridgeX, m_FridgeY), 2.5f, outlineColor);

    // Left Perspective Side Wall
    renderer.DrawQuadVertices(
        glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f),
        glm::vec2(m_FridgeX, m_FridgeY),
        glm::vec2(m_FridgeX, m_FridgeY + m_FridgeH),
        glm::vec2(m_FridgeX - 12.0f, m_FridgeY + m_FridgeH - 10.0f),
        fridgeBodyColor
    );
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY - 14.0f), glm::vec2(m_FridgeX - 12.0f, m_FridgeY + m_FridgeH - 10.0f), 2.5f, outlineColor);
    renderer.DrawLine(glm::vec2(m_FridgeX - 12.0f, m_FridgeY + m_FridgeH - 10.0f), glm::vec2(m_FridgeX, m_FridgeY + m_FridgeH), 2.5f, outlineColor);

    // Main Fridge Backing Body
    renderer.DrawQuad(m_FridgeX, m_FridgeY, m_FridgeW, m_FridgeH, fridgeBodyColor);
    renderer.DrawRectOutline(m_FridgeX, m_FridgeY, m_FridgeW, m_FridgeH, 3.0f, outlineColor);

    // Single Door Layout: One unified fridge door
    float doorMargin = 10.0f;
    float doorW = m_FridgeW - doorMargin * 2.0f;
    float doorH = m_FridgeH - doorMargin * 2.0f;
    float doorX = m_FridgeX + doorMargin;
    float doorY = m_FridgeY + doorMargin;

    renderer.DrawQuad(doorX, doorY, doorW, doorH, doorFillColor);
    renderer.DrawRectOutline(doorX, doorY, doorW, doorH, 3.0f, outlineColor);

    // Centered "HOME FRIDGE" text on the single door
    float homeTextW = renderer.GetTextWidth("HOME", 2.6f);
    float fridgeTextW = renderer.GetTextWidth("FRIDGE", 2.6f);
    renderer.DrawText("HOME", doorX + (doorW - homeTextW) * 0.5f, doorY + doorH * 0.5f - 24.0f, 2.6f, textColor, 2.4f);
    renderer.DrawText("FRIDGE", doorX + (doorW - fridgeTextW) * 0.5f, doorY + doorH * 0.5f + 8.0f, 2.6f, textColor, 2.4f);
}

void Playfield::RenderCountertopGrid(Renderer2D& renderer) {
    // 4. The Kitchen Countertop / Cutting Board Playfield
    // Wooden board outer borders and 3D side-view perspective edge
    float tableX = m_GridStartX;
    float tableY = m_GridStartY;
    float tableW = m_GridWidth;
    float tableH = m_GridHeight;

    // Drop shadow under the countertop board
    renderer.DrawQuad(tableX + 4.0f, tableY + 6.0f, tableW, tableH + 18.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.20f));

    // Left perspective edge bevel (as seen in the reference picture!)
    // Angled trapezoid connecting fridge base to table top
    renderer.DrawQuadVertices(
        glm::vec2(tableX - 16.0f, tableY + 8.0f),
        glm::vec2(tableX, tableY),
        glm::vec2(tableX, tableY + tableH + 18.0f),
        glm::vec2(tableX - 16.0f, tableY + tableH + 12.0f),
        glm::vec4(0.70f, 0.54f, 0.38f, 1.0f) // Darker wood side profile
    );
    renderer.DrawLine(
        glm::vec2(tableX - 16.0f, tableY + 8.0f),
        glm::vec2(tableX - 16.0f, tableY + tableH + 12.0f),
        3.0f, glm::vec4(0.32f, 0.20f, 0.12f, 1.0f)
    );

    // Front apron rim (thick wooden countertop edge along the bottom)
    renderer.DrawQuad(tableX, tableY + tableH, tableW, 18.0f, glm::vec4(0.82f, 0.68f, 0.50f, 1.0f));
    renderer.DrawQuad(tableX, tableY + tableH + 15.0f, tableW, 3.0f, glm::vec4(0.45f, 0.30f, 0.18f, 1.0f)); // Underlip shadow

    // Wooden cutting board main surface (warm cream/beige wood)
    renderer.DrawQuad(tableX, tableY, tableW, tableH, glm::vec4(0.96f, 0.93f, 0.86f, 1.0f));

    // Outer thick dark wood perimeter frame
    renderer.DrawRectOutline(tableX, tableY, tableW, tableH, 4.0f, glm::vec4(0.35f, 0.22f, 0.14f, 1.0f));

    // Subtle wood grain lines running along the cutting board
    glm::vec4 woodGrainColor(0.92f, 0.88f, 0.80f, 0.6f);
    for (float gy = tableY + 12.0f; gy < tableY + tableH; gy += 24.0f) {
        renderer.DrawLine(glm::vec2(tableX + 4.0f, gy), glm::vec2(tableX + tableW - 4.0f, gy), 1.0f, woodGrainColor);
    }

    // 5 Horizontal Lanes (Rows) Divider Lines
    glm::vec4 laneLineColor(0.72f, 0.66f, 0.58f, 1.0f);
    for (int i = 1; i < NUM_LANES; ++i) {
        float ly = tableY + (float)i * m_LaneHeight;
        renderer.DrawLine(glm::vec2(tableX, ly), glm::vec2(tableX + tableW, ly), 2.5f, laneLineColor);
    }

    // Vertical Columns (Cells) Divider Lines
    for (int j = 1; j < NUM_COLS; ++j) {
        float cx = tableX + (float)j * m_CellWidth;
        renderer.DrawLine(glm::vec2(cx, tableY), glm::vec2(cx, tableY + tableH), 1.5f, laneLineColor);
    }

    // Safe zone boundary marker at the left edge of the board (Player starting line)
    renderer.DrawLine(glm::vec2(tableX + 2.0f, tableY), glm::vec2(tableX + 2.0f, tableY + tableH), 4.0f, glm::vec4(0.40f, 0.65f, 0.40f, 0.8f));

    // Enemy spawn boundary marker at the rightmost edge of the board
    renderer.DrawLine(glm::vec2(tableX + tableW - 3.0f, tableY), glm::vec2(tableX + tableW - 3.0f, tableY + tableH), 4.0f, glm::vec4(0.85f, 0.35f, 0.35f, 0.8f));
}

void Playfield::RenderLaneHighlights(Renderer2D& renderer) {
    // 5. Lane Highlighting and Interactive Selection
    // Per Project Spec 3.2:
    // - Hovering over the grid highlights the entire horizontal lane under the cursor.
    // - Clicking toggles that lane between selected and deselected. Multiple lanes can be selected at once.

    float pulse = 0.5f + 0.5f * std::sin(m_AnimTimer * 5.0f);

    for (int i = 0; i < NUM_LANES; ++i) {
        float ly = GetLaneY(i);
        bool isHovered = (m_HoveredLane == i);
        bool isSelected = m_SelectedLanes[i];

        if (isSelected) {
            // Selected Lane Highlight: Vibrant green/gold glowing banner
            glm::vec4 selFill(0.20f, 0.85f, 0.35f, 0.28f + pulse * 0.08f);
            renderer.DrawQuad(m_GridStartX + 2.0f, ly + 2.0f, m_GridWidth - 4.0f, m_LaneHeight - 4.0f, selFill);

            // Glowing border
            renderer.DrawRectOutline(m_GridStartX + 2.0f, ly + 2.0f, m_GridWidth - 4.0f, m_LaneHeight - 4.0f, 3.0f, glm::vec4(0.25f, 0.95f, 0.40f, 0.9f));

            // Selected Lane Tag on the left edge
            renderer.DrawQuad(m_GridStartX + 6.0f, ly + 6.0f, 75.0f, 22.0f, glm::vec4(0.15f, 0.70f, 0.30f, 0.95f));
            std::string laneTag = "LANE " + std::to_string(i + 1);
            renderer.DrawText(laneTag, m_GridStartX + 12.0f, ly + 10.0f, 1.4f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f);

        } else if (isHovered) {
            // Hovered Lane: Warm kitchen amber translucent highlight
            glm::vec4 hoverFill(1.0f, 0.92f, 0.55f, 0.25f);
            renderer.DrawQuad(m_GridStartX + 2.0f, ly + 2.0f, m_GridWidth - 4.0f, m_LaneHeight - 4.0f, hoverFill);

            // Subtle top/bottom guide lines
            renderer.DrawLine(glm::vec2(m_GridStartX, ly + 1.0f), glm::vec2(m_GridStartX + m_GridWidth, ly + 1.0f), 2.0f, glm::vec4(1.0f, 0.85f, 0.30f, 0.8f));
            renderer.DrawLine(glm::vec2(m_GridStartX, ly + m_LaneHeight - 1.0f), glm::vec2(m_GridStartX + m_GridWidth, ly + m_LaneHeight - 1.0f), 2.0f, glm::vec4(1.0f, 0.85f, 0.30f, 0.8f));

            // Hover indicator tag
            renderer.DrawQuad(m_GridStartX + 6.0f, ly + 6.0f, 65.0f, 20.0f, glm::vec4(0.85f, 0.70f, 0.20f, 0.85f));
            std::string hoverTag = "LANE " + std::to_string(i + 1);
            renderer.DrawText(hoverTag, m_GridStartX + 12.0f, ly + 10.0f, 1.3f, glm::vec4(0.2f, 0.15f, 0.05f, 1.0f), 1.4f);
        }
    }
}

void Playfield::RenderTopAndBottomUI(Renderer2D& renderer) {
    // 6. Top Header Bar (From screenshot: Score, Wave, Spice, Cards)
    renderer.DrawQuad(0.0f, 0.0f, 1280.0f, 75.0f, glm::vec4(0.18f, 0.18f, 0.18f, 1.0f));
    renderer.DrawLine(glm::vec2(0.0f, 75.0f), glm::vec2(1280.0f, 75.0f), 3.0f, glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));

    // Stats on the top-left (Arcade Green terminal text)
    glm::vec4 statColor(0.38f, 0.88f, 0.48f, 1.0f);
    renderer.DrawText("SCORE: 1200", 20.0f, 12.0f, 2.0f, statColor, 2.0f);
    renderer.DrawText("WAVE 1/3", 20.0f, 32.0f, 2.0f, statColor, 2.0f);
    renderer.DrawText("SPICE: 175", 20.0f, 52.0f, 2.0f, statColor, 2.0f);

    // Mini spice sun icon
    renderer.DrawCircle(glm::vec2(170.0f, 58.0f), 6.0f, glm::vec4(0.95f, 0.80f, 0.20f, 1.0f), 12);

    // "CARDS" Label
    renderer.DrawText("CARDS", 280.0f, 28.0f, 2.2f, statColor, 2.0f);

    // Cards row (matching the picture: Chef, Salt, Bread, Honey, Honey, Toaster)
    struct CardInfo {
        const char* name;
        int cost;
        bool selected;
    };
    CardInfo cards[] = {
        { "CHEF", 50, false },
        { "SALT", 100, true },  // Selected in screenshot with green frame
        { "BREAD", 50, false },
        { "HONEY", 125, false },
        { "HONEY", 125, false },
        { "TOASTER", 200, false }
    };

    float cardStartX = 390.0f;
    float cardW = 75.0f;
    float cardH = 65.0f;
    float cardGap = 12.0f;

    for (int i = 0; i < 6; ++i) {
        float cx = cardStartX + (float)i * (cardW + cardGap);
        float cy = 6.0f;

        // Card base
        renderer.DrawQuad(cx, cy, cardW, cardH, glm::vec4(0.32f, 0.33f, 0.35f, 1.0f));

        if (cards[i].selected) {
            // Bright green border for active card selection
            renderer.DrawRectOutline(cx - 2.0f, cy - 2.0f, cardW + 4.0f, cardH + 4.0f, 3.0f, glm::vec4(0.35f, 0.90f, 0.40f, 1.0f));
        } else {
            renderer.DrawRectOutline(cx, cy, cardW, cardH, 2.0f, glm::vec4(0.15f, 0.16f, 0.18f, 1.0f));
        }

        // Card title
        float textW = renderer.GetTextWidth(cards[i].name, 1.2f);
        renderer.DrawText(cards[i].name, cx + (cardW - textW) * 0.5f, cy + 6.0f, 1.2f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), 1.2f);

        // Icon placeholder
        renderer.DrawCircle(glm::vec2(cx + cardW * 0.5f, cy + 32.0f), 11.0f, glm::vec4(0.50f, 0.52f, 0.55f, 1.0f));

        // Cost
        std::string costStr = std::to_string(cards[i].cost);
        float costW = renderer.GetTextWidth(costStr, 1.3f);
        renderer.DrawText(costStr, cx + (cardW - costW) * 0.5f, cy + 48.0f, 1.3f, statColor, 1.4f);
    }

    // Exit [X] button at top-right
    renderer.DrawQuad(1235.0f, 12.0f, 32.0f, 32.0f, glm::vec4(0.32f, 0.33f, 0.35f, 1.0f));
    renderer.DrawRectOutline(1235.0f, 12.0f, 32.0f, 32.0f, 2.0f, glm::vec4(0.12f, 0.12f, 0.14f, 1.0f));
    renderer.DrawLine(glm::vec2(1243.0f, 20.0f), glm::vec2(1259.0f, 36.0f), 2.5f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));
    renderer.DrawLine(glm::vec2(1259.0f, 20.0f), glm::vec2(1243.0f, 36.0f), 2.5f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));

    // 7. Bottom Level Progress Bar (matching screenshot: "LEVEL PROGRESS: 60%")
    float progX = 15.0f;
    float progY = 680.0f;
    float progW = 580.0f;
    float progH = 26.0f;

    renderer.DrawQuad(progX, progY, progW, progH, glm::vec4(0.20f, 0.22f, 0.23f, 1.0f));
    renderer.DrawRectOutline(progX, progY, progW, progH, 2.5f, glm::vec4(0.10f, 0.11f, 0.12f, 1.0f));

    // Progress bar fill (60%)
    float fillW = (progW - 6.0f) * 0.60f;
    renderer.DrawQuad(progX + 3.0f, progY + 3.0f, fillW, progH - 6.0f, glm::vec4(0.40f, 0.70f, 0.45f, 1.0f));

    // Progress bar label
    renderer.DrawText("LEVEL PROGRESS: 60%", progX + 10.0f, progY + 6.0f, 1.6f, glm::vec4(0.30f, 0.88f, 0.40f, 1.0f), 1.6f);
}
