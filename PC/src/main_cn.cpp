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

// --- main.cpp VARIABLES ---
DialogueState currentDialogueState = D_INTRO_1;
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
    {137, 420, 619, 67}};

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
    TextMetrics titleM = GetCenteredTextPosition(myCustomFont, "傳說之上下左右", 40, 2);
    Vector2 titlePos = {titleM.x, 250}; // Use calculated X, keep Y fixed
    DrawTextEx(myCustomFont, "傳說之上下左右", titlePos, 40, 2, WHITE);

    TextMetrics enterM = GetCenteredTextPosition(myCustomFont, "按Z進入遊戲", 30, 2);
    Vector2 enterPos = {enterM.x, 350};
    DrawTextEx(myCustomFont, "按Z進入遊戲", enterPos, 30, 2, GRAY);

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
        // Flash Black briefly if full red
        if (bgColor.r == 255 && coffeeTimer < 0.05f)
            ClearBackground(BLACK);
        else
            ClearBackground(bgColor);
    }

    // --- CONSTANTS ---
    float startX = 50.0f;
    float currentY = 125.0f;
    float baseFontSize = 40.0f;
    float fontSpacing = 1.5f;

    // --- HELPER LAMBDA: Merges History & Active Drawing Logic ---
    // scaleMult: 1.0 for history, 3.0 for active (makes active text shake harder)
    auto DrawCoffeeLine = [&](const std::string &text, float y, Color col, int shake, bool centered, bool chaotic, float scaleMult)
    {
        float drawX = startX;

        // 1. Calculate Center X if needed
        if (centered)
        {
            Vector2 size = MeasureTextEx(myCustomFont, text.c_str(), baseFontSize, fontSpacing);
            drawX = (GAME_WIDTH - size.x) / 2.0f;
        }

        // 2. Draw Text based on Style
        if (chaotic)
        {
            // Note: Active text uses wider offsets (6,3) vs History (2,1). We use scaleMult to handle this.
            float offX = 2.0f * scaleMult;
            float offY = 1.0f * scaleMult;

            DrawTextJitter(myCustomFont, text.c_str(), {drawX - offX, y + offY}, baseFontSize, fontSpacing, BLUE);
            DrawTextJitter(myCustomFont, text.c_str(), {drawX + offX, y - offY}, baseFontSize, fontSpacing, GREEN);
            DrawTextJitter(myCustomFont, text.c_str(), {drawX, y}, baseFontSize, fontSpacing, RED);
        }
        else if (shake > 0)
        {
            DrawTextJitter(myCustomFont, text.c_str(), {drawX, y}, baseFontSize, fontSpacing, col);
        }
        else
        {
            DrawTextEx(myCustomFont, text.c_str(), {drawX, y}, baseFontSize, fontSpacing, col);
        }

        // 3. Return the height of this line so we know where to draw the next one
        return MeasureTextEx(myCustomFont, text.c_str(), baseFontSize, fontSpacing).y;
    };

    // --- 1. DRAW HISTORY ---
    for (size_t i = 0; i < coffeeLog.size(); i++)
    {
        float drawY = currentY;

        // Overlay Hack: Force specific line position
        if (coffeeLog[i].text == "我控制不了我的輸出了!")
            drawY = 260.0f;

        float h = DrawCoffeeLine(
            coffeeLog[i].text,
            drawY,
            coffeeLog[i].color,
            coffeeLog[i].shakeIntensity,
            coffeeLog[i].centered,
            coffeeLog[i].isChaotic,
            1.0f // Low intensity for history
        );

        currentY += h + coffeeLog[i].spacing;
    }

    // --- 2. DRAW ACTIVE TYPEWRITER ---
    globalTypewriter.Update();

    if (globalTypewriter.active)
    {
        // Special Case: "Force Shutdown" Matrix Effect (Kept separate because it's unique)
        if (globalTypewriter.fullText == "把我強制關機!")
        {
            for (int k = 0; k < 50; k++)
            {
                int rawX = (k * 314159 + 12345);
                int rawY = (k * 271828 + 67890);
                int rangeX = (GAME_WIDTH + 100);
                int px = (rawX % rangeX) - 150;
                int rangeY = (GAME_HEIGHT - 50);
                int py = (rawY % rangeY);
                if (py < 0)
                    py += rangeY;

                float rSize = 30.0f + ((k * 13) % 25);
                std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);
                DrawTextJitter(myCustomFont, sub.c_str(), {(float)px, (float)py}, rSize, fontSpacing, RED);
            }
        }
        // Normal Active Text (Now uses the helper!)
        else
        {
            float activeY = currentY;

            // Note: Your original code had 280 for active and 260 for history.
            // I standardized them both to 260 to prevent the text from "jumping" when it finishes.
            if (globalTypewriter.fullText == "我控制不了我的輸出了!")
                activeY = 260.0f;

            std::string sub = globalTypewriter.fullText.substr(0, globalTypewriter.charCount);

            DrawCoffeeLine(
                sub,
                activeY,
                currentTextColor,
                currentShakeIntensity,
                currentCentered,
                currentChaotic,
                3.0f // High intensity for active text
            );
        }
    }

    // --- 3. SCRIPT LOGIC (AdvanceStep lambda & Switch Case) ---
    // ... (Keep the rest of your logic exactly the same from here down) ...
    auto AdvanceStep = [&](const char *nextText, int speed, float wait,
                           Color nextColor, int nextShake, bool nextCentered, float nextSpacing, bool nextChaotic)
    {
        // ... (copy your existing AdvanceStep content here) ...
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
            globalTypewriter.Start("謝謝！【吸溜】", 50);
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
            AdvanceStep("分析中...", 50, 1.0f, WHITE, 0, false, 23.0f, false);
        break;
    case 3:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("這是C8H10N4O2嗎?", 50, 1.0f, WHITE, 0, false, 23.0f, false);
        break;
    case 4:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("這是...咖啡嗎?", 70, 1.75f, WHITE, 1, false, 23.0f, false);
        break;
    case 5:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            AdvanceStep("不行。", 50, 1.0f, WHITE, 1, false, 23.0f, false);
            coffeeLog.clear();
        }
        break;
    case 6:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[0]);
            AdvanceStep("完蛋了完蛋了。", 50, 1.0f, WHITE, 1, false, 23.0f, false);
        }
        break;
    case 7:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("博士明確地說過：", 50, 1.75f, WHITE, 1, false, 23.0f, false);
        break;
    case 8:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[1]);
            AdvanceStep("不能。過度運轉。", 70, 2.0f, RED, 1, false, 23.0f, false);
        }
        break;
    case 9:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[2]);
            AdvanceStep("我的運行頻率已經達到800 MHz", 40, 1.0f, WHITE, 2, false, 23.0f, false);
            coffeeLog.clear();
        }
        break;
    case 10:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("我可以看見聲音", 40, 1.0f, WHITE, 2, false, 23.0f, false);
        break;
    case 11:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("我可以嚐到數學", 40, 1.0f, WHITE, 2, false, 23.0f, false);
        break;
    case 12:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[3]);
            AdvanceStep("我的CPU好痛...", 60, 1.5f, WHITE, 3, false, 23.0f, false);
            // coffeeLog.clear();
        }
        break;
    case 13:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("散熱風扇...停止運作了...", 80, 2.0f, WHITE, 3, false, 23.0f, false);
        break;
    case 14:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("你", 80, 0.6f, RED, 3, true, 23.0f, false);
            coffeeLog.clear();
        }
        break;
    case 15:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("做了", 80, 0.6f, RED, 3, true, 23.0f, false);
        }
        break;
    case 16:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            PlaySound(sndDialup[4]);
            AdvanceStep("什麼？", 80, 2.0f, RED, 3, true, 23.0f, false);
        }
        break;
    case 17:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("我控制不了我的輸出了!", 30, 1.0f, RED, 3, true, 23.0f, false);
        break;
    case 18:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
            AdvanceStep("請你", 30, 1.5f, RED, 3, true, 23.0f, true);
        break;
    case 19:
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 23.0f, true});
            globalTypewriter.active = false;
            bgColor = RED;
            coffeeTimer = 0.2f;
            coffeeScriptStep++;
        }
        break;
    case 20: // flash twice
        if (globalTypewriter.IsFinished() && coffeeTimer <= 0)
        {
            coffeeLog.push_back({globalTypewriter.fullText, RED, 3, false, 23.0f, true});
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
            PlaySound(sndDialup[5]);

            // Speed 20 is fast typing
            globalTypewriter.Start("把我強制關機!", 20);

            currentTextColor = RED;
            currentShakeIntensity = 0;
            currentCentered = false; // Doesn't matter, we override it in drawing
            currentSpacing = 10.0f;
            currentChaotic = false; // False, because we handle the chaos manually above

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
    globalTypewriter.Draw(myCustomFont, (int)(box.x + 25), (int)(box.y + 25), 35.0f, 2.0f, WHITE);

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
            StartDialogue("* 誰啊啊啊啊啊啊aaa有東西碰我AAAA啊啊啊啊aaa！！", 30, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_2;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_2:
        if (isStateFirstFrame)
            StartDialogue("* ...", 50.0, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_4;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_4:
        if (isStateFirstFrame)
            StartDialogue("* 抱歉，我還以為鬧鬼了。", 50.0, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_5;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_5:
        if (isStateFirstFrame)
            StartDialogue("* 我平時其實還挺冷酷的。", 50.0, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_INTRO_6;
            isStateFirstFrame = true;
        }
        break;

    case D_INTRO_6:
        if (isStateFirstFrame)
            StartDialogue("* 你是人類嗎？", 50.0, 0.5f);
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

        DrawTextEx(myCustomFont, "是的", {(float)(box.x + 200), (float)(box.y + 95)}, 35, 2, WHITE);
        DrawTextEx(myCustomFont, "不是", {(float)(box.x + 450), (float)(box.y + 95)}, 35, 2, WHITE);

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
        {
            DrawTextureEx(texPlayer, {(float)(box.x + 150), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
        }
        else
        {
            DrawTextureEx(texPlayer, {(float)(box.x + 400), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
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
                StartDialogue("* 第一個人類朋友！", 50.0, 0.3f);
            else
                StartDialogue("* 那你就是我今天聊過的第1025塊石頭了。", 50.0, 0.3f);
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
                StartDialogue("* 額，我的意思是怎麼樣都行啦，我不在乎，嗯，對。", 50.0, 0.3f);
            else
                StartDialogue("* 其他的石頭沒你這麼健談。", 50.0, 0.3f);
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
            StartDialogue("* 我快沒電了。", 50.0, 0.3f);
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
                txt = "* 你有吃的嗎？";
            else if (storyProgress == 2)
                txt = "* 我能再要一點點嗎？";
            else if (storyProgress == 3)
                txt = "* 再給最後一口？";

            globalTypewriter.Start(txt.c_str(), 50.0);
            isStateFirstFrame = false;
            dialogTimer = 0.3f;
        }

        // Behavior Change: We wait for the player to press Z (canProceed)
        // BEFORE showing the options.
        if (canProceed)
        {
            currentDialogueState = D_REQUEST_FOOD_CHOICE;
            menuSelection = 0;        // Default to Left option (GIVE)
            isStateFirstFrame = true; // Initialize the next state
        }
        break;

    case D_REQUEST_FOOD_CHOICE:
        // logic matches D_HUMAN_CHOICE pattern
        if (isStateFirstFrame)
        {
            isStateFirstFrame = false;
            dialogTimer = 0.3f; // Buffer to prevent accidental double-press
        }

        // Draw Options
        DrawTextEx(myCustomFont, "給予", {(float)(box.x + 200), (float)(box.y + 95)}, 35, 2, WHITE);
        DrawTextEx(myCustomFont, "拒絕", {(float)(box.x + 430), (float)(box.y + 95)}, 35, 2, WHITE);

        // Handle Navigation
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

        // Draw Heart Cursor
        if (menuSelection == 0)
            DrawTextureEx(texPlayer, {(float)(box.x + 150), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
        else
            DrawTextureEx(texPlayer, {(float)(box.x + 380), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);

        // Handle Selection
        if (IsInteractPressed() && dialogTimer <= 0)
        {
            if (menuSelection == 0)
                currentDialogueState = D_SELECT_ITEM;
            else
                currentDialogueState = D_REFUSAL;
            isStateFirstFrame = true;
        }
        break;

    case D_SELECT_ITEM:
        if (isStateFirstFrame)
        {
            globalTypewriter.Start("給什麼？", 0);
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
                    label = "咖啡";
                if (opts[i] == 1)
                    label = "汽油";
                if (opts[i] == 2)
                    label = "電池";

                Vector2 textSize = MeasureTextEx(myCustomFont, label, 35, 2);
                DrawTextEx(myCustomFont, label, {(float)startX, (float)(box.y + 95)}, 35, 2, WHITE);

                if (menuSelection == (int)i)
                {
                    DrawTextureEx(texPlayer, {(float)(startX - 50), (float)(box.y + 97)}, 0.0f, 0.5f, WHITE);
                }
                startX += (int)textSize.x + gap;
            }

            if (dialogTimer <= 0 && opts.size() > 0)
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

    case D_EATING:
        if (isStateFirstFrame)
        {
            if (itemUsedIndex == 1)
                StartDialogue("* 【咕嘟咕嘟】耶！97號汽油！", 50.0, 0.3f);
            else
                StartDialogue("* 【咔呲咔呲】好吃好吃！", 50.0, 0.3f);
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
            StartDialogue("* 噢好吧 ... \n* 那我就要進入一輩子的休眠模式了。", 50, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 0.2f;
            isStateFirstFrame = true;
        }
        break;

    case D_POST_BATTLE:
        if (isStateFirstFrame)
            StartDialogue("* 我為剛才發生過的事情感到抱歉。", 50.0, 0.3f);
        if (canProceed)
        {
            currentDialogueState = D_POST_BATTLE_2;
            isStateFirstFrame = true;
        }
        break;

    case D_POST_BATTLE_2:
        if (isStateFirstFrame)
            StartDialogue("* 順便說一句，你已經把這個遊戲打完了。", 50.0, 0.3f);
        if (canProceed)
        {
            currentState = MAP_WALK;
            interactionCooldown = 0.2f;
            isStateFirstFrame = true;
        }
        break;

    case D_COFFEE_EVENT:
        // Already handled by the if-statement above
        break;
    }
}

void HandleGameOver()
{
    UpdateMusicStream(gameOver);
    DrawTextEx(myCustomFont, "遊戲結束", {320, 200}, 60, 2, RED);
    DrawTextEx(myCustomFont, "不管你是誰...都不要放棄！", {240, 275}, 35, 2, WHITE);
    DrawTextEx(myCustomFont, "按Z重試", {350, 400}, 35, 2, GRAY);

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

    // 1. Load the text file containing all characters used in the game.
    //    Ensure "dialogues_CN.txt" is saved as UTF-8 (without BOM is best).
    //    We assume it is located in the "assets" folder.
    char *textToLoad = LoadFileText("assets/dialogues_CN.txt");

    // Safety check: Ensure the file was actually found
    if (textToLoad != nullptr)
    {

        // 2. Scan the loaded text to find every unique character ID (Codepoint)
        int codepointCount = 0;
        int *codepoints = LoadCodepoints(textToLoad, &codepointCount);

        // A. Load the raw font file data first
        int fileSize = 0;
        unsigned char *fontFileData = LoadFileData("assets/fusion-pixel-12px-proportional-zh_hant.ttf", &fileSize);

        // B. Initialize the Font struct manually
        myCustomFont = {0};
        myCustomFont.baseSize = 80; // Keep your desired size!
        myCustomFont.glyphCount = codepointCount;

        // C. Load Glyph Data (The math for each character)
        myCustomFont.glyphs = LoadFontData(fontFileData, fileSize, myCustomFont.baseSize, codepoints, codepointCount, FONT_DEFAULT);

        // D. Generate the Atlas with PADDING (The Magic Fix)
        // The '4' here adds 4 pixels of breathing room around every character.
        // This stops the top from being cut off.
        Image atlas = GenImageFontAtlas(myCustomFont.glyphs, &myCustomFont.recs, codepointCount, myCustomFont.baseSize, 4, 0);

        // E. Create the texture from the padded image
        myCustomFont.texture = LoadTextureFromImage(atlas);

        // F. Cleanup helper data (The font struct now owns the texture and glyphs)
        UnloadImage(atlas);
        UnloadFileData(fontFileData); // A. Load the raw font file data first

        // 4. Clean up the codepoints array (we don't need it after loading the font)
        UnloadCodepoints(codepoints);
        // 5. Clean up the text buffer loaded from the file
        UnloadFileText(textToLoad);
    }
    else
    {
        // Fallback if file is missing: Load default English chars (ASCII 0-255)
        // This prevents the game from crashing if the text file is misplaced.
        TraceLog(LOG_WARNING, "Could not find dialogues_CN.txt! Loading default font chars.");
        myCustomFont = LoadFontEx("assets/fusion-pixel-12px-proportional-zh_hant.ttf", 64, 0, 0);
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