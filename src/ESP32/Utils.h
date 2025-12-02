#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "Globals.h"

// Prototypes for helper functions
void drawSpriteMixed(int x, int y, const uint16_t* sprite, int w, int h, const uint16_t* bgMap);
void typeText(const char* text, int delaySpeed, bool shake = false);

#endif