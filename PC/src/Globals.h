#ifndef GLOBALS_H
#define GLOBALS_H

#include "game_defs.h"

// Assets
extern Texture2D texBackground;
extern Texture2D texPlayer; // Heart
extern Texture2D texRobot;  // NPC

// Audio
extern Sound sndText;
extern Sound sndHurt;
extern Sound sndDialup[6]; // 0-5

// Global State
extern GameState currentState;
extern Font gameFont; // Default or custom font

// Functions to load/unload
void LoadGameAssets();
void UnloadGameAssets();

#endif