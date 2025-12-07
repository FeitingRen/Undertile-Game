#ifndef PLAYER_CLASS_H
#define PLAYER_CLASS_H

#include "game_defs.h"

class Player
{
public:
    Vector2 pos;
    float speed = 350.0f; // Pixels per second
    int hp;

    // Collision zones
    std::vector<Rect> zones;

    void Init(int startX, int startY);
    void SetZones(const std::vector<Rect> &newZones);
    void Update(float dt, NPC *enemy = nullptr);
    void Draw();

    // Bounds checking
    bool CheckCollision(Vector2 nextPos, int w, int h);
};

extern Player player;

#endif