#include "GameLogic.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

GameLogic::GameLogic() {}

void GameLogic::Init(float gridStartX, float gridStartY, float gridWidth, float gridHeight, float laneHeight) {
    m_GridStartX = gridStartX;
    m_GridStartY = gridStartY;
    m_GridWidth  = gridWidth;
    m_GridHeight = gridHeight;
    m_LaneHeight = laneHeight;
    Reset();
}

void GameLogic::Reset() {
    m_Players.clear();
    m_Enemies.clear();
    m_Projectiles.clear();
    m_Collectibles.clear();

    m_SpiceCoins = 30; // Starts with 30 coins (enough for 3 Melee players as per spec)
    m_Score = 0;
    m_SelectedCard = PLAYER_NONE;

    m_CurrentWave = 1;
    m_EnemiesSpawnedInWave = 0;
    m_EnemiesToSpawnInWave = 3; // Wave 1: 3 enemies (Easy only)
    m_SpawnCooldown = 3.0f;
    m_WaveBannerTimer = 3.5f;
    m_WaveBannerText = "WAVE 1 - ANTS ON THE COUNTER!";
    m_TotalDefeatedEnemies = 0;

    m_GameState = STATE_PLAYING;
    m_GlobalTimer = 0.0f;
}

float GameLogic::GetLevelProgress() const {
    float progress = (float)m_TotalDefeatedEnemies / (float)TOTAL_GAME_ENEMIES;
    if (progress > 1.0f) progress = 1.0f;
    return progress;
}

int GameLogic::GetCardCost(PlayerType type) const {
    switch (type) {
        case PLAYER_MELEE:   return 10;
        case PLAYER_RANGED:  return 20;
        case PLAYER_DEFENSE: return 30;
        default: return 0;
    }
}

bool GameLogic::IsCardUnlocked(PlayerType type) const {
    // Section 4.2:
    // Once player has collected enough points, card becomes unlocked.
    // If available points fall below required cost, it returns to locked state.
    switch (type) {
        case PLAYER_MELEE:   return m_SpiceCoins >= 10;
        case PLAYER_RANGED:  return m_SpiceCoins >= 20;
        case PLAYER_DEFENSE: return m_SpiceCoins >= 30;
        default: return false;
    }
}

void GameLogic::SelectCard(PlayerType type) {
    if (IsCardUnlocked(type)) {
        if (m_SelectedCard == type) {
            m_SelectedCard = PLAYER_NONE; // Toggle off
        } else {
            m_SelectedCard = type;
        }
    }
}

void GameLogic::TryDeployPlayer(int lane) {
    if (m_GameState != STATE_PLAYING) return;
    if (lane < 0 || lane >= 5) return;
    if (m_SelectedCard == PLAYER_NONE) return;

    int cost = GetCardCost(m_SelectedCard);
    if (m_SpiceCoins < cost) return;

    PlayerCharacter p;
    p.type = m_SelectedCard;
    p.lane = lane;
    p.x = m_GridStartX + 40.0f;
    p.y = m_GridStartY + (float)lane * m_LaneHeight + m_LaneHeight * 0.5f;
    p.active = true;

    if (p.type == PLAYER_MELEE) {
        p.health = 5;
        p.maxHealth = 5;
        p.speed = 60.0f;
    } else if (p.type == PLAYER_RANGED) {
        p.health = 5;
        p.maxHealth = 5;
        p.speed = 48.0f;
    } else if (p.type == PLAYER_DEFENSE) {
        p.health = 7;
        p.maxHealth = 7;
        p.speed = 0.0f; // Stationary blocker
        p.x = m_GridStartX + 75.0f; // Positioned right in front of home base to guard
    }

    m_Players.push_back(p);
    m_SpiceCoins -= cost;
    m_SelectedCard = PLAYER_NONE;
}

void GameLogic::TriggerJumpForLanes(const bool selectedLanes[5]) {
    if (m_GameState != STATE_PLAYING) return;
    for (auto& p : m_Players) {
        if (p.type != PLAYER_DEFENSE && selectedLanes[p.lane]) {
            if (!p.isJumping && !p.isDucking) {
                p.isJumping = true;
                p.jumpTimer = 0.75f;
            }
        }
    }
}

void GameLogic::TriggerDuckForLanes(const bool selectedLanes[5]) {
    if (m_GameState != STATE_PLAYING) return;
    for (auto& p : m_Players) {
        if (p.type != PLAYER_DEFENSE && selectedLanes[p.lane]) {
            if (!p.isJumping && !p.isDucking) {
                p.isDucking = true;
                p.duckTimer = 0.75f;
            }
        }
    }
}

void GameLogic::OnClickAt(float mouseX, float mouseY) {
    if (m_GameState != STATE_PLAYING) return;

    // Check if player clicked any dropped collectible
    for (auto& c : m_Collectibles) {
        if (!c.collected) {
            float dist = std::hypot(mouseX - c.x, mouseY - c.y);
            if (dist < 32.0f) {
                c.collected = true;
                m_SpiceCoins += c.value;
                m_Score += 50;
            }
        }
    }
}

