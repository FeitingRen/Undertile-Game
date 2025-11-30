#include "Player.h"
#include "characters.h" 
#include "Utils.h"      

Player player; 

void Player::init(int startX, int startY) { x = startX; y = startY; oldX = x; oldY = y; }

void Player::setZones(Rect* newZones, int count) { zones = newZones; zoneCount = count; }

bool Player::isWalkable(int px, int py) {
      if (zoneCount == 0) return true; 
      for(int i=0; i<zoneCount; i++) {
          if (px >= zones[i].x && px <= zones[i].x + zones[i].w &&
              py >= zones[i].y && py <= zones[i].y + zones[i].h) return true;
      }
      return false;
}

bool Player::checkCollision(float newX, float newY, int objX, int objY, int objW, int objH) {
    return (newX < objX + objW && newX + PLAYER_W > objX && newY < objY + objH && newY + PLAYER_H > objY);
}

void Player::update(NPC* enemy) {
    oldX = x; oldY = y;
    float nextX = x, nextY = y;
    
    // Read Input
    for (int i=0; i<LIST_MAX; i++) {
        if (customKeypad.key[i].kstate == PRESSED || customKeypad.key[i].kstate == HOLD) {
            char k = customKeypad.key[i].kchar;
            if (k == 'L') nextX -= speed;
            if (k == 'R') nextX += speed;
            if (k == 'U') nextY -= speed;
            if (k == 'D') nextY += speed;
        }
    }

    // FIX ISSUE 1: Strict Bounds for Battle Mode
    if (currentState == BATTLE && zones != nullptr && zoneCount > 0) {
        // In Battle, zones[0] is always the currentBox.
        Rect box = zones[0];

        // STRICT CLAMPING: Ensure the ENTIRE sprite stays inside the box.
        // We do not use isWalkable() here because isWalkable checks for "feet", 
        // which allows the body to clip into walls.
        
        // Clamp X
        if (nextX < box.x) nextX = box.x;
        if (nextX + PLAYER_W > box.x + box.w) nextX = box.x + box.w - PLAYER_W;
        
        // Clamp Y
        if (nextY < box.y) nextY = box.y;
        if (nextY + PLAYER_H > box.y + box.h) nextY = box.y + box.h - PLAYER_H;

        x = nextX;
        y = nextY;
    } 
    else {
        // Standard Map Movement (Original Logic)
        int feetX = (int)nextX + (PLAYER_W / 2); int feetY = (int)y + PLAYER_H;
        if (isWalkable(feetX, feetY)) { if (nextX >= 0 && nextX <= SCREEN_W - PLAYER_W) x = nextX; }
        nextY = (nextY == y) ? y : nextY; 
        feetX = (int)x + (PLAYER_W / 2); feetY = (int)nextY + PLAYER_H;
        if (isWalkable(feetX, feetY)) { if (nextY >= 0 && nextY <= SCREEN_H - PLAYER_H) y = nextY; }
    }

    // FIX: currentState is now visible via Globals.h
    if (currentState == MAP_WALK && enemy != nullptr) {
       if (checkCollision(x, y, enemy->x, enemy->y, NPC_SIZE, NPC_SIZE)) { x = oldX; y = oldY; }
    }
}

void Player::draw(const uint16_t* bgMap) {
    if ((int)x != (int)oldX || (int)y != (int)oldY) {
      if (bgMap != nullptr) {
        int rx = (int)oldX; int ry = (int)oldY;
        for(int row = 0; row < PLAYER_H; row++) {
           int currentY = ry + row;
           if(currentY >= 0 && currentY < SCREEN_H) {
              tft.drawRGBBitmap(rx, currentY, (uint16_t*)(bgMap + (currentY * SCREEN_W) + rx), PLAYER_W, 1);
           }
        }
        // FIX: drawSpriteMixed is now visible via Utils.h
        drawSpriteMixed((int)x, (int)y, heart_sprite, PLAYER_W, PLAYER_H, bgMap);
      } else {
        if (x > oldX) tft.fillRect((int)oldX, (int)oldY, (int)x - (int)oldX, PLAYER_H, ST7735_BLACK);
        else if (x < oldX) tft.fillRect((int)x + PLAYER_W, (int)oldY, (int)oldX - (int)x, PLAYER_H, ST7735_BLACK);
        if (y > oldY) tft.fillRect((int)oldX, (int)oldY, PLAYER_W, (int)y - (int)oldY, ST7735_BLACK);
        else if (y < oldY) tft.fillRect((int)oldX, (int)y + PLAYER_H, PLAYER_W, (int)oldY - (int)y, ST7735_BLACK);
        tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite_blk, PLAYER_W, PLAYER_H);
      }
    }
}

void Player::forceDraw(const uint16_t* bgMap) {
    if (bgMap != nullptr) drawSpriteMixed((int)x, (int)y, heart_sprite, PLAYER_W, PLAYER_H, bgMap);
    else tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite_blk, PLAYER_W, PLAYER_H);
}