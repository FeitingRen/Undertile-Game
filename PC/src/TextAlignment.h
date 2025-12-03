#ifndef TEXT_ALIGNMENT_H
#define TEXT_ALIGNMENT_H

#include "raylib.h"

// A simple structure to hold the calculated width and the X position
struct TextMetrics {
    float width;
    float x;
};

// Calculates the width of the text and the X coordinate needed to center it
// screenWidth defaults to 800 (your game width), but can be changed if needed.
inline TextMetrics GetCenteredTextPosition(Font font, const char* text, float fontSize, float spacing, float screenWidth = 800.0f) {
    // Measure the text using the custom font
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    
    // Calculate the centered X position
    // Formula: (Screen Width - Text Width) / 2
    float x = (screenWidth - size.x) / 2.0f;
    
    return { size.x, x };
}

#endif