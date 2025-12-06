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
int itemUsedIndex = 0;

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

        // --- FIX START: RESET TYPEWRITER ---
        // We ensure the typewriter is empty and inactive before the first Draw frame.
        globalTypewriter.fullText = "";
        globalTypewriter.charCount = 0;
        globalTypewriter.active = false;
        // --- FIX END ---

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
    float currentY = 125.0f;
    float baseFontSize = 40.0f;
    float fontSpacing = 1.5f;

    // 1. Draw History
    for (size_t i = 0; i < coffeeLog.size(); i++)
    {
        float drawX = startX;
        float fontSize = baseFontSize;
        float drawY = currentY; // Default Y position

        // --- OVERLAY HACK START ---
        // If this is the specific "Control" line, force it to the middle of the screen
        // to cover the previous "W H A T H A V E Y O U D O N E" text.
        if (coffeeLog[i].text == "I CANNOT CONTROL THE OUTPUT!")
        {
            drawY = 260.0f;
        }
        // --- OVERLAY HACK END ---

        if (coffeeLog[i].centered)
        {
            Vector2 size = MeasureTextEx(myCustomFont, coffeeLog[i].text.c_str(), fontSize, fontSpacing);
            drawX = (GAME_WIDTH - size.x) / 2.0f;
        }

        if (coffeeLog[i].isChaotic)
        {
            DrawTextJitter(myCustomFont, coffeeLog[i].text.c_str(), {drawX - 2, drawY + 1}, fontSize, fontSpacing, BLUE);
            DrawTextJitter(myCustomFont, coffeeLog[i].text.c_str(), {drawX + 2, drawY - 1}, fontSize, fontSpacing, GREEN);
            DrawTextJitter(myCustomFont, coffeeLog[i].text.c_str(), {drawX, drawY}, fontSize, fontSpacing, RED);
        }
        else if (coffeeLog[i].shakeIntensity > 0)
        {
            DrawTextJitter(myCustomFont, coffeeLog[i].text.c_str(), {drawX, drawY}, fontSize, fontSpacing, coffeeLog[i].color);
        }
        else
        {
            DrawTextEx(myCustomFont, coffeeLog[i].text.c_str(), {drawX, drawY}, fontSize, fontSpacing, coffeeLog[i].color);
        }

        Vector2 size = MeasureTextEx(myCustomFont, coffeeLog[i].text.c_str(), fontSize, fontSpacing);
        currentY += size.y + coffeeLog[i].spacing;
    }

    // 2. Draw Active Typewriter
    globalTypewriter.Update();

    if (globalTypewriter.active)
    {
        // --- CHAOS MODE: CTRL+ALT+DELETE ME! ---
        if (globalTypewriter.fullText == "CTRL+ALT+DELETE ME!")
        {
            for (int k = 0; k < 50; k++)
            {
                // STATIC CHAOS LOGIC (UPDATED):

                // 1. Calculate a raw pseudo-random number based on K
                // We use distinct large primes for X and Y to avoid diagonal patterns
                int rawX = (k * 314159 + 12345);
                int rawY = (k * 271828 + 67890);

                // 2. Map X to the FULL screen width (plus some overlap)
                // Range: -50 to (GAME_WIDTH - 100)
                // This allows the text to start slightly off-screen to the left (-50),
                // filling that empty gap.
                int rangeX = (GAME_WIDTH - 150);
                int px = (rawX % rangeX) - 150;

                // 3. Map Y to the screen height
                int rangeY = (GAME_HEIGHT - 50);
                int py = (rawY % rangeY);

                // Ensure py is positive (modulo of negative can be negative in C++)
                if (py < 0)
                    py += rangeY;

                // Note: We do NOT force px to be positive anymore!
                // We want px to be negative sometimes.

                float rx = (float)px;
                float ry = (float)py;

                // Static size variation
                float rSize = 30.0f + ((k * 13) % 25);

                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);

                DrawTextJitter(myCustomFont, sub.c_str(), {rx, ry}, rSize, fontSpacing, RED);
            }
        }
        // --- NORMAL DRAWING ---
        else
        {
            // ... (Keep the rest of your normal drawing logic exactly the same) ...
            float activeX = startX;
            float fontSize = baseFontSize;
            float activeY = currentY;

            if (globalTypewriter.fullText == "I CANNOT CONTROL THE OUTPUT!")
            {
                activeY = 260.0f;
            }

            if (currentCentered)
            {
                Vector2 size = MeasureTextEx(myCustomFont, globalTypewriter.fullText.substr(0, globalTypewriter.charCount).c_str(), fontSize, fontSpacing);
                activeX = (GAME_WIDTH - size.x) / 2.0f;
            }

            if (currentChaotic)
            {
                fontSize += GetRandomValue(-0.5, 0.5);
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                DrawTextJitter(myCustomFont, sub.c_str(), {activeX - 6, activeY + 3}, fontSize, fontSpacing, BLUE);
                DrawTextJitter(myCustomFont, sub.c_str(), {activeX + 6, activeY - 3}, fontSize, fontSpacing, GREEN);
                DrawTextJitter(myCustomFont, sub.c_str(), {activeX, activeY}, fontSize, fontSpacing, RED);
            }
            else if (currentShakeIntensity > 0)
            {
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                DrawTextJitter(myCustomFont, sub.c_str(), {activeX, activeY}, fontSize, fontSpacing, currentTextColor);
            }
            else
            {
                globalTypewriter.Draw(myCustomFont, (int)activeX, (int)activeY, fontSize, fontSpacing, currentTextColor);
            }
        }
    }

    auto AdvanceStep = [&](const char *nextText, int speed, float wait,
                           Color nextColor, int nextShake, bool nextCentered, float nextSpacing, bool nextChaotic)
    {
        if (globalTypewriter.fullText.length() > 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText,
                                 currentTextColor,
                                 currentShakeIntensity,
                                 currentCentered,
                                 currentSpacing,
                                 currentChaotic});
        }

        currentTextColor = nextColor;
        currentShakeIntensity = nextShake;
        currentCentered = nextCentered;
        currentSpacing = nextSpacing;
        currentChaotic = nextChaotic;

        globalTypewriter.Start(nextText, speed);
        coffeeTimer = wait;
        coffeeScriptStep++;
    };

    switch (coffeeScriptStep)
    {
    case 0:
        if (coffeeLog.empty() && !globalTypewriter.active)
        {
            currentTextColor = WHITE;
            currentShakeIntensity = 0;
            currentCentered = false;
            currentSpacing = 23.0f;
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
            AdvanceStep("Analyzing...", 50, 1.0f, WHITE, 0, false, 23.0f, false);
        break;
    case 3:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Is this C8H10N4O2?", 50, 1.0f, WHITE, 0, false, 23.0f, false);
        break;
    case 4:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Was that... COFFEE?", 70, 1.75f, WHITE, 1, false, 23.0f, false);
        break;
    case 5:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("Oh no.", 50, 1.0f, WHITE, 1, false, 23.0f, false);
            coffeeLog.clear();
        }
        break;
    case 6:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[0]);
            AdvanceStep("Oh no no no.", 50, 1.0f, WHITE, 1, false, 23.0f, false);
        }
        break;
    case 7:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Doctor explicitly said:", 50, 1.75f, WHITE, 1, false, 23.0f, false);
        break;
    case 8:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[1]);
            AdvanceStep("NO. OVERCLOCKING.", 70, 2.0f, RED, 1, false, 23.0f, false);
        }
        break;
    case 9:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[2]);
            AdvanceStep("My Clock Frequency is\nreaching 800 MHz.", 40, 0.8f, WHITE, 2, false, 23.0f, false);
            coffeeLog.clear();
        }
        break;
    case 10:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I can see sounds.", 40, 0.5f, WHITE, 2, false, 23.0f, false);
        break;
    case 11:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I can taste math.", 40, 1.0f, WHITE, 2, false, 23.0f, false);
        break;
    case 12:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[3]);
            AdvanceStep("My CPU hurts...", 60, 1.0f, WHITE, 3, false, 23.0f, false);
            // coffeeLog.clear();
        }
        break;
    case 13:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("The fan... it stopped...", 80, 2.0f, WHITE, 3, false, 23.0f, false);
        break;
    case 14:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("W H A T", 80, 0.6f, RED, 3, true, 23.0f, false);
            coffeeLog.clear();
        }
        break;
    case 15:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("H A V E", 80, 0.6f, RED, 3, true, 23.0f, false);
        }
        break;
    case 16:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("Y O U", 80, 0.6f, RED, 3, true, 23.0f, false);
        }
        break;
    case 17:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("D O N E ?", 80, 2.0f, RED, 3, true, 23.0f, false);
        }
        break;
    case 18:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I CANNOT CONTROL THE OUTPUT!", 30, 1.0f, RED, 3, true, 23.0f, false);
        break;
    case 19:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("P L E A S E", 30, 1.5f, RED, 3, true, 23.0f, true);
        break;
    case 20:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 23.0f, true});
            globalTypewriter.active = false;
            bgColor = RED;
            coffeeTimer = 0.2f;
            coffeeScriptStep++;
        }
        break;
    case 21: // flash twice
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 23.0f, true});
            globalTypewriter.active = false;
            bgColor = RED;
            coffeeTimer = 0.2f;
            coffeeScriptStep++;
        }
        break;
    case 22:
        if (coffeeTimer <= 0)
        {
            bgColor = BLACK;
            coffeeLog.clear();
            PlaySound(sndDialup[5]);

            // Speed 20 is fast typing
            globalTypewriter.Start("CTRL+ALT+DELETE ME!", 20);

            currentTextColor = RED;
            currentShakeIntensity = 0;
            currentCentered = false; // Doesn't matter, we override it in drawing
            currentSpacing = 10.0f;
            currentChaotic = false; // False, because we handle the chaos manually above

            coffeeTimer = 2.0f;
            coffeeScriptStep++;
        }
        break;
    case 23:
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
        // Note: We typically don't allow pausing/closing the coffee cutscene
        // because it relies on precise timers.
        HandleCoffeeEvent();
        return;
    }

    // --- GOAL 2: PRESS X TO CLOSE ---
    if (IsCancelPressed())
    {
        currentState = MAP_WALK;
        interactionCooldown = 0.2f;

        // IMPORTANT: We set this to true so that when the player talks again,
        // the current line restarts from the beginning (re-typing it).
        isStateFirstFrame = true;
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
    globalTypewriter.Draw(myCustomFont, (int)(box.x + 25), (int)(box.y + 25), 30.0f, 2.0f, WHITE);

    // --- GOAL 1: Z BUTTON LOGIC (SKIP vs NEXT) ---
    bool canProceed = false;

    if (IsInteractPressed())
    {
        // Check if text is still typing
        if (!globalTypewriter.IsFinished())
        {
            // Case 1.1: Text is still typing -> Finish it INSTANTLY.
            // We do NOT check dialogTimer here. We let the player skip
            // the effect immediately even if the line just started.
            globalTypewriter.Skip();
        }
        else if (dialogTimer <= 0)
        {
            // Case 1.2: Text is finished -> Proceed to next step.
            // We DO check dialogTimer here. This ensures that if the player
            // mashed Z to skip the text, they don't accidentally skip
            // to the next dialogue state in the same split second.
            canProceed = true;
        }
    }

    // [The Switch Statement remains exactly the same, using the new canProceed logic]
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
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
        }

        DrawTextEx(myCustomFont, "YES", {(float)(box.x + 200), (float)(box.y + 100)}, 30, 2, WHITE);
        DrawTextEx(myCustomFont, "NO", {(float)(box.x + 400), (float)(box.y + 100)}, 30, 2, WHITE);

        if (dialogTimer <= 0)
        {
            if (IsRightPressed())
                menuSelection = 1;
            if (IsLeftPressed())
                menuSelection = 0;
        }

        if (menuSelection == 0)
        {
            DrawTextureEx(texPlayer, {(float)(box.x + 160), (float)(box.y + 95)}, 0.0f, 0.4f, WHITE);
        }
        else
        {
            DrawTextureEx(texPlayer, {(float)(box.x + 360), (float)(box.y + 95)}, 0.0f, 0.4f, WHITE);
        }

        // Only proceed if a selection is made AND text is fully visible (which it is here)
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
                StartDialogue("* Then you are the 1,025th rock I've\n* met today.", 30, 0.3f);
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
                StartDialogue("* The other rocks were less talkative.", 30, 0.3f);
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
                DrawTextureEx(texPlayer, {(float)(box.x + 160), (float)(box.y + 95)}, 0.0f, 0.4f, WHITE);
            else
                DrawTextureEx(texPlayer, {(float)(box.x + 410), (float)(box.y + 95)}, 0.0f, 0.4f, WHITE);

            // Note: We use IsInteractPressed directly here because this is a choice menu
            // and we don't want "canProceed" logic to auto-confirm choices.
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
            int gap = 120;

            for (size_t i = 0; i < opts.size(); i++)
            {
                const char *label = "";
                if (opts[i] == 0)
                    label = "Coffee";
                if (opts[i] == 1)
                    label = "Gas";
                if (opts[i] == 2)
                    label = "Battery";

                Vector2 textSize = MeasureTextEx(myCustomFont, label, 30, 2);
                DrawTextEx(myCustomFont, label, {(float)startX, (float)(box.y + 100)}, 30, 2, WHITE);

                if (menuSelection == (int)i)
                {
                    DrawTextureEx(texPlayer, {(float)(startX - 40), (float)(box.y + 97)}, 0.0f, 0.4f, WHITE);
                }
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
                        globalTypewriter.active = false;
                    }
                    else
                    {
                        if (chosen == 1)
                        {
                            playerInventory.hasGas = false;
                            itemUsedIndex = 1;
                        }
                        if (chosen == 2)
                        {
                            playerInventory.hasBattery = false;
                            itemUsedIndex = 2;
                        }
                        currentDialogueState = D_EATING;
                    }
                    isStateFirstFrame = true;
                }
            }
        }
        break;

    case D_EATING:
        if (isStateFirstFrame)
        {
            if (itemUsedIndex == 1)
                StartDialogue("* GLUG GLUG...\n* Premium Octane!", 30, 0.3f);
            else
                StartDialogue("* CRUNCH CRUNCH.\n* That flavor!", 30, 0.3f);
        }
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
            interactionCooldown = 0.2f;
            isStateFirstFrame = true;
        }
        break;

    case D_POST_BATTLE:
        if (isStateFirstFrame)
            StartDialogue("* I am sorry about what just happened.", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_POST_BATTLE_2;
            isStateFirstFrame = true;
        }
        break;

    case D_POST_BATTLE_2:
        if (isStateFirstFrame)
            StartDialogue("* And by the way you just finished\n* the game.", 30, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 0.2f;
            isStateFirstFrame = true;
        }
        break;
    }
}

