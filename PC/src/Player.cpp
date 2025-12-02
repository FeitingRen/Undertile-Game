#include "Player.h"
#include "Globals.h"

Player player;

void Player::Init(int startX, int startY) {
    pos = {(float)startX, (float)startY};
    hp = PLAYER_MAX_HP;
}

void Player::SetZones(const std::vector<Rect>& newZones) {
    zones = newZones;
}

bool Player::CheckCollision(Vector2 nextPos, int w, int h) {
    // If no zones, free movement (or restrict to screen)
    if (zones.empty()) return true;

    // Check feet position against defined zones
    int feetX = (int)nextPos.x + (w / 2);
    int feetY = (int)nextPos.y + h;

    for (const auto& box : zones) {
        if (feetX >= box.x && feetX <= box.x + box.w &&
            feetY >= box.y && feetY <= box.y + box.h) {
            return true;
        }
    }
    return false;
}

void Player::Update(float dt, NPC* enemy) {
    Vector2 nextPos = pos;

    if (IsKeyDown(KEY_LEFT)) nextPos.x -= speed * dt;
    if (IsKeyDown(KEY_RIGHT)) nextPos.x += speed * dt;
    if (IsKeyDown(KEY_UP)) nextPos.y -= speed * dt;
    if (IsKeyDown(KEY_DOWN)) nextPos.y += speed * dt;

    // Map Walk Collision
    if (currentState == MAP_WALK) {
        // Horizontal Check
        if (CheckCollision({nextPos.x, pos.y}, PLAYER_W, PLAYER_H)) {
            // Keep screen bounds
            if (nextPos.x >= 0 && nextPos.x <= GAME_WIDTH - PLAYER_W) pos.x = nextPos.x;
        }
        // Vertical Check
        if (CheckCollision({pos.x, nextPos.y}, PLAYER_W, PLAYER_H)) {
            if (nextPos.y >= 0 && nextPos.y <= GAME_HEIGHT - PLAYER_H) pos.y = nextPos.y;
        }

        // Enemy Collision (Simple box check)
        if (enemy != nullptr) {
            Rectangle pRect = {pos.x, pos.y, PLAYER_W, PLAYER_H};
            Rectangle eRect = {enemy->x, enemy->y, NPC_SIZE, NPC_SIZE};
            if (CheckCollisionRecs(pRect, eRect)) {
                // Revert if colliding with enemy
                pos.x -= (nextPos.x - pos.x); 
                pos.y -= (nextPos.y - pos.y);
            }
        }
    }
    // Battle Box Collision (Strict Containment)
    else if (currentState == BATTLE && !zones.empty()) {
        Rect box = zones[0]; // In battle, zones[0] is the bounding box
        
        // Clamp X
        if (nextPos.x < box.x) nextPos.x = box.x;
        if (nextPos.x + PLAYER_W > box.x + box.w) nextPos.x = box.x + box.w - PLAYER_W;
        
        // Clamp Y
        if (nextPos.y < box.y) nextPos.y = box.y;
        if (nextPos.y + PLAYER_H > box.y + box.h) nextPos.y = box.y + box.h - PLAYER_H;

        pos = nextPos;
    }
}

void Player::Draw() {
    // Draw Texture
    DrawTexture(texPlayer, (int)pos.x, (int)pos.y, WHITE);
}