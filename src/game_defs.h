#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#include <Arduino.h>

// --- WIRING DEFINITIONS (ESP32) ---
// Display (ST7735)
#define TFT_CS        5
#define TFT_RST       4
#define TFT_DC        2

// --- INPUT: 4x4 KEYPAD ---
#define KEYPAD_R2     13 
#define KEYPAD_R3     14 
#define KEYPAD_R4     27 

#define KEYPAD_C3     32 
#define KEYPAD_C4     33 

// Audio
#define I2S_BCLK      26
#define I2S_LRC       25
#define I2S_DOUT      22

// MicroSD
#define SD_CS         21

// --- GAME CONSTANTS ---
#define SCREEN_W      160
#define SCREEN_H      128

#define PLAYER_W      12
#define PLAYER_H      12
#define NPC_SIZE      16

#define PLAYER_MAX_HP 20

// --- ENUMS ---
enum GameState { 
  MENU, 
  MAP_WALK, 
  DIALOGUE, 
  BATTLE, 
  GAME_OVER 
};

enum DialogueState {
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
  D_SELECT_ITEM,     
  D_EATING,          
  D_REFUSAL,         
  D_COFFEE_EVENT     
};

// --- DATA STRUCTURES ---
struct Inventory {
  bool hasCoffee;
  bool hasGas;
  bool hasBattery;
};

struct Rect { 
  int x, y, w, h; 
};

struct NPC { 
  int x, y; 
};

#endif