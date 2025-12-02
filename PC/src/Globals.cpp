#include "Globals.h"
#include <cstdio>

Texture2D texBackground;
Texture2D texPlayer;
Texture2D texRobot;

Sound sndText;
Sound sndHurt;
Sound sndDialup[6];

GameState currentState = MENU;
Font gameFont;

void LoadGameAssets() {
    // Graphics
    texBackground = LoadTexture("../assets/background.png");
    texPlayer = LoadTexture("../assets/heart.png");
    texRobot = LoadTexture("../assets/robot.png");

    // Audio
    sndText = LoadSound("../assets/text.wav");
    sndHurt = LoadSound("../assets/hurt.wav");
    
    char buffer[64];
    for(int i=0; i<6; i++) {
        sprintf(buffer, "../assets/dialup%d.wav", i);
        sndDialup[i] = LoadSound(buffer);
    }

    gameFont = GetFontDefault(); // Uses default raylib font
}

void UnloadGameAssets() {
    UnloadTexture(texBackground);
    UnloadTexture(texPlayer);
    UnloadTexture(texRobot);
    
    UnloadSound(sndText);
    UnloadSound(sndHurt);
    for(int i=0; i<6; i++) UnloadSound(sndDialup[i]);
}