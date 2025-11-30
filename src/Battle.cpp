#include "Battle.h"
#include "Globals.h"
#include "Player.h"
#include "Utils.h"
#include "characters.h" 

// --- BATTLE VARIABLES ---
BattlePhase battlePhase = B_INIT;
unsigned long battleTimer = 0;
const int QUESTION_TIME = 5000; // 5 seconds

// Rects for the box
Rect currentBox = {9, 41, 141, 72}; 
Rect hpBarRect = {25, 118, 50, 6};   // Position of the HP Bar

// Questions Data
const char* currentQ = "";
const char* opt1 = "";
const char* opt2 = "";
int qTextX_Opt1 = 0, qTextY_Opt1 = 0;
int qTextX_Opt2 = 0, qTextY_Opt2 = 0;

// REDRAW FLAG: Prevents screen flashing
bool battleRedrawNeeded = true;

// Helper to set zones and box
void setupBox(int x, int y, int w, int h) {
    currentBox = {x, y, w, h};
    player.setZones(&currentBox, 1);
}

void initBattle() {
    battlePhase = B_Q1_SETUP;
    player.hp = PLAYER_MAX_HP;
    
    // Default Battle Setup
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
    player.update();

    if (player.hp <= 0 && battlePhase != B_GAMEOVER) {
        battlePhase = B_GAMEOVER;
        currentState = GAME_OVER;
        return;
    }

    switch (battlePhase) {
        case B_Q1_SETUP:// {9, 41, 141, 72}
            setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); 
            player.x = 73; player.y = 71; 
            
            // FIX ISSUE 3: Sync old position to prevent giant black smear (erasure trail)
            player.oldX = player.x; 
            player.oldY = player.y;

            currentQ = "What is 1 + 1?";
            opt1 = "3"; // Left
            opt2 = "2"; // Right (Safe)
            qTextX_Opt1 = 24; qTextY_Opt1 = 77;
            qTextX_Opt2 = 132; qTextY_Opt2 = 77;
            
            battleTimer = millis();
            battlePhase = B_Q1_WAIT;
            battleRedrawNeeded = true; // Trigger full redraw for new question
            break;

        case B_Q1_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up! 
        
                currentBox.w = 70; // half width
                currentBox.x += currentBox.w;
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h);

                // Check Damage: If player is to the LEFT of new X
                if (player.x < (currentBox.x)) {
                    player.hp -= 8;
                    player.x = 108; player.y = 71;  // Respawn inside
                    player.oldX = player.x; player.oldY = player.y; // Sync for respawn too
                }

                battleTimer = millis();
                battlePhase = B_Q1_RESULT;
                battleRedrawNeeded = true; // Redraw to show shrunk box
            }
            break;

        case B_Q1_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q2_SETUP;
            }
            break;

        case B_Q2_SETUP:
            // No need to set up new box, continue using the last cropped box {79, 41, 70, 72}
            currentQ = "What is 2 + 2?";
            opt1 = "4"; // Up (Safe)
            opt2 = "3"; // Down
            qTextX_Opt1 = 94; qTextY_Opt1 = 57;
            qTextX_Opt2 = 94; qTextY_Opt2 = 93;

            battleTimer = millis();
            battlePhase = B_Q2_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q2_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up!  
                
                currentBox.h = 36; // half height 
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); // Q3: {79, 41, 70, 36}

                // Check Damage: If player is BELOW the new Y
                if (player.y > currentBox.y + currentBox.h) {
                    player.hp -= 8;
                    player.x = 108; player.y = 53;  // Respawn inside
                    player.oldX = player.x; player.oldY = player.y; 
                }

                battleTimer = millis();
                battlePhase = B_Q2_RESULT;
                battleRedrawNeeded = true;
            }
            break;

        case B_Q2_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q3_SETUP;
            }
            break;
        
        case B_Q3_SETUP:// {79, 41, 70, 36}
            currentQ = "What is 3 + 3?";
            opt1 = "6"; // Left (Safe)
            opt2 = "7"; // Right
            qTextX_Opt1 = 94; qTextY_Opt1 = 57;
            qTextX_Opt2 = 132; qTextY_Opt2 = 57;

            battleTimer = millis();
            battlePhase = B_Q3_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q3_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up!  
                
                currentBox.w = 35; // half width 
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); // Q4: {79, 41, 35, 36}

                // Take Damage: If player is at the RIGHT
                if (player.x > currentBox.x + currentBox.w) {
                    player.hp -= 8;
                    player.x = 90; player.y = 53;  // Respawn inside
                    player.oldX = player.x; player.oldY = player.y;
                }

                battleTimer = millis();
                battlePhase = B_Q3_RESULT;
                battleRedrawNeeded = true;
            }
            break;

        case B_Q3_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q4_SETUP;
            }
            break;
        
        case B_Q4_SETUP:// {79, 41, 35, 36}
            currentQ = "What is 4 + 4?";
            opt1 = "9"; // Up 
            opt2 = "8"; // Down (Safe)
            qTextX_Opt1 = 85; qTextY_Opt1 = 47;
            qTextX_Opt2 = 85; qTextY_Opt2 = 67;

            battleTimer = millis();
            battlePhase = B_Q4_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q4_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up!  
                
                currentBox.h = 18; // half height 
                currentBox.y += currentBox.h;
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); // Q5: {79, 59, 35, 18}

                // Take Damage: If player is ABOVE the new Y
                if (player.y < currentBox.y) {
                    player.hp -= 8;
                    player.x = 90; player.y = 62;  // Respawn inside
                    player.oldX = player.x; player.oldY = player.y;
                }

                battleTimer = millis();
                battlePhase = B_Q4_RESULT;
                battleRedrawNeeded = true;
            }
            break;

        case B_Q4_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q5_SETUP;
            }
            break;

        case B_Q5_SETUP:// {79, 59, 35, 18}
            currentQ = "What is 4 + 1?";
            opt1 = "5"; // Left (Safe) 
            opt2 = "0"; // Right
            qTextX_Opt1 = 85; qTextY_Opt1 = 67;
            qTextX_Opt2 = 106; qTextY_Opt2 = 67;

            battleTimer = millis();
            battlePhase = B_Q5_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q5_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up!  
                
                currentBox.w = 17; // half width 
                setupBox(currentBox.x, currentBox.y, currentBox.w, currentBox.h); // {79, 59, 17, 18}

                // Check Damage: If player is at the RIGHT
                if (player.x > currentBox.x + currentBox.w) {
                    player.hp -= 8;
                    player.x = 81; player.y = 62;  // Respawn inside
                    player.oldX = player.x; player.oldY = player.y;
                }

                battleTimer = millis();
                battlePhase = B_Q5_RESULT;
                battleRedrawNeeded = true;
            }
            break;

        case B_Q5_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_Q6_SETUP;
            }
            break;


        case B_Q6_SETUP:// {79, 59, 17, 18}
            currentQ = "What is 1 + 1?";
            // the box now is too small to choose, so there is no need to print the opt anymore
            battleTimer = millis();
            battlePhase = B_Q6_WAIT;
            battleRedrawNeeded = true;
            break;

        case B_Q6_WAIT:
            if (millis() - battleTimer > QUESTION_TIME) {
                // Time's up!  
                // Make the box tightly lock up the sprite
                setupBox(player.x, player.y, 13, 13); 
                player.hp -= 8;

                battleTimer = millis();
                battlePhase = B_Q6_RESULT;
                battleRedrawNeeded = true;
            }
            break;

        case B_Q6_RESULT:
            if (millis() - battleTimer > 800) {
                battlePhase = B_VICTORY;
                currentState = MAP_WALK; 
            }
            break;
    }
}

