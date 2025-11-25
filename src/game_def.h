#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#include <Arduino.h>

// --- WIRING DEFINITIONS (ESP32 Standard VSPI) ---
#define TFT_CS        5
#define TFT_RST       4
#define TFT_DC        2
#define JOYSTICK_X    34
#define JOYSTICK_Y    35
#define BUTTON_PIN    0 

// --- GAME CONSTANTS ---
#define PLAYER_W      16
#define PLAYER_H      16
#define NPC_SIZE      20

// --- ENUMS ---
enum GameState { 
  MENU, 
  MAP_WALK, 
  DIALOGUE, 
  BATTLE, 
  GAME_OVER 
};

enum DialogueState {
  D_INTRO,         // "WHAT!!? ... Are you human?"
  D_HUMAN_CHOICE,  // Yes/No selection
  D_HUMAN_RESULT,  // Reaction to Yes/No
  D_REQUEST_FOOD,  // "Battery low..." or "More?" (Based on story progress)
  D_SELECT_ITEM,   // Inventory list
  D_EATING,        // "Crunch crunch"
  D_REFUSAL,       // "Sleep mode..."
  D_COFFEE_EVENT   // "Analyzing..." -> Breakdown
};

// --- DATA STRUCTURES ---
struct Inventory {
  bool hasCoffee;
  bool hasGas;
  bool hasBattery;
};

struct PlayerData {
  float x, y;
  int hp;
};

#endif