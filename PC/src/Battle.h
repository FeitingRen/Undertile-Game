#ifndef BATTLE_H
#define BATTLE_H

#include "game_defs.h"

enum BattlePhase {
    B_INIT,
    B_DIALOGUE,
    B_SETUP_QUESTION,
    B_WAIT_ANSWER,
    B_RESULT,
    B_VICTORY,
    B_GAMEOVER_PHASE
};

void InitBattle();
void UpdateBattle();
void DrawBattle();

extern bool battleCompleted;
extern float preBattleX;
extern float preBattleY;

#endif