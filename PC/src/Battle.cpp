#include "Battle.h"
#include "Globals.h"
#include "Player.h"
#include "Utils.h"
#include <string>

// External State
bool battleCompleted = false;
float preBattleX = 0;
float preBattleY = 0;

// Internal Battle State
BattlePhase battlePhase = B_INIT;
int questionIndex = 0;
float battleTimer = 0.0f;
const float QUESTION_TIME = 3.0f; 

Rect currentBox = {9, 41, 141, 72};
Rect hpBarRect = {25, 118, 50, 6};
Rect timerBarRect = {30, 37, 120, 3};

// Question Data
struct Question {
    std::string text;
    std::string opt1; // Option 1 Text
    std::string opt2; // Option 2 Text
    int safeZone;     // 0=Left/Up, 1=Right/Down
    bool vertical;    // True if split left/right, False if Up/Down
};

// Simplified questions array for the port
std::vector<Question> questions = {
    {"How do I spell\ncongrats?", "Congrate\nlevision", "Congratu\nlations", 1, true},
    {"Should I wear\njacket?", "Yes", "How do I\nknow", 0, false}, // False = Up/Down split
    {"Buy $TSLA?", "No", "Yes", 0, true}
};

bool isAnswerCorrect = true;
std::string resultText = "";

void SetupBox(int x, int y, int w, int h) {
    currentBox = {x, y, w, h};
    // Update player constraints (Zone 0 is the box)
    std::vector<Rect> z;
    z.push_back(currentBox);
    player.SetZones(z);
}

void InitBattle() {
    battlePhase = B_DIALOGUE;
    questionIndex = 0;
    player.hp = PLAYER_MAX_HP;
    player.pos = {73, 71};
    SetupBox(9, 41, 141, 72);
    
    globalTypewriter.Start("I really HATE coffee.\nIt makes me jittery.", 30);
}

void UpdateBattle() {
    float dt = GetFrameTime();

    if (player.hp <= 0) {
        currentState = GAME_OVER;
        return;
    }

    switch (battlePhase) {
        case B_DIALOGUE:
            globalTypewriter.Update();
            if (globalTypewriter.IsFinished() && IsInteractPressed()) {
                battlePhase = B_SETUP_QUESTION;
            }
            break;

        case B_SETUP_QUESTION:
            if (questionIndex >= questions.size()) {
                battlePhase = B_VICTORY;
                globalTypewriter.Start("You survived my quiz!\nFarewell human.", 30);
                return;
            }
            
            // Reset Box and Player
            SetupBox(9, 41, 141, 72); 
            player.pos = {73, 71}; 
            
            battleTimer = 0;
            battlePhase = B_WAIT_ANSWER;
            break;

        case B_WAIT_ANSWER:
            player.Update(dt); // Player moves freely in box
            battleTimer += dt;

            // Timer Logic / Attack Logic
            if (battleTimer > QUESTION_TIME) {
                // Time Up! Check position
                Question& q = questions[questionIndex];
                
                bool hit = false;
                
                // Logic based on split type
                if (q.vertical) {
                    // Left vs Right
                    int midX = currentBox.x + (currentBox.w / 2);
                    // If safe is 0 (Left), hit if x > mid
                    if (q.safeZone == 0 && player.pos.x > midX) hit = true;
                    if (q.safeZone == 1 && player.pos.x < midX) hit = true;
                } else {
                    // Up vs Down
                    int midY = currentBox.y + (currentBox.h / 2);
                    if (q.safeZone == 0 && player.pos.y > midY) hit = true;
                    if (q.safeZone == 1 && player.pos.y < midY) hit = true;
                }

                if (hit) {
                    PlaySound(sndHurt);
                    player.hp -= 5;
                    isAnswerCorrect = false;
                    resultText = "Wrong!";
                } else {
                    isAnswerCorrect = true;
                    resultText = "Correct!";
                }

                battlePhase = B_RESULT;
                battleTimer = 0;
            }
            break;

        case B_RESULT:
            battleTimer += dt;
            if (battleTimer > 1.0f) {
                // Next question
                questionIndex++;
                if (isAnswerCorrect) globalTypewriter.Start("Hmm. Lucky guess.", 30);
                else globalTypewriter.Start("Ha! Take that!", 30);
                battlePhase = B_DIALOGUE;
            }
            break;

        case B_VICTORY:
            globalTypewriter.Update();
            if (globalTypewriter.IsFinished() && IsInteractPressed()) {
                currentState = MAP_WALK;
                battleCompleted = true;
                player.pos = {preBattleX, preBattleY};
                // Reset map zones
                // (Handled in main loop or map handler)
            }
            break;
    }
}

