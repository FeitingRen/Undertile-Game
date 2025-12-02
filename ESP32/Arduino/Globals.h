#ifndef GLOBALS_H
#define GLOBALS_H

#include <Adafruit_ST7735.h>
#include <Keypad.h>
#include "game_defs.h" // Needed for GameState enum

// Hardware Objects
extern Adafruit_ST7735 tft;
extern Keypad customKeypad;

// Shared Game State variables
extern GameState currentState; 

#endif