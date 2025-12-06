#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>

// --- GAME CONSTANTS ---
#define GAME_WIDTH 800
#define GAME_HEIGHT 640

#define PLAYER_W 80
#define PLAYER_H 80
#define NPC_SIZE 80

#define PLAYER_MAX_HP 20

// --- ENUMS ---
enum GameState
{
  MENU,
  MAP_WALK,
  DIALOGUE,
  BATTLE,
  GAME_OVER
};

enum DialogueState
{
  D_INTRO_1,
  D_INTRO_2,
  D_INTRO_4,
  D_INTRO_5,
  D_INTRO_6,
  D_HUMAN_CHOICE,
  D_HUMAN_RESULT_1,
  D_HUMAN_RESULT_2,
  D_REQUEST_FOOD_PART1,
  D_REQUEST_FOOD,
  D_REQUEST_FOOD_CHOICE,
  D_SELECT_ITEM,
  D_EATING,
  D_REFUSAL,
  D_COFFEE_EVENT,
  D_POST_BATTLE,
  D_POST_BATTLE_2
};

// --- DATA STRUCTURES ---
struct Inventory
{
  bool hasCoffee;
  bool hasGas;
  bool hasBattery;
};

struct Rect
{
  int x, y, w, h;
};

struct NPC
{
  float x, y;
};

#endif