void GameLogic::Update(float deltaTime, const bool selectedLanes[5]) {
    m_GlobalTimer += deltaTime;

    if (m_WaveBannerTimer > 0.0f) {
        m_WaveBannerTimer -= deltaTime;
    }

    if (m_GameState != STATE_PLAYING) {
        return; // Everything stops moving and attacking on win/lose!
    }

    UpdateWaveSpawner(deltaTime);
    UpdatePlayers(deltaTime);
    UpdateEnemies(deltaTime);
    UpdateProjectiles(deltaTime);
    UpdateCollectibles(deltaTime);
    CheckCollisionsAndCombat(deltaTime);
}

void GameLogic::UpdateWaveSpawner(float deltaTime) {
    // If still enemies to spawn in current wave
    if (m_EnemiesSpawnedInWave < m_EnemiesToSpawnInWave) {
        m_SpawnCooldown -= deltaTime;
        if (m_SpawnCooldown <= 0.0f) {
            int randomLane = std::rand() % 5;
            EnemyType enemyType = ENEMY_EASY_UPPER;

            if (m_CurrentWave == 1) {
                enemyType = ENEMY_EASY_UPPER; // Easy only
            } else if (m_CurrentWave == 2) {
                // Mix of easy and medium
                enemyType = (std::rand() % 2 == 0) ? ENEMY_EASY_UPPER : ENEMY_MEDIUM_LOWER;
            } else if (m_CurrentWave == 3) {
                // Mix of easy, medium, and hard
                int roll = std::rand() % 10;
                if (roll < 4) enemyType = ENEMY_EASY_UPPER;
                else if (roll < 7) enemyType = ENEMY_MEDIUM_LOWER;
                else enemyType = ENEMY_HARD_MELEE;
            }

            SpawnEnemy(enemyType, randomLane);
            m_EnemiesSpawnedInWave++;
            m_SpawnCooldown = 3.5f - ((float)m_CurrentWave * 0.4f);
        }
    } else {
        // All enemies in current wave spawned; wait until all active enemies in wave are defeated
        if (m_Enemies.empty()) {
            if (m_CurrentWave < TOTAL_WAVES) {
                m_CurrentWave++;
                m_EnemiesSpawnedInWave = 0;
                if (m_CurrentWave == 2) {
                    m_EnemiesToSpawnInWave = 5;
                    m_WaveBannerText = "WAVE 2 - ROACHES INCOMING!";
                } else if (m_CurrentWave == 3) {
                    m_EnemiesToSpawnInWave = 10;
                    m_WaveBannerText = "FINAL WAVE 3 - THE RAT BOSS!";
                }
                m_WaveBannerTimer = 3.5f;
                m_SpawnCooldown = 2.5f;
            } else {
                // All 3 waves defeated! Victory!
                m_GameState = STATE_VICTORY;
                m_Score += 1000;
            }
        }
    }
}

void GameLogic::SpawnEnemy(EnemyType type, int lane) {
    EnemyCharacter e;
    e.type = type;
    e.lane = lane;
    e.x = m_GridStartX + m_GridWidth - 30.0f;
    e.y = m_GridStartY + (float)lane * m_LaneHeight + m_LaneHeight * 0.5f;
    e.active = true;

    if (type == ENEMY_EASY_UPPER) {
        e.health = 5;
        e.maxHealth = 5;
        e.speed = 40.0f;
        e.damage = 1;
    } else if (type == ENEMY_MEDIUM_LOWER) {
        e.health = 5;
        e.maxHealth = 5;
        e.speed = 46.0f;
        e.damage = 1;
    } else if (type == ENEMY_HARD_MELEE) {
        e.health = 7;
        e.maxHealth = 7;
        e.speed = 78.0f;
        e.damage = 2; // Hard melee deals 2 points of damage
    }

    m_Enemies.push_back(e);
}

void GameLogic::DropCollectible(float x, float y, int lane) {
    Collectible c;
    c.x = x;
    c.y = y;
    c.lane = lane;
    c.value = 25;
    c.animTimer = (float)(std::rand() % 100) * 0.1f;
    c.collected = false;
    m_Collectibles.push_back(c);
}

