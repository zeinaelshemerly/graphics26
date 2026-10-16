#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

enum PlayerType {
    PLAYER_NONE = -1,
    PLAYER_MELEE = 0,    // Chef / Spatula: Close range, 5 lives, cost 10
    PLAYER_RANGED = 1,   // Salt Shaker: Ranged shots, 5 lives, cost 20
    PLAYER_DEFENSE = 2   // Bread Loaf / Toaster: Blocker, 7 hits, cost 30
};

enum EnemyType {
    ENEMY_EASY_UPPER = 0,    // Ant Pest: Upper ranged attack (duck to avoid), 5 lives, 1 dmg
    ENEMY_MEDIUM_LOWER = 1,  // Cockroach: Lower ranged attack (jump to avoid), 5 lives, 1 dmg
    ENEMY_HARD_MELEE = 2     // Chef Rat: Fast close range runner, 7 lives, 2 dmg
};

enum AttackHeight {
    ATTACK_NORMAL = 0,
    ATTACK_HIGH = 1,  // Avoided by ducking
    ATTACK_LOW = 2    // Avoided by jumping
};

struct Projectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    int lane = 0;
    bool isFromPlayer = true;
    int damage = 1;
    AttackHeight heightType = ATTACK_NORMAL;
    bool active = true;
};

struct Collectible {
    float x = 0.0f;
    float y = 0.0f;
    int lane = 0;
    int value = 25; // Spice coins rewarded
    float animTimer = 0.0f;
    bool collected = false;
};

struct PlayerCharacter {
    PlayerType type = PLAYER_MELEE;
    int lane = 0;
    float x = 0.0f;
    float y = 0.0f;
    int health = 5;
    int maxHealth = 5;
    float speed = 55.0f;
    float attackCooldown = 0.0f;
    float damageFlashTimer = 0.0f;
    bool isJumping = false;
    bool isDucking = false;
    float jumpTimer = 0.0f;
    float duckTimer = 0.0f;
    float jumpOffsetY = 0.0f;
    bool isAttacking = false;
    float animTimer = 0.0f;
    bool active = true;
};

struct EnemyCharacter {
    EnemyType type = ENEMY_EASY_UPPER;
    int lane = 0;
    float x = 0.0f;
    float y = 0.0f;
    int health = 5;
    int maxHealth = 5;
    float speed = 45.0f;
    int damage = 1;
    float attackCooldown = 0.0f;
    float damageFlashTimer = 0.0f;
    bool isAttacking = false;
    float animTimer = 0.0f;
    bool active = true;
};

enum GameState {
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_VICTORY
};
