#ifndef UTILS_H
#define UTILS_H

#include "game_defs.h"

// Non-blocking Typewriter class
class Typewriter {
public:
    std::string fullText;
    int charCount;
    float timer;
    float speedMs;
    bool active;
    bool finished;

    void Start(const char* text, int speed);
    void Update();
    void Draw(int x, int y, Color color = WHITE);
    bool IsFinished();
    void Skip();
};

extern Typewriter globalTypewriter;

// Helper to check for "Interact" key (Z or Enter)
bool IsInteractPressed();
bool IsCancelPressed();

#endif