void GameLogic::UpdatePlayers(float deltaTime) {
    for (auto& p : m_Players) {
        if (!p.active) continue;

        p.animTimer += deltaTime;
        if (p.damageFlashTimer > 0.0f) p.damageFlashTimer -= deltaTime;
        if (p.attackCooldown > 0.0f) p.attackCooldown -= deltaTime;

        // Jump physics
        if (p.isJumping) {
            p.jumpTimer -= deltaTime;
            float progress = 1.0f - (p.jumpTimer / 0.75f);
            p.jumpOffsetY = -std::sin(progress * 3.14159f) * 45.0f;
            if (p.jumpTimer <= 0.0f) {
                p.isJumping = false;
                p.jumpOffsetY = 0.0f;
            }
        }

        // Duck physics
        if (p.isDucking) {
            p.duckTimer -= deltaTime;
            if (p.duckTimer <= 0.0f) {
                p.isDucking = false;
            }
        }

        // Ranged player shooting logic
        if (p.type == PLAYER_RANGED && p.attackCooldown <= 0.0f) {
            // Check if there is an enemy ahead in the same lane
            bool enemyAhead = false;
            for (const auto& e : m_Enemies) {
                if (e.active && e.lane == p.lane && e.x > p.x) {
                    enemyAhead = true;
                    break;
                }
            }

            if (enemyAhead) {
                Projectile proj;
                proj.x = p.x + 25.0f;
                proj.y = p.y;
                proj.vx = 280.0f;
                proj.lane = p.lane;
                proj.isFromPlayer = true;
                proj.damage = 1;
                proj.heightType = ATTACK_NORMAL;
                proj.active = true;
                m_Projectiles.push_back(proj);

                p.attackCooldown = 1.6f;
            }
        }
    }
}

void GameLogic::UpdateEnemies(float deltaTime) {
    for (auto& e : m_Enemies) {
        if (!e.active) continue;

        e.animTimer += deltaTime;
        if (e.damageFlashTimer > 0.0f) e.damageFlashTimer -= deltaTime;
        if (e.attackCooldown > 0.0f) e.attackCooldown -= deltaTime;

        // Ranged enemy attack logic (Upper Ant and Lower Cockroach)
        if (e.attackCooldown <= 0.0f) {
            if (e.type == ENEMY_EASY_UPPER) {
                // Fires upper projectile (duck to avoid)
                Projectile proj;
                proj.x = e.x - 25.0f;
                proj.y = e.y - 18.0f; // High
                proj.vx = -180.0f;
                proj.lane = e.lane;
                proj.isFromPlayer = false;
                proj.damage = 1;
                proj.heightType = ATTACK_HIGH;
                proj.active = true;
                m_Projectiles.push_back(proj);
                e.attackCooldown = 2.4f;
            } else if (e.type == ENEMY_MEDIUM_LOWER) {
                // Fires lower projectile (jump to avoid)
                Projectile proj;
                proj.x = e.x - 25.0f;
                proj.y = e.y + 18.0f; // Low
                proj.vx = -190.0f;
                proj.lane = e.lane;
                proj.isFromPlayer = false;
                proj.damage = 1;
                proj.heightType = ATTACK_LOW;
                proj.active = true;
                m_Projectiles.push_back(proj);
                e.attackCooldown = 2.2f;
            }
        }
    }
}

void GameLogic::UpdateProjectiles(float deltaTime) {
    for (auto& proj : m_Projectiles) {
        if (!proj.active) continue;

        proj.x += proj.vx * deltaTime;

        // Boundary checks
        if (proj.x < m_GridStartX || proj.x > (m_GridStartX + m_GridWidth)) {
            proj.active = false;
            continue;
        }

        if (proj.isFromPlayer) {
            // Check collision with enemies on the same lane
            for (auto& e : m_Enemies) {
                if (e.active && e.lane == proj.lane && std::abs(proj.x - e.x) < 25.0f) {
                    e.health -= proj.damage;
                    e.damageFlashTimer = 0.25f;
                    proj.active = false;
                    m_Score += 20;

                    if (e.health <= 0) {
                        e.active = false;
                        DropCollectible(e.x, e.y, e.lane);
                        m_Score += 100;
                        m_TotalDefeatedEnemies++;
                    }
                    break;
                }
            }
        } else {
            // Projectile is from enemy, flying left toward players
            for (auto& p : m_Players) {
                if (p.active && p.lane == proj.lane && std::abs(proj.x - p.x) < 25.0f) {
                    // Check if player avoided attack via jumping or ducking (Section 3.2)
                    bool dodged = false;
                    if (proj.heightType == ATTACK_HIGH && p.isDucking) {
                        dodged = true; // Duck avoided high attack
                    } else if (proj.heightType == ATTACK_LOW && p.isJumping) {
                        dodged = true; // Jump avoided low attack
                    }

                    if (!dodged) {
                        p.health -= proj.damage;
                        p.damageFlashTimer = 0.25f;
                        proj.active = false;

                        if (p.health <= 0) {
                            p.active = false;
                        }
                        break;
                    }
                }
            }
        }
    }

    // Cleanup inactive projectiles
    m_Projectiles.erase(
        std::remove_if(m_Projectiles.begin(), m_Projectiles.end(), [](const Projectile& pr) { return !pr.active; }),
        m_Projectiles.end()
    );
}

void GameLogic::UpdateCollectibles(float deltaTime) {
    for (auto& c : m_Collectibles) {
        if (c.collected) continue;
        c.animTimer += deltaTime;

        // Auto-collect if any player touches it on the same lane
        for (const auto& p : m_Players) {
            if (p.active && p.lane == c.lane && std::abs(p.x - c.x) < 40.0f) {
                c.collected = true;
                m_SpiceCoins += c.value;
                m_Score += 50;
                break;
            }
        }
    }

    // Cleanup collected
    m_Collectibles.erase(
        std::remove_if(m_Collectibles.begin(), m_Collectibles.end(), [](const Collectible& col) { return col.collected; }),
        m_Collectibles.end()
    );
}

