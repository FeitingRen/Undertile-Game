#include "Utils.h"
#include "Globals.h"

Typewriter globalTypewriter;

// --- HELPER FUNCTION: Draw Text with Static Random Jitter ---
// Updated to support Multi-byte UTF-8 Characters (Chinese)
void DrawTextJitter(Font font, const char *text, Vector2 pos, float fontSize, float spacing, Color color)
{
    float startX = pos.x;
    float currentX = pos.x;
    float currentY = pos.y;

    int i = 0; // Byte index
    int k = 0; // Character count (for random seed)

    while (text[i] != '\0')
    {
        // 1. Get the codepoint and how many bytes it uses (1 for English, 3 for Chinese)
        int bytesProcessed = 0;
        int codepoint = GetCodepointNext(&text[i], &bytesProcessed);

        // 2. Handle Newlines
        if (codepoint == '\n')
        {
            currentX = startX;
            currentY += fontSize;
            i += bytesProcessed;
            k++;
            continue;
        }

        // 3. Extract the full Multi-byte character into a temp string
        // Max UTF-8 length is 4 bytes, plus null terminator = 5
        char tempStr[5] = {0};
        for (int b = 0; b < bytesProcessed; b++)
        {
            tempStr[b] = text[i + b];
        }

        // 4. Deterministic Random Jitter Logic (Using 'k' instead of 'i')
        // We use 'k' (character index) so the jitter stays consistent regardless of byte length
        int hashX = k * 43758 + 293;
        int hashY = k * 91238 + 582;

        int rawOx = (hashX % 3) - 1; // -1, 0, 1
        int rawOy = (hashY % 3) - 1; // -1, 0, 1

        float ox = (float)rawOx * 2.0f;
        float oy = (float)rawOy * 2.0f;

        // Apply offset
        Vector2 charPos = {currentX + ox, currentY + oy};

        // Draw the full character (1-4 bytes)
        DrawTextEx(font, tempStr, charPos, fontSize, spacing, color);

        // 5. Advance position by the REAL width of this specific character
        Vector2 size = MeasureTextEx(font, tempStr, fontSize, spacing);
        currentX += size.x + spacing; // Add spacing only between chars, not inside

        // Advance to next character in the string
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

        // Safety check to prevent reading past the end
        if (charCount < fullText.length())
        {
            int bytesProcessed = 0;
            // Raylib helper: Peeks at the text and tells us if next char is 1, 2, 3, or 4 bytes
            GetCodepointNext(&fullText[charCount], &bytesProcessed);

            // Advance by the FULL character length
            charCount += bytesProcessed;
        }
        else
        {
            // Just in case we are somehow at the end
            charCount++;
        }

        // Clamp to length
        if (charCount > fullText.length())
        {
            charCount = fullText.length();
        }

        // Sound Logic (Checks the character *before* the current cursor)
        if (charCount <= fullText.length() && charCount > 0)
        {
            // We look back at the previous character.
            // Note: This naive check [charCount - 1] is risky with UTF-8,
            // but since we only care about Space (' ') and Newline ('\n')
            // which are ALWAYS 1 byte in UTF-8, this specific line is actually safe!
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