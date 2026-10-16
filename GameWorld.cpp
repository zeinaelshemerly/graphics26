// ============================================================================
// GameWorld.cpp  — Person 2: Game World & Progression
//
// Implements all Person 2 requirements:
//   * Implement enemy spawning and waves
//   * Handle collectibles and coin rewards
//   * Implement purchasing and unlocking characters
//   * Handle winning and losing conditions
//   * Implement level progression and difficulty
//
// The visual rendering of the grid / lanes / home base / background
// is handled in Playfield.cpp (also Person 2).
// ============================================================================

#include "GameWorld.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

GameWorld::GameWorld() {}

// ---------------------------------------------------------------------------
// Init / Reset
// ---------------------------------------------------------------------------

void GameWorld::Init(float gridStartX, float gridStartY,
                     float gridWidth,  float gridHeight, float laneHeight)
{
    m_GridStartX = gridStartX;
    m_GridStartY = gridStartY;
    m_GridWidth  = gridWidth;
    m_GridHeight = gridHeight;
    m_LaneHeight = laneHeight;
    Reset();
}

void GameWorld::Reset()
{
    m_Enemies.clear();
    m_PlacedPlayers.clear();
    m_Collectibles.clear();

    // Economy
    m_SpiceCoins   = 30;              // Enough for 3 Melee players right away
    m_Score        = 0;
    m_SelectedCard = P2_PLAYER_NONE;

    // Wave progression
    m_CurrentWave          = 1;
    m_EnemiesSpawnedInWave = 0;
    m_EnemiesToSpawnInWave = 3;       // Wave 1: 3 enemies
    m_SpawnCooldown        = 2.5f;
    m_WaveBannerTimer      = 3.5f;
    m_WaveBannerText       = "WAVE 1 - PESTS ON THE COUNTER!";
    m_TotalDefeatedEnemies = 0;

    m_Status      = P2_STATUS_PLAYING;
    m_GlobalTimer = 0.0f;
}

// ---------------------------------------------------------------------------
// Task: Implement level progression and difficulty
// ---------------------------------------------------------------------------

float GameWorld::GetLevelProgress() const
{
    float p = (float)m_TotalDefeatedEnemies / (float)TOTAL_ENEMIES_ALL_WAVES;
    if (p > 1.0f) p = 1.0f;
    return p;
}

// ---------------------------------------------------------------------------
// Task: Implement purchasing and unlocking characters
// ---------------------------------------------------------------------------

int GameWorld::GetCharacterCost(P2PlayerType type) const
{
    switch (type) {
        case P2_PLAYER_MELEE:   return 10;
        case P2_PLAYER_RANGED:  return 20;
        case P2_PLAYER_DEFENSE: return 30;
        default:                return 0;
    }
}

bool GameWorld::IsCharacterUnlocked(P2PlayerType type) const
{
    // A card is available (unlocked) when the player has at least enough
    // spice-coins to cover its cost.  Falls back to locked if coins drop.
    return m_SpiceCoins >= GetCharacterCost(type);
}

bool GameWorld::CanPurchase(P2PlayerType type) const
{
    return IsCharacterUnlocked(type) && (m_SpiceCoins >= GetCharacterCost(type));
}

void GameWorld::SelectCard(P2PlayerType type)
{
    if (IsCharacterUnlocked(type)) {
        // Toggle: clicking the active card deselects it
        m_SelectedCard = (m_SelectedCard == type) ? P2_PLAYER_NONE : type;
    }
}