void GameLogic::CheckCollisionsAndCombat(float deltaTime) {
    // 1. Move and encounter collisions between players and enemies
    for (auto& p : m_Players) {
        if (!p.active) continue;

        bool blockedByEnemy = false;

        for (auto& e : m_Enemies) {
            if (!e.active || e.lane != p.lane) continue;

            float dist = e.x - p.x;
            if (dist > -15.0f && dist < 42.0f) {
                // Player-Enemy encounter! Both stop moving and fight until one is defeated
                blockedByEnemy = true;
                p.isAttacking = true;
                e.isAttacking = true;

                // Player attacks enemy
                if (p.attackCooldown <= 0.0f && p.type != PLAYER_DEFENSE) {
                    e.health -= 1; // 1 point of damage per player attack
                    e.damageFlashTimer = 0.25f;
                    p.attackCooldown = 1.0f;
                    m_Score += 15;

                    if (e.health <= 0) {
                        e.active = false;
                        DropCollectible(e.x, e.y, e.lane);
                        m_Score += 100;
                        m_TotalDefeatedEnemies++;
                    }
                }

                // Enemy attacks player
                if (e.attackCooldown <= 0.0f) {
                    p.health -= e.damage;
                    p.damageFlashTimer = 0.25f;
                    e.attackCooldown = 1.1f;

                    if (p.health <= 0) {
                        p.active = false;
                    }
                }
            }
        }

        // If not blocked by enemy, advance automatically toward the right
        if (!blockedByEnemy && p.type != PLAYER_DEFENSE) {
            p.x += p.speed * deltaTime;
            p.isAttacking = false;
        }
    }

    // 2. Enemy movement and losing condition
    for (auto& e : m_Enemies) {
        if (!e.active) continue;

        bool blockedByPlayer = false;
        for (const auto& p : m_Players) {
            if (p.active && p.lane == e.lane) {
                float dist = e.x - p.x;
                if (dist > -15.0f && dist < 42.0f) {
                    blockedByPlayer = true;
                    break;
                }
            }
        }

        if (!blockedByPlayer) {
            e.x -= e.speed * deltaTime;
            e.isAttacking = false;
        }

        // Section 3.4: Lose condition
        // If at least one enemy reaches the Home Base from any lane, player loses!
        if (e.x <= (m_GridStartX + 8.0f)) {
            m_GameState = STATE_GAME_OVER;
            return;
        }
    }

    // Cleanup defeated
    m_Players.erase(
        std::remove_if(m_Players.begin(), m_Players.end(), [](const PlayerCharacter& p) { return !p.active; }),
        m_Players.end()
    );
    m_Enemies.erase(
        std::remove_if(m_Enemies.begin(), m_Enemies.end(), [](const EnemyCharacter& e) { return !e.active; }),
        m_Enemies.end()
    );
}

// ==========================================
// RENDERING IMPLEMENTATIONS
// ==========================================

