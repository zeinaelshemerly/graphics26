#pragma once

#include "Renderer2D.h"
#include <vector>
#include <string>

// ============================================================================
// Person 2 — Game World & Progression
// Requirements:
//   * Draw the grid, lanes, and home base         (Playfield.cpp)
//   * Draw the background and environmental objects (Playfield.cpp)
//   * Implement enemy spawning and waves
//   * Handle collectibles and coin rewards
//   * Implement purchasing and unlocking characters
//   * Handle winning and losing conditions
//   * Implement level progression and difficulty
// ============================================================================

// ---------------------------------------------------------------------------
// Enumerations — owned by Person 2
// ---------------------------------------------------------------------------

enum P2PlayerType {
    P2_PLAYER_NONE    = -1,
    P2_PLAYER_MELEE   =  0,   // Melee Defender (Spatula / Chef): Cost 10 coins
    P2_PLAYER_RANGED  =  1,   // Ranged Attacker (Salt Shaker):   Cost 20 coins
    P2_PLAYER_DEFENSE =  2    // Defense Blocker (Bread / Toaster): Cost 30 coins
};

enum P2EnemyTier {
    P2_ENEMY_EASY   = 0,   // Wave 1+: Normal speed, easy pest
    P2_ENEMY_MEDIUM = 1,   // Wave 2+: Faster, medium pest
    P2_ENEMY_HARD   = 2    // Wave 3:  Fastest, hard melee runner
};

enum P2GameStatus {
    P2_STATUS_PLAYING,
    P2_STATUS_GAME_OVER,
    P2_STATUS_VICTORY
};

// ---------------------------------------------------------------------------
// Data Structs — owned by Person 2
// ---------------------------------------------------------------------------

struct P2Enemy {
    P2EnemyTier tier  = P2_ENEMY_EASY;
    int   lane        = 0;
    float x           = 0.0f;
    float y           = 0.0f;
    float speed       = 40.0f;
    int   health      = 5;
    int   maxHealth   = 5;
    float animTimer   = 0.0f;
    bool  active      = true;
};

struct P2Player {
    P2PlayerType type  = P2_PLAYER_MELEE;
    int   lane         = 0;
    float x            = 0.0f;
    float y            = 0.0f;
    float speed        = 50.0f;
    int   health       = 5;
    int   maxHealth    = 5;
    bool  active       = true;
};

struct P2Collectible {
    float x          = 0.0f;
    float y          = 0.0f;
    int   lane       = 0;
    int   value      = 25;   // Spice-coin reward when collected
    float animTimer  = 0.0f;
    bool  collected  = false;
};

// ---------------------------------------------------------------------------
// GameWorld — Person 2's core game-world class
// ---------------------------------------------------------------------------
class GameWorld {
public:
    static constexpr int TOTAL_WAVES            = 3;
    static constexpr int TOTAL_ENEMIES_ALL_WAVES = 18; // Wave1(3) + Wave2(5) + Wave3(10)

    GameWorld();

    // Lifecycle
    void Init(float gridStartX, float gridStartY,
              float gridWidth,  float gridHeight, float laneHeight);
    void Reset();
    void Update(float deltaTime);

    // Rendering (Person 2 only — simple shape characters)
    void RenderEntities(Renderer2D& renderer);

    // --- Task: Handle collectibles and coin rewards ---
    int  GetSpiceCoins() const        { return m_SpiceCoins; }
    void AddSpiceCoins(int amount);
    void SpawnCollectible(float x, float y, int lane, int value = 25);
    bool TryCollectAt(float mouseX, float mouseY);

    // --- Task: Implement purchasing and unlocking characters ---
    bool         IsCharacterUnlocked(P2PlayerType type) const;
    int          GetCharacterCost(P2PlayerType type)    const;
    bool         CanPurchase(P2PlayerType type)         const;
    bool         PurchaseAndDeploy(P2PlayerType type, int lane);
    void         SelectCard(P2PlayerType type);
    P2PlayerType GetSelectedCard() const { return m_SelectedCard; }

    // --- Task: Implement level progression and difficulty ---
    int               GetCurrentWave()            const { return m_CurrentWave; }
    float             GetLevelProgress()           const;
    const std::string& GetWaveAnnouncement()       const { return m_WaveBannerText; }
    float             GetWaveAnnouncementTimer()   const { return m_WaveBannerTimer; }
    int               GetScore()                   const { return m_Score; }

    // --- Task: Handle winning and losing conditions ---
    P2GameStatus GetStatus()  const { return m_Status; }
    bool         HasLost()    const { return m_Status == P2_STATUS_GAME_OVER; }
    bool         HasWon()     const { return m_Status == P2_STATUS_VICTORY; }

    // Read-only entity access (for wave-banner rendering)
    const std::vector<P2Enemy>&       GetEnemies()       const { return m_Enemies; }
    const std::vector<P2Player>&      GetPlacedPlayers() const { return m_PlacedPlayers; }
    const std::vector<P2Collectible>& GetCollectibles()  const { return m_Collectibles; }

private:
    // Internal update helpers
    void UpdateWaveSpawner(float deltaTime);
    void UpdateEnemyMovement(float deltaTime);
    void UpdatePlayerMovement(float deltaTime);
    void UpdateCollectibles(float deltaTime);
    void CheckWinningAndLosing();

    void SpawnWaveEnemy(P2EnemyTier tier, int lane);

    // Rendering helpers
    void DrawPlayerShape(Renderer2D& renderer, const P2Player& p);
    void DrawEnemyShape(Renderer2D& renderer,  const P2Enemy&  e);
    void DrawHealthBar(Renderer2D& renderer,
                       float x, float y, float w,
                       int health, int maxHealth, bool isPlayer);

    // Layout (set by Init)
    float m_GridStartX = 245.0f;
    float m_GridStartY =  95.0f;
    float m_GridWidth  = 1010.0f;
    float m_GridHeight =  550.0f;
    float m_LaneHeight =  110.0f;

    // --- Wave / progression state ---
    int   m_CurrentWave           = 1;
    int   m_EnemiesSpawnedInWave  = 0;
    int   m_EnemiesToSpawnInWave  = 3;   // Wave 1: 3 enemies
    float m_SpawnCooldown         = 2.5f;
    float m_WaveBannerTimer       = 3.5f;
    std::string m_WaveBannerText  = "WAVE 1 - PESTS ON THE COUNTER!";
    int   m_TotalDefeatedEnemies  = 0;

    // --- Economy ---
    int          m_SpiceCoins   = 30;          // Starts with 30 coins
    int          m_Score        = 0;
    P2PlayerType m_SelectedCard = P2_PLAYER_NONE;

    // --- Win/Lose status ---
    P2GameStatus m_Status      = P2_STATUS_PLAYING;
    float        m_GlobalTimer = 0.0f;

    // --- Entity lists ---
    std::vector<P2Enemy>       m_Enemies;
    std::vector<P2Player>      m_PlacedPlayers;
    std::vector<P2Collectible> m_Collectibles;
};