bool GameWorld::PurchaseAndDeploy(P2PlayerType type, int lane)
{
    if (m_Status != P2_STATUS_PLAYING) return false;
    if (lane < 0 || lane >= 5)         return false;
    if (!CanPurchase(type))            return false;

    P2Player p;
    p.type   = type;
    p.lane   = lane;
    p.x      = m_GridStartX + 35.0f;
    p.y      = m_GridStartY + (float)lane * m_LaneHeight + m_LaneHeight * 0.5f;
    p.active = true;

    switch (type) {
        case P2_PLAYER_MELEE:
            p.health = p.maxHealth = 5;
            p.speed  = 55.0f;
            break;
        case P2_PLAYER_RANGED:
            p.health = p.maxHealth = 5;
            p.speed  = 45.0f;
            break;
        case P2_PLAYER_DEFENSE:
            p.health = p.maxHealth = 7;
            p.speed  = 0.0f;                     // Stationary blocker
            p.x      = m_GridStartX + 70.0f;     // Placed near home base
            break;
        default: break;
    }

    m_PlacedPlayers.push_back(p);
    m_SpiceCoins -= GetCharacterCost(type);
    m_SelectedCard = P2_PLAYER_NONE;
    return true;
}

// ---------------------------------------------------------------------------
// Task: Handle collectibles and coin rewards
// ---------------------------------------------------------------------------

void GameWorld::AddSpiceCoins(int amount)
{
    m_SpiceCoins += amount;
    if (m_SpiceCoins < 0) m_SpiceCoins = 0;
}

void GameWorld::SpawnCollectible(float x, float y, int lane, int value)
{
    P2Collectible item;
    item.x         = x;
    item.y         = y;
    item.lane      = lane;
    item.value     = value;
    item.animTimer = (float)(std::rand() % 100) * 0.1f;
    item.collected = false;
    m_Collectibles.push_back(item);
}

