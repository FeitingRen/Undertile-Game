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
extern Sound sndSelect;
extern Sound sndDialup[6]; // 0-5
extern Music battleBGMusic;
extern Music gameOver;
extern Music menuMusic;

// Global State
extern GameState currentState;
extern Font gameFont; // Default or custom font

enum Language
{
    LANG_EN,
    LANG_CN
};

extern Language currentLanguage;
extern Font fontEN;
extern Font fontCN;

// Helper to get the correct font based on current settings
Font GetCurrentFont();

// Helper to pick text based on language
const char *Text(const char *en, const char *cn);

// Functions to load/unload
void LoadGameAssets();
void UnloadGameAssets();

#endif