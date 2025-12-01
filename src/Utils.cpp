#include "Utils.h"
#include "AudioSys.h" 

// --- GRAPHICS HELPER ---
void drawSpriteMixed(int x, int y, const uint16_t* sprite, int w, int h, const uint16_t* bgMap) {
    if (x < 0 || y < 0 || x + w > SCREEN_W || y + h > SCREEN_H) return; 
    uint16_t buffer[256]; 
    for (int row = 0; row < h; row++) {
        int screenY = y + row;
        int bgIndexStart = (screenY * SCREEN_W) + x;
        for (int col = 0; col < w; col++) {
             buffer[row * w + col] = bgMap[bgIndexStart + col];
        }
    }
    for (int i = 0; i < w * h; i++) {
        uint16_t pixel = sprite[i];
        if (pixel != 0x07C0) buffer[i] = pixel; 
    }
    tft.drawRGBBitmap(x, y, buffer, w, h);
}

void typeText(const char* text, int delaySpeed, bool shake) {
  int startX = tft.getCursorX(); int startY = tft.getCursorY(); int originalX = startX; 
  bool hasSkipped = false;

  for (int i = 0; i < strlen(text); i++) {
    if(text[i] == '\n') { startY += 10; startX = originalX; tft.setCursor(startX, startY); continue; }
    
    // --- TIMING FIX ---
    unsigned long timeBeforeAudio = millis();
    
    if (text[i] != ' ') { 
        playVoice(); // Now optimized and uses the dedicated voice buffer
    }
    
    unsigned long timeTaken = millis() - timeBeforeAudio;
    
    if (shake) {
       int ox = random(-1, 2); int oy = random(-1, 2);
       tft.setCursor(startX + ox, startY + oy); tft.print(text[i]);
       startX = tft.getCursorX() - ox;
    } else {
       tft.setCursor(startX, startY); tft.print(text[i]);
       startX = tft.getCursorX(); startY = tft.getCursorY();
    }
    
    customKeypad.getKeys(); 
    for (int k=0; k<LIST_MAX; k++) {
       if (customKeypad.key[k].kstate == PRESSED && customKeypad.key[k].kchar == 'E') {
           delaySpeed = 0; hasSkipped = true;
       }
    }
    
    // --- COMPENSATE FOR AUDIO LATENCY ---
    // Subtract the time it took to queue audio from the visual delay
    // This creates a consistent rhythm regardless of audio glitches
    int remainingDelay = delaySpeed - timeTaken;
    if (remainingDelay > 0) delay(remainingDelay);
  }
  
  if (hasSkipped) delay(200);
}