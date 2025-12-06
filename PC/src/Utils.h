#ifndef UTILS_H
#define UTILS_H

#include "game_defs.h"

// Non-blocking Typewriter class
class Typewriter
{
public:
    std::string fullText;
    int charCount;
    float timer;
    float speedMs;
    bool active;
    bool finished;

    void Start(const char *text, int speed);
    void Update();

    // Draws text with a static vertical offset per character
    void Draw(Font font, int x, int y, float fontSize, float spacing, Color color);

    bool IsFinished();
    void Skip();
};

extern Typewriter globalTypewriter;

// Helper to check for "Interact" key (Z or Enter)
bool IsInteractPressed();
bool IsCancelPressed();

// --- RENDERING HELPERS ---
// New helper to draw any text with the "Jitter" style (Up/Down offsets)
void DrawTextJitter(Font font, const char *text, Vector2 pos, float fontSize, float spacing, Color color);
#endif