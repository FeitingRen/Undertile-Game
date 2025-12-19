#ifndef BATTLE_H
#define BATTLE_H

#include "game_defs.h"

enum BattlePhase
{
    B_INIT,

    // Intro
    B_Q1_DIALOGUE,

    // Q1: Spelling
    B_Q1_SETUP,
    B_Q1_WAIT,
    B_Q1_RESULT,

    // Q2: Jacket
    B_Q2_DIALOGUE,
    B_Q2_SETUP,
    B_Q2_WAIT,
    B_Q2_RESULT,

    // Q3: Partner
    B_Q3_DIALOGUE,
    B_Q3_SETUP,
    B_Q3_WAIT,
    B_Q3_RESULT,

    // Q4: Grok
    B_Q4_DIALOGUE,
    B_Q4_SETUP,
    B_Q4_WAIT,
    B_Q4_RESULT,

    // Q5: TSLA
    B_Q5_DIALOGUE,
    B_Q5_SETUP,
    B_Q5_WAIT,
    B_Q5_RESULT,

    // Q6: AI Awareness (Tiny Box)
    B_Q6_DIALOGUE,
    B_Q6_SETUP,
    B_Q6_WAIT,
    B_Q6_RESULT,

    // Q7: Consciousness (Tiny Box)
    B_Q7_DIALOGUE,
    B_Q7_SETUP,
    B_Q7_WAIT,
    B_Q7_RESULT,

    // Ending
    B_VICTORY,
    B_GAMEOVER_PHASE
};

void InitBattle();
void UpdateBattle();
void NewFunction();
void DrawBattle();

extern bool battleCompleted;
extern float preBattleX;
extern float preBattleY;

#endif