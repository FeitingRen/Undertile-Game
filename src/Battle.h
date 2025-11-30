#ifndef BATTLE_H
#define BATTLE_H

#include <Arduino.h>

enum BattlePhase {
    B_INIT,
    B_Q1_SETUP,
    B_Q1_WAIT,
    B_Q1_RESULT,
    B_Q2_SETUP,
    B_Q2_WAIT,
    B_Q2_RESULT,
    B_Q3_SETUP,
    B_Q3_WAIT,
    B_Q3_RESULT,
    B_Q4_SETUP,
    B_Q4_WAIT,
    B_Q4_RESULT,
    B_Q5_SETUP,
    B_Q5_WAIT,
    B_Q5_RESULT,
    B_Q6_SETUP,
    B_Q6_WAIT,
    B_Q6_RESULT,
    B_VICTORY,
    B_GAMEOVER
};

void initBattle();
void updateBattle();
void drawBattle();

#endif