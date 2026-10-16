#pragma once

#include "GameTypes.h"
#include "Renderer2D.h"
#include <vector>

class GameLogic {
public:
    static constexpr int TOTAL_WAVES = 3;
    static constexpr int TOTAL_GAME_ENEMIES = 18; // 3 + 5 + 10

    GameLogic();

    void Init(float gridStartX, float gridStartY, float gridWidth, float gridHeight, float laneHeight);
    void Reset();
    void Update(float deltaTime, const bool selectedLanes[5]);
    void Render(Renderer2D& renderer);

    // Card & Purchasing
    bool IsCardUnlocked(PlayerType type) const;
    int GetCardCost(PlayerType type) const;
    void SelectCard(PlayerType type);
    PlayerType GetSelectedCard() const { return m_SelectedCard; }
    void TryDeployPlayer(int lane);

    // Controls
    void TriggerJumpForLanes(const bool selectedLanes[5]);
    void TriggerDuckForLanes(const bool selectedLanes[5]);
    void OnClickAt(float x, float y);

    // Queries
    int GetScore() const { return m_Score; }
    int GetSpice() const { return m_SpiceCoins; }
    int GetCurrentWave() const { return m_CurrentWave; }
    GameState GetGameState() const { return m_GameState; }
    float GetLevelProgress() const;
    const std::string& GetWaveAnnouncement() const { return m_WaveBannerText; }
    float GetWaveBannerTimer() const { return m_WaveBannerTimer; }

private:
    void UpdateWaveSpawner(float deltaTime);
    void UpdatePlayers(float deltaTime);
    void UpdateEnemies(float deltaTime);
    void UpdateProjectiles(float deltaTime);
    void UpdateCollectibles(float deltaTime);
    void CheckCollisionsAndCombat(float deltaTime);

    void SpawnEnemy(EnemyType type, int lane);
    void DropCollectible(float x, float y, int lane);

    // Rendering Helpers
    void DrawPlayer(Renderer2D& renderer, const PlayerCharacter& player);
    void DrawEnemy(Renderer2D& renderer, const EnemyCharacter& enemy);
    void DrawHealthBar(Renderer2D& renderer, float x, float y, float width, int currentHealth, int maxHealth, bool isPlayer);
    void DrawProjectile(Renderer2D& renderer, const Projectile& proj);
    void DrawCollectible(Renderer2D& renderer, const Collectible& col);

    // Layout
    float m_GridStartX = 245.0f;
    float m_GridStartY = 95.0f;
    float m_GridWidth  = 1010.0f;
    float m_GridHeight = 550.0f;
    float m_LaneHeight = 110.0f;

    // Economy & Cards
    int m_SpiceCoins = 30; // Starts with 30 coins as required by spec (enough for 3 Melee players)
    int m_Score = 0;
    PlayerType m_SelectedCard = PLAYER_NONE;

    // Entities
    std::vector<PlayerCharacter> m_Players;
    std::vector<EnemyCharacter>  m_Enemies;
    std::vector<Projectile>      m_Projectiles;
    std::vector<Collectible>     m_Collectibles;

    // Wave Spawner
    int m_CurrentWave = 1;
    int m_EnemiesSpawnedInWave = 0;
    int m_EnemiesToSpawnInWave = 3;
    float m_SpawnCooldown = 2.0f;
    float m_WaveBannerTimer = 3.5f;
    std::string m_WaveBannerText = "WAVE 1 - PEST INVASION!";
    int m_TotalDefeatedEnemies = 0;

    // Game state
    GameState m_GameState = STATE_PLAYING;
    float m_GlobalTimer = 0.0f;
};