void GameLogic::Render(Renderer2D& renderer) {
    // 1. Collectibles
    for (const auto& c : m_Collectibles) {
        DrawCollectible(renderer, c);
    }

    // 2. Players
    for (const auto& p : m_Players) {
        DrawPlayer(renderer, p);
    }

    // 3. Enemies
    for (const auto& e : m_Enemies) {
        DrawEnemy(renderer, e);
    }

    // 4. Projectiles
    for (const auto& proj : m_Projectiles) {
        DrawProjectile(renderer, proj);
    }

    // 5. Wave Banner Announcement
    if (m_WaveBannerTimer > 0.0f) {
        float alpha = std::min(1.0f, m_WaveBannerTimer);
        renderer.DrawQuad(m_GridStartX + 120.0f, m_GridStartY + 180.0f, 600.0f, 60.0f, glm::vec4(0.12f, 0.12f, 0.14f, 0.88f * alpha));
        renderer.DrawRectOutline(m_GridStartX + 120.0f, m_GridStartY + 180.0f, 600.0f, 60.0f, 3.0f, glm::vec4(0.95f, 0.85f, 0.25f, 0.95f * alpha));
        float textW = renderer.GetTextWidth(m_WaveBannerText, 2.2f);
        renderer.DrawText(m_WaveBannerText, m_GridStartX + 120.0f + (600.0f - textW) * 0.5f, m_GridStartY + 200.0f, 2.2f, glm::vec4(1.0f, 0.90f, 0.30f, alpha), 2.2f);
    }

    // 6. Win / Game Over Screen Overlay
    if (m_GameState == STATE_GAME_OVER) {
        // Red tinted modal overlay
        renderer.DrawQuad(m_GridStartX + 150.0f, m_GridStartY + 120.0f, 580.0f, 240.0f, glm::vec4(0.15f, 0.05f, 0.05f, 0.92f));
        renderer.DrawRectOutline(m_GridStartX + 150.0f, m_GridStartY + 120.0f, 580.0f, 240.0f, 4.0f, glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));

        renderer.DrawText("GAME OVER!", m_GridStartX + 300.0f, m_GridStartY + 150.0f, 3.2f, glm::vec4(1.0f, 0.25f, 0.25f, 1.0f), 3.0f);
        renderer.DrawText("PESTS REACHED THE HOME FRIDGE!", m_GridStartX + 200.0f, m_GridStartY + 210.0f, 1.8f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), 1.8f);
        renderer.DrawText("PRESS 'R' TO RESTART GAME", m_GridStartX + 240.0f, m_GridStartY + 270.0f, 2.0f, glm::vec4(0.95f, 0.85f, 0.25f, 1.0f), 2.0f);
    } else if (m_GameState == STATE_VICTORY) {
        // Golden green victory modal overlay
        renderer.DrawQuad(m_GridStartX + 150.0f, m_GridStartY + 120.0f, 580.0f, 240.0f, glm::vec4(0.05f, 0.15f, 0.08f, 0.92f));
        renderer.DrawRectOutline(m_GridStartX + 150.0f, m_GridStartY + 120.0f, 580.0f, 240.0f, 4.0f, glm::vec4(0.35f, 0.95f, 0.45f, 1.0f));

        renderer.DrawText("VICTORY!", m_GridStartX + 320.0f, m_GridStartY + 150.0f, 3.2f, glm::vec4(0.35f, 0.95f, 0.45f, 1.0f), 3.0f);
        renderer.DrawText("ALL 3 WAVES DEFEATED - FRIDGE SAVED!", m_GridStartX + 175.0f, m_GridStartY + 210.0f, 1.7f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), 1.8f);
        renderer.DrawText("FINAL SCORE: " + std::to_string(m_Score), m_GridStartX + 280.0f, m_GridStartY + 250.0f, 2.0f, glm::vec4(1.0f, 0.90f, 0.30f, 1.0f), 2.0f);
        renderer.DrawText("PRESS 'R' TO PLAY AGAIN", m_GridStartX + 265.0f, m_GridStartY + 295.0f, 1.8f, glm::vec4(0.95f, 0.85f, 0.25f, 1.0f), 1.8f);
    }
}

void GameLogic::DrawHealthBar(Renderer2D& renderer, float x, float y, float width, int currentHealth, int maxHealth, bool isPlayer) {
    float barH = 6.0f;
    float border = 1.0f;

    // Background
    renderer.DrawQuad(x, y, width, barH, glm::vec4(0.12f, 0.12f, 0.14f, 0.9f));
    renderer.DrawRectOutline(x, y, width, barH, border, glm::vec4(0.25f, 0.25f, 0.28f, 1.0f));

    // Fill
    float fillFrac = (float)currentHealth / (float)maxHealth;
    if (fillFrac < 0.0f) fillFrac = 0.0f;
    if (fillFrac > 1.0f) fillFrac = 1.0f;

    glm::vec4 fillColor;
    if (isPlayer) {
        fillColor = (fillFrac > 0.4f) ? glm::vec4(0.30f, 0.88f, 0.40f, 1.0f) : glm::vec4(0.95f, 0.30f, 0.20f, 1.0f);
    } else {
        fillColor = (fillFrac > 0.4f) ? glm::vec4(0.95f, 0.75f, 0.20f, 1.0f) : glm::vec4(0.90f, 0.20f, 0.20f, 1.0f);
    }

    renderer.DrawQuad(x + border, y + border, (width - 2.0f * border) * fillFrac, barH - 2.0f * border, fillColor);
}

