#include "Player.h"
#include "Globals.h"

Player player;

void Player::Init(int startX, int startY)
{
    pos = {(float)startX, (float)startY};
    hp = PLAYER_MAX_HP;
}

void Player::SetZones(const std::vector<Rect> &newZones)
{
    zones = newZones;
}

bool Player::CheckCollision(Vector2 nextPos, int w, int h)
{
    // If no zones, free movement (or restrict to screen)
    if (zones.empty())
        return true;

    // Check feet position against defined zones
    int feetX = (int)nextPos.x + (w / 2);
    int feetY = (int)nextPos.y + h;

    for (const auto &box : zones)
    {
        if (feetX >= box.x && feetX <= box.x + box.w &&
            feetY >= box.y && feetY <= box.y + box.h)
        {
            return true;
        }
    }
    return false;
}

void Player::Update(float dt, NPC *enemy)
{
    // 1. Save where we were BEFORE moving
    Vector2 originalPos = pos;
    Vector2 nextPos = pos;

    if (IsKeyDown(KEY_LEFT))
        nextPos.x -= speed * dt;
    if (IsKeyDown(KEY_RIGHT))
        nextPos.x += speed * dt;
    if (IsKeyDown(KEY_UP))
        nextPos.y -= speed * dt;
    if (IsKeyDown(KEY_DOWN))
        nextPos.y += speed * dt;

    // Map Walk Collision
    if (currentState == MAP_WALK)
    {
        // Horizontal Check
        if (CheckCollision({nextPos.x, pos.y}, PLAYER_W, PLAYER_H))
        {
            // Keep screen bounds
            if (nextPos.x >= 0 && nextPos.x <= GAME_WIDTH - PLAYER_W)
            {
                pos.x = nextPos.x;
            }
        }
        // Vertical Check
        if (CheckCollision({pos.x, nextPos.y}, PLAYER_W, PLAYER_H))
        {
            if (nextPos.y >= 0 && nextPos.y <= GAME_HEIGHT - PLAYER_H)
            {
                pos.y = nextPos.y;
            }
        }

        // --- ENEMY COLLISION FIX ---
        if (enemy != nullptr)
        {
            // Define the hitboxes
            // We shrink the boxes slightly (by 10 pixels) than the Sprite actual size so you don't get stuck on invisible corners
            // Player rect
            Rectangle pRect = {pos.x + 10, pos.y + 10, PLAYER_W - 10, PLAYER_H - 10}; // Focus on feet
            // Enemy rect
            Rectangle eRect = {enemy->x + 10, enemy->y + 10, NPC_SIZE - 10, NPC_SIZE - 10};

            if (CheckCollisionRecs(pRect, eRect))
            {
                // We hit the enemy! Go back to where we started this frame.
                pos = originalPos;
            }
        }
    }
    // Battle Box Collision (Strict Containment)
    else if (currentState == BATTLE && !zones.empty())
    {
        Rect box = zones[0]; //// In battle, zones[0] is the bounding box

        // FIX: Add offset for the white border thickness
        // In Battle.cpp, DrawRectangleLinesEx uses a thickness of 4.0f.
        float borderOffset = 4.0f;

        // Clamp X
        // Prevent going left past the border
        if (nextPos.x < box.x + borderOffset)
        {
            nextPos.x = box.x + borderOffset;
        }
        // Prevent going right past the border
        if (nextPos.x + PLAYER_W > box.x + box.w - borderOffset)
        {
            nextPos.x = box.x + box.w - PLAYER_W - borderOffset;
        }

        // Clamp Y
        // Prevent going up past the border
        if (nextPos.y < box.y + borderOffset)
        {
            nextPos.y = box.y + borderOffset;
        }
        // Prevent going down past the border
        if (nextPos.y + PLAYER_H > box.y + box.h - borderOffset)
        {
            nextPos.y = box.y + box.h - PLAYER_H - borderOffset;
        }

        pos = nextPos;
    }
}

void Player::Draw()
{
    // Draw Texture
    DrawTexture(texPlayer, (int)pos.x, (int)pos.y, WHITE);
}