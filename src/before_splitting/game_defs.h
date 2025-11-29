#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#include <Arduino.h>

// --- WIRING DEFINITIONS (ESP32) ---
// Display (ST7735)
#define TFT_CS        5
#define TFT_RST       4
#define TFT_DC        2
// MOSI = 23, SCK = 18 (VSPI default, no define needed for HW SPI)

// Joystick
#define JOYSTICK_X    34
#define JOYSTICK_Y    35
#define BUTTON_PIN    32  // Updated from 0 to 32

// Audio (MAX98357A) - Ready for future use
#define I2S_BCLK      26
#define I2S_LRC       25
#define I2S_DOUT      22

// MicroSD Card - Ready for future use
#define SD_CS         21
#define SD_MISO       19
#define SD_MOSI       23
#define SD_SCK        18

// --- GAME CONSTANTS ---
// Adjusted screen size for ST7735 Landscape
#define SCREEN_W      160
#define SCREEN_H      128

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

struct PlayerData {
  float x, y;
  int hp;
};

#endif