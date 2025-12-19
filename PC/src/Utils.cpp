#include "Utils.h"
#include "Globals.h"
#include <vector>
#include <sstream>

Typewriter globalTypewriter;

// --- HELPER: Manual Line Splitting ---
// We use this to ensure lines are drawn with consistent spacing on ALL platforms.
std::vector<std::string> SplitLines(const std::string &text)
{
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line, '\n'))
    {
        lines.push_back(line);
    }
    return lines;
}

// --- HELPER FUNCTION: Draw Text with Static Random Jitter ---
// Updated to support Multi-byte UTF-8 Characters (Chinese)
void DrawTextJitter(Font font, const char *text, Vector2 pos, float fontSize, float spacing, Color color)
{
    float startX = pos.x;
    float currentX = pos.x;
    float currentY = pos.y;

    // Define a consistent line height.
    float lineHeight = fontSize * 1.5f;

    int i = 0; // Byte index
    int k = 0; // Character count (for random seed)

    while (text[i] != '\0')
    {
        // 1. Get the codepoint and how many bytes it uses
        int bytesProcessed = 0;
        int codepoint = GetCodepointNext(&text[i], &bytesProcessed);

        // 2. Handle Newlines
        if (codepoint == '\n')
        {
            currentX = startX;
            currentY += lineHeight; // FIX: Use explicit line height
            i += bytesProcessed;
            k++;
            continue;
        }

        // 3. Extract the full Multi-byte character
        char tempStr[5] = {0};
        for (int b = 0; b < bytesProcessed; b++)
        {
            tempStr[b] = text[i + b];
        }

        // 4. Deterministic Random Jitter Logic
        int hashX = k * 43758 + 293;
        int hashY = k * 91238 + 582;

        int rawOx = (hashX % 3) - 1;
        int rawOy = (hashY % 3) - 1;

        float ox = (float)rawOx * 2.0f;
        float oy = (float)rawOy * 2.0f;

        Vector2 charPos = {currentX + ox, currentY + oy};

        DrawTextEx(font, tempStr, charPos, fontSize, spacing, color);

        // 5. Advance position
        Vector2 size = MeasureTextEx(font, tempStr, fontSize, spacing);
        currentX += size.x + spacing;

        i += bytesProcessed;
        k++;
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

        if (charCount < fullText.length())
        {
            int bytesProcessed = 0;
            GetCodepointNext(&fullText[charCount], &bytesProcessed);
            charCount += bytesProcessed;
        }
        else
        {
            charCount++;
        }

        if (charCount > fullText.length())
        {
            charCount = fullText.length();
        }

        // Sound Logic
        if (charCount <= fullText.length() && charCount > 0)
        {
            char prevChar = fullText[charCount - 1];
            if (prevChar != ' ' && prevChar != '\n')
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

// Replaces DrawTextEx with a manual loop to control Line Height
void Typewriter::Draw(Font font, int x, int y, float fontSize, float spacing, Color color)
{
    if (!active)
        return;

    // 1. Get current visible string
    std::string sub = fullText.substr(0, charCount);

    // 2. Set strict line height (Undertale style usually has gaps)
    // Adjust this multiplier (1.2f to 1.5f) to taste.
    float lineHeight = fontSize * 1.4f;

    float currentY = (float)y;
    int startIdx = 0;

    // 3. Loop through string looking for \n
    for (size_t i = 0; i < sub.length(); i++)
    {
        if (sub[i] == '\n')
        {
            // Draw the line segment we found so far
            std::string line = sub.substr(startIdx, i - startIdx);
            DrawTextEx(font, line.c_str(), {(float)x, currentY}, fontSize, spacing, color);

            // Move cursor down manually
            currentY += lineHeight;
            startIdx = i + 1;
        }
    }

    // 4. Draw the remaining part (or the only part if no newlines)
    if (startIdx < sub.length())
    {
        std::string line = sub.substr(startIdx);
        DrawTextEx(font, line.c_str(), {(float)x, currentY}, fontSize, spacing, color);
    }
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