void GameLogic::DrawPlayer(Renderer2D& renderer, const PlayerCharacter& player) {
    float drawY = player.y + player.jumpOffsetY;
    float drawX = player.x;

    // Damage flash red shading factor (Section 3.2: intensity increases as health decreases)
    float healthRatio = (float)player.health / (float)player.maxHealth;
    float redIntensity = (1.0f - healthRatio) * 0.45f;
    if (player.damageFlashTimer > 0.0f) redIntensity += 0.45f;

    glm::vec4 damageTint(redIntensity, 0.0f, 0.0f, 0.0f);
    glm::vec4 outlineColor(0.12f, 0.12f, 0.14f, 1.0f);

    // Health bar above player
    DrawHealthBar(renderer, drawX - 18.0f, drawY - 26.0f, 36.0f, player.health, player.maxHealth, true);

    if (player.type == PLAYER_MELEE) {
        // --- SPATULA MELEE DEFENDER (Clean cartoon shape as in picture) ---
        float swingAngle = player.isAttacking ? (std::sin(player.animTimer * 16.0f) * 10.0f) : 0.0f;
        float sy = drawY + swingAngle;

        glm::vec4 metalColor = glm::vec4(0.82f, 0.84f, 0.86f, 1.0f) + damageTint;
        glm::vec4 handleColor = glm::vec4(0.22f, 0.22f, 0.24f, 1.0f) + damageTint;

        // 1. Spatula Grip Handle (Left side)
        renderer.DrawQuad(drawX - 22.0f, sy - 3.5f, 18.0f, 7.0f, handleColor);
        renderer.DrawRectOutline(drawX - 22.0f, sy - 3.5f, 18.0f, 7.0f, 1.5f, outlineColor);
        // Hanging hole
        renderer.DrawCircle(glm::vec2(drawX - 18.0f, sy), 1.5f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));

        // 2. Metal Neck
        renderer.DrawQuad(drawX - 4.0f, sy - 2.0f, 8.0f, 4.0f, metalColor);
        renderer.DrawRectOutline(drawX - 4.0f, sy - 2.0f, 8.0f, 4.0f, 1.2f, outlineColor);

        // 3. Spatula Turner Blade (Rectangular head with 3 slots, facing right)
        renderer.DrawQuad(drawX + 4.0f, sy - 13.0f, 18.0f, 26.0f, metalColor);
        renderer.DrawRectOutline(drawX + 4.0f, sy - 13.0f, 18.0f, 26.0f, 2.0f, outlineColor);

        // 3 Clean vertical slots
        renderer.DrawLine(glm::vec2(drawX + 8.5f, sy - 7.0f), glm::vec2(drawX + 8.5f, sy + 7.0f), 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX + 13.0f, sy - 7.0f), glm::vec2(drawX + 13.0f, sy + 7.0f), 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX + 17.5f, sy - 7.0f), glm::vec2(drawX + 17.5f, sy + 7.0f), 2.0f, outlineColor);

    } else if (player.type == PLAYER_RANGED) {
        // --- SALT SHAKER RANGED (Clean cartoon shape as in picture) ---
        glm::vec4 shakerBody = glm::vec4(0.78f, 0.80f, 0.84f, 1.0f) + damageTint;
        glm::vec4 capColor   = glm::vec4(0.40f, 0.42f, 0.45f, 1.0f) + damageTint;

        // Horizontal cylindrical shaker body
        renderer.DrawQuad(drawX - 18.0f, drawY - 10.0f, 26.0f, 20.0f, shakerBody);
        renderer.DrawRectOutline(drawX - 18.0f, drawY - 10.0f, 26.0f, 20.0f, 2.0f, outlineColor);

        // Dark shaker cap on the right
        renderer.DrawQuad(drawX + 8.0f, drawY - 8.0f, 7.0f, 16.0f, capColor);
        renderer.DrawRectOutline(drawX + 8.0f, drawY - 8.0f, 7.0f, 16.0f, 2.0f, outlineColor);

        // Perforated hole dots
        renderer.DrawCircle(glm::vec2(drawX + 11.5f, drawY - 4.0f), 1.3f, outlineColor);
        renderer.DrawCircle(glm::vec2(drawX + 11.5f, drawY), 1.3f, outlineColor);
        renderer.DrawCircle(glm::vec2(drawX + 11.5f, drawY + 4.0f), 1.3f, outlineColor);

        // Clean pepper/salt dots inside body
        renderer.DrawCircle(glm::vec2(drawX - 8.0f, drawY - 3.0f), 1.2f, outlineColor);
        renderer.DrawCircle(glm::vec2(drawX - 3.0f, drawY + 4.0f), 1.2f, outlineColor);
        renderer.DrawCircle(glm::vec2(drawX - 11.0f, drawY + 2.0f), 1.2f, outlineColor);

    } else if (player.type == PLAYER_DEFENSE) {
        // --- BREAD LOAF SLICE DEFENSE (Clean cartoon shape as in picture) ---
        glm::vec4 crustColor = glm::vec4(0.68f, 0.42f, 0.20f, 1.0f) + damageTint;
        glm::vec4 crumbColor = glm::vec4(0.96f, 0.90f, 0.78f, 1.0f) + damageTint;

        // Bread outer crust (arched top, rectangular bottom)
        renderer.DrawQuad(drawX - 14.0f, drawY - 8.0f, 28.0f, 24.0f, crustColor);
        renderer.DrawCircle(glm::vec2(drawX - 7.0f, drawY - 8.0f), 8.0f, crustColor);
        renderer.DrawCircle(glm::vec2(drawX + 7.0f, drawY - 8.0f), 8.0f, crustColor);

        // Bread soft inner face
        renderer.DrawQuad(drawX - 11.0f, drawY - 6.0f, 22.0f, 20.0f, crumbColor);
        renderer.DrawCircle(glm::vec2(drawX - 6.0f, drawY - 6.0f), 6.0f, crumbColor);
        renderer.DrawCircle(glm::vec2(drawX + 6.0f, drawY - 6.0f), 6.0f, crumbColor);

        // Clean outline
        renderer.DrawRectOutline(drawX - 14.0f, drawY - 8.0f, 28.0f, 24.0f, 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX - 14.0f, drawY - 8.0f), glm::vec2(drawX - 7.0f, drawY - 16.0f), 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX - 7.0f, drawY - 16.0f), glm::vec2(drawX, drawY - 12.0f), 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX, drawY - 12.0f), glm::vec2(drawX + 7.0f, drawY - 16.0f), 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX + 7.0f, drawY - 16.0f), glm::vec2(drawX + 14.0f, drawY - 8.0f), 2.0f, outlineColor);
    }
}

