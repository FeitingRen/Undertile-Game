#include "Utils.h"
#include "Globals.h"

Typewriter globalTypewriter;

void Typewriter::Start(const char* text, int speed) {
    fullText = text;
    speedMs = (float)speed / 1000.0f; // Convert ms to seconds
    charCount = 0;
    timer = 0;
    active = true;
    finished = false;
}

void Typewriter::Update() {
    if (!active || finished) return;

    // Handle Skip
    if (IsInteractPressed() && charCount > 1) {
        Skip();
        return;
    }

    timer += GetFrameTime();
    if (timer >= speedMs) {
        timer = 0;
        charCount++;
        
        // Play sound for non-space characters
        if (charCount <= fullText.length()) {
            if (fullText[charCount-1] != ' ' && fullText[charCount-1] != '\n') {
                if (!IsSoundPlaying(sndText)) PlaySound(sndText);
            }
        }

        if (charCount >= fullText.length()) {
            charCount = fullText.length();
            finished = true;
        }
    }
}

void Typewriter::Skip() {
    charCount = fullText.length();
    finished = true;
}

void Typewriter::Draw(int x, int y, Color color) {
    if (!active) return;
    std::string sub = fullText.substr(0, charCount);
    DrawText(sub.c_str(), x, y, 10, color); // Font size 10 matches typical low-res
}

bool Typewriter::IsFinished() {
    return finished;
}

bool IsInteractPressed() {
    return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER);
}

bool IsCancelPressed() {
    return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_LEFT_SHIFT);
}