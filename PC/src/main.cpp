#include "raylib.h"
#include "game_defs.h"
#include "Globals.h"
#include "Utils.h"
#include "Player.h"
#include "Battle.h"
#include "TextAlignment.h"
#include <vector>
#include <string>
#include <cmath>

const bool DEBUG_SKIP_TO_BATTLE = true;

// --- GLOBAL VARIABLES ---
// 1. currentState is defined in Globals.cpp, so we don't define it here.
// 2. currentDialogueState is NOT in Globals.cpp, so we MUST define it here.
DialogueState currentDialogueState = D_INTRO_1;

// 3. playerInventory is NOT in Globals.cpp, so we define it here.
Inventory playerInventory = {true, true, true};

// Map Data
std::vector<Rect> walkableFloors = {
    // top section
    {135, 221, 611, 38},
    {135, 259, 558, 43},
    // middle
    {134, 302, 510, 29},
    {125, 331, 528, 30},
    {114, 361, 579, 25},
    // bottom section
    {104, 386, 652, 34},
    {137, 420, 619, 51}};

NPC mapEnemy = {425, 280};

// Game Logic Globals
int storyProgress = 0;

// These are defined in Battle.cpp
extern bool battleCompleted;
extern float preBattleX;
extern float preBattleY;

float interactionCooldown = 0.0f;

// Dialogue Helpers
bool isStateFirstFrame = true;
float dialogTimer = 0.0f;
int menuSelection = 0;
int playerChoiceYesNo = 0;

// --- COFFEE EVENT DATA ---
struct LogEntry
{
    std::string text;
    Color color;
    int shakeIntensity; // 0 = None, 1 = Low, 2 = Medium, 3+ = High
    bool centered;      // true = Center on screen, false = Left aligned
    float spacing;      // Space after this line (default 10)
    bool isChaotic;
};

std::vector<LogEntry> coffeeLog;

// State tracking
int coffeeScriptStep = 0;
float coffeeTimer = 0.0f;
Color bgColor = BLACK;

// Current active text settings
Color currentTextColor = WHITE;
int currentShakeIntensity = 0;
bool currentCentered = false;
float currentSpacing = 10.0f;
bool currentChaotic = false;

// Font needs to be declared here, but initialized in main()
Font myCustomFont;

// --- INPUT HELPERS ---

// IsInteractPressed() is in Utils.cpp

bool IsLeftPressed()
{
    return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
}

bool IsRightPressed()
{
    return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
}

// --- STATE HANDLERS ---

void HandleMenu()
{
    TextMetrics titleM = GetCenteredTextPosition(myCustomFont, "UNDERTALE ESP32 PORT", 40, 2);
    Vector2 titlePos = {titleM.x, 250}; // Use calculated X, keep Y fixed
    DrawTextEx(myCustomFont, "UNDERTALE ESP32 PORT", titlePos, 40, 2, WHITE);

    TextMetrics enterM = GetCenteredTextPosition(myCustomFont, "Press Z to Enter", 30, 2);
    Vector2 enterPos = {enterM.x, 350};
    DrawTextEx(myCustomFont, "Press Z to Enter", enterPos, 30, 2, GRAY);

    TextMetrics creditM = GetCenteredTextPosition(myCustomFont, "By Molly", 20, 2);
    Vector2 creditPos = {creditM.x, 600};
    DrawTextEx(myCustomFont, "By Molly", creditPos, 20, 2, DARKGRAY);

    if (IsInteractPressed())
    {
        currentState = MAP_WALK;
        player.Init(125, 300);
        player.SetZones(walkableFloors);
        storyProgress = 0;
        isStateFirstFrame = true;
    }
}

void HandleMap()
{
    float dt = GetFrameTime();

    if (interactionCooldown > 0)
        interactionCooldown -= dt;

    player.Update(dt, &mapEnemy);

    // Interaction Check
    float dist = (float)sqrt(pow(player.pos.x - mapEnemy.x, 2) + pow(player.pos.y - mapEnemy.y, 2));

    if (dist < 100 && IsInteractPressed() && interactionCooldown <= 0)
    {
        currentState = DIALOGUE;

        if (battleCompleted)
            currentDialogueState = D_POST_BATTLE;
        else if (storyProgress == 0)
            currentDialogueState = D_INTRO_1;
        else if (storyProgress == 1)
            currentDialogueState = D_REQUEST_FOOD_PART1;
        else
            currentDialogueState = D_REQUEST_FOOD;

        isStateFirstFrame = true;
    }

    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();
}

