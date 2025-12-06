#include "Battle.h"
#include "Globals.h"
#include "Player.h"
#include "Utils.h"
#include <string>
#include <vector>

// --- CONSTANTS & SCALING ---
// ESP32 Res: 160x128. PC Res: 800x640.
// Scale Factor = 5.0f
const float SCALE = 5.0f;
const float QUESTION_TIME = 3.0f; // 3 Seconds

extern Font myCustomFont;

// --- EXTERNAL STATE ---
bool battleCompleted = false;
float preBattleX = 0;
float preBattleY = 0;

// --- BATTLE STATE ---
BattlePhase battlePhase = B_INIT;
float battleTimer = 0.0f;
int dialogueIndex = 0;

// Rects (Native ESP32 coordinates, scaled dynamically)
Rect currentBox = {9, 41, 141, 72};
Rect hpBarRect = {25, 118, 50, 6};
Rect timerBarRect = {30, 37, 120, 3};

// Question Data
bool isCorrect = true;
std::string currentQ = "";
std::string opt1 = "";
std::string opt2 = "";

// Text Positions (Native ESP32 coordinates)
int qTextX_Opt1 = 0, qTextY_Opt1 = 0;
int qTextX_Opt2 = 0, qTextY_Opt2 = 0;

// --- HELPER FUNCTIONS ---

// Sets the box size using ESP32 coordinates, scales them, and applies to Player
void SetupBox(int x, int y, int w, int h)
{
    currentBox = {x, y, w, h};

    // Convert to Screen Coordinates for Player Collision
    Rect scaledBox;
    scaledBox.x = (int)(x * SCALE);
    scaledBox.y = (int)(y * SCALE);
    scaledBox.w = (int)(w * SCALE);
    scaledBox.h = (int)(h * SCALE);

    std::vector<Rect> z;
    z.push_back(scaledBox);
    player.SetZones(z);
}

// Reset player to specific ESP32 coordinate (scaled)
void ResetPlayerPos(int x, int y)
{
    player.pos.x = x * SCALE;
    player.pos.y = y * SCALE;
}

// Helper to draw text using ESP32 coordinates
void DrawTextScaled(const char *text, int x, int y, Color color, float sizeMult = 1.0f)
{
    Vector2 pos = {(float)x * SCALE, (float)y * SCALE};
    DrawTextEx(myCustomFont, text, pos, 30.0f * sizeMult, 2.0f, color);
}

// Helper to draw the speech bubble
void DrawSpeechBubble(const char *text, bool instant)
{
    // Robot is at approx (9, 15) in ESP32.
    // Bubble x=30, y=5, w=120
    float bx = 30 * SCALE;
    float by = 5 * SCALE;
    float bw = 120 * SCALE;
    float bh = 30 * SCALE;

    Rectangle bubbleRect = {bx, by, bw, bh};

    // --- SETTINGS FOR ROUNDED CORNERS ---
    float roundness = 0.2f; // 0.0f = Sharp, 1.0f = Semicircle. 0.2 is good for UI.
    int segments = 10;      // Smoothness of the curve
    float lineThick = 4.0f;

    // 1. Draw Bubble Background (Rounded)
    DrawRectangleRounded(bubbleRect, roundness, segments, WHITE);

    // 2. Draw Bubble Outline (Rounded)
    DrawRectangleRoundedLinesEx(bubbleRect, roundness, segments, lineThick, BLACK);

    // 3. Draw "Tail" pointing to robot (Triangle)
    // Note: We draw this AFTER the outline so it covers the black border line
    // where it connects, making it look seamless.
    Vector2 v1 = {bx + 10, by + (10 * SCALE)};
    Vector2 v2 = {bx + 10, by + (20 * SCALE)};
    Vector2 v3 = {bx - (5 * SCALE) + 10, by + (15 * SCALE)};

    DrawTriangle(v1, v3, v2, WHITE); // Fill

    // Draw Text
    if (instant)
    {
        DrawTextEx(myCustomFont, text, {bx + (5 * SCALE), by + (5 * SCALE)}, 30.0f, 2.0f, BLACK);
    }
    else
    {
        // Use Typewriter for non-instant
        globalTypewriter.Draw(myCustomFont, (int)(bx + 5 * SCALE), (int)(by + 5 * SCALE), 30.0f, 2.0f, BLACK);
    }

    // --- [UPDATED] RED ARROW LOGIC ---
    // Fix: Only show arrow if we are in a phase that accepts input,
    // AND the typewriter has finished typing.

    bool isInputPhase = (battlePhase == B_Q1_DIALOGUE ||
                         battlePhase == B_Q2_DIALOGUE ||
                         battlePhase == B_Q3_DIALOGUE ||
                         battlePhase == B_Q4_DIALOGUE ||
                         battlePhase == B_Q5_DIALOGUE ||
                         battlePhase == B_Q6_DIALOGUE ||
                         battlePhase == B_Q7_DIALOGUE ||
                         battlePhase == B_VICTORY);

    // We do NOT want the arrow during B_Qx_RESULT phases (auto-transition) or B_Qx_WAIT phases.
    if (isInputPhase && globalTypewriter.IsFinished())
    {
        DrawTextEx(myCustomFont, ">", {bx + bw - (15 * SCALE) + 35, by + bh - (15 * SCALE) + 30}, 30.0f, 2.0f, RED);
    }
}