void GameLogic::DrawEnemy(Renderer2D& renderer, const EnemyCharacter& enemy) {
    float drawX = enemy.x;
    float drawY = enemy.y;

    // Damage flash red shading factor (Section 3.3: intensity increases as health decreases)
    float healthRatio = (float)enemy.health / (float)enemy.maxHealth;
    float redIntensity = (1.0f - healthRatio) * 0.5f;
    if (enemy.damageFlashTimer > 0.0f) redIntensity += 0.5f;

    glm::vec4 damageTint(redIntensity, 0.0f, 0.0f, 0.0f);
    glm::vec4 outlineColor(0.12f, 0.12f, 0.14f, 1.0f);

    // Health bar above enemy
    DrawHealthBar(renderer, drawX - 18.0f, drawY - 26.0f, 36.0f, enemy.health, enemy.maxHealth, false);

    if (enemy.type == ENEMY_EASY_UPPER) {
        // --- EASY: BLACK ANT (Simple cartoon shape as in picture) ---
        glm::vec4 antColor = glm::vec4(0.12f, 0.12f, 0.14f, 1.0f) + damageTint;

        // 3 Simple Connected Circles: Head (left), Thorax, Abdomen (right)
        renderer.DrawCircle(glm::vec2(drawX - 13.0f, drawY), 6.0f, antColor);   // Head
        renderer.DrawCircle(glm::vec2(drawX - 2.0f, drawY), 5.0f, antColor);    // Thorax
        renderer.DrawCircle(glm::vec2(drawX + 10.0f, drawY), 8.5f, antColor);   // Abdomen

        // 2 Simple Bent Antennae
        renderer.DrawLine(glm::vec2(drawX - 15.0f, drawY - 4.0f), glm::vec2(drawX - 21.0f, drawY - 11.0f), 1.8f, antColor);
        renderer.DrawLine(glm::vec2(drawX - 21.0f, drawY - 11.0f), glm::vec2(drawX - 26.0f, drawY - 8.0f), 1.8f, antColor);

        // 6 Simple Stick Legs
        float legWag = std::sin(enemy.animTimer * 12.0f) * 3.0f;
        for (int l = -1; l <= 1; ++l) {
            float lx = drawX + (float)l * 7.0f;
            renderer.DrawLine(glm::vec2(lx, drawY), glm::vec2(lx - 4.0f, drawY + 10.0f + (float)l * legWag), 1.8f, antColor);
            renderer.DrawLine(glm::vec2(lx, drawY), glm::vec2(lx - 4.0f, drawY - 10.0f - (float)l * legWag), 1.8f, antColor);
        }

    } else if (enemy.type == ENEMY_MEDIUM_LOWER) {
        // --- MEDIUM: BROWN COCKROACH (Simple cartoon shape as in picture) ---
        glm::vec4 roachColor = glm::vec4(0.58f, 0.32f, 0.16f, 1.0f) + damageTint;
        glm::vec4 roachDark  = glm::vec4(0.20f, 0.10f, 0.05f, 1.0f);

        // Simple smooth oval shell body
        renderer.DrawQuad(drawX - 14.0f, drawY - 7.5f, 28.0f, 15.0f, roachColor);
        renderer.DrawCircle(glm::vec2(drawX - 14.0f, drawY), 7.5f, roachColor);
        renderer.DrawCircle(glm::vec2(drawX + 14.0f, drawY), 7.5f, roachColor);

        // Outline
        renderer.DrawLine(glm::vec2(drawX - 14.0f, drawY - 7.5f), glm::vec2(drawX + 14.0f, drawY - 7.5f), 2.0f, roachDark);
        renderer.DrawLine(glm::vec2(drawX - 14.0f, drawY + 7.5f), glm::vec2(drawX + 14.0f, drawY + 7.5f), 2.0f, roachDark);

        // Wing center division line
        renderer.DrawLine(glm::vec2(drawX - 16.0f, drawY), glm::vec2(drawX + 16.0f, drawY), 1.8f, roachDark);

        // 2 Simple Curved Antennae
        renderer.DrawLine(glm::vec2(drawX - 16.0f, drawY - 3.0f), glm::vec2(drawX - 30.0f, drawY - 13.0f), 1.5f, roachDark);
        renderer.DrawLine(glm::vec2(drawX - 16.0f, drawY + 3.0f), glm::vec2(drawX - 30.0f, drawY + 13.0f), 1.5f, roachDark);

        // 6 Simple Angled Legs
        for (int l = -1; l <= 1; ++l) {
            float lx = drawX + (float)l * 9.0f;
            renderer.DrawLine(glm::vec2(lx, drawY + 7.0f), glm::vec2(lx + 5.0f, drawY + 15.0f), 1.8f, roachDark);
            renderer.DrawLine(glm::vec2(lx, drawY - 7.0f), glm::vec2(lx + 5.0f, drawY - 15.0f), 1.8f, roachDark);
        }

    } else if (enemy.type == ENEMY_HARD_MELEE) {
        // --- HARD: GREY RAT WITH TOP HAT (Simple cartoon shape as in picture) ---
        glm::vec4 ratGrey = glm::vec4(0.60f, 0.62f, 0.65f, 1.0f) + damageTint;
        glm::vec4 tailColor = glm::vec4(0.85f, 0.72f, 0.70f, 1.0f);

        // Smooth grey body tapering to snout on the left
        renderer.DrawCircle(glm::vec2(drawX + 4.0f, drawY), 15.0f, ratGrey);
        renderer.DrawCircle(glm::vec2(drawX - 7.0f, drawY + 1.5f), 10.0f, ratGrey);
        // Snout
        renderer.DrawQuadVertices(
            glm::vec2(drawX - 24.0f, drawY + 3.0f),
            glm::vec2(drawX - 7.0f, drawY - 7.0f),
            glm::vec2(drawX + 4.0f, drawY),
            glm::vec2(drawX - 7.0f, drawY + 13.0f),
            ratGrey
        );
        // Outline
        renderer.DrawLine(glm::vec2(drawX - 24.0f, drawY + 3.0f), glm::vec2(drawX - 7.0f, drawY - 7.0f), 2.0f, outlineColor);
        renderer.DrawLine(glm::vec2(drawX - 24.0f, drawY + 3.0f), glm::vec2(drawX - 7.0f, drawY + 13.0f), 2.0f, outlineColor);

        // Eye dot
        renderer.DrawCircle(glm::vec2(drawX - 11.0f, drawY - 1.0f), 2.0f, outlineColor);

        // Black stylish Top Hat (as in picture)
        renderer.DrawQuad(drawX - 11.0f, drawY - 18.0f, 16.0f, 3.5f, outlineColor); // Brim
        renderer.DrawQuad(drawX - 8.0f, drawY - 28.0f, 10.0f, 10.0f, outlineColor); // Crown

        // Thin curved tail trailing backwards
        float wag = std::sin(enemy.animTimer * 10.0f) * 4.0f;
        renderer.DrawLine(glm::vec2(drawX + 18.0f, drawY + 3.0f), glm::vec2(drawX + 30.0f, drawY + 1.0f + wag), 2.0f, tailColor);
        renderer.DrawLine(glm::vec2(drawX + 30.0f, drawY + 1.0f + wag), glm::vec2(drawX + 42.0f, drawY + 10.0f), 2.0f, tailColor);
    }
}