void StartDialogue(const char *text, int speed, float waitTime)
{
    globalTypewriter.Start(text, speed);
    dialogTimer = waitTime;
    isStateFirstFrame = false;
}

void HandleCoffeeEvent()
{
    float dt = GetFrameTime();
    if (coffeeTimer > 0)
        coffeeTimer -= dt;

    // --- FLASHING BACKGROUND LOGIC ---
    ClearBackground(bgColor);
    if (bgColor.r != 0 || bgColor.g != 0)
    {
        if (bgColor.r == 255 && coffeeTimer < 0.05f)
            ClearBackground(BLACK);
        else
            ClearBackground(bgColor);
    }

    // --- DRAWING LOGIC ---
    float startX = 50.0f;
    float currentY = 150.0f;
    float baseFontSize = 40.0f;
    float fontSpacing = 2.0f;

    // 1. Draw History
    for (size_t i = 0; i < coffeeLog.size(); i++)
    {
        float drawX = startX;
        float fontSize = baseFontSize;

        // --- CALCULATE POSITION ---
        if (coffeeLog[i].centered)
        {
            Vector2 size = MeasureTextEx(myCustomFont, coffeeLog[i].text.c_str(), fontSize, fontSpacing);
            drawX = (GAME_WIDTH - size.x) / 2.0f;
        }
        float drawY = currentY;

        // --- APPLY EFFECTS ---
        // 1. Standard Shake
        int intensity = coffeeLog[i].shakeIntensity;
        if (intensity > 0)
        {
            drawX += GetRandomValue(-intensity, intensity);
            drawY += GetRandomValue(-intensity, intensity);
        }

        // 2. CHAOS MODE (Glitch Effect)
        if (coffeeLog[i].isChaotic)
        {

            // Draw RGB Split (Chromatic Aberration)
            // Blue Layer
            DrawTextEx(myCustomFont, coffeeLog[i].text.c_str(),
                       {drawX - 2, drawY + 1}, fontSize, fontSpacing, BLUE);
            // Green Layer
            DrawTextEx(myCustomFont, coffeeLog[i].text.c_str(),
                       {drawX + 2, drawY - 1}, fontSize, fontSpacing, GREEN);
            // Red Layer (Main)
            DrawTextEx(myCustomFont, coffeeLog[i].text.c_str(),
                       {drawX, drawY}, fontSize, fontSpacing, RED);
        }
        else
        {
            // Standard Draw
            DrawTextEx(myCustomFont, coffeeLog[i].text.c_str(),
                       {drawX, drawY}, fontSize, fontSpacing, coffeeLog[i].color);
        }

        // Add height + custom spacing
        Vector2 size = MeasureTextEx(myCustomFont, coffeeLog[i].text.c_str(), fontSize, fontSpacing);
        currentY += size.y + coffeeLog[i].spacing;
    }

    // 2. Draw Active Typewriter
    globalTypewriter.Update();

    if (globalTypewriter.active)
    {
        float activeX = startX;
        float fontSize = baseFontSize;

        // Check alignment
        if (currentCentered)
        {
            Vector2 size = MeasureTextEx(myCustomFont, globalTypewriter.fullText.substr(0, globalTypewriter.charCount).c_str(), fontSize, fontSpacing);
            activeX = (GAME_WIDTH - size.x) / 2.0f;
        }
        float activeY = currentY;

        // Apply Shake
        if (currentShakeIntensity > 0)
        {
            activeX += GetRandomValue(-currentShakeIntensity, currentShakeIntensity);
            activeY += GetRandomValue(-currentShakeIntensity, currentShakeIntensity);
        }

        // Apply Active Chaos
        if (currentChaotic)
        {
            fontSize += GetRandomValue(-0.5, 0.5); // size changes

            // Draw partial string with glitch
            std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);

            // RGB Split
            DrawTextEx(myCustomFont, sub.c_str(), {activeX - 6, activeY + 3}, fontSize, fontSpacing, BLUE);
            DrawTextEx(myCustomFont, sub.c_str(), {activeX + 6, activeY - 3}, fontSize, fontSpacing, GREEN);
            DrawTextEx(myCustomFont, sub.c_str(), {activeX, activeY}, fontSize, fontSpacing, RED);
        }
        else
        {
            globalTypewriter.Draw(myCustomFont, (int)activeX, (int)activeY, fontSize, fontSpacing, currentTextColor);
        }
    }

    // --- HELPER: ADVANCE STEP ---
    // UPDATED: Now accepts 'nextChaotic'
    auto AdvanceStep = [&](const char *nextText, int speed, float wait,
                           Color nextColor, int nextShake, bool nextCentered, float nextSpacing, bool nextChaotic)
    {
        // 1. Archive OLD text
        if (globalTypewriter.fullText.length() > 0)
        {
            coffeeLog.push_back({
                globalTypewriter.fullText,
                currentTextColor,
                currentShakeIntensity,
                currentCentered,
                currentSpacing,
                currentChaotic // Save chaos state
            });
        }

        // 2. Set NEW styles
        currentTextColor = nextColor;
        currentShakeIntensity = nextShake;
        currentCentered = nextCentered;
        currentSpacing = nextSpacing;
        currentChaotic = nextChaotic; // Set new chaos state

        // 3. Start NEW text
        globalTypewriter.Start(nextText, speed);
        coffeeTimer = wait;
        coffeeScriptStep++;
    };

    // --- SCRIPT ---
    switch (coffeeScriptStep)
    {
    case 0:
        if (coffeeLog.empty() && !globalTypewriter.active)
        {
            // Reset Defaults
            currentTextColor = WHITE;
            currentShakeIntensity = 0;
            currentCentered = false;
            currentSpacing = 10.0f;
            currentChaotic = false;

            globalTypewriter.Start("THANKS! SLURP...", 50);
            coffeeScriptStep++;
        }
        break;
    case 1:
        if (globalTypewriter.IsFinished())
        {
            coffeeTimer = 0.5f;
            coffeeScriptStep++;
        }
        break;
    case 2:
        if (coffeeTimer <= 0)
            AdvanceStep("Analyzing...", 50, 1.0f, WHITE, 0, false, 20.0f, false);
        break;
    case 3:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Is this C8H10N4O2?", 50, 1.0f, WHITE, 0, false, 20.0f, false);
        break;

    // --- GRADUAL SHAKE START ---
    case 4:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Was that... COFFEE?", 60, 1.75f, WHITE, 1, false, 20.0f, false);
        break;

    case 5:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("Oh no.", 50, 1.0f, WHITE, 1, false, 20.0f, false);
            coffeeLog.clear();
        }
        break;

    case 6:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Oh no no no.", 50, 1.0f, WHITE, 1, false, 20.0f, false);
        break;
    case 7:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Doctor explicitly said:", 50, 1.75f, WHITE, 1, false, 20.0f, false);
        break;

    case 8:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("NO. OVERCLOCKING.", 40, 2.0f, RED, 1, false, 20.0f, false);
        }
        break;

    case 9:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("My Clock Frequency is\nreaching 800 MHz.", 30, 0.8f, WHITE, 2, false, 20.0f, false);
            coffeeLog.clear();
        }
        break;

    // --- SPLIT: SOUNDS & MATH ---
    case 10:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I can see sounds.", 30, 0.5f, WHITE, 2, false, 20.0f, false);
        break;

    case 11:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I can taste math.", 30, 1.0f, WHITE, 2, false, 20.0f, false);
        break;

    // --- SPLIT: CPU & FAN ---
    case 12:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("My CPU hurts...", 60, 1.0f, WHITE, 3, false, 20.0f, false);
            coffeeLog.clear();
        }
        break;

    case 13:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("The fan... it stopped...", 60, 2.0f, WHITE, 3, false, 20.0f, false);
        break;

    // --- SPLIT & CENTERED "WHAT HAVE YOU DONE" ---
    case 14:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("W H A T", 80, 0.6f, RED, 3, true, 20.0f, false);
            coffeeLog.clear();
        }
        break;

    case 15:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("H A V E", 80, 0.6f, RED, 3, true, 20.0f, false);
        break;

    case 16:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Y O U", 80, 0.6f, RED, 3, true, 20.0f, false);
        break;

    case 17:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("D O N E ?", 80, 2.25f, RED, 3, true, 20.0f, false);
        break;

    // --- CHAOTIC GLITCH STEP ---
    case 18:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            // ENABLE CHAOS: true at the end
            AdvanceStep("I CANNOT CONTROL THE OUTPUT!", 10, 1.0f, RED, 2, false, 20.0f, true);
        }
        break;

    case 19:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            // KEEP CHAOS: true
            AdvanceStep("P L E A S E", 10, 1.5f, RED, 3, false, 20.0f, true);
        break;

    case 20:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            // Push final line with Chaos=true
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 20.0f, true});
            globalTypewriter.active = false;

            bgColor = RED;
            coffeeTimer = 0.2f;
            coffeeScriptStep++;
        }
        break;
    case 21:
        if (coffeeTimer <= 0)
        {
            bgColor = BLACK;
            coffeeLog.clear();

            globalTypewriter.Start("CTRL+ALT+DELETE ME!", 10);

            currentTextColor = RED;
            currentShakeIntensity = 0;
            currentCentered = true;
            currentSpacing = 10.0f;
            currentChaotic = false; // Turn off chaos for final message

            coffeeTimer = 2.0f;
            coffeeScriptStep++;
        }
        break;
    case 22:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.clear();
            preBattleX = player.pos.x;
            preBattleY = player.pos.y;
            currentState = BATTLE;
            InitBattle();

            bgColor = BLACK;
            currentTextColor = WHITE;
            currentShakeIntensity = 0;
            currentCentered = false;
            currentChaotic = false;
        }
        break;
    }
}

