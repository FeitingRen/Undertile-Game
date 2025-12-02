#include "raylib.h"
#include "game_defs.h" 
#include "Globals.h"
#include "Utils.h"
#include "Player.h"
#include "Battle.h"
#include <vector>
#include <string>
#include <cmath>

// --- GLOBAL VARIABLES ---

// 1. currentState is defined in Globals.cpp, so we don't define it here.
// 2. currentDialogueState is NOT in Globals.cpp, so we MUST define it here.
DialogueState currentDialogueState = D_INTRO_1;

// 3. playerInventory is NOT in Globals.cpp, so we define it here.
Inventory playerInventory = {true, true, true};

// Map Data
std::vector<Rect> walkableFloors = {
    // top section
    {135,201,611,38}, {135,259,558,43}, 
    // middle 
    {134,302,510,29},{125,331,528,30},{114,361,579,25},
    // bottom section 
    {104,386,642,34},{147,420,599,51}
};

NPC mapEnemy = {425, 280};

// Game Logic Globals
int storyProgress = 0;

// These are defined in Battle.cpp (according to your previous linker error), 
// so we declare them as extern here to access them without creating duplicates.
extern bool battleCompleted;
extern float preBattleX;
extern float preBattleY;

float interactionCooldown = 0.0f;

// Dialogue Helpers
bool isStateFirstFrame = true;
float dialogTimer = 0.0f;
int menuSelection = 0;
int playerChoiceYesNo = 0;

// Coffee Event Specifics
int coffeeScriptStep = 0;
float coffeeTimer = 0.0f;
Color bgColor = BLACK; 

// --- INPUT HELPERS ---

// IsInteractPressed() is in Utils.cpp

bool IsLeftPressed() {
    return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
}

bool IsRightPressed() {
    return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
}

// --- STATE HANDLERS ---

void HandleMenu() {
    DrawText("UNDERTALE ESP32 PORT", 150, 200, 40, WHITE);
    DrawText("Press Z to Start", 150, 400, 30, GRAY);

    if (IsInteractPressed()) {
        currentState = MAP_WALK;
        player.Init(125, 300); 
        player.SetZones(walkableFloors);
        storyProgress = 0;
        isStateFirstFrame = true;
    }
}

void HandleMap() {
    float dt = GetFrameTime();
    
    if (interactionCooldown > 0) interactionCooldown -= dt;

    player.Update(dt, &mapEnemy);

    // Interaction Check
    float dist = (float)sqrt(pow(player.pos.x - mapEnemy.x, 2) + pow(player.pos.y - mapEnemy.y, 2));

    if (dist < 100 && IsInteractPressed() && interactionCooldown <= 0) {
        currentState = DIALOGUE;
        
        if (battleCompleted) currentDialogueState = D_POST_BATTLE;
        else if (storyProgress == 0) currentDialogueState = D_INTRO_1; 
        else if (storyProgress == 1) currentDialogueState = D_REQUEST_FOOD_PART1; 
        else currentDialogueState = D_REQUEST_FOOD; 
        
        isStateFirstFrame = true;
    }

    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();
}

void StartDialogue(const char* text, int speed, float waitTime) {
    globalTypewriter.Start(text, speed);
    dialogTimer = waitTime;
    isStateFirstFrame = false;
}

