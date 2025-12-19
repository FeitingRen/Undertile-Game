#ifndef TEXT_ALIGNMENT_H
#define TEXT_ALIGNMENT_H

#include "raylib.h"

struct TextMetrics
{
    float width;
    float x;
};

// Calculates the X coordinate needed to center it
inline TextMetrics GetCenteredTextPosition(Font font, const char *text, float fontSize, float spacing, float screenWidth = 800.0f)
{
    // Measure the text using the custom font
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    float x = (screenWidth - size.x) / 2.0f;
    return {size.x, x};
}

#endif