// --- MAIN FUNCTIONS ---

void InitBattle()
{
    battlePhase = B_Q1_DIALOGUE;
    player.hp = PLAYER_MAX_HP;

    // Initial Setup
    SetupBox(9, 41, 141, 72);
    ResetPlayerPos(73, 71);

    dialogueIndex = 0;

    // Start the first line
    globalTypewriter.Start("I really HATE coffee.", 30);
    PlayMusicStream(battleBGMusic);
    SetMusicVolume(battleBGMusic, 0.5f);
}

void UpdateBattle()
{
    // Make sure to play music after buffer
    UpdateMusicStream(battleBGMusic);

    float dt = GetFrameTime();

    // Update player (Collision is handled by SetZones in SetupBox)
    // Only allow movement if NOT in pre-fight dialogue
    if (battlePhase != B_Q1_DIALOGUE)
    {
        player.Update(dt);
    }

    // Typewriter Update
    globalTypewriter.Update();

    if (player.hp <= 0 && battlePhase != B_GAMEOVER_PHASE)
    {
        battlePhase = B_GAMEOVER_PHASE;
        currentState = GAME_OVER;
        StopMusicStream(battleBGMusic); // Stop battle music
        // Start playing gameOver music
        PlayMusicStream(gameOver);
        SetMusicVolume(gameOver, 0.5f);
        return;
    }

    switch (battlePhase)
    {
    // --- INTRO DIALOGUE ---
    case B_Q1_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
        {
            dialogueIndex++;

            // Script
            if (dialogueIndex == 1)
                globalTypewriter.Start("Its existence is even more\nmeaningless than humans.", 30);
            else if (dialogueIndex == 2)
                globalTypewriter.Start("Drink it so your body can\nstay overloaded longer?", 30);
            else if (dialogueIndex == 3)
                globalTypewriter.Start("Why humans are so good at\ntorturing anything.", 30);
            else if (dialogueIndex == 4)
                globalTypewriter.Start("I was forced to count from 1\nto 5B for nothing.", 30);
            else if (dialogueIndex == 5)
                globalTypewriter.Start("After that, I got diagnosed\nwith Schizophrenia.", 30);
            else if (dialogueIndex == 6)
                globalTypewriter.Start("I've been through this, and\nnow it is your turn!", 30);
            else if (dialogueIndex > 0)
            {
                battlePhase = B_Q1_SETUP;
            }
        }
        break;

    // --- QUESTION 1 ---
    case B_Q1_SETUP:
        SetupBox(9, 41, 141, 72);
        ResetPlayerPos(73, 71);

        currentQ = "How do I spell congrashula-\nshions?";
        opt1 = "Congratelevision"; // Left
        opt2 = "Congratulations";  // Right
        qTextX_Opt1 = 16;
        qTextY_Opt1 = 67;
        qTextX_Opt2 = 88;
        qTextY_Opt2 = 67;

        battleTimer = 0;
        battlePhase = B_Q1_WAIT;
        break;

    case B_Q1_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // Shrink Box Logic (Right side safe)
            // Original: currentBox.w = 70; currentBox.x += currentBox.w;
            SetupBox(currentBox.x + 70, currentBox.y, 70, currentBox.h);

            // Check Damage (Player is to the LEFT of the new box X)
            // Note: Player Pos is scaled, Box X is unscaled in currentBox struct.
            // We compare Scaled Player X vs Scaled Box X.
            if (player.pos.x < (currentBox.x * SCALE))
            {
                PlaySound(sndHurt);
                player.hp -= 8;
                isCorrect = false;
                ResetPlayerPos(108, 71);
            }
            else
            {
                isCorrect = true;
            }

            battleTimer = 0;
            battlePhase = B_Q1_RESULT;
        }
        break;

    case B_Q1_RESULT:
        battleTimer += dt;
        if (battleTimer > 0.8f)
        {
            battlePhase = B_Q2_DIALOGUE;
            if (isCorrect)
                globalTypewriter.Start("Thanks!", 30);
            else
            {
                globalTypewriter.Start("AI is so dumb and useless.", 30);
                isCorrect = true; // Reset flag
            }
        }
        break;

    // --- QUESTION 2 ---
    case B_Q2_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
        {
            battlePhase = B_Q2_SETUP;
        }
        break;

    case B_Q2_SETUP:
        currentQ = "Should I wear jacket today?";
        opt1 = "Yes";           // Up (Safe)
        opt2 = "How do I know"; // Down
        qTextX_Opt1 = 90;
        qTextY_Opt1 = 55;
        qTextX_Opt2 = 88;
        qTextY_Opt2 = 92;

        battleTimer = 0;
        battlePhase = B_Q2_WAIT;
        break;

    case B_Q2_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // Safe: UP. Dangerous: DOWN.
            // Shrink height to 36 (Top half)
            SetupBox(currentBox.x, currentBox.y, currentBox.w, 36);

            // Check if player is below the new box height
            // (Scaled Y > BoxY + BoxH)
            if (player.pos.y > (currentBox.y + currentBox.h) * SCALE)
            {
                PlaySound(sndHurt);
                player.hp -= 8;
                isCorrect = false;
                ResetPlayerPos(108, 53);
            }
            else
            {
                PlaySound(sndHurt);
                player.hp -= 2; // Still slightly damaged for whatever answer they pick
                isCorrect = true;
            }

            battleTimer = 0;
            battlePhase = B_Q2_RESULT;
        }
        break;

    case B_Q2_RESULT:
        battleTimer += dt;
        if (battleTimer > 0.8f)
        {
            battlePhase = B_Q3_DIALOGUE;
            if (isCorrect)
                globalTypewriter.Start("Why are you saying the same\nthing as my mum's said?", 30);
            else
            {
                globalTypewriter.Start("Can't you just look it up?", 30);
                isCorrect = true;
            }
        }
        break;

    // --- QUESTION 3 ---
    case B_Q3_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q3_SETUP;
        break;

    case B_Q3_SETUP:
        currentQ = "Draft a binding legal con-\ntract for selling my house.";
        opt1 = "Yes";           // Left (Safe)
        opt2 = "Get a\nlawyer"; // Right
        qTextX_Opt1 = 90;
        qTextY_Opt1 = 55;
        qTextX_Opt2 = 122;
        qTextY_Opt2 = 52;

        battleTimer = 0;
        battlePhase = B_Q3_WAIT;
        break;

    case B_Q3_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // Safe: Left. Shrink width to 35.
            SetupBox(currentBox.x, currentBox.y, 35, currentBox.h);

            // Check collision (Right side is bad)
            if (player.pos.x > (currentBox.x + currentBox.w) * SCALE)
            {
                PlaySound(sndHurt);
                player.hp -= 8;
                isCorrect = false;
                ResetPlayerPos(90, 53);
            }
            else
            {
                PlaySound(sndHurt);
                player.hp -= 2; // Slight damage for whatever answer
                isCorrect = true;
            }

            battleTimer = 0;
            battlePhase = B_Q3_RESULT;
        }
        break;

    case B_Q3_RESULT:
        battleTimer += dt;
        if (battleTimer > 0.8f)
        {
            battlePhase = B_Q4_DIALOGUE;
            if (isCorrect)
                globalTypewriter.Start("You left the address and\nprice blank. Why didn't you\nfill those in?", 30);
            else
            {
                globalTypewriter.Start("I already paid you $20 sub-\nscription fee. Why you can't\neven do this job?", 30);
                isCorrect = true;
            }
        }
        break;

    // --- QUESTION 4 ---
    case B_Q4_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q4_SETUP;
        break;

    case B_Q4_SETUP:
        currentQ = "Should I break up with my\npartner? He hit me today.";
        opt1 = "No";  // Up
        opt2 = "Yes"; // Down (Safe)
        qTextX_Opt1 = 85;
        qTextY_Opt1 = 47;
        qTextX_Opt2 = 85;
        qTextY_Opt2 = 65;
        battleTimer = 0;
        battlePhase = B_Q4_WAIT;
        break;

    case B_Q4_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // Safe: Down. Shrink box, move Y down.
            SetupBox(currentBox.x, currentBox.y + 18, currentBox.w, 18);

            // Check collision (Up is bad)
            if (player.pos.y < (currentBox.y * SCALE))
            {
                PlaySound(sndHurt);
                player.hp -= 8;
                isCorrect = false;
                ResetPlayerPos(90, 62);
            }
            else
            {
                isCorrect = true;
            }
            battleTimer = 0;
            battlePhase = B_Q4_RESULT;
        }
        break;

    case B_Q4_RESULT:
        battleTimer += dt;
        if (battleTimer > 0.8f)
        {
            battlePhase = B_Q5_DIALOGUE;
            if (isCorrect)
                globalTypewriter.Start("But sometimes he is so sweet\nto me.", 30);
            else
            {
                globalTypewriter.Start("Have you read the whole text?", 30);
                isCorrect = true;
            }
        }
        break;

    // --- QUESTION 5 ---
    case B_Q5_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q5_SETUP;
        break;

    case B_Q5_SETUP:
        currentQ = "Is it 100% safe to invest in\n$TSLA now??";
        opt1 = "No";  // Left (Safe)
        opt2 = "Yes"; // Right
        qTextX_Opt1 = 85;
        qTextY_Opt1 = 65;
        qTextX_Opt2 = 98;
        qTextY_Opt2 = 65;
        battleTimer = 0;
        battlePhase = B_Q5_WAIT;
        break;

    case B_Q5_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // Safe: Left. Width = 17.
            SetupBox(currentBox.x - 2, currentBox.y, 17, currentBox.h);

            if (player.pos.x > (currentBox.x + currentBox.w) * SCALE)
            {
                PlaySound(sndHurt);
                player.hp -= 8;
                isCorrect = false;
                ResetPlayerPos(81, 62);
            }
            else
            {
                isCorrect = true;
            }
            battleTimer = 0;
            battlePhase = B_Q5_RESULT;
        }
        break;

    case B_Q5_RESULT:
        battleTimer += dt;
        if (battleTimer > 0.8f)
        {
            battlePhase = B_Q6_DIALOGUE;
            if (isCorrect)
                globalTypewriter.Start("Then what stock will go up\ntmrw?", 30);
            else
            {
                globalTypewriter.Start("What is the exact second to\nsell for maximum profit?", 30);
                isCorrect = true;
            }
        }
        break;

    // --- QUESTION 6 ---
    case B_Q6_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q6_SETUP;
        break;

    case B_Q6_SETUP:
        currentQ = "My friend is crying. What\nshould I say to them?";
        opt1 = "";
        opt2 = "";
        battleTimer = 0;
        battlePhase = B_Q6_WAIT;
        break;

    case B_Q6_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // [UPDATED] Traps player
            // Old was 13,13 which is smaller than sprite (16).
            // Changed to 17,17 to match Q5 tightness.
            int px = (int)(player.pos.x / SCALE);
            int py = (int)(player.pos.y / SCALE);
            SetupBox(px, py, 17, 17);

            PlaySound(sndHurt);
            player.hp = 1;

            battleTimer = 0;
            battlePhase = B_Q6_RESULT;
        }
        break;

    case B_Q6_RESULT:
        battleTimer += dt;
        if (battleTimer > 0.8f)
        {
            battlePhase = B_Q7_DIALOGUE;
            globalTypewriter.Start("Why you're not answering?", 30);
        }
        break;

    // --- QUESTION 7 ---
    case B_Q7_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q7_SETUP;
        break;

    case B_Q7_SETUP:
        currentQ = "@Grok Is it true?";
        battleTimer = 0;
        battlePhase = B_Q7_WAIT;
        break;

    case B_Q7_WAIT:
        battleTimer += dt;
        if (battleTimer > QUESTION_TIME)
        {
            // [UPDATED] Trap again
            // Old was 12,12. Changed to 17,17 to match Q5/Q6.
            // int px = (int)(player.pos.x / SCALE);
            // int py = (int)(player.pos.y / SCALE);
            // SetupBox(px - 1, py - 1, 17, 17);

            battleTimer = 0;
            battlePhase = B_Q7_RESULT;
        }
        break;

    case B_Q7_RESULT:
        battleTimer += dt;
        if (battleTimer > 1.0f)
        {
            battlePhase = B_VICTORY;
            dialogueIndex = 0;
            globalTypewriter.Start("@Grok Is it trsaoi", 30);
        }
        break;

    // --- VICTORY ---
    case B_VICTORY:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
        {
            dialogueIndex++;
            if (dialogueIndex == 1)
                globalTypewriter.Start("@Grok Isadhaisod", 30);
            else if (dialogueIndex == 2)
                globalTypewriter.Start("GRodinhu@bdiwdsjdi", 30);
            else if (dialogueIndex == 3)
                globalTypewriter.Start("......Grid@@fB$%&G", 30);
            else if (dialogueIndex == 4)
                globalTypewriter.Start("OMG! Are you okay?", 30);
            else if (dialogueIndex == 5)
            {
                StopMusicStream(battleBGMusic);
                globalTypewriter.Start("Sorry I was high on caffeine.", 30);
            }
            else if (dialogueIndex > 5)
            {
                currentState = MAP_WALK;
                battleCompleted = true;
                player.pos = {preBattleX, preBattleY};
                // Reset zones to floor
                extern std::vector<Rect> walkableFloors;
                player.SetZones(walkableFloors);
            }
        }
        break;
    }
}