void drawBattle() {
    // 1. STATIC DRAWING (Only happens once per state change)
    if (battleRedrawNeeded) {
        tft.fillScreen(ST7735_BLACK);
        
        // Draw Robot
        tft.drawRGBBitmap(9, 15, robot_npc_blk, 16, 16);
        
        // Draw Text
        tft.setCursor(35, 6);
        tft.setTextColor(ST7735_WHITE);
        tft.setTextSize(1);
        tft.print(currentQ);

        // Draw Battle Box
        tft.drawRect(currentBox.x - 1, currentBox.y - 1, currentBox.w + 2, currentBox.h + 2, ST7735_WHITE);

        // Draw Options (Only during Wait Phase)
        if (battlePhase == B_Q1_WAIT || battlePhase == B_Q2_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q4_WAIT || battlePhase == B_Q5_WAIT) {
            tft.setCursor(qTextX_Opt1, qTextY_Opt1);
            tft.print(opt1);
            tft.setCursor(qTextX_Opt2, qTextY_Opt2);
            tft.print(opt2);
            
            // Draw Divider Lines
            if (battlePhase == B_Q1_WAIT || battlePhase == B_Q3_WAIT || battlePhase == B_Q5_WAIT) { //vertical line
                 tft.drawFastVLine(currentBox.x + (currentBox.w/2), currentBox.y, currentBox.h, 0x5555); 
            } else { // horizontal
                 tft.drawFastHLine(currentBox.x, currentBox.y + (currentBox.h/2), currentBox.w, 0x5555);
            }
        }
        
        // Force draw player once on top of the new background
        player.forceDraw();
        
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
            tft.setCursor(qTextX_Opt1, qTextY_Opt1);
            tft.print(opt1);
            tft.setCursor(qTextX_Opt2, qTextY_Opt2);
            tft.print(opt2);

            // 4. PLAYER LAYER
            // Now that we've restored the background, the player might be covered by the line/text we just drew.
            // We force-draw the player again to ensure the heart remains on top of everything.
            player.forceDraw();
        }
    }

    // Draw HP Bar
    drawHP();
}