void DrawBattle() {
    ClearBackground(BLACK);

    // 1. Draw Enemy
    DrawTexture(texRobot, 5, 15, WHITE);

    // 2. Draw Dialogue/Question Bubble
    Rectangle bubble = {30, 5, 120, 30};
    DrawRectangleRec(bubble, WHITE);
    DrawRectangleLinesEx(bubble, 1, BLACK);
    
    // Draw Text inside bubble
    if (battlePhase == B_WAIT_ANSWER) {
        DrawText(questions[questionIndex].text.c_str(), bubble.x + 5, bubble.y + 5, 10, BLACK);
    } else {
        globalTypewriter.Draw(bubble.x + 5, bubble.y + 5, BLACK);
    }

    // 3. Draw Box
    DrawRectangleLines(currentBox.x, currentBox.y, currentBox.w, currentBox.h, WHITE);

    // 4. Draw Options & Divider (Only in Wait Phase)
    if (battlePhase == B_WAIT_ANSWER) {
        Question& q = questions[questionIndex];
        int midX = currentBox.x + (currentBox.w / 2);
        int midY = currentBox.y + (currentBox.h / 2);

        if (q.vertical) {
            DrawLine(midX, currentBox.y, midX, currentBox.y + currentBox.h, GRAY);
            DrawText(q.opt1.c_str(), currentBox.x + 10, midY - 5, 10, WHITE);
            DrawText(q.opt2.c_str(), midX + 10, midY - 5, 10, WHITE);
        } else {
            DrawLine(currentBox.x, midY, currentBox.x + currentBox.w, midY, GRAY);
            DrawText(q.opt1.c_str(), midX - 10, currentBox.y + 10, 10, WHITE);
            DrawText(q.opt2.c_str(), midX - 10, midY + 10, 10, WHITE);
        }
    }

    // 5. Draw Result Overlay
    if (battlePhase == B_RESULT) {
        DrawText(resultText.c_str(), 60, 60, 20, isAnswerCorrect ? GREEN : RED);
    }

    // 6. Draw Player
    if (battlePhase != B_VICTORY) player.Draw();

    // 7. HP Bar
    DrawText("HP", 5, 118, 10, WHITE);
    DrawRectangleLines(hpBarRect.x, hpBarRect.y, hpBarRect.w, hpBarRect.h, WHITE);
    float hpPct = (float)player.hp / PLAYER_MAX_HP;
    if (hpPct < 0) hpPct = 0;
    DrawRectangle(hpBarRect.x + 1, hpBarRect.y + 1, (int)((hpBarRect.w - 2) * hpPct), hpBarRect.h - 2, RED);
    DrawText(TextFormat("%d / %d", player.hp, PLAYER_MAX_HP), hpBarRect.x + hpBarRect.w + 5, 118, 10, WHITE);

    // 8. Timer Bar
    if (battlePhase == B_WAIT_ANSWER) {
        float timePct = 1.0f - (battleTimer / QUESTION_TIME);
        DrawRectangle(timerBarRect.x, timerBarRect.y, (int)(timerBarRect.w * timePct), timerBarRect.h, YELLOW);
    }
}