void HandleDialogue()
{
    float dt = GetFrameTime();
    if (dialogTimer > 0)
        dialogTimer -= dt;

    if (currentDialogueState == D_COFFEE_EVENT)
    {
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

    // UPDATED: Passing font, size, spacing, and color
    globalTypewriter.Draw(myCustomFont, (int)(box.x + 25), (int)(box.y + 25), 30.0f, 2.0f, WHITE);

    bool canProceed = IsInteractPressed() && dialogTimer <= 0 && globalTypewriter.IsFinished();

    switch (currentDialogueState)
    {
    case D_INTRO_1:
        if (isStateFirstFrame)
            StartDialogue("* WHAT!!?", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_2;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_2:
        if (isStateFirstFrame)
            StartDialogue("* ...", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_4;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_4:
        if (isStateFirstFrame)
            StartDialogue("* Sorry, I've been here alone for so\n* long.", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_5;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_5:
        if (isStateFirstFrame)
            StartDialogue("* I'm actually a nonchalant robot.", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_6;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_6:
        if (isStateFirstFrame)
            StartDialogue("* Are you a human?", 30, 0.5f);
        if (canProceed)
        {
            currentDialogueState = D_HUMAN_CHOICE;
            menuSelection = 0;
            isStateFirstFrame = true;
        }
        break;

    case D_HUMAN_CHOICE:
        if (isStateFirstFrame)
        {
            globalTypewriter.Start("", 0);
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
        }

        // UPDATED: Using DrawTextEx for YES/NO choices
        DrawTextEx(myCustomFont, "YES", {(float)(box.x + 200), (float)(box.y + 80)}, 30, 2, WHITE);
        DrawTextEx(myCustomFont, "NO", {(float)(box.x + 400), (float)(box.y + 80)}, 30, 2, WHITE);

        if (dialogTimer <= 0)
        {
            if (IsRightPressed())
                menuSelection = 1;
            if (IsLeftPressed())
                menuSelection = 0;
        }

        if (menuSelection == 0)
        {
            // Syntax: Texture, Vector2 Position, Rotation, Scale, Tint
            DrawTextureEx(texPlayer, {(float)(box.x + 140), (float)(box.y + 85)}, 0.0f, 0.4f, WHITE);
        }
        else
        {
            DrawTextureEx(texPlayer, {(float)(box.x + 360), (float)(box.y + 85)}, 0.0f, 0.4f, WHITE);
        }

        if (canProceed)
        {
            playerChoiceYesNo = menuSelection;
            currentDialogueState = D_HUMAN_RESULT_1;
            isStateFirstFrame = true;
        }
        break;

    case D_HUMAN_RESULT_1:
        if (isStateFirstFrame)
        {
            if (playerChoiceYesNo == 0)
                StartDialogue("* First human friend!", 30, 0.3f);
            else
                StartDialogue("Then you are the 1,025th rock I've met\n* today.", 30, 0.3f);
        }
        if (canProceed)
        {
            currentDialogueState = D_HUMAN_RESULT_2;
            isStateFirstFrame = true;
        }
        break;

    case D_HUMAN_RESULT_2:
        if (isStateFirstFrame)
        {
            if (playerChoiceYesNo == 0)
                StartDialogue("* I mean. Cool. Whatever.", 30, 0.3f);
            else
                StartDialogue("* The other rocks were less\n* talkative.", 30, 0.3f);
            storyProgress = 1;
        }
        if (canProceed)
        {
            currentDialogueState = D_REQUEST_FOOD_PART1;
            isStateFirstFrame = true;
        }
        break;

    case D_REQUEST_FOOD_PART1:
        if (isStateFirstFrame)
            StartDialogue("* My battery is low.", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_REQUEST_FOOD;
            isStateFirstFrame = true;
        }
        break;

    case D_REQUEST_FOOD:
        if (isStateFirstFrame)
        {
            std::string txt = "";
            if (storyProgress == 1)
                txt = "* Do you have any food?";
            else if (storyProgress == 2)
                txt = "* Can I have one more?";
            else if (storyProgress == 3)
                txt = "* Just one last byte?";

            globalTypewriter.Start(txt.c_str(), 30);
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
            menuSelection = 0;
        }

        if (globalTypewriter.IsFinished())
        {
            // UPDATED: Using DrawTextEx for GIVE/REFUSE

            DrawTextEx(myCustomFont, "GIVE", {(float)(box.x + 200), (float)(box.y + 100)}, 30, 2, WHITE);
            DrawTextEx(myCustomFont, "REFUSE", {(float)(box.x + 450), (float)(box.y + 100)}, 30, 2, WHITE);

            if (dialogTimer <= 0)
            {
                if (IsRightPressed())
                    menuSelection = 1;
                if (IsLeftPressed())
                    menuSelection = 0;
            }

            if (menuSelection == 0)
            {
                // Syntax: Texture, Vector2 Position, Rotation, Scale, Tint
                DrawTextureEx(texPlayer, {(float)(box.x + 160), (float)(box.y + 100)}, 0.0f, 0.4f, WHITE);
            }
            else
            {
                DrawTextureEx(texPlayer, {(float)(box.x + 410), (float)(box.y + 100)}, 0.0f, 0.4f, WHITE);
            }

            if (IsInteractPressed() && dialogTimer <= 0)
            {
                if (menuSelection == 0)
                    currentDialogueState = D_SELECT_ITEM;
                else
                    currentDialogueState = D_REFUSAL;
                isStateFirstFrame = true;
            }
        }
        break;

    case D_SELECT_ITEM:
        if (isStateFirstFrame)
        {
            globalTypewriter.Start("Give what?", 0);
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
            menuSelection = 0;
        }

        {
            std::vector<int> opts;
            if (playerInventory.hasCoffee)
                opts.push_back(0);
            if (playerInventory.hasGas)
                opts.push_back(1);
            if (playerInventory.hasBattery)
                opts.push_back(2);

            int startX = (int)box.x + 100;
            int gap = 120; // The fixed space between the end of one word and start of next

            for (size_t i = 0; i < opts.size(); i++)
            {
                const char *label = "";
                if (opts[i] == 0)
                    label = "Coffee";
                if (opts[i] == 1)
                    label = "Gas";
                if (opts[i] == 2)
                    label = "Battery";

                // 1. Measure the width of the current label
                // Arguments: Font, Text, FontSize, Spacing (Must match DrawTextEx)
                Vector2 textSize = MeasureTextEx(myCustomFont, label, 30, 2);

                // 2. Draw the text at the current startX
                DrawTextEx(myCustomFont, label, {(float)startX, (float)(box.y + 100)}, 30, 2, WHITE);

                if (menuSelection == (int)i)
                {
                    // Heart position is still relative to the start of the word
                    DrawTextureEx(texPlayer, {(float)(startX - 40), (float)(box.y + 105)}, 0.0f, 0.4f, WHITE);
                }

                // 3. Increment startX by the text width PLUS the gap
                // This ensures the next word starts 'gap' pixels after this word ends
                startX += (int)textSize.x + gap;
            }

            if (dialogTimer <= 0 && opts.size() > 0)
            {
                if (IsRightPressed() && menuSelection < (int)opts.size() - 1)
                    menuSelection++;
                if (IsLeftPressed() && menuSelection > 0)
                    menuSelection--;

                if (IsInteractPressed())
                {
                    int chosen = opts[menuSelection];
                    if (chosen == 0)
                    {
                        playerInventory.hasCoffee = false;
                        currentDialogueState = D_COFFEE_EVENT;
                        coffeeScriptStep = 0;
                        coffeeTimer = 0.0f;
                        coffeeLog.clear();
                        bgColor = BLACK;

                        // Force the typewriter to stop so case 0 in CoffeeEvent can start.
                        globalTypewriter.active = false;
                    }
                    else
                    {
                        if (chosen == 1)
                            playerInventory.hasGas = false;
                        if (chosen == 2)
                            playerInventory.hasBattery = false;
                        currentDialogueState = D_EATING;
                    }
                    isStateFirstFrame = true;
                }
            }
        }
        break;

    case D_EATING:
        if (isStateFirstFrame)
            StartDialogue("* CRUNCH CRUNCH.\n* That flavor!", 30, 0.3f);
        if (canProceed)
        {
            storyProgress++;
            if (storyProgress > 3)
                storyProgress = 3;
            currentDialogueState = D_REQUEST_FOOD;
            isStateFirstFrame = true;
        }
        break;

    case D_REFUSAL:
        if (isStateFirstFrame)
            StartDialogue("* Oh... okay.\n* I'll just go into Sleep Mode\n* FOREVER.", 40, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 1.0f;
            isStateFirstFrame = true;
        }
        break;

    case D_POST_BATTLE:
        if (isStateFirstFrame)
            StartDialogue("* I am sorry about what just happened.", 30, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 1.0f;
            isStateFirstFrame = true;
        }
        break;
    }
}

void HandleGameOver()
{
    UpdateMusicStream(gameOver);
    DrawTextEx(myCustomFont, "GAME OVER", {250, 200}, 60, 2, RED);
    DrawTextEx(myCustomFont, "Whoever you are... Stay determined!", {85, 350}, 30, 2, WHITE);
    DrawTextEx(myCustomFont, "Press Z to Retry", {250, 400}, 30, 2, GRAY);

    if (IsInteractPressed())
    {
        StopMusicStream(gameOver); // Stop gameOver music
        currentState = BATTLE;
        InitBattle();
    }
}

int main()
{
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Undertale ESP32 Port");
    InitAudioDevice();
    SetTargetFPS(60);

    LoadGameAssets();

    // UPDATED: Load the font AFTER InitWindow to prevent crashes
    myCustomFont = LoadFontEx("assets/determination-mono.otf", 64, 0, 0);
    SetTextureFilter(myCustomFont.texture, TEXTURE_FILTER_BILINEAR);

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    player.Init(125, 300);
    player.SetZones(walkableFloors);

    // --- DEBUG CHECK ---
    // This allows you to bypass the map and dialogue for testing battle mechanics.
    if (DEBUG_SKIP_TO_BATTLE)
    {
        // 1. Force the state
        currentState = BATTLE;

        // 2. Save current position so the battle system has a return coordinate
        // (Even if we don't return, the variables need to be set to avoid trash values)
        preBattleX = player.pos.x;
        preBattleY = player.pos.y;

        // 3. Initialize Battle specific variables (HP, Phases, Typewriter)
        InitBattle();
    }

    while (!WindowShouldClose())
    {
        BeginTextureMode(target);
        ClearBackground(BLACK);

        switch (currentState)
        {
        case MENU:
            HandleMenu();
            break;
        case MAP_WALK:
            HandleMap();
            break;
        case DIALOGUE:
            HandleDialogue();
            break;
        case BATTLE:
            UpdateBattle();
            DrawBattle();
            break;
        case GAME_OVER:
            HandleGameOver();
            break;
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

    // UPDATED: Unload font
    UnloadFont(myCustomFont);

    UnloadRenderTexture(target);
    UnloadGameAssets();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}