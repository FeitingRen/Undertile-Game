#include "Globals.h"
#include <cstdio>

Texture2D texBackground;
Texture2D texPlayer;
Texture2D texRobot;

Sound sndText;
Sound sndHurt;
Sound sndSelect;
Sound sndDialup[6];

Music battleBGMusic;
Music gameOver;
Music menuMusic;

GameState currentState = MENU;
Font gameFont;

// Default to English (or whatever you prefer)
Language currentLanguage = LANG_EN;
Font fontEN;
Font fontCN;

void LoadGameAssets()
{
    // Graphics
    texBackground = LoadTexture("assets/background.png");
    texPlayer = LoadTexture("assets/heart.png");
    texRobot = LoadTexture("assets/robot.png");

    // Audio
    sndText = LoadSound("assets/text.wav");
    sndHurt = LoadSound("assets/hurt.wav");
    sndSelect = LoadSound("assets/select.wav");

    char buffer[64];
    for (int i = 0; i < 6; i++)
    {
        sprintf(buffer, "assets/dialup%d.wav", i);
        sndDialup[i] = LoadSound(buffer);
    }

    gameFont = GetFontDefault(); // Uses default raylib font

    battleBGMusic = LoadMusicStream("assets/battleBGMusic.ogg");
    gameOver = LoadMusicStream("assets/gameOver.ogg");
    menuMusic = LoadMusicStream("assets/menu.ogg");

    // This tells Raylib to automatically loop it when it reaches the end
    battleBGMusic.looping = true;
    gameOver.looping = true;
    menuMusic.looping = true;
}

void UnloadGameAssets()
{
    UnloadTexture(texBackground);
    UnloadTexture(texPlayer);
    UnloadTexture(texRobot);

    UnloadSound(sndText);
    UnloadSound(sndHurt);
    UnloadSound(sndSelect);
    for (int i = 0; i < 6; i++)
        UnloadSound(sndDialup[i]);

    UnloadMusicStream(battleBGMusic);
    UnloadMusicStream(gameOver);
    UnloadMusicStream(menuMusic);
}

Font GetCurrentFont()
{
    if (currentLanguage == LANG_CN)
        return fontCN;
    return fontEN;
}

// This is the Magic Function to merge your battles
const char *Text(const char *en, const char *cn)
{
    if (currentLanguage == LANG_CN)
        return cn;
    return en;
}