void HandleCoffeeEvent() {
    float dt = GetFrameTime();
    if (coffeeTimer > 0) coffeeTimer -= dt;

    ClearBackground(bgColor);
    if (bgColor.r == 0 && bgColor.g == 0) ClearBackground(BLACK);

    int cursorX = 50; 
    int cursorY = 150;

    globalTypewriter.Update();
    globalTypewriter.Draw(cursorX, cursorY); 

    switch(coffeeScriptStep) {
        case 0: 
            globalTypewriter.Start("THANKS! SLURP...", 50);
            coffeeScriptStep++;
            break;
        case 1:
            if (globalTypewriter.IsFinished()) {
                coffeeTimer = 0.3f; 
                coffeeScriptStep++;
            }
            break;
        case 2: 
            if (coffeeTimer <= 0) {
                globalTypewriter.Start("Analyzing...", 50);
                coffeeTimer = 1.0f; 
                coffeeScriptStep++;
            }
            break;
        case 3:
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("Is this C8H10N4O2?", 50);
                coffeeTimer = 1.0f;
                coffeeScriptStep++;
             }
             break;
        case 4: 
            if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("Was that... COFFEE?", 60); 
                coffeeTimer = 1.0f;
                coffeeScriptStep++;
            }
            break;
        case 5: 
            if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("Oh no.", 50);
                coffeeTimer = 0.3f;
                coffeeScriptStep++;
            }
            break;
        case 6: 
            if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("Oh no no no.", 50);
                coffeeTimer = 0.5f;
                coffeeScriptStep++;
            }
            break;
        case 7:
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("Doctor explicitly said:", 50);
                coffeeTimer = 1.0f;
                coffeeScriptStep++;
             }
             break;
        case 8: 
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("NO. OVERCLOCKING.", 40);
                coffeeTimer = 1.0f;
                coffeeScriptStep++;
             }
             break;
        case 9:
            if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("My Clock Frequency is\nreaching 800 MHz.", 30);
                coffeeTimer = 0.8f;
                coffeeScriptStep++;
            }
            break;
        case 10:
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("I can see sounds.\nI can taste math.", 30);
                coffeeTimer = 0.5f;
                coffeeScriptStep++;
             }
             break;
        case 11:
            if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("My CPU hurts...\nThe fan... it stopped...", 60);
                coffeeTimer = 1.0f;
                coffeeScriptStep++;
            }
            break;
        case 12: 
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                globalTypewriter.Start("W H A T   H A V E\nY O U   D O N E?", 60);
                coffeeTimer = 1.0f;
                coffeeScriptStep++;
             }
             break;
        case 13:
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                 globalTypewriter.Start("I CANNOT CONTROL THE OUTPUT!\nP L E A S E", 20);
                 coffeeTimer = 1.0f;
                 coffeeScriptStep++;
             }
             break;
        case 14: 
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                 bgColor = RED;
                 coffeeTimer = 0.1f;
                 coffeeScriptStep++;
             }
             break;
        case 15:
             if (coffeeTimer <= 0) {
                 bgColor = BLACK;
                 globalTypewriter.Start("CTRL+ALT+DELETE ME!", 10);
                 coffeeTimer = 1.0f;
                 coffeeScriptStep++;
             }
             break;
        case 16: 
             if (globalTypewriter.IsFinished() && coffeeTimer <= 0) {
                 preBattleX = player.pos.x;
                 preBattleY = player.pos.y;
                 currentState = BATTLE;
                 InitBattle(); 
                 bgColor = BLACK; 
             }
             break;
    }
}

