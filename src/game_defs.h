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
#define PLAYER_W      12
#define PLAYER_H      12
#define NPC_SIZE      16

// --- ENUMS ---
enum GameState { 
  MENU, 
  MAP_WALK, 
  DIALOGUE, 
  BATTLE, 
  GAME_OVER 
};

enum DialogueState {
  D_INTRO_1,         // "WHAT!!? "
  D_INTRO_2,         // "..."
  D_INTRO_4,         // "Sorry, I've been here alone for so long."
  D_INTRO_5,         // "I am actually a nonchalant robot."
  D_INTRO_6,         // "Are you a human?"

  D_HUMAN_CHOICE,    // Yes/No selection
  D_HUMAN_RESULT_1,  // "You are my first friend" OR "You are a rock"
  D_HUMAN_RESULT_2,  // "Cool whatever" OR "Rocks were less talkative"
  
  D_REQUEST_FOOD_PART1, // NEW: "My battery is low..." (Wait)
  D_REQUEST_FOOD,       // "Do you have any food?" OR Round 2/3 texts + Choices
  
  D_SELECT_ITEM,     // Inventory list
  D_EATING,          // "Crunch crunch"
  D_REFUSAL,         // "Sleep mode..."
  D_COFFEE_EVENT     // "Analyzing..." -> Breakdown
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