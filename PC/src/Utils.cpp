#include "Utils.h"
#include "Globals.h"

Typewriter globalTypewriter;

// --- HELPER FUNCTION: Draw Text with Static Random Jitter ---
// Replaces the "Sine Wave" look with a "Messy/Chaotic" look
// matching the ESP32 random(-1, 2) logic, but deterministic (static).
void DrawTextJitter(Font font, const char *text, Vector2 pos, float fontSize, float spacing, Color color)
{
    float startX = pos.x;
    float currentX = pos.x;
    float currentY = pos.y;

    for (int i = 0; text[i] != '\0'; i++)
    {
        char c = text[i];

        if (c == '\n')
        {
            currentX = startX;
            currentY += fontSize;
            continue;
        }

        // --- NEW LOGIC: Deterministic Random ---
        // mimics ESP32's random(-1, 2) which produces -1, 0, or 1.
        // We use the index 'i' multiplied by large primes to create a pseudo-random pattern.
        // This ensures the text looks "messy" but DOES NOT shake (it is static).

        int hashX = i * 43758 + 293;
        int hashY = i * 91238 + 582;

        // (hash % 3) -> 0, 1, 2
        // Subtract 1 -> -1, 0, 1
        int rawOx = (hashX % 3) - 1;
        int rawOy = (hashY % 3) - 1;

        // Scale up by 2.0f because PC resolution (800x640) is much higher than ESP32 (160x128).
        // On ESP32, 1 pixel is huge. On PC, 1 pixel is barely visible.
        float ox = (float)rawOx * 2.0f;
        float oy = (float)rawOy * 2.0f;

        char tempStr[2] = {c, '\0'};

        // Apply offset to drawing position
        Vector2 charPos = {currentX + ox, currentY + oy};

        DrawTextEx(font, tempStr, charPos, fontSize, spacing, color);

        // Advance X by the REAL width (ignoring jitter) so spacing stays consistent
        // This matches your ESP32 logic: "startX = tft.getCursorX() - ox"
        Vector2 size = MeasureTextEx(font, tempStr, fontSize, spacing);
        currentX += size.x + spacing;
    }
}

void Typewriter::Start(const char *text, int speed)
{
    fullText = text;
    speedMs = (float)speed / 1000.0f;
    charCount = 0;
    timer = 0;
    active = true;
    finished = false;
}

void Typewriter::Update()
{
    if (!active || finished)
        return;

    timer += GetFrameTime();
    if (timer >= speedMs)
    {
        timer = 0;
        charCount++;

        if (charCount <= fullText.length())
        {
            if (fullText[charCount - 1] != ' ' && fullText[charCount - 1] != '\n')
            {
                if (!IsSoundPlaying(sndText))
                    PlaySound(sndText);
            }
        }

        if (charCount >= fullText.length())
        {
            charCount = fullText.length();
            finished = true;
        }
    }
}

void Typewriter::Skip()
{
    charCount = fullText.length();
    finished = true;
}

void Typewriter::Draw(Font font, int x, int y, float fontSize, float spacing, Color color)
{
    if (!active)
        return;

    std::string sub = fullText.substr(0, charCount);
    Vector2 position = {(float)x, (float)y};

    // Standard straight drawing (Default)
    DrawTextEx(font, sub.c_str(), position, fontSize, spacing, color);
}

bool Typewriter::IsFinished()
{
    return finished;
}

bool IsInteractPressed()
{
    return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER);
}

bool IsCancelPressed()
{
    return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_LEFT_SHIFT);
}