void GameLogic::DrawProjectile(Renderer2D& renderer, const Projectile& proj) {
    if (proj.isFromPlayer) {
        // Player Salt Crystal / Chili projectile
        renderer.DrawCircle(glm::vec2(proj.x, proj.y), 4.5f, glm::vec4(0.98f, 0.98f, 1.0f, 1.0f), 8);
        renderer.DrawCircle(glm::vec2(proj.x - 6.0f, proj.y), 3.0f, glm::vec4(0.95f, 0.85f, 0.35f, 0.8f), 8);
    } else {
        if (proj.heightType == ATTACK_HIGH) {
            // Upper attack (Ant flying projectile): Aerial green venom
            renderer.DrawCircle(glm::vec2(proj.x, proj.y), 5.5f, glm::vec4(0.35f, 0.95f, 0.30f, 1.0f), 10);
            renderer.DrawCircle(glm::vec2(proj.x + 6.0f, proj.y), 3.5f, glm::vec4(0.85f, 0.95f, 0.20f, 0.8f), 8);
        } else {
            // Lower attack (Cockroach ground projectile): Ground brown/yellow sludge
            renderer.DrawQuad(proj.x - 7.0f, proj.y - 4.0f, 14.0f, 8.0f, glm::vec4(0.85f, 0.55f, 0.15f, 1.0f));
            renderer.DrawCircle(glm::vec2(proj.x, proj.y), 4.5f, glm::vec4(0.70f, 0.35f, 0.10f, 1.0f), 8);
        }
    }
}

void GameLogic::DrawCollectible(Renderer2D& renderer, const Collectible& col) {
    // Bobbing animation
    float bob = std::sin(col.animTimer * 6.0f) * 6.0f;
    float cx = col.x;
    float cy = col.y + bob;

    // Glowing aura
    renderer.DrawCircle(glm::vec2(cx, cy), 14.0f, glm::vec4(1.0f, 0.88f, 0.25f, 0.35f), 16);
    // Golden sun / spice jar
    renderer.DrawCircle(glm::vec2(cx, cy), 9.0f, glm::vec4(0.98f, 0.85f, 0.15f, 1.0f), 16);
    renderer.DrawCircle(glm::vec2(cx, cy), 5.0f, glm::vec4(1.0f, 0.98f, 0.80f, 1.0f), 12);
}
