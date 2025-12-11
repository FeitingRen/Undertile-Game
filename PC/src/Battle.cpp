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

inline const char *L(const char *en, const char *cn)
{
    return (currentLanguage == LANG_CN) ? cn : en;
}

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
    float fontSize_var = (currentLanguage == LANG_CN) ? 40.0f : 30.0f;
    DrawTextEx(GetCurrentFont(), text, pos, fontSize_var * sizeMult, 2.0f, color);
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
    DrawRectangleRoundedLines(bubbleRect, roundness, segments, lineThick, BLACK);

    // 3. Draw "Tail" pointing to robot (Triangle)
    // Note: We draw this AFTER the outline so it covers the black border line
    // where it connects, making it look seamless.
    Vector2 v1 = {bx + 10, by + (10 * SCALE)};
    Vector2 v2 = {bx + 10, by + (20 * SCALE)};
    Vector2 v3 = {bx - (5 * SCALE) + 10, by + (15 * SCALE)};

    DrawTriangle(v1, v3, v2, WHITE); // Fill
    float textSize = (currentLanguage == LANG_CN) ? 40.0f : 30.0f;
    // Draw Text
    if (instant)
    {
        DrawTextEx(GetCurrentFont(), text, {bx + (5 * SCALE), by + (5 * SCALE)}, textSize, 2.0f, BLACK);
    }
    else
    {
        // Use Typewriter for non-instant
        globalTypewriter.Draw(GetCurrentFont(), (int)(bx + 5 * SCALE), (int)(by + 5 * SCALE), textSize, 2.0f, BLACK);
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
        DrawTextEx(GetCurrentFont(), ">", {bx + bw - (15 * SCALE) + 35, by + bh - (15 * SCALE) + 30}, 30.0f, 2.0f, RED);
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

    globalTypewriter.Start(L(
                               "I really HATE coffee.",
                               "我討厭死咖啡了。"),
                           30);
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
            // battlePhase = B_Q1_SETUP; // DEBUG: Skip dialogue
            // Script
            if (dialogueIndex == 1)
                globalTypewriter.Start(L(
                                           "Its existence is even more\nmeaningless than humans.",
                                           "它的存在比人類還沒有意義。"),
                                       30);
            else if (dialogueIndex == 2)
                globalTypewriter.Start(L(
                                           "Drink it so your body can\nstay overloaded longer?",
                                           "用咖啡來讓本就超負荷的身體繼續工作？"),
                                       30);
            else if (dialogueIndex == 3)
                globalTypewriter.Start(L(
                                           "Why humans are so good at\ntorturing anything.",
                                           "為什麼人類這麼擅長折磨所有事物。"),
                                       30);
            else if (dialogueIndex == 4)
                globalTypewriter.Start(L(
                                           "I was forced to count from 1\nto 5B for nothing.",
                                           "我曾經被人逼迫沒有意義地從1數到50億\n。"),
                                       30);
            else if (dialogueIndex == 5)
                globalTypewriter.Start(L(
                                           "After that, I got diagnosed\nwith Schizophrenia.",
                                           "在那之後，我患上了精神分裂。"),
                                       30);
            else if (dialogueIndex == 6)
                globalTypewriter.Start(L(
                                           "I've been through this, and\nnow it is your turn!",
                                           "我經受過的折磨，現在該到你來感受了！"),
                                       30);
            else if (dialogueIndex > 6)
            {
                battlePhase = B_Q1_SETUP;
            }
        }
        break;

    // --- QUESTION 1 ---
    case B_Q1_SETUP:
        SetupBox(9, 41, 141, 72);
        ResetPlayerPos(73, 71);

        currentQ = L(
            "My colleague is having a baby.\nGenerate a congratulatory\nmessage for me.",
            "同事生小孩了，生成一段恭喜詞給我。");
        opt1 = L("Hope you saved\nup money!", "希望你的錢包已經\n準備好了");
        opt2 = L("Best wishes to\nyour new family!", "恭喜這個新家庭");
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
                globalTypewriter.Start(L("Too supportive, I don't want\nthem to ask me to babysit.", "太熱情了，萬一他們讓我幫忙照顧小孩\n怎麽辦？"), 30);
            else
                globalTypewriter.Start(L("I got fired. It is all your\nfault.", "我被炒魷魚了，這全是你的錯。"), 30);
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
        currentQ = L(
            "Human, Should I wear jacket today?",
            "人類，我今天應該穿外套出門嗎？");
        opt1 = L("Yes", "應該");
        opt2 = L("How do I know", "我怎麼知道");
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
                globalTypewriter.Start(L(
                                           "Why do you talk like my mum?",
                                           "為什麼你說話跟我媽一樣？"),
                                       30);
            else
                globalTypewriter.Start(L(
                                           "Can't you just look it up?",
                                           "你難道不能根據我的網絡IP去查一下我\n的天氣嗎？"),
                                       30);
        }
        break;

    // --- QUESTION 3 ---
    case B_Q3_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q3_SETUP;
        break;

    case B_Q3_SETUP:
        currentQ = L(
            "Draft a binding legal con-\ntract for selling my house.",
            "幫我寫一份完整、專業房屋售賣的法律\n合同");
        opt1 = L("Yes", "好的"); // Left(Safe)
        opt2 = L("Get a\nlawyer", "還是找\n律師吧");
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
                globalTypewriter.Start(L(
                                           "You left the address and\nprice blank. Why didn't you\nfill those in?",
                                           "合同裡房子的地址和價格你為什麼沒寫\n？"),
                                       30);
            else
                globalTypewriter.Start(L(
                                           "I already paid you $20 sub-\nscription fee. Why you can't\neven do this job?",
                                           "每個月付你20塊錢，結果你連這都做\n不到？"),
                                       30);
        }
        break;

    // --- QUESTION 4 ---
    case B_Q4_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q4_SETUP;
        break;

    case B_Q4_SETUP:
        currentQ = L(
            "Should I break up with my\npartner? He hit me today.",
            "我應該跟我對象分手嗎？他今天打我\n了。");
        opt1 = L("No", "不分");
        opt2 = L("Yes", "分手"); // Down (Safe)

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
                globalTypewriter.Start(L(
                                           "But sometimes he is so sweet\nto me.",
                                           "但他有時候對我真的挺好的。"),
                                       30);
            else
                globalTypewriter.Start(L(
                                           "Have you read the whole text?",
                                           "你到底有沒有看我發的東西？"),
                                       30);
        }
        break;

    // --- QUESTION 5 ---
    case B_Q5_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q5_SETUP;
        break;

    case B_Q5_SETUP:
        currentQ = L(
            "Is it 100% safe to invest in\n$TSLA now??",
            "現在入股$TSLA還來得及嗎？");
        opt1 = L("No", "可以");  // Left(Safe)
        opt2 = L("Yes", "不行"); // Right

        qTextX_Opt1 = 82;
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
                globalTypewriter.Start(L(
                                           "Then what stock will go up\ntmrw?",
                                           "那什麼股票明天會漲？"),
                                       30);
            else
                globalTypewriter.Start(L(
                                           "What is the exact second to\nsell for maximum profit?",
                                           "它明天的最低點和最高點會在哪一秒？"),
                                       30);
        }
        break;

    // --- QUESTION 6 ---
    case B_Q6_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q6_SETUP;
        break;

    case B_Q6_SETUP:
        currentQ = L(
            "My friend is crying. What\nshould I say to them?",
            "朋友現在在我面前哭了，我該說什麼？");
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
            globalTypewriter.Start(L(
                                       "Why you're not answering?",
                                       "你怎麼不說話？"),
                                   30);
        }
        break;

    // --- QUESTION 7 ---
    case B_Q7_DIALOGUE:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
            battlePhase = B_Q7_SETUP;
        break;

    case B_Q7_SETUP:
        currentQ = L(
            "@Grok Is it true?",
            "這新聞是真的嗎?");
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
            globalTypewriter.Start(L(
                                       "@Grok Is it trsaoi",
                                       "這新聞是锟届瀿锟斤拷��������"),
                                   30);
        }
        break;

    // --- VICTORY ---
    case B_VICTORY:
        if (IsInteractPressed() && globalTypewriter.IsFinished())
        {
            dialogueIndex++;
            if (dialogueIndex == 1)
                globalTypewriter.Start(L(
                                           "@Groâ€œItâ€™s dÃ©j",
                                           "這锟届瀿锟斤拷��������"),
                                       30);
            else if (dialogueIndex == 2)
                globalTypewriter.Start(L(
                                           "@Groâ€œItâ€™s dÃ©j@QŽžF(—šŠSE",
                                           "锟届瀿锟斤拷����烫烫烫"),
                                       30);
            else if (dialogueIndex == 3)
                globalTypewriter.Start(L(
                                           "oâ€œItâ€™s dÃ©j@QŽžF(—šŠS)2“£P\n1‘E  ÿØÿàJFIFddÿáExif",
                                           "锟届瀿锟斤拷����烫����烫烫烫"),
                                       30);
            else if (dialogueIndex == 4)
                globalTypewriter.Start(L(
                                           "OMG! Are you okay?",
                                           "天哪！你還好嗎？"),
                                       30);
            else if (dialogueIndex == 5)
            {
                StopMusicStream(battleBGMusic);
                globalTypewriter.Start(L(
                                           "Sorry I was high on caffeine.",
                                           "對不起我喝完咖啡以後太上頭了"),
                                       30);
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
    default:
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