bool GameWorld::TryCollectAt(float mouseX, float mouseY)
{
    if (m_Status != P2_STATUS_PLAYING) return false;

    for (auto& item : m_Collectibles) {
        if (!item.collected) {
            float dist = std::hypot(mouseX - item.x, mouseY - item.y);
            if (dist < 32.0f) {
                item.collected = true;
                AddSpiceCoins(item.value);
                m_Score += 50;
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Main update loop
// ---------------------------------------------------------------------------

void GameWorld::Update(float deltaTime)
{
    m_GlobalTimer += deltaTime;

    if (m_WaveBannerTimer > 0.0f)
        m_WaveBannerTimer -= deltaTime;

    // Everything freezes on win/lose
    if (m_Status != P2_STATUS_PLAYING)
        return;

    UpdateWaveSpawner(deltaTime);
    UpdateEnemyMovement(deltaTime);
    UpdatePlayerMovement(deltaTime);
    UpdateCollectibles(deltaTime);
    CheckWinningAndLosing();
}

// ---------------------------------------------------------------------------
// Task: Implement enemy spawning and waves
// ---------------------------------------------------------------------------

void GameWorld::UpdateWaveSpawner(float deltaTime)
{
    if (m_EnemiesSpawnedInWave < m_EnemiesToSpawnInWave) {
        m_SpawnCooldown -= deltaTime;
        if (m_SpawnCooldown <= 0.0f) {
            int randomLane = std::rand() % 5;
            P2EnemyTier tier = P2_ENEMY_EASY;

            // Difficulty scales per wave
            if (m_CurrentWave == 1) {
                tier = P2_ENEMY_EASY;
            } else if (m_CurrentWave == 2) {
                tier = (std::rand() % 2 == 0) ? P2_ENEMY_EASY : P2_ENEMY_MEDIUM;
            } else if (m_CurrentWave == 3) {
                int roll = std::rand() % 10;
                if      (roll < 4) tier = P2_ENEMY_EASY;
                else if (roll < 7) tier = P2_ENEMY_MEDIUM;
                else               tier = P2_ENEMY_HARD;
            }

            SpawnWaveEnemy(tier, randomLane);
            m_EnemiesSpawnedInWave++;

            // Spawn interval shortens each wave (harder difficulty)
            m_SpawnCooldown = 3.6f - ((float)m_CurrentWave * 0.5f);
        }
    } else {
        // All enemies for this wave are spawned; wait for all to be defeated
        if (m_Enemies.empty()) {
            if (m_CurrentWave < TOTAL_WAVES) {
                m_CurrentWave++;
                m_EnemiesSpawnedInWave = 0;

                if (m_CurrentWave == 2) {
                    m_EnemiesToSpawnInWave = 5;
                    m_WaveBannerText       = "WAVE 2 - ROACHES INCOMING!";
                } else if (m_CurrentWave == 3) {
                    m_EnemiesToSpawnInWave = 10;
                    m_WaveBannerText       = "FINAL WAVE 3 - RAT INVASION!";
                }

                m_WaveBannerTimer = 3.5f;
                m_SpawnCooldown   = 2.5f;
            } else {
                // All 3 waves defeated — victory!
                m_Status  = P2_STATUS_VICTORY;
                m_Score  += 1000;
            }
        }
    }
}

void GameWorld::SpawnWaveEnemy(P2EnemyTier tier, int lane)
{
    P2Enemy e;
    e.tier   = tier;
    e.lane   = lane;
    e.x      = m_GridStartX + m_GridWidth - 25.0f;  // Spawn at right edge
    e.y      = m_GridStartY + (float)lane * m_LaneHeight + m_LaneHeight * 0.5f;
    e.active = true;

    switch (tier) {
        case P2_ENEMY_EASY:
            e.health = e.maxHealth = 5;
            e.speed  = 38.0f;
            break;
        case P2_ENEMY_MEDIUM:
            e.health = e.maxHealth = 5;
            e.speed  = 48.0f;   // Faster than easy
            break;
        case P2_ENEMY_HARD:
            e.health = e.maxHealth = 7;
            e.speed  = 72.0f;   // Fast melee runner
            break;
    }

    m_Enemies.push_back(e);
}

// ---------------------------------------------------------------------------
// Movement / combat updates
// ---------------------------------------------------------------------------

void GameWorld::UpdateEnemyMovement(float deltaTime)
{
    for (auto& e : m_Enemies) {
        if (!e.active) continue;

        e.animTimer += deltaTime;

        bool blocked = false;
        for (auto& p : m_PlacedPlayers) {
            if (!p.active || p.lane != e.lane) continue;

            float dist = e.x - p.x;
            if (dist > -10.0f && dist < 38.0f) {
                blocked = true;
                // Simple combat: both take 1 point of damage per second
                static const float COMBAT_DPS = 1.0f;
                p.health -= (int)(COMBAT_DPS * deltaTime + 0.5f);
                e.health -= (int)(COMBAT_DPS * deltaTime + 0.5f);

                if (p.health <= 0) p.active = false;
                if (e.health <= 0) {
                    e.active = false;
                    // Task: Handle collectibles and coin rewards
                    SpawnCollectible(e.x, e.y, e.lane, 25);
                    m_Score += 100;
                    m_TotalDefeatedEnemies++;
                }
                break;
            }
        }

        // Advance towards home base (left)
        if (!blocked)
            e.x -= e.speed * deltaTime;
    }

    // Cleanup defeated enemies
    m_Enemies.erase(
        std::remove_if(m_Enemies.begin(), m_Enemies.end(),
                       [](const P2Enemy& e){ return !e.active; }),
        m_Enemies.end()
    );
}

void GameWorld::UpdatePlayerMovement(float deltaTime)
{
    for (auto& p : m_PlacedPlayers) {
        if (!p.active) continue;

        bool blocked = false;
        for (const auto& e : m_Enemies) {
            if (!e.active || e.lane != p.lane) continue;
            float dist = e.x - p.x;
            if (dist > -10.0f && dist < 38.0f) {
                blocked = true;
                break;
            }
        }

        // Melee/Ranged advance; Defense stays put
        if (!blocked && p.type != P2_PLAYER_DEFENSE)
            p.x += p.speed * deltaTime;
    }

    // Cleanup defeated players
    m_PlacedPlayers.erase(
        std::remove_if(m_PlacedPlayers.begin(), m_PlacedPlayers.end(),
                       [](const P2Player& p){ return !p.active; }),
        m_PlacedPlayers.end()
    );
}

void GameWorld::UpdateCollectibles(float deltaTime)
{
    for (auto& item : m_Collectibles) {
        if (item.collected) continue;
        item.animTimer += deltaTime;

        // Auto-collect when a player character walks over it
        for (const auto& p : m_PlacedPlayers) {
            if (p.active && p.lane == item.lane && std::abs(p.x - item.x) < 35.0f) {
                item.collected = true;
                AddSpiceCoins(item.value);
                m_Score += 50;
                break;
            }
        }
    }

    m_Collectibles.erase(
        std::remove_if(m_Collectibles.begin(), m_Collectibles.end(),
                       [](const P2Collectible& c){ return c.collected; }),
        m_Collectibles.end()
    );
}

// ---------------------------------------------------------------------------
// Task: Handle winning and losing conditions
// ---------------------------------------------------------------------------

void GameWorld::CheckWinningAndLosing()
{
    // Losing: any enemy reaches the Home Base (left edge of the grid)
    for (const auto& e : m_Enemies) {
        if (e.active && e.x <= (m_GridStartX + 10.0f)) {
            m_Status = P2_STATUS_GAME_OVER;
            return;
        }
    }

    // Winning: all 3 waves spawned and all enemies defeated
    if (m_CurrentWave >= TOTAL_WAVES &&
        m_EnemiesSpawnedInWave >= m_EnemiesToSpawnInWave &&
        m_Enemies.empty())
    {
        m_Status  = P2_STATUS_VICTORY;
    }
}

// ---------------------------------------------------------------------------
// Rendering — Person 2's simple geometric characters + HUD overlays
// ---------------------------------------------------------------------------

void GameWorld::DrawHealthBar(Renderer2D& renderer,
                               float x, float y, float w,
                               int health, int maxHealth, bool isPlayer)
{
    const float h      = 5.0f;
    const float border = 1.0f;

    renderer.DrawQuad(x, y, w, h, glm::vec4(0.12f, 0.12f, 0.14f, 0.9f));
    renderer.DrawRectOutline(x, y, w, h, border, glm::vec4(0.25f, 0.25f, 0.28f, 1.0f));

    float frac = (float)health / (float)maxHealth;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    glm::vec4 fill = isPlayer
        ? ((frac > 0.4f) ? glm::vec4(0.30f,0.88f,0.40f,1.0f) : glm::vec4(0.95f,0.30f,0.20f,1.0f))
        : ((frac > 0.4f) ? glm::vec4(0.95f,0.75f,0.20f,1.0f) : glm::vec4(0.90f,0.20f,0.20f,1.0f));

    renderer.DrawQuad(x + border, y + border,
                      (w - 2.0f * border) * frac, h - 2.0f * border, fill);
}

void GameWorld::DrawPlayerShape(Renderer2D& renderer, const P2Player& p)
{
    float px = p.x, py = p.y;
    glm::vec4 outline(0.12f, 0.12f, 0.14f, 1.0f);

    // Health bar
    DrawHealthBar(renderer, px - 18.0f, py - 26.0f, 36.0f, p.health, p.maxHealth, true);

    if (p.type == P2_PLAYER_MELEE) {
        // Spatula Defender — horizontal grip + vertical blade
        renderer.DrawQuad(px - 20.0f, py - 3.0f, 16.0f, 6.0f,
                          glm::vec4(0.22f, 0.22f, 0.24f, 1.0f)); // handle
        renderer.DrawQuad(px - 4.0f,  py - 2.0f,  8.0f, 4.0f,
                          glm::vec4(0.82f, 0.84f, 0.86f, 1.0f)); // neck
        renderer.DrawQuad(px + 4.0f,  py - 13.0f, 16.0f, 26.0f,
                          glm::vec4(0.82f, 0.84f, 0.86f, 1.0f)); // blade
        renderer.DrawRectOutline(px + 4.0f, py - 13.0f, 16.0f, 26.0f, 2.0f, outline);
        // Slots on blade
        renderer.DrawLine(glm::vec2(px + 8.5f,  py - 7.0f), glm::vec2(px + 8.5f,  py + 7.0f), 2.0f, outline);
        renderer.DrawLine(glm::vec2(px + 13.0f, py - 7.0f), glm::vec2(px + 13.0f, py + 7.0f), 2.0f, outline);
        renderer.DrawLine(glm::vec2(px + 17.5f, py - 7.0f), glm::vec2(px + 17.5f, py + 7.0f), 2.0f, outline);

    } else if (p.type == P2_PLAYER_RANGED) {
        // Salt Shaker — cylindrical body with cap
        renderer.DrawQuad(px - 16.0f, py - 9.0f, 24.0f, 18.0f,
                          glm::vec4(0.78f, 0.80f, 0.84f, 1.0f));
        renderer.DrawRectOutline(px - 16.0f, py - 9.0f, 24.0f, 18.0f, 2.0f, outline);
        renderer.DrawQuad(px + 8.0f, py - 7.5f, 6.0f, 15.0f,
                          glm::vec4(0.40f, 0.42f, 0.45f, 1.0f)); // dark cap
        renderer.DrawRectOutline(px + 8.0f, py - 7.5f, 6.0f, 15.0f, 2.0f, outline);
        // Salt dot holes on cap
        renderer.DrawCircle(glm::vec2(px + 11.0f, py - 4.0f), 1.2f, outline);
        renderer.DrawCircle(glm::vec2(px + 11.0f, py),        1.2f, outline);
        renderer.DrawCircle(glm::vec2(px + 11.0f, py + 4.0f), 1.2f, outline);

    } else {
        // Bread Loaf — arched crust + soft crumb interior
        renderer.DrawQuad(px - 13.0f, py - 7.0f, 26.0f, 22.0f,
                          glm::vec4(0.68f, 0.42f, 0.20f, 1.0f));
        renderer.DrawCircle(glm::vec2(px - 6.5f, py - 7.0f), 7.5f,
                            glm::vec4(0.68f, 0.42f, 0.20f, 1.0f));
        renderer.DrawCircle(glm::vec2(px + 6.5f, py - 7.0f), 7.5f,
                            glm::vec4(0.68f, 0.42f, 0.20f, 1.0f));
        renderer.DrawQuad(px - 10.0f, py - 5.0f, 20.0f, 18.0f,
                          glm::vec4(0.96f, 0.90f, 0.78f, 1.0f)); // crumb
        renderer.DrawRectOutline(px - 13.0f, py - 7.0f, 26.0f, 22.0f, 2.0f, outline);
    }
}

void GameWorld::DrawEnemyShape(Renderer2D& renderer, const P2Enemy& e)
{
    float ex = e.x, ey = e.y;
    glm::vec4 outline(0.12f, 0.12f, 0.14f, 1.0f);

    // Health bar
    DrawHealthBar(renderer, ex - 18.0f, ey - 26.0f, 36.0f, e.health, e.maxHealth, false);

    if (e.tier == P2_ENEMY_EASY) {
        // Black Ant — 3 connected circles + antennae + legs
        glm::vec4 antColor(0.12f, 0.12f, 0.14f, 1.0f);
        renderer.DrawCircle(glm::vec2(ex - 12.0f, ey), 5.5f, antColor); // head
        renderer.DrawCircle(glm::vec2(ex - 2.0f,  ey), 4.5f, antColor); // thorax
        renderer.DrawCircle(glm::vec2(ex + 9.0f,  ey), 7.5f, antColor); // abdomen
        // Antennae
        renderer.DrawLine(glm::vec2(ex - 14.0f, ey - 3.0f),
                          glm::vec2(ex - 22.0f, ey - 10.0f), 1.5f, antColor);
        renderer.DrawLine(glm::vec2(ex - 22.0f, ey - 10.0f),
                          glm::vec2(ex - 27.0f, ey - 7.0f),  1.5f, antColor);
        // Legs
        float wag = std::sin(e.animTimer * 12.0f) * 3.0f;
        for (int l = -1; l <= 1; ++l) {
            float lx = ex + (float)l * 7.0f;
            renderer.DrawLine(glm::vec2(lx, ey), glm::vec2(lx - 4.0f, ey + 10.0f + l * wag), 1.5f, antColor);
            renderer.DrawLine(glm::vec2(lx, ey), glm::vec2(lx - 4.0f, ey - 10.0f - l * wag), 1.5f, antColor);
        }

    } else if (e.tier == P2_ENEMY_MEDIUM) {
        // Brown Cockroach — oval shell + antennae + legs
        glm::vec4 roachColor(0.58f, 0.32f, 0.16f, 1.0f);
        glm::vec4 roachDark (0.20f, 0.10f, 0.05f, 1.0f);
        renderer.DrawQuad(ex - 13.0f, ey - 7.0f, 26.0f, 14.0f, roachColor);
        renderer.DrawCircle(glm::vec2(ex - 13.0f, ey), 7.0f, roachColor);
        renderer.DrawCircle(glm::vec2(ex + 13.0f, ey), 7.0f, roachColor);
        renderer.DrawLine(glm::vec2(ex - 13.0f, ey),
                          glm::vec2(ex + 13.0f, ey), 1.8f, roachDark); // wing divide
        // Antennae
        renderer.DrawLine(glm::vec2(ex - 15.0f, ey - 3.0f),
                          glm::vec2(ex - 29.0f, ey - 12.0f), 1.5f, roachDark);
        renderer.DrawLine(glm::vec2(ex - 15.0f, ey + 3.0f),
                          glm::vec2(ex - 29.0f, ey + 12.0f), 1.5f, roachDark);
        // Legs
        for (int l = -1; l <= 1; ++l) {
            float lx = ex + (float)l * 8.0f;
            renderer.DrawLine(glm::vec2(lx, ey + 7.0f),
                              glm::vec2(lx + 5.0f, ey + 14.0f), 1.5f, roachDark);
            renderer.DrawLine(glm::vec2(lx, ey - 7.0f),
                              glm::vec2(lx + 5.0f, ey - 14.0f), 1.5f, roachDark);
        }

    } else {
        // Grey Rat — body circles + top hat + wagging tail
        glm::vec4 ratGrey (0.60f, 0.62f, 0.65f, 1.0f);
        glm::vec4 tailColor(0.85f, 0.72f, 0.70f, 1.0f);
        renderer.DrawCircle(glm::vec2(ex + 4.0f,  ey),       15.0f, ratGrey); // body
        renderer.DrawCircle(glm::vec2(ex - 7.0f,  ey + 1.5f),10.0f, ratGrey); // head
        // Snout wedge
        renderer.DrawQuadVertices(
            glm::vec2(ex - 24.0f, ey + 3.0f),
            glm::vec2(ex - 7.0f,  ey - 7.0f),
            glm::vec2(ex + 4.0f,  ey),
            glm::vec2(ex - 7.0f,  ey + 13.0f),
            ratGrey
        );
        // Eye
        renderer.DrawCircle(glm::vec2(ex - 11.0f, ey - 1.0f), 2.0f, outline);
        // Top hat
        renderer.DrawQuad(ex - 11.0f, ey - 18.0f, 16.0f, 3.5f, outline);  // brim
        renderer.DrawQuad(ex - 8.0f,  ey - 28.0f, 10.0f, 10.0f, outline); // crown
        // Wagging tail
        float wag = std::sin(e.animTimer * 10.0f) * 4.0f;
        renderer.DrawLine(glm::vec2(ex + 18.0f, ey + 3.0f),
                          glm::vec2(ex + 30.0f, ey + 1.0f + wag), 2.0f, tailColor);
        renderer.DrawLine(glm::vec2(ex + 30.0f, ey + 1.0f + wag),
                          glm::vec2(ex + 42.0f, ey + 10.0f),      2.0f, tailColor);
    }
}

void GameWorld::RenderEntities(Renderer2D& renderer)
{
    // 1. Collectibles (bobbing spice-coin icons)
    for (const auto& item : m_Collectibles) {
        float bob = std::sin(item.animTimer * 6.0f) * 5.0f;
        float cy  = item.y + bob;
        renderer.DrawCircle(glm::vec2(item.x, cy), 13.0f,
                            glm::vec4(1.0f, 0.88f, 0.25f, 0.35f), 16); // glow halo
        renderer.DrawCircle(glm::vec2(item.x, cy),  8.5f,
                            glm::vec4(0.98f, 0.85f, 0.15f, 1.0f),  16); // gold coin
        renderer.DrawCircle(glm::vec2(item.x, cy),  4.5f,
                            glm::vec4(1.0f,  0.98f, 0.80f, 1.0f),  12); // bright centre
    }

    // 2. Placed player characters
    for (const auto& p : m_PlacedPlayers)
        DrawPlayerShape(renderer, p);

    // 3. Enemy waves advancing on lanes
    for (const auto& e : m_Enemies)
        DrawEnemyShape(renderer, e);

    // 4. Wave announcement banner
    if (m_WaveBannerTimer > 0.0f) {
        float alpha = std::min(1.0f, m_WaveBannerTimer);
        float banX  = m_GridStartX + 120.0f;
        float banY  = m_GridStartY + 180.0f;

        renderer.DrawQuad(banX, banY, 600.0f, 60.0f,
                          glm::vec4(0.12f, 0.12f, 0.14f, 0.88f * alpha));
        renderer.DrawRectOutline(banX, banY, 600.0f, 60.0f, 3.0f,
                                 glm::vec4(0.95f, 0.85f, 0.25f, 0.95f * alpha));
        float tw = renderer.GetTextWidth(m_WaveBannerText, 2.2f);
        renderer.DrawText(m_WaveBannerText,
                          banX + (600.0f - tw) * 0.5f, banY + 20.0f,
                          2.2f, glm::vec4(1.0f, 0.90f, 0.30f, alpha), 2.2f);
    }

    // 5. Win / Game-Over overlays
    if (m_Status == P2_STATUS_GAME_OVER) {
        float mx = m_GridStartX + 150.0f, my = m_GridStartY + 120.0f;
        renderer.DrawQuad(mx, my, 580.0f, 240.0f,
                          glm::vec4(0.15f, 0.05f, 0.05f, 0.92f));
        renderer.DrawRectOutline(mx, my, 580.0f, 240.0f, 4.0f,
                                 glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        renderer.DrawText("GAME OVER!", mx + 150.0f, my + 30.0f,
                          3.2f, glm::vec4(1.0f, 0.25f, 0.25f, 1.0f), 3.0f);
        renderer.DrawText("PESTS REACHED THE HOME FRIDGE!",
                          mx + 50.0f, my + 90.0f,
                          1.8f, glm::vec4(0.95f,0.95f,0.95f,1.0f), 1.8f);
        renderer.DrawText("PRESS 'R' TO RESTART",
                          mx + 90.0f, my + 150.0f,
                          2.0f, glm::vec4(0.95f,0.85f,0.25f,1.0f), 2.0f);

    } else if (m_Status == P2_STATUS_VICTORY) {
        float mx = m_GridStartX + 150.0f, my = m_GridStartY + 120.0f;
        renderer.DrawQuad(mx, my, 580.0f, 240.0f,
                          glm::vec4(0.05f, 0.15f, 0.08f, 0.92f));
        renderer.DrawRectOutline(mx, my, 580.0f, 240.0f, 4.0f,
                                 glm::vec4(0.35f, 0.95f, 0.45f, 1.0f));
        renderer.DrawText("VICTORY!", mx + 170.0f, my + 30.0f,
                          3.2f, glm::vec4(0.35f, 0.95f, 0.45f, 1.0f), 3.0f);
        renderer.DrawText("ALL 3 WAVES DEFEATED - FRIDGE SAVED!",
                          mx + 25.0f, my + 90.0f,
                          1.7f, glm::vec4(0.95f,0.95f,0.95f,1.0f), 1.8f);
        renderer.DrawText("FINAL SCORE: " + std::to_string(m_Score),
                          mx + 130.0f, my + 130.0f,
                          2.0f, glm::vec4(1.0f,0.90f,0.30f,1.0f), 2.0f);
        renderer.DrawText("PRESS 'R' TO PLAY AGAIN",
                          mx + 115.0f, my + 175.0f,
                          1.8f, glm::vec4(0.95f,0.85f,0.25f,1.0f), 1.8f);
    }
}