void HandleGameOver()
{
    UpdateMusicStream(gameOver);
    DrawTextEx(myCustomFont, "GAME OVER", {250, 200}, 60, 2, RED);
    DrawTextEx(myCustomFont, "Whoever you are... stay determined!", {85, 275}, 30, 2, WHITE);
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

    // 1. Get the directory where the .exe is running
    const char *dir = GetApplicationDirectory();

    // 2. Change the current working directory to the .exe's location
    //    This ensures that "assets/file.png" always looks next to the .exe
    ChangeDirectory(dir);

    // Optional: Debug log to see where the game is looking
    // TraceLog(LOG_INFO, "Target Working Directory: %s", dir);

    LoadGameAssets();

    // NOW you can use a relative path safely.
    // Ensure the "assets" folder is inside the "Debug" or "Release" folder
    // where your .exe is generated.
    myCustomFont = LoadFontEx("../assets/determination-mono.otf", 64, 0, 0);
    if (myCustomFont.texture.id == 0)
    {
        TraceLog(LOG_WARNING, "FONT FAILED TO LOAD! Check assets folder location.");
    }
    else
    {
        SetTextureFilter(myCustomFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    player.Init(125, 300);
    player.SetZones(walkableFloors);

    // --- DEBUG CHECK ---
    // This allows you to bypass the map and dialogue for testing battle mechanics.
    if (DEBUG_SKIP_TO_BATTLE)
    {
        // 1. Force the state
        currentState = GAME_OVER;

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