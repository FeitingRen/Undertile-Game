#include "Globals.h"

// Actually create the variables here
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- KEYPAD SETUP ---
const byte ROWS = 3; 
const byte COLS = 2; 

char hexaKeys[ROWS][COLS] = {
  {0, 'R'},   // Row 2
  {'U', 'D'}, // Row 3
  {'E', 'L'}  // Row 4
};

byte rowPins[ROWS] = {KEYPAD_R2, KEYPAD_R3, KEYPAD_R4}; 
byte colPins[COLS] = {KEYPAD_C3, KEYPAD_C4}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);