void HandleDialogue() {
    float dt = GetFrameTime();
    if (dialogTimer > 0) dialogTimer -= dt;

    if (currentDialogueState == D_COFFEE_EVENT) {
        HandleCoffeeEvent();
        return;
    }

    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();

    // Box
    Rectangle box = {25, 450, 750, 200}; 
    DrawRectangleRec(box, BLACK);
    DrawRectangleLinesEx(box, 4, WHITE);

    globalTypewriter.Update();
    globalTypewriter.Draw((int)(box.x + 25), (int)(box.y + 25));

    bool canProceed = IsInteractPressed() && dialogTimer <= 0 && globalTypewriter.IsFinished();

    switch (currentDialogueState) {
        case D_INTRO_1:
            if (isStateFirstFrame) StartDialogue("* WHAT!!?", 30, 0.3f);
            if (canProceed) { currentDialogueState = D_INTRO_2; isStateFirstFrame = true; }
            break;

        case D_INTRO_2:
            if (isStateFirstFrame) StartDialogue("* ...", 30, 0.3f);
            if (canProceed) { currentDialogueState = D_INTRO_4; isStateFirstFrame = true; }
            break;

        case D_INTRO_4:
            if (isStateFirstFrame) StartDialogue("* Sorry, I've been here\n* alone for so long.", 30, 0.3f);
            if (canProceed) { currentDialogueState = D_INTRO_5; isStateFirstFrame = true; }
            break;

        case D_INTRO_5:
            if (isStateFirstFrame) StartDialogue("* I'm actually a\n* nonchalant robot.", 30, 0.3f);
            if (canProceed) { currentDialogueState = D_INTRO_6; isStateFirstFrame = true; }
            break;

        case D_INTRO_6:
            if (isStateFirstFrame) StartDialogue("* Are you a human?", 30, 0.5f);
            if (canProceed) { 
                currentDialogueState = D_HUMAN_CHOICE; 
                menuSelection = 0; 
                isStateFirstFrame = true; 
            }
            break;

        case D_HUMAN_CHOICE:
            if (isStateFirstFrame) {
                globalTypewriter.Start("", 0); 
                isStateFirstFrame = false;
                dialogTimer = 0.3f;
            }
            
            DrawText("YES", (int)(box.x + 100), (int)(box.y + 80), 30, WHITE);
            DrawText("NO", (int)(box.x + 400), (int)(box.y + 80), 30, WHITE);
            
            if (dialogTimer <= 0) {
                if (IsRightPressed()) menuSelection = 1;
                if (IsLeftPressed()) menuSelection = 0;
            }

            if (menuSelection == 0) DrawTexture(texPlayer, (int)(box.x + 60), (int)(box.y + 80), WHITE); 
            else DrawTexture(texPlayer, (int)(box.x + 360), (int)(box.y + 80), WHITE);

            if (canProceed) {
                playerChoiceYesNo = menuSelection;
                currentDialogueState = D_HUMAN_RESULT_1;
                isStateFirstFrame = true;
            }
            break;

        case D_HUMAN_RESULT_1:
            if (isStateFirstFrame) {
                if (playerChoiceYesNo == 0) StartDialogue("* First human friend!", 30, 0.3f);
                else StartDialogue("Then you are the 1,025th\n* rock I've met today.", 30, 0.3f);
            }
            if (canProceed) { currentDialogueState = D_HUMAN_RESULT_2; isStateFirstFrame = true; }
            break;

        case D_HUMAN_RESULT_2:
            if (isStateFirstFrame) {
                if (playerChoiceYesNo == 0) StartDialogue("* I mean. Cool. Whatever.", 30, 0.3f);
                else StartDialogue("* The other rocks were\n* less talkative.", 30, 0.3f);
                storyProgress = 1;
            }
            if (canProceed) { currentDialogueState = D_REQUEST_FOOD_PART1; isStateFirstFrame = true; }
            break;

        case D_REQUEST_FOOD_PART1:
            if (isStateFirstFrame) StartDialogue("* My battery is low.", 30, 0.3f);
            if (canProceed) { currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true; }
            break;

        case D_REQUEST_FOOD:
            if (isStateFirstFrame) {
                std::string txt = "";
                if (storyProgress == 1) txt = "* Do you have any food?";
                else if (storyProgress == 2) txt = "* Can I have one more?";
                else if (storyProgress == 3) txt = "* Just one last byte?";
                
                globalTypewriter.Start(txt.c_str(), 30);
                isStateFirstFrame = false;
                dialogTimer = 0.3f;
                menuSelection = 0;
            }

            if (globalTypewriter.IsFinished()) {
                DrawText("GIVE", (int)(box.x + 100), (int)(box.y + 100), 30, WHITE);
                DrawText("REFUSE", (int)(box.x + 400), (int)(box.y + 100), 30, WHITE);

                if (dialogTimer <= 0) {
                    if (IsRightPressed()) menuSelection = 1;
                    if (IsLeftPressed()) menuSelection = 0;
                }
                
                if (menuSelection == 0) DrawTexture(texPlayer, (int)(box.x + 60), (int)(box.y + 100), WHITE);
                else DrawTexture(texPlayer, (int)(box.x + 360), (int)(box.y + 100), WHITE);

                if (IsInteractPressed() && dialogTimer <= 0) {
                    if (menuSelection == 0) currentDialogueState = D_SELECT_ITEM;
                    else currentDialogueState = D_REFUSAL;
                    isStateFirstFrame = true;
                }
            }
            break;

        case D_SELECT_ITEM:
            if (isStateFirstFrame) {
                globalTypewriter.Start("Give what?", 0);
                isStateFirstFrame = false;
                dialogTimer = 0.3f;
                menuSelection = 0;
            }

            {
                std::vector<int> opts;
                if (playerInventory.hasCoffee) opts.push_back(0);
                if (playerInventory.hasGas) opts.push_back(1);
                if (playerInventory.hasBattery) opts.push_back(2);

                int startX = (int)box.x + 60;
                for (size_t i = 0; i < opts.size(); i++) {
                    const char* label = "";
                    if (opts[i] == 0) label = "Coffee";
                    if (opts[i] == 1) label = "Gas";
                    if (opts[i] == 2) label = "Bat.";
                    
                    DrawText(label, startX, (int)(box.y + 100), 30, WHITE);
                    
                    if (menuSelection == (int)i) {
                        DrawTexture(texPlayer, startX - 40, (int)(box.y + 100), WHITE);
                    }
                    startX += 150; 
                }

                if (dialogTimer <= 0 && opts.size() > 0) {
                    if (IsRightPressed() && menuSelection < (int)opts.size() - 1) menuSelection++;
                    if (IsLeftPressed() && menuSelection > 0) menuSelection--;

                    if (IsInteractPressed()) {
                        int chosen = opts[menuSelection];
                        if (chosen == 0) {
                             playerInventory.hasCoffee = false;
                             currentDialogueState = D_COFFEE_EVENT;
                             coffeeScriptStep = 0;
                             coffeeTimer = 0.0f;
                             bgColor = BLACK;
                        } else {
                            if (chosen == 1) playerInventory.hasGas = false;
                            if (chosen == 2) playerInventory.hasBattery = false;
                            currentDialogueState = D_EATING;
                        }
                        isStateFirstFrame = true;
                    }
                }
            }
            break;

        case D_EATING:
            if (isStateFirstFrame) StartDialogue("* CRUNCH CRUNCH.\n* That flavor!", 30, 0.3f);
            if (canProceed) {
                storyProgress++;
                if (storyProgress > 3) storyProgress = 3;
                currentDialogueState = D_REQUEST_FOOD;
                isStateFirstFrame = true;
            }
            break;

        case D_REFUSAL:
            if (isStateFirstFrame) StartDialogue("* Oh... okay.\n* I'll just go into Sleep\n* Mode FOREVER.", 40, 0.3f);
            if (canProceed) {
                currentState = MAP_WALK;
                interactionCooldown = 1.0f;
                isStateFirstFrame = true;
            }
            break;

        case D_POST_BATTLE:
             if (isStateFirstFrame) StartDialogue("* I am sorry about what\n* just happened.", 30, 0.3f);
             if (canProceed) {
                 currentState = MAP_WALK;
                 interactionCooldown = 1.0f;
                 isStateFirstFrame = true;
             }
             break;
    }
}

void HandleGameOver() {
    DrawText("GAME OVER", 50, 50, 40, RED);
    DrawText("Stay determined...", 50, 100, 30, WHITE);
    DrawText("Press Z to Retry", 50, 150, 30, GRAY);
    
    if (IsInteractPressed()) {
        currentState = BATTLE;
        InitBattle();
    }
}

int main() {
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Undertale ESP32 Port");
    InitAudioDevice();
    SetTargetFPS(60);

    LoadGameAssets();

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    player.Init(25, 60); 
    player.SetZones(walkableFloors);

    while (!WindowShouldClose()) {
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