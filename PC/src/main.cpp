#include "raylib.h"
#include "game_defs.h"
#include "Globals.h"
#include "Utils.h"
#include "Player.h"
#include "Battle.h"
#include <vector>

// --- MAP DATA ---
std::vector<Rect> walkableFloors = {
    { 105, 200, 610, 50 }, { 105, 255, 545, 215 }, { 50, 410, 55, 10 },
    { 55, 395, 45, 10 }, { 65, 370, 40, 20 }, { 60, 375, 35, 10 },
    { 75, 340, 30, 10 }, { 85, 330, 20, 5 }, { 90, 315, 15, 10 },
    { 90, 305, 10, 5 }, { 95, 290, 5, 10 }, { 650, 345, 35, 125 }, { 685, 410, 30, 60 }
};

NPC mapEnemy = {425, 280};
int storyProgress = 0;

// --- STATE HANDLERS ---

void HandleMenu() {
    DrawText("UNDERTALE PORT", 150, 200, 50, WHITE);
    DrawText("Press Z to Start", 150, 400, 50, GRAY);

    if (IsInteractPressed()) {
        currentState = MAP_WALK;
        player.Init(125, 300);
        player.SetZones(walkableFloors);
    }
}

void HandleMap() {
    float dt = GetFrameTime();
    
    // Update
    player.Update(dt, &mapEnemy);

    // Interaction Check
    float dist = (float)sqrt(pow(player.pos.x - mapEnemy.x, 2) + pow(player.pos.y - mapEnemy.y, 2));

    if (dist < 20 && IsInteractPressed()) {
        if (!battleCompleted) {
            preBattleX = player.pos.x;
            preBattleY = player.pos.y;
            currentState = DIALOGUE;
            globalTypewriter.Start("* I am a robot.\n* Do you like coffee?", 30);
        } else {
            globalTypewriter.Start("* Keep moving forward.", 30);
            currentState = DIALOGUE;
        }
    }

    // Draw
    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();
}

void HandleDialogue() {
    // Draw Map underneath
    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();

    // Draw Dialogue Box
    Rectangle box = {5, 90, 150, 35};
    DrawRectangleRec(box, BLACK);
    DrawRectangleLinesEx(box, 1, WHITE);

    globalTypewriter.Update();
    globalTypewriter.Draw(box.x + 5, box.y + 5);

    if (globalTypewriter.IsFinished() && IsInteractPressed()) {
        if (!battleCompleted) {
            currentState = BATTLE;
            InitBattle();
        } else {
            currentState = MAP_WALK;
        }
    }
}

void HandleGameOver() {
    DrawText("GAME OVER", 50, 50, 10, RED);
    DrawText("Press Z to Retry", 40, 70, 10, WHITE);
    
    if (IsInteractPressed()) {
        currentState = BATTLE;
        InitBattle();
    }
}

int main() {
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Undertale ESP32 Port");
    InitAudioDevice();
    SetTargetFPS(60);

    // Load Resources
    LoadGameAssets();

    // Virtual Screen
    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    player.Init(25, 60);
    player.SetZones(walkableFloors);

    while (!WindowShouldClose()) {
        // --- UPDATE & DRAW on Virtual Screen ---
        BeginTextureMode(target);
            ClearBackground(BLACK);
            
            switch (currentState) {
                case MENU: HandleMenu(); break;
                case MAP_WALK: HandleMap(); break;
                case DIALOGUE: HandleDialogue(); break;
                case BATTLE: 
                    UpdateBattle();
                    DrawBattle();
                    break;
                case GAME_OVER: HandleGameOver(); break;
            }
            
        EndTextureMode();

        // --- DRAW SCALED ---
        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle srcRec = {0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height};
            Rectangle dstRec = {0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()};
            Vector2 origin = {0.0f, 0.0f};
            DrawTexturePro(target.texture, srcRec, dstRec, origin, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadGameAssets();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}