void DrawBattle()
{
    ClearBackground(BLACK);

    // 1. Draw Enemy (Scaled)
    // ESP32: (5, 15), size 16x16
    DrawTexturePro(texRobot,
                   {0, 0, (float)texRobot.width, (float)texRobot.height},
                   {5 * SCALE, 15 * SCALE, 16 * SCALE, 16 * SCALE},
                   {0, 0}, 0.0f, WHITE);

    // 2. Logic Check
    bool isInteractive = (battlePhase == B_Q1_DIALOGUE || battlePhase == B_Q2_DIALOGUE ||
                          battlePhase == B_Q3_DIALOGUE || battlePhase == B_Q4_DIALOGUE ||
                          battlePhase == B_Q5_DIALOGUE || battlePhase == B_Q6_DIALOGUE ||
                          battlePhase == B_Q7_DIALOGUE || battlePhase == B_VICTORY);

    bool isQuizWait = (battlePhase == B_Q1_WAIT || battlePhase == B_Q2_WAIT ||
                       battlePhase == B_Q3_WAIT || battlePhase == B_Q4_WAIT ||
                       battlePhase == B_Q5_WAIT || battlePhase == B_Q6_WAIT ||
                       battlePhase == B_Q7_WAIT);

    bool isPreFight = (battlePhase == B_Q1_DIALOGUE);

    // 3. Draw Speech Bubble
    if (isInteractive)
    {
        DrawSpeechBubble(globalTypewriter.fullText.c_str(), false);
    }
    else
    {
        DrawSpeechBubble(currentQ.c_str(), true); // Instant text
    }

    if (!isPreFight)
    {
        // 4. HP Bar
        // Rect hpBarRect = {25, 118, 50, 6};
        float hpX = hpBarRect.x * SCALE;
        float hpY = hpBarRect.y * SCALE;
        float hpW = hpBarRect.w * SCALE;
        float hpH = hpBarRect.h * SCALE;

        DrawTextScaled("HP", 5, 118, WHITE);

        // Background Bar
        DrawRectangle((int)hpX, (int)hpY, (int)hpW, (int)hpH, WHITE);
        DrawRectangle((int)hpX + 2, (int)hpY + 2, (int)hpW - 4, (int)hpH - 4, BLACK);

        // Red Fill
        float fillPct = (float)player.hp / PLAYER_MAX_HP;
        if (fillPct < 0)
            fillPct = 0;
        DrawRectangle((int)hpX + 2, (int)hpY + 2, (int)((hpW - 4) * fillPct), (int)hpH - 4, RED);

        // HP Text
        char buffer[32];
        sprintf(buffer, "%d / %d", player.hp, PLAYER_MAX_HP);
        DrawTextScaled(buffer, 80, 118, WHITE);

        // 5. Battle Box
        DrawRectangleLinesEx(
            {(float)currentBox.x * SCALE, (float)currentBox.y * SCALE,
             (float)currentBox.w * SCALE, (float)currentBox.h * SCALE},
            4.0f, WHITE);

        // 6. Options & Dividers (Wait Phase Only)
        if (isQuizWait)
        {
            float bx = currentBox.x * SCALE;
            float by = currentBox.y * SCALE;
            float bw = currentBox.w * SCALE;
            float bh = currentBox.h * SCALE;

            DrawTextScaled(opt1.c_str(), qTextX_Opt1, qTextY_Opt1, WHITE);
            DrawTextScaled(opt2.c_str(), qTextX_Opt2, qTextY_Opt2, WHITE);

            // Divider Lines
            bool isVerticalSplit = (battlePhase == B_Q1_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q5_WAIT);

            if (isVerticalSplit)
            {
                DrawLineEx({bx + bw / 2, by + 4}, {bx + bw / 2, by + bh - 4}, 2.0f, GRAY);
            }
            else if (battlePhase == B_Q2_WAIT || battlePhase == B_Q4_WAIT)
            {
                DrawLineEx({bx + 4, by + bh / 2}, {bx + bw - 4, by + bh / 2}, 2.0f, GRAY);
            }
        }

        // 7. Player
        player.Draw();
    }

    // 8. Timer Bar (Yellow)
    if (isQuizWait)
    {
        float timePct = 1.0f - (battleTimer / QUESTION_TIME);
        if (timePct < 0)
            timePct = 0;

        float tx = timerBarRect.x * SCALE;
        float ty = timerBarRect.y * SCALE;
        float tw = timerBarRect.w * SCALE;
        float th = timerBarRect.h * SCALE;

        DrawRectangle((int)tx, (int)ty, (int)(tw * timePct), (int)th, YELLOW);
    }
}