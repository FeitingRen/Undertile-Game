#ifndef PLAYER_CLASS_H
#define PLAYER_CLASS_H

#include "Globals.h" 
#include "game_defs.h"

class Player {
public:
    // variables
    float x, y, oldX, oldY;
    float speed = 2.0; 
    Rect* zones = nullptr;
    int zoneCount = 0;

    // functions
    void init(int startX, int startY);
    void setZones(Rect* newZones, int count);
    int hp; 
    bool isWalkable(int px, int py);
    bool checkCollision(float newX, float newY, int objX, int objY, int objW, int objH);
    void forceDraw(const uint16_t* bgMap = nullptr);
    void update(NPC* enemy = nullptr);
    void draw(const uint16_t* bgMap = nullptr);
};

extern Player player; 

#endif