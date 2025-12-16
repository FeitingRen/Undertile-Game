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

const bool DEBUG_SKIP_TO_BATTLE = false;

// --- GLOBAL VARIABLES ---
// 1. currentState is defined in Globals.cpp, so we don't define it here.
// 2. currentDialogueState is NOT in Globals.cpp, so we MUST define it here.
DialogueState currentDialogueState = D_INTRO_1;

// 3. playerInventory is NOT in Globals.cpp, so we define it here.
Inventory playerInventory = {true, true, true};

// Map Data
std::vector<Rect> walkableFloors = {
    // top section
    {135, 227, 627, 48},
    {135, 259, 558, 43},
    // middle
    {134, 302, 510, 29},
    {125, 331, 528, 30},
    {114, 361, 579, 25},
    // bottom section
    {104, 386, 589, 50},
    {693, 402, 63, 18},
    {137, 420, 619, 59}};

NPC mapEnemy = {425, 280};

// Game Logic Globals
int storyProgress = 0;
bool showTutorialText = true;

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
    if (!IsMusicStreamPlaying(menuMusic))
    {
        PlayMusicStream(menuMusic);
    }
    UpdateMusicStream(menuMusic);

    // --- 1. LANGUAGE SELECTION INPUT ---
    if (IsKeyPressed(KEY_ONE))
    {
        PlaySound(sndSelect);
        currentLanguage = LANG_EN;
    }
    if (IsKeyPressed(KEY_TWO))
    {
        PlaySound(sndSelect);
        currentLanguage = LANG_CN;
    }

    // --- 2. GET CURRENT FONT & COLORS ---
    Font activeFont = GetCurrentFont();
    Color enColor = (currentLanguage == LANG_EN) ? YELLOW : GRAY;
    Color cnColor = (currentLanguage == LANG_CN) ? YELLOW : GRAY;

    // --- 3. DRAW TITLE (Dynamic Language) ---
    // Uses the Text() helper to pick the string, and activeFont for the style
    const char *titleStr = Text("UNDERTILE", "傳說之下水道");
    if (currentLanguage == LANG_EN)
    {
        TextMetrics titleM = GetCenteredTextPosition(activeFont, titleStr, 60, 2);
        DrawTextEx(activeFont, titleStr, {titleM.x, 150}, 60, 2, WHITE);
    }
    else
    {
        TextMetrics titleM = GetCenteredTextPosition(activeFont, titleStr, 76, 2);
        DrawTextEx(activeFont, titleStr, {titleM.x, 150}, 76, 2, WHITE);
    }

    // --- 4. DRAW LANGUAGE OPTIONS (Fixed Fonts) ---
    // Line 1: English (Always uses English Font)
    const char *optEn = "PRESS [1] FOR ENGLISH";
    TextMetrics enM = GetCenteredTextPosition(fontEN, optEn, 25, 2);
    DrawTextEx(fontEN, optEn, {enM.x, 310}, 25, 2, enColor);

    // Line 2: Chinese (Always uses Chinese Font)
    const char *optCn = "按 [2] 切換中文";
    TextMetrics cnM = GetCenteredTextPosition(fontCN, optCn, 32, 2);
    DrawTextEx(fontCN, optCn, {cnM.x, 350}, 32, 2, cnColor);

    // --- 5. DRAW ENTER PROMPT (Dynamic Language) ---
    const char *enterStr = Text("Press Z to Enter", "按Z進入遊戲");
    if (currentLanguage == LANG_EN)
    {
        TextMetrics titleM = GetCenteredTextPosition(activeFont, enterStr, 30, 2);
        DrawTextEx(activeFont, enterStr, {titleM.x, 450}, 30, 2, WHITE);
    }
    else
    {
        TextMetrics titleM = GetCenteredTextPosition(activeFont, enterStr, 38, 2);
        DrawTextEx(activeFont, enterStr, {titleM.x, 450}, 38, 2, WHITE);
    }

    // --- 6. CREDITS ---
    TextMetrics creditM = GetCenteredTextPosition(fontEN, "By Molly", 20, 2);
    DrawTextEx(fontEN, "By Molly", {creditM.x, 600}, 20, 2, DARKGRAY);

    // --- 7. START GAME ---
    if (IsInteractPressed())
    {
        PlaySound(sndSelect);
        StopMusicStream(menuMusic);
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
        showTutorialText = false;
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
            currentDialogueState = D_INTRO_1; // DEBUG: D_COFFEE_EVENT
        else if (storyProgress == 1)
            currentDialogueState = D_REQUEST_FOOD_PART1;
        else
            currentDialogueState = D_REQUEST_FOOD;

        isStateFirstFrame = true;
    }

    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();
    if (showTutorialText)
    {
        Font activeFont = GetCurrentFont();
        const char *guideText;
        float fontSize;

        if (currentLanguage == LANG_EN)
        {
            guideText = "[Arrow Keys] Move   [Z] Interact with Robot";
            fontSize = 23.0f;
        }
        else
        {
            guideText = "[方向鍵] 移動   [Z] 與機器人互動";
            fontSize = 30.0f; // Slightly larger for Chinese readability
        }

        // Use your TextAlignment helper to center it
        TextMetrics tm = GetCenteredTextPosition(activeFont, guideText, fontSize, 2.0f);

        // Draw at the bottom of the screen (GAME_HEIGHT - 40)
        // Using GRAY so it looks like a UI hint, not dialogue
        DrawTextEx(activeFont, guideText, {tm.x, GAME_HEIGHT - 40.0f}, fontSize, 2.0f, WHITE);
    }
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

    // Pick English or Chinese line depending on currentLanguage
    auto L = [&](const char *en, const char *cn)
    {
        return (currentLanguage == LANG_CN) ? cn : en;
    };

    // Lines we need to recognize specially in drawing/logic
    const char *controlLine = L("I CANNOT CONTROL THE OUTPUT!", "我控制不了我的輸出了!");
    const char *deleteLine = L("CTRL+ALT+DELETE ME!", "把我強制關機!");

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
    float baseFontSize = (currentLanguage == LANG_CN) ? 45.0f : 40.0f;
    ;
    float fontSpacing = 1.5f;
    Font activeFont = GetCurrentFont();

    // 1. Draw history
    for (size_t i = 0; i < coffeeLog.size(); i++)
    {
        float drawX = startX;
        float fontSize = baseFontSize;
        float drawY = currentY;

        // overlay hack: keep control line locked in the middle
        if (coffeeLog[i].text == controlLine)
            drawY = 290.0f;

        if (coffeeLog[i].centered)
        {
            Vector2 size = MeasureTextEx(activeFont, coffeeLog[i].text.c_str(), fontSize, fontSpacing);
            drawX = (GAME_WIDTH - size.x) / 2.0f;
        }

        if (coffeeLog[i].isChaotic)
        {
            DrawTextJitter(activeFont, coffeeLog[i].text.c_str(), {drawX - 2, drawY + 1}, fontSize, fontSpacing, BLUE);
            DrawTextJitter(activeFont, coffeeLog[i].text.c_str(), {drawX + 2, drawY - 1}, fontSize, fontSpacing, GREEN);
            DrawTextJitter(activeFont, coffeeLog[i].text.c_str(), {drawX, drawY}, fontSize, fontSpacing, RED);
        }
        else if (coffeeLog[i].shakeIntensity > 0)
        {
            DrawTextJitter(activeFont, coffeeLog[i].text.c_str(), {drawX, drawY}, fontSize, fontSpacing, coffeeLog[i].color);
        }
        else
        {
            DrawTextEx(activeFont, coffeeLog[i].text.c_str(), {drawX, drawY}, fontSize, fontSpacing, coffeeLog[i].color);
        }

        Vector2 size = MeasureTextEx(activeFont, coffeeLog[i].text.c_str(), fontSize, fontSpacing);
        currentY += size.y + coffeeLog[i].spacing;
    }

    // 2. Draw active typewriter
    globalTypewriter.Update();

    if (globalTypewriter.active)
    {
        // Chaos “matrix” effect: CTRL+ALT+DELETE ME / 把我強制關機!
        if (globalTypewriter.fullText == deleteLine)
        {
            int repeatCount = (currentLanguage == LANG_CN) ? 70 : 60;
            int rangeX_var = (currentLanguage == LANG_CN) ? 100 : -150;
            int rangeY_var = (currentLanguage == LANG_CN) ? -50 : 20;
            for (int k = 0; k < repeatCount; k++)
            {
                int rawX = (k * 314159 + 12345);
                int rawY = (k * 271828 + 67890);
                int rangeX = (GAME_WIDTH + rangeX_var);
                int px = (rawX % rangeX) - 150;
                int rangeY = (GAME_HEIGHT + rangeY_var);
                int py = (rawY % rangeY);
                if (py < 0)
                    py += rangeY;

                float rSize = 30.0f + ((k * 13) % 25);
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                DrawTextJitter(GetCurrentFont(), sub.c_str(), {(float)px, (float)py}, rSize, fontSpacing, RED);
            }
        }
        else
        {
            float activeX = startX;
            float fontSize = baseFontSize;
            float activeY = currentY;

            // keep “control output” line on same Y as history version
            if (globalTypewriter.fullText == controlLine)
                activeY = 290.0f;

            if (currentCentered)
            {
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                Vector2 size = MeasureTextEx(activeFont, sub.c_str(), fontSize, fontSpacing);
                activeX = (GAME_WIDTH - size.x) / 2.0f;
            }

            if (currentChaotic)
            {
                fontSize += GetRandomValue(-5, 5) / 10.0f;
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                DrawTextJitter(activeFont, sub.c_str(), {activeX - 6, activeY + 3}, fontSize, fontSpacing, BLUE);
                DrawTextJitter(activeFont, sub.c_str(), {activeX + 6, activeY - 3}, fontSize, fontSpacing, GREEN);
                DrawTextJitter(activeFont, sub.c_str(), {activeX, activeY}, fontSize, fontSpacing, RED);
            }
            else if (currentShakeIntensity > 0)
            {
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                DrawTextJitter(activeFont, sub.c_str(), {activeX, activeY}, fontSize, fontSpacing, currentTextColor);
            }
            else
            {
                globalTypewriter.Draw(activeFont, (int)activeX, (int)activeY, fontSize, fontSpacing, currentTextColor);
            }
        }
    }

    // 3. Script logic
    auto AdvanceStep = [&](const char *nextEn, const char *nextCn, int speed, float wait,
                           Color nextColor, int nextShake, bool nextCentered, float nextSpacing, bool nextChaotic)
    {
        if (!globalTypewriter.fullText.empty())
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

        globalTypewriter.Start(L(nextEn, nextCn), speed);
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
            currentSpacing = 30.0f;
            currentChaotic = false;

            globalTypewriter.Start(
                L("THANKS! SLURP...", "謝謝！【吸溜】"),
                50);
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
            AdvanceStep("Analyzing...", "分析中...", 50, 1.0f, WHITE, 0, false, 30.0f, false);
        break;

    case 3:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Is this C8H10N4O2?", "這是C8H10N4O2嗎?", 50, 1.0f, WHITE, 0, false, 30.0f, false);
        break;

    case 4:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Was that... COFFEE?", "這是...咖啡嗎?", 70, 2.0f, WHITE, 1, false, 30.0f, false);
        break;

    case 5:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("Oh no.", "不行。", 50, 1.0f, WHITE, 1, false, 30.0f, false);
            coffeeLog.clear();
        }
        break;

    case 6:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[0]);
            AdvanceStep("Oh no no no.", "完蛋了完蛋了。", 50, 1.0f, WHITE, 1, false, 30.0f, false);
        }
        break;

    case 7:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("Doctor explicitly said:", "博士明確地說過：", 50, 1.75f, WHITE, 1, false, 30.0f, false);
        break;

    case 8:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[1]);
            AdvanceStep("NO. OVERCLOCKING.", "不能。過度運轉。", 70, 2.0f, RED, 1, false, 30.0f, false);
        }
        break;

    case 9:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[2]);
            AdvanceStep("My Clock Frequency is\nreaching 800 MHz.",
                        "我的運行頻率已經達到800 MHz", 40, 1.0f, WHITE, 2, false, 30.0f, false);
            coffeeLog.clear();
        }
        break;

    case 10:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I can see sounds.", "我可以看見聲音", 40, 1.0f, WHITE, 2, false, 30.0f, false);
        break;

    case 11:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I can taste math.", "我可以嚐到數學", 40, 1.0f, WHITE, 2, false, 30.0f, false);
        break;

    case 12:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[3]);
            AdvanceStep("My CPU hurts...", "我的CPU好痛...", 60, 1.5f, WHITE, 3, false, 30.0f, false);
        }
        break;

    case 13:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("The fan... it stopped...", "散熱風扇...停止運作了...", 80, 2.0f, WHITE, 3, false, 30.0f, false);
        break;

    case 14:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("W H A T", "你", 80, 0.6f, RED, 3, true, 30.0f, false);
            coffeeLog.clear();
        }
        break;

    case 15:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("H A V E", "做了", 80, 0.6f, RED, 3, true, 30.0f, false);
        }
        break;

    case 16:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("Y O U", "什麼？", 80, 0.6f, RED, 3, true, 30.0f, false);
        }
        break;

    case 17:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            float wait = 1.4f;
            if (currentLanguage == LANG_EN)
            {
                PlaySound(sndDialup[4]);
                wait = 2.0f;
            }
            AdvanceStep("D O N E ?", "", 80, wait, RED, 3, true, 30.0f, false);
        }
        break;

    case 18:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("I CANNOT CONTROL THE OUTPUT!", "我控制不了我的輸出了!", 30, 1.0f,
                        RED, 3, true, 30.0f, false);
        break;

    case 19:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("P L E A S E", "請你", 30, 1.5f, RED, 3, true, 30.0f, true);
        break;

    case 20:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 30.0f, true});
            globalTypewriter.active = false;
            bgColor = RED;
            coffeeTimer = 0.2f;
            coffeeScriptStep++;
        }
        break;

    case 21: // flash twice
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 30.0f, true});
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

            // fast typing, bilingual
            globalTypewriter.Start(deleteLine, 20);

            currentTextColor = RED;
            currentShakeIntensity = 0;
            currentCentered = false;
            currentSpacing = 10.0f;
            currentChaotic = false;

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
        // Coffee cutscene is already bilingual via HandleCoffeeEvent()
        HandleCoffeeEvent();
        return;
    }

    // --- GOAL 2: PRESS X TO CLOSE ---
    if (IsCancelPressed())
    {
        currentState = MAP_WALK;
        interactionCooldown = 0.2f;
        isStateFirstFrame = true;
        return;
    }

    DrawTexture(texBackground, 0, 0, WHITE);
    DrawTexture(texRobot, (int)mapEnemy.x, (int)mapEnemy.y, WHITE);
    player.Draw();
    int printSpeed = (currentLanguage == LANG_CN) ? 50.0f : 30.0f;
    // Box
    Rectangle box = {25, 450, 750, 200};
    DrawRectangleRec(box, BLACK);
    DrawRectangleLinesEx(box, 4, WHITE);

    // Dialogue font size: slightly bigger for Chinese
    float dialogueFontSize = (currentLanguage == LANG_CN) ? 35.0f : 30.0f;

    globalTypewriter.Update();
    globalTypewriter.Draw(
        GetCurrentFont(),
        (int)(box.x + 25),
        (int)(box.y + 25),
        dialogueFontSize,
        2.0f,
        WHITE);

    // --- GOAL 1: Z BUTTON LOGIC (SKIP vs NEXT) ---
    bool canProceed = false;

    if (IsInteractPressed())
    {
        if (!globalTypewriter.IsFinished())
        {
            // Finish current line instantly
            globalTypewriter.Skip();
        }
        else if (dialogTimer <= 0)
        {
            // Move to next state (with debounce)
            canProceed = true;
        }
    }

    // Helper: choose EN/CN text
    auto L = [&](const char *en, const char *cn)
    {
        return (currentLanguage == LANG_CN) ? cn : en;
    };

    switch (currentDialogueState)
    {
    // ---------------- INTRO ----------------
    case D_INTRO_1:
        if (isStateFirstFrame)
            StartDialogue(
                L("* AAAaaaaa Something is touching me \naaahhhHGGGGAAAAA!!!",
                  "* 誰啊啊啊啊啊啊aaa有東西碰我AAAA啊啊啊啊aaa！！"),
                30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_2;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_2:
        if (isStateFirstFrame)
            StartDialogue("* ...", 40, 0.3f); // same in both languages
        if (canProceed)
        {
            currentDialogueState = D_INTRO_4;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_4:
        if (isStateFirstFrame)
            StartDialogue(
                L("* Sorry, I've been here alone for so\nlong.",
                  "* 抱歉，我還以為鬧鬼了。"),
                printSpeed, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_5;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_5:
        if (isStateFirstFrame)
            StartDialogue(
                L("* I'm actually a nonchalant robot.",
                  "* 我平時其實還挺冷酷的。"),
                printSpeed, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_6;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_6:
        if (isStateFirstFrame)
            StartDialogue(
                L("* Are you a human?",
                  "* 你是人類嗎？"),
                printSpeed, 0.5f);
        if (canProceed)
        {
            currentDialogueState = D_HUMAN_CHOICE;
            menuSelection = 0;
            isStateFirstFrame = true;
        }
        break;

    // ---------------- HUMAN? YES / NO ----------------
    case D_HUMAN_CHOICE:
    {
        if (isStateFirstFrame)
        {
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
        }

        float choiceFontSize = (currentLanguage == LANG_CN) ? 35.0f : 30.0f;
        float choiceY = (currentLanguage == LANG_CN) ? (box.y + 95.0f) : (box.y + 100.0f);

        DrawTextEx(GetCurrentFont(), L("YES", "是的"),
                   {(float)(box.x + 200), choiceY},
                   choiceFontSize, 2, WHITE);
        DrawTextEx(GetCurrentFont(), L("NO", "不是"),
                   {(float)(box.x + 450), choiceY},
                   choiceFontSize, 2, WHITE);

        if (dialogTimer <= 0)
        {
            if (IsRightPressed())
            {
                PlaySound(sndSelect);
                menuSelection = 1;
            }
            if (IsLeftPressed())
            {
                PlaySound(sndSelect);
                menuSelection = 0;
            }
        }

        if (menuSelection == 0)
            DrawTextureEx(texPlayer, {(float)(box.x + 150), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
        else
            DrawTextureEx(texPlayer, {(float)(box.x + 400), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);

        if (canProceed)
        {
            playerChoiceYesNo = menuSelection;
            currentDialogueState = D_HUMAN_RESULT_1;
            isStateFirstFrame = true;
        }
    }
    break;

    case D_HUMAN_RESULT_1:
        if (isStateFirstFrame)
        {
            if (playerChoiceYesNo == 0)
                StartDialogue(
                    L("* First human friend!",
                      "* 第一個人類朋友！"),
                    printSpeed, 0.3f);
            else
                StartDialogue(
                    L("* Then you are the 1,025th rock I've\nmet today.",
                      "* 那你就是我今天聊過的第1025塊石頭了。"),
                    printSpeed, 0.3f);
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
            {
                StartDialogue(
                    L("* I mean. Cool. Whatever.",
                      "* 額，我的意思是怎麼樣都行啦，我不在乎，嗯，對。"),
                    printSpeed, 0.3f);
            }
            else
            {
                StartDialogue(
                    L("* The other rocks were less talkative.",
                      "* 其他的石頭沒你這麼健談。"),
                    printSpeed, 0.3f);
            }
            storyProgress = 1;
        }
        if (canProceed)
        {
            currentDialogueState = D_REQUEST_FOOD_PART1;
            isStateFirstFrame = true;
        }
        break;

    // ---------------- NEED FOOD ----------------
    case D_REQUEST_FOOD_PART1:
        if (isStateFirstFrame)
            StartDialogue(
                L("* My battery is low.",
                  "* 我快沒電了。"),
                printSpeed, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_REQUEST_FOOD;
            isStateFirstFrame = true;
        }
        break;

    case D_REQUEST_FOOD:
    {
        if (isStateFirstFrame)
        {
            std::string txt;
            if (currentLanguage == LANG_CN)
            {
                if (storyProgress == 1)
                    txt = "* 你有吃的嗎？";
                else if (storyProgress == 2)
                    txt = "* 我能再要一點點嗎？";
                else if (storyProgress == 3)
                    txt = "* 再給最後一口？";
            }
            else
            {
                if (storyProgress == 1)
                    txt = "* Do you have any food?";
                else if (storyProgress == 2)
                    txt = "* Can I have one more?";
                else if (storyProgress == 3)
                    txt = "* Just one last byte?";
            }

            globalTypewriter.Start(txt.c_str(), printSpeed);
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
        }

        if (canProceed)
        {
            currentDialogueState = D_REQUEST_FOOD_CHOICE;
            menuSelection = 0;
            isStateFirstFrame = true;
        }
    }
    break;

    case D_REQUEST_FOOD_CHOICE:
    {
        if (isStateFirstFrame)
        {
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
        }

        float choiceFontSize = (currentLanguage == LANG_CN) ? 35.0f : 30.0f;
        float choiceY = (currentLanguage == LANG_CN) ? (box.y + 95.0f) : (box.y + 100.0f);

        DrawTextEx(GetCurrentFont(), L("GIVE", "給予"),
                   {(float)(box.x + 200), choiceY},
                   choiceFontSize, 2, WHITE);
        DrawTextEx(GetCurrentFont(), L("REFUSE", "拒絕"),
                   {(float)(box.x + 430), choiceY},
                   choiceFontSize, 2, WHITE);

        if (dialogTimer <= 0)
        {
            if (IsRightPressed())
            {
                PlaySound(sndSelect);
                menuSelection = 1;
            }
            if (IsLeftPressed())
            {
                PlaySound(sndSelect);
                menuSelection = 0;
            }
        }

        if (menuSelection == 0)
            DrawTextureEx(texPlayer, {(float)(box.x + 150), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
        else
            DrawTextureEx(texPlayer, {(float)(box.x + 380), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);

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

    // ---------------- SELECT ITEM ----------------
    case D_SELECT_ITEM:
    {
        if (isStateFirstFrame)
        {
            globalTypewriter.Start(
                L("Give what?", "給什麼？"),
                0);
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
            menuSelection = 0;
        }

        std::vector<int> opts;
        if (playerInventory.hasCoffee)
            opts.push_back(0);
        if (playerInventory.hasGas)
            opts.push_back(1);
        if (playerInventory.hasBattery)
            opts.push_back(2);

        int itemstartX_var = (currentLanguage == LANG_CN) ? 175 : 100;
        int startX = (int)box.x + itemstartX_var;
        int gap = 120;
        float itemFontSize = (currentLanguage == LANG_CN) ? 35.0f : 30.0f;
        float itemY = (currentLanguage == LANG_CN) ? (box.y + 95.0f) : (box.y + 100.0f);

        for (size_t i = 0; i < opts.size(); i++)
        {
            const char *label = "";
            if (opts[i] == 0)
                label = L("Coffee", "咖啡");
            if (opts[i] == 1)
                label = L("Gas", "汽油");
            if (opts[i] == 2)
                label = L("Battery", "電池");

            Vector2 textSize = MeasureTextEx(GetCurrentFont(), label, itemFontSize, 2);
            DrawTextEx(GetCurrentFont(), label, {(float)startX, itemY},
                       itemFontSize, 2, WHITE);

            if (menuSelection == (int)i)
            {
                DrawTextureEx(texPlayer, {(float)(startX - 50), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
            }
            startX += (int)textSize.x + gap;
        }

        if (dialogTimer <= 0 && !opts.empty())
        {
            if (IsRightPressed() && menuSelection < (int)opts.size() - 1)
            {
                PlaySound(sndSelect);
                menuSelection++;
            }
            if (IsLeftPressed() && menuSelection > 0)
            {
                PlaySound(sndSelect);
                menuSelection--;
            }
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

    // ---------------- EATING ----------------
    case D_EATING:
        if (isStateFirstFrame)
        {
            if (itemUsedIndex == 1)
            {
                StartDialogue(
                    L("* GLUG GLUG...\n* Premium Octane!",
                      "* 【咕嘟咕嘟】耶！97號汽油！"),
                    printSpeed, 0.3f);
            }
            else
            {
                StartDialogue(
                    L("* CRUNCH CRUNCH.\n* That flavor!",
                      "* 【咔呲咔呲】好吃好吃！"),
                    printSpeed, 0.3f);
            }
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

    // ---------------- REFUSAL ----------------
    case D_REFUSAL:
        if (isStateFirstFrame)
            StartDialogue(
                L("* Oh... okay.\n* I'll just go into Sleep Mode\nFOREVER.",
                  "* 噢好吧 ... \n* 那我就要進入一輩子的休眠模式了。"),
                printSpeed, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 0.2f;
            isStateFirstFrame = true;
        }
        break;

    // ---------------- POST BATTLE ----------------
    case D_POST_BATTLE:
        if (isStateFirstFrame)
            StartDialogue(
                L("* I am sorry about what just happened.",
                  "* 我為剛才發生過的事情感到抱歉。"),
                printSpeed, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_POST_BATTLE_2;
            isStateFirstFrame = true;
        }
        break;

    case D_POST_BATTLE_2:
        if (isStateFirstFrame)
            StartDialogue(
                L("* And by the way you just finished\nthe game.",
                  "* 順便說一句，你已經把這個遊戲打完了。"),
                printSpeed, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 0.2f;
            isStateFirstFrame = true;
        }
        break;

    default:
        break;
    }
}

void HandleGameOver()
{
    UpdateMusicStream(gameOver);
    Font activeFont = GetCurrentFont();
    // Choose text based on language
    const char *gameoverStr = Text("GAME OVER", "遊戲結束");
    if (currentLanguage == LANG_EN)
    {
        TextMetrics gameoverM = GetCenteredTextPosition(activeFont, gameoverStr, 80, 2);
        DrawTextEx(activeFont, gameoverStr, {gameoverM.x, 150}, 80, 2, RED);
    }
    else
    {
        TextMetrics gameoverM = GetCenteredTextPosition(activeFont, gameoverStr, 101, 2);
        DrawTextEx(activeFont, gameoverStr, {gameoverM.x, 150}, 101, 2, RED);
    }
    const char *deterStr = Text("Whoever you are... stay determined!", "不管你是誰...都不要放棄!");
    if (currentLanguage == LANG_EN)
    {
        TextMetrics gameoverM = GetCenteredTextPosition(activeFont, deterStr, 25, 2);
        DrawTextEx(activeFont, deterStr, {gameoverM.x, 350}, 25, 2, WHITE);
    }
    else
    {
        TextMetrics gameoverM = GetCenteredTextPosition(activeFont, deterStr, 32, 2);
        DrawTextEx(activeFont, deterStr, {gameoverM.x, 375}, 32, 2, WHITE);
    }
    const char *retryStr = Text("Press Z to Retry", "按Z重試");
    if (currentLanguage == LANG_EN)
    {
        TextMetrics gameoverM = GetCenteredTextPosition(activeFont, retryStr, 30, 2);
        DrawTextEx(activeFont, retryStr, {gameoverM.x, 450}, 30, 2, GRAY);
    }
    else
    {
        TextMetrics gameoverM = GetCenteredTextPosition(activeFont, retryStr, 38, 2);
        DrawTextEx(activeFont, retryStr, {gameoverM.x, 450}, 38, 2, GRAY);
    }

    if (IsInteractPressed())
    {
        StopMusicStream(gameOver); // Stop gameOver music
        currentState = BATTLE;
        InitBattle();
    }
}

int main()
{
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Undertail");
    InitAudioDevice();
    SetTargetFPS(60);

    ChangeDirectory(GetApplicationDirectory());
    LoadGameAssets();

    fontEN = LoadFontEx("assets/determination-mono.otf", 64, 0, 0);

    // ================== SAFE CHINESE FONT LOADING ==================
    // 1. Prepare Codepoints (File Text + UI Chars)
    // 1. Prepare Codepoints (File Text + UI Chars)
    char *textToLoad = LoadFileText("assets/dialogues_CN.txt");
    int codepointCount = 0;
    int *codepoints = NULL;

    if (textToLoad != nullptr)
    {
        std::string allText = std::string(textToLoad);
        codepoints = LoadCodepoints(allText.c_str(), &codepointCount);
        UnloadFileText(textToLoad);
    }
    else
    {
        TraceLog(LOG_WARNING, "dialogues_CN.txt not found! Loading UI chars only.");

        // --- FIX START: Manual Fallback Characters ---
        // If the file is missing, we MUST manually tell Raylib which Chinese characters to load.
        // I included the menu words and title from your code.
        std::string fallbackText = "傳說之上下左右按切換中文是的不是給予拒絕咖啡汽油電池";
        codepoints = LoadCodepoints(fallbackText.c_str(), &codepointCount);
        // --- FIX END ---
    }

    // 2. Load Font Data (WITH SAFETY CHECK)
    unsigned int fileSize = 0;
    unsigned char *fontFileData = LoadFileData("assets/fusion-pixel-12px-proportional-zh_hant.ttf", &fileSize);

    if (fontFileData != NULL && codepointCount > 0)
    {
        fontCN = {0};
        fontCN.baseSize = 64;

        int glyphsLoadedCount = 0;
        GlyphInfo *glyphs = LoadFontData(fontFileData, fileSize, fontCN.baseSize,
                                         codepoints, codepointCount,
                                         FONT_DEFAULT);

        // --- FIX IS HERE: Update the count if glyphs were returned! ---
        if (glyphs != NULL)
        {
            glyphsLoadedCount = codepointCount;
        }

        // Now this check will pass because glyphsLoadedCount is no longer 0
        if (glyphs == NULL || glyphsLoadedCount == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load Chinese glyphs, using default font");
            fontCN = GetFontDefault();
        }
        else
        {
            fontCN.glyphs = glyphs;
            fontCN.glyphCount = glyphsLoadedCount;

            // Generate the texture atlas (the actual image of the letters)
            Image atlas = GenImageFontAtlas(fontCN.glyphs, &fontCN.recs,
                                            glyphsLoadedCount, fontCN.baseSize, 4, 0);
            fontCN.texture = LoadTextureFromImage(atlas);
            UnloadImage(atlas);
        }

        UnloadFileData(fontFileData); // Don't forget to free the raw file data
    }
    else
    {
        TraceLog(LOG_ERROR, "Chinese font file missing, using default font");
        fontCN = GetFontDefault();
    }

    if (codepoints != NULL)
        UnloadCodepoints(codepoints);
    // =================================================================

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    player.Init(125, 300);
    player.SetZones(walkableFloors);

    if (DEBUG_SKIP_TO_BATTLE)
    {
        currentState = BATTLE;
        preBattleX = player.pos.x;
        preBattleY = player.pos.y;
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
        DrawTexturePro(target.texture, {0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height}, {0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()}, {0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadFont(fontEN);
    UnloadFont(fontCN);
    UnloadRenderTexture(target);
    UnloadGameAssets();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}