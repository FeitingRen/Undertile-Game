#ifndef BATTLE_H
#define BATTLE_H

#include <Arduino.h>

enum BattlePhase {
    B_INIT,
    
    // Question 1 Flow
    B_Q1_DIALOGUE, 
    B_Q1_SETUP,
    B_Q1_WAIT,
    B_Q1_RESULT,

    // Question 2 Flow
    B_Q2_DIALOGUE, 
    B_Q2_SETUP,
    B_Q2_WAIT,
    B_Q2_RESULT,

    // Question 3 Flow
    B_Q3_DIALOGUE, 
    B_Q3_SETUP,
    B_Q3_WAIT,
    B_Q3_RESULT,

    // Question 4 Flow
    B_Q4_DIALOGUE, 
    B_Q4_SETUP,
    B_Q4_WAIT,
    B_Q4_RESULT,

    // Question 5 Flow
    B_Q5_DIALOGUE, 
    B_Q5_SETUP,
    B_Q5_WAIT,
    B_Q5_RESULT,

    // Question 6 Flow
    B_Q6_DIALOGUE, 
    B_Q6_SETUP,
    B_Q6_WAIT,
    B_Q6_RESULT,

    // Question 7 Flow
    B_Q7_DIALOGUE, 
    B_Q7_SETUP,
    B_Q7_WAIT,
    B_Q7_RESULT,

    B_VICTORY,
    B_GAMEOVER
};

void initBattle();
bool isInteractPressed();
void setupBox(int x, int y, int w, int h);
void printAligned(const char* text, int x, int y);
void drawSpeechBubble(const char* text, bool instant);
void drawHP();
void updateBattle();
void drawBattle();

#endif