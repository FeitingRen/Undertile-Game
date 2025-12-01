#include "Battle.h"
#include "Globals.h"
#include "Player.h"
#include "Utils.h"
#include "characters.h" 

// --- BATTLE VARIABLES ---
BattlePhase battlePhase = B_INIT;
unsigned long battleTimer = 0;
const int QUESTION_TIME = 3000; // 3 seconds
//const int AUTO_DIALOGUE_TIME = 2000; // 2 seconds for in-battle text

// DIALOGUE TRACKER
int dialogueIndex = 0; 

// Rects for the box
Rect currentBox = {9, 41, 141, 72}; 
Rect hpBarRect = {25, 118, 50, 6};   // Position of the HP Bar

// Questions Data
bool isCorrect = true;
const char* currentQ = "";
const char* opt1 = "";
const char* opt2 = "";
int qTextX_Opt1 = 0, qTextY_Opt1 = 0;
int qTextX_Opt2 = 0, qTextY_Opt2 = 0;

// REDRAW FLAG: Prevents screen flashing
bool battleRedrawNeeded = true;

// Helper to check for 'E' key press inside Battle
bool isInteractPressed() {
    customKeypad.getKeys(); // Update keypad state
    for (int i=0; i<LIST_MAX; i++) {
        // Check for newly PRESSED 'E'
        if (customKeypad.key[i].kstate == PRESSED && customKeypad.key[i].kchar == 'E') {
            return true;
        }
    }
    return false;
}

// Helper to set zones and box
void setupBox(int x, int y, int w, int h) {
    currentBox = {x, y, w, h};
    player.setZones(&currentBox, 1);
}

void printAligned(const char* text, int x, int y) {
    tft.setCursor(x, y);
    while (*text) {
        if (*text == '\n') {
            y += 10; // Move down 10 pixels (8px text + 2px gap)
            tft.setCursor(x, y); // RESET X to the start position, not 0
        } else {
            tft.print(*text);
        }
        text++;
    }
}

// Helper to draw the speech bubble
// 'instant' = true for questions (so we don't delay the timer)
// 'instant' = false for dialogue (for typing effect)
void drawSpeechBubble(const char* text, bool instant) {
    // Robot is at approx (9, 15), size 16x16.
    // Bubble connects to right side.
    int bx = 30; int by = 5; int bw = 120;
    
    // ADJUST SIZE: Make bubble taller during interactive dialogue (Pre-Fight) to fit more text
    int bh = (battlePhase == B_Q1_DIALOGUE) ? 45 : 30;
    
    // Draw Bubble Background
    tft.fillRoundRect(bx, by, bw, bh, 4, ST7735_WHITE);
    tft.drawRoundRect(bx, by, bw, bh, 4, ST7735_BLACK);
    
    // Draw "Tail" pointing to robot
    tft.fillTriangle(bx, by+10, bx, by+20, bx-5, by+15, ST7735_WHITE);
    
    // Setup Text
    tft.setTextColor(ST7735_BLACK); 
    tft.setTextSize(1);
    
    if (instant) {
        printAligned(text, bx + 5, by + 5);
    } else {
        tft.setCursor(bx + 5, by + 5);
        typeText(text, 30);
    }

    // DRAW INDICATOR (Updated Logic)
    // Show the red ">" for everything EXCEPT the timed Quiz Questions.
    bool isQuizWait = (battlePhase == B_Q1_WAIT || battlePhase == B_Q2_WAIT || 
                       battlePhase == B_Q3_WAIT || battlePhase == B_Q4_WAIT || 
                       battlePhase == B_Q5_WAIT || battlePhase == B_Q6_WAIT || 
                       battlePhase == B_Q7_WAIT);

    if (!isQuizWait) {
        tft.setCursor(bx + bw - 10, by + bh - 8);
        tft.setTextColor(ST7735_RED);
        tft.print(">");
    }
}

void initBattle() {
    battlePhase = B_Q1_DIALOGUE;
    player.hp = PLAYER_MAX_HP;
    
    // SETUP INITIAL VIEW (Hidden in Dialogue, Visible later)
    setupBox(9, 41, 141, 72); 
    player.x = 73; player.y = 71; 
    player.oldX = player.x; player.oldY = player.y; 

    // RESET DIALOGUE
    dialogueIndex = 0; 
    currentQ = "I really HATE\ncoffee."; // Line 0

    battleRedrawNeeded = true; 
}

void drawHP() {
    // Only redraw the text with background color to prevent text overlap
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // Set Bg color to black to overwrite old text
    tft.setTextSize(1);
    tft.setCursor(5, 118);
    tft.print("HP");

    // Redraw Bar Container
    tft.drawRect(hpBarRect.x, hpBarRect.y, hpBarRect.w, hpBarRect.h, ST7735_WHITE);

    // Update Bar Fill
    int fillW = map(player.hp, 0, PLAYER_MAX_HP, 0, hpBarRect.w - 2);
    if (fillW < 0) fillW = 0;
    
    // 1. Clear inner bar (Black)
    tft.fillRect(hpBarRect.x + 1, hpBarRect.y + 1, hpBarRect.w - 2, hpBarRect.h - 2, ST7735_BLACK);
    // 2. Draw Red Fill
    if (fillW > 0) {
        tft.fillRect(hpBarRect.x + 1, hpBarRect.y + 1, fillW, hpBarRect.h - 2, ST7735_RED);
    }

    // Digital Counter
    tft.setCursor(hpBarRect.x + hpBarRect.w + 5, 118);
    tft.printf("%d/%d  ", player.hp, PLAYER_MAX_HP); // Extra spaces to clear old digits
}

void updateBattle() {
    // Only allow movement if we are NOT in the pre-fight dialogue
    if (battlePhase != B_Q1_DIALOGUE) {
        player.update();
    }

    if (player.hp <= 0 && battlePhase != B_GAMEOVER) {
        battlePhase = B_GAMEOVER;
        currentState = GAME_OVER;
        return;
    }

    switch (battlePhase) {
        // --- PRE-FIGHT DIALOGUE FLOW ---
        case B_Q1_DIALOGUE:
            if (isInteractPressed()) { 
                // This ensures the 'E' press doesn't accidentally trigger the "skip typing" 
                // logic in typeText() if it checks for button holds.
                delay(200);

                dialogueIndex++; 
                battleRedrawNeeded = true; 

                if (dialogueIndex == 1) {
                    currentQ = "Its existence is\neven more meaning-\nless than humans.";
                }/* Commenting it out for now to debug faster
                else if (dialogueIndex == 2) {
                    currentQ = "Drink it so your\nbody can stay over-\nloaded longer?";
                }
                else if (dialogueIndex == 3) {
                    currentQ = "Why humans are so\ngood at torturing\nanything.";
                }
                else if (dialogueIndex == 4) {
                    currentQ = "I was forced to\ncount from 1 to 5B\nfor nothing.";
                }
                else if (dialogueIndex == 5) {
                    currentQ = "After that, I got\ndiagnosed with\nSchizo.";
                }
                else if (dialogueIndex == 6) {
                    currentQ = "I've been through\nthis, and now it\nis your turn!";
                }*/
                else if (dialogueIndex > 1) {
                    // End of conversation, START BATTLE
                    battlePhase = B_Q1_SETUP; 
                    
                    // CRITICAL FIX: Prevent drawing this frame.
                    // If we draw now, we'll draw the OLD text ("Its existence")
                    // with the NEW bubble height (30), creating the glitch.
                    // We wait for the next loop where B_Q1_SETUP runs to update the text.
                    battleRedrawNeeded = false; 
                }
            }
            break;
        
        case B_Q1_SETUP:// {9, 41, 141, 72}
            setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); 
            player.x = 73; player.y = 71; 
            player.oldX = player.x; 
            player.oldY = player.y;
            currentQ = "How do I spell con-\ngrashulashions?";
            opt1 = "Congrate\nlevision"; // Left
            opt2 = "Congratu\nlations"; // Right (Safe)
            qTextX_Opt1 = 19; qTextY_Opt1 = 67;
            qTextX_Opt2 = 94; qTextY_Opt2 = 67;
            
            battleTimer = millis();
            battlePhase = B_Q1_WAIT;
            battleRedrawNeeded = true; // Now we are ready to draw the new Question
            break;

        case B_Q1_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up! 
                currentBox.w = 70; // half width
                currentBox.x += currentBox.w;
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h);

                // Check Damage
                if (player.x < (currentBox.x)) {
                    player.hp -= 8;
                    isCorrect = false;
                    player.x = 108; player.y = 71; 
                    player.oldX = player.x; player.oldY = player.y; 
                }
                battleTimer = millis();
                battlePhase = B_Q1_RESULT;
                //battleRedrawNeeded = true; 
            }
            break;

        case B_Q1_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q2_DIALOGUE;
                if (isCorrect){currentQ = "Thanks!";}
                else {
                    currentQ = "AI is so dumb\nand useless.";
                    isCorrect = true;
                }
                battleRedrawNeeded = true; // Ensure transition updates screen
            }
            break;

        // --- QUESTION 2 FLOW ---
        case B_Q2_DIALOGUE:
            if (isInteractPressed()) {
                 battlePhase = B_Q2_SETUP;
            }
            break;

        case B_Q2_SETUP:
            currentQ = "Should I wear jack-\net today?";
            opt1 = "Yes"; // Up (Safe)
            opt2 = "How do I\nknow"; // Down
            qTextX_Opt1 = 88; qTextY_Opt1 = 55;
            qTextX_Opt2 = 88; qTextY_Opt2 = 85;

            battleTimer = millis();
            battlePhase = B_Q2_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q2_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                currentBox.h = 36; 
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); 

                if (player.y > currentBox.y + currentBox.h) {
                    player.hp -= 8;
                    isCorrect = false;
                    player.x = 108; player.y = 53; 
                    player.oldX = player.x; player.oldY = player.y; 
                }

                battleTimer = millis();
                battlePhase = B_Q2_RESULT;
                //battleRedrawNeeded = true;
            }
            break;

        case B_Q2_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q3_DIALOGUE;
                if (isCorrect){
                    currentQ = "Okay.";
                }else {
                    currentQ = "Can't you just look it\nup?";
                    isCorrect = true;
                }
                battleRedrawNeeded = true; // Ensure transition updates screen
            }
            break;
        // --- QUESTION 3 FLOW ---
         case B_Q3_DIALOGUE:
            if (isInteractPressed()) {
                 battlePhase = B_Q3_SETUP;
            }
            break;

        case B_Q3_SETUP:
            currentQ = "Should I break up\nwith my partner?";
            opt1 = "Yes"; // Left (Safe)
            opt2 = "No"; // Right
            qTextX_Opt1 = 84; qTextY_Opt1 = 55;
            qTextX_Opt2 = 119; qTextY_Opt2 = 55;

            battleTimer = millis();
            battlePhase = B_Q3_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q3_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                currentBox.w = 35; 
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); 

                if (player.x > currentBox.x + currentBox.w) {
                    player.hp -= 8;
                    isCorrect = false;
                    player.x = 90; player.y = 53;  
                    player.oldX = player.x; player.oldY = player.y;
                }

                battleTimer = millis();
                battlePhase = B_Q3_RESULT;
                //battleRedrawNeeded = true;
            }
            break;

        case B_Q3_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q4_DIALOGUE;
                if (isCorrect){
                    currentQ = "But sometimes he is\nso sweet to me.";
                }else {
                    currentQ = "Have you read the\nwhole text?";
                    isCorrect = true;
                }
                battleRedrawNeeded = true; // Ensure transition updates screen
            }
            break;
        // --- QUESTION 4 FLOW ---
        case B_Q4_DIALOGUE:
            if (isInteractPressed()) {
                 battlePhase = B_Q4_SETUP;
            }
            break;

        case B_Q4_SETUP:
            currentQ = "@Grok Is it true?";
            opt1 = "No"; // Up
            opt2 = "Yes"; // Down (Safe)
            qTextX_Opt1 = 85; qTextY_Opt1 = 47;
            qTextX_Opt2 = 85; qTextY_Opt2 = 65;

            battleTimer = millis();
            battlePhase = B_Q4_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q4_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                currentBox.h = 18; 
                currentBox.y += currentBox.h;
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); 

                if (player.y < currentBox.y) {
                    player.hp -= 8;
                    isCorrect = false;
                    player.x = 90; player.y = 62; 
                    player.oldX = player.x; player.oldY = player.y;
                }

                battleTimer = millis();
                battlePhase = B_Q4_RESULT;
                //battleRedrawNeeded = true;
            }
            break;

        case B_Q4_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q5_DIALOGUE;
                if (isCorrect){
                    currentQ = "Just double checking.";
                }else {
                    currentQ = "Haha, I know I can't\ntrust Internet.";
                    isCorrect = true;
                }
                battleRedrawNeeded = true; // Ensure transition updates screen
            }
            break;
        // --- QUESTION 5 FLOW ---
        case B_Q5_DIALOGUE:
            if (isInteractPressed()) {
                 battlePhase = B_Q5_SETUP;
            }
            break;

        case B_Q5_SETUP:
            currentQ = "Is it 100% safe to\nbuy $TSLA now??";
            opt1 = "N"; // Left (Safe) 
            opt2 = "Y"; // Right
            qTextX_Opt1 = 85; qTextY_Opt1 = 65;
            qTextX_Opt2 = 106; qTextY_Opt2 = 65;

            battleTimer = millis();
            battlePhase = B_Q5_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q5_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                currentBox.w = 17; 
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); 

                if (player.x > currentBox.x + currentBox.w) {
                    player.hp -= 8;
                    isCorrect = false;
                    player.x = 81; player.y = 62; 
                    player.oldX = player.x; player.oldY = player.y;
                }

                battleTimer = millis();
                battlePhase = B_Q5_RESULT;
               // battleRedrawNeeded = true;
            }
            break;

        case B_Q5_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q6_DIALOGUE;
                if (isCorrect){
                    currentQ = "Then what stock\nwill go up tmrw?";
                }else {
                    currentQ = "Thanks I will all in $TSLA.";
                    isCorrect = true;
                }
                battleRedrawNeeded = true; // Ensure transition updates screen
            }
            break;
        // --- QUESTION 6 FLOW ---
        case B_Q6_DIALOGUE:
            if (isInteractPressed()) {
                 battlePhase = B_Q6_SETUP;
            }
            break;

        case B_Q6_SETUP:
            currentQ = "Are you aware\nthat you're an AI?";
            battleTimer = millis();
            battlePhase = B_Q6_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q6_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                setupBox(player.x, player.y, 13, 13); 
                player.hp = 1;
                battleTimer = millis();
                battlePhase = B_Q6_RESULT;
                //battleRedrawNeeded = true;
            }
            break;

        case B_Q6_RESULT:
           if (millis() - battleTimer > 800) {
                battlePhase = B_Q7_DIALOGUE;
                currentQ = "Why you're not\nanswering?";
                battleRedrawNeeded = true; // Ensure transition updates screen
            }
            break;
        // --- QUESTION 7 FLOW ---
        case B_Q7_DIALOGUE:
            if (isInteractPressed()) {
                 battlePhase = B_Q7_SETUP;
            }
            break;

        case B_Q7_SETUP:
            currentQ = "Do you have consci-\nousness?";
            battleTimer = millis();
            battlePhase = B_Q7_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q7_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                setupBox(player.x, player.y, 13, 13); 
                player.hp = 1;
                battleTimer = millis();
                battlePhase = B_Q7_RESULT;
                //battleRedrawNeeded = true;
            }
            break;

        case B_Q7_RESULT:
            // Wait 2 seconds before starting the ending dialogue
            if (millis() - battleTimer > 1000) {
                battlePhase = B_VICTORY; 
                dialogueIndex = 0;
                currentQ = "Do you have codjsu-\noadishi";
                battleRedrawNeeded = true;
            }
            break;
        
        case B_VICTORY: // Reused as Ending Dialogue Phase
             if (isInteractPressed()) {
                delay(200); // Debounce
                dialogueIndex++;
                battleRedrawNeeded = true;
                if (dialogueIndex == 1) currentQ = "Do you hubdiwdsjdi";
                else if (dialogueIndex == 2) currentQ = "Dodinhubdiwdsjdi";
                else if (dialogueIndex == 3) currentQ = "......Doiddfb$%&G";
                else if (dialogueIndex == 4) currentQ = "OMG! Are you okay?";
                else if (dialogueIndex == 5) currentQ = "Sorry I was high on\ncaffeine.";
                else if (dialogueIndex > 5) {
                     currentState = MAP_WALK;
                }
             }
             break;
    }
}

void drawBattle() {
    bool isPreFight = (battlePhase == B_Q1_DIALOGUE);

    // 1. STATIC DRAWING (Only happens once per state change)
    if (battleRedrawNeeded) {
        tft.fillScreen(ST7735_BLACK);
        
        // Draw Robot
        tft.drawRGBBitmap(5, 15, robot_npc_blk, 16, 16);
        
        // LOGIC UPDATE:
        // You want Dialogues to be Interactive (Typing effect + Red Arrow).
        // You want Questions (WAIT phases) to be Instant.
        // Therefore, we ONLY set this to true for the explicit DIALOGUE phases.
        bool isInteractive = (battlePhase == B_Q1_DIALOGUE || battlePhase == B_Q2_DIALOGUE);
        
        drawSpeechBubble(currentQ, !isInteractive); // !Interactive = Instant

        // --- ONLY DRAW THESE IF NOT IN PRE-FIGHT ---
        if (!isPreFight) {
            // Draw HP Bar
            drawHP();

            // Draw Battle Box
            tft.drawRect(currentBox.x - 1, currentBox.y - 1, currentBox.w + 2, currentBox.h + 2, ST7735_WHITE);

            // Draw Options (Only during Wait Phase)
            if (battlePhase == B_Q1_WAIT || battlePhase == B_Q2_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q4_WAIT || battlePhase == B_Q5_WAIT) {

                tft.setTextColor(ST7735_WHITE);
                // Use new helper instead of raw print to fix alignment
                printAligned(opt1, qTextX_Opt1, qTextY_Opt1);
                printAligned(opt2, qTextX_Opt2, qTextY_Opt2);
                
                // Draw Divider Lines
                if (battlePhase == B_Q1_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q5_WAIT) { //vertical line
                     tft.drawFastVLine(currentBox.x + (currentBox.w/2), currentBox.y, currentBox.h, 0x5555); 
                } else { // horizontal
                     tft.drawFastHLine(currentBox.x, currentBox.y + (currentBox.h/2), currentBox.w, 0x5555);
                }
            }

            // Force draw player once on top of the new background
            player.forceDraw();
        }
        
        battleRedrawNeeded = false;
    }

    // 2. DYNAMIC DRAWING (Every frame)
    player.draw();

    // 3. RESTORE UI ELEMENTS (Fixes erasing issues)
    // Only perform restoration if the player actually moved (to save performance)
    if ((int)player.x != (int)player.oldX || (int)player.y != (int)player.oldY) {
        
        // Only restore during relevant phases where UI exists
        if (battlePhase == B_Q1_WAIT || battlePhase == B_Q2_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q4_WAIT || battlePhase == B_Q5_WAIT) {
            
            // Restore Divider Lines (Draw full line unconditionally to fill any erased gaps)
            if (battlePhase == B_Q1_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q5_WAIT) { 
                 tft.drawFastVLine(currentBox.x + (currentBox.w/2), currentBox.y, currentBox.h, 0x5555); 
            } else { 
                 tft.drawFastHLine(currentBox.x, currentBox.y + (currentBox.h/2), currentBox.w, 0x5555);
            }

            // Restore Option Text
            // We use white text without a background color. Since the erased area is black, this restores the text perfectly.
            tft.setTextColor(ST7735_WHITE);
            tft.setTextSize(1);
            // Previous code used tft.print() which resets X to 0 on newline.
            printAligned(opt1, qTextX_Opt1, qTextY_Opt1);
            printAligned(opt2, qTextX_Opt2, qTextY_Opt2);

            // 4. PLAYER LAYER
            // Now that we've restored the background, the player might be covered by the line/text we just drew.
            // We force-draw the player again to ensure the heart remains on top of everything.
            player.forceDraw();
        }
    }
}