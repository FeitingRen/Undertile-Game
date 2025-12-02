#include "Utils.h"
#include "AudioSys.h" 

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
  unsigned long startFuncTime = millis(); // Track when typing started

  for (int i = 0; i < strlen(text); i++) {
    // Pump audio at start of loop
    updateSFX(); 

    if(text[i] == '\n') { startY += 10; startX = originalX; tft.setCursor(startX, startY); continue; }
    
    if (text[i] != ' ') { 
        playVoice();
    }

    if (shake) {
       int ox = random(-1, 2); int oy = random(-1, 2);
       tft.setCursor(startX + ox, startY + oy); tft.print(text[i]);
       startX = tft.getCursorX() - ox;
    } else {
       tft.setCursor(startX, startY); tft.print(text[i]);
       startX = tft.getCursorX(); startY = tft.getCursorY();
    }
    
    // --- FIX: INPUT DEBOUNCE ---
    // Only check for skip ('E') if 200ms has passed since the text started.
    // This prevents the button press that *started* the dialogue from immediately *skipping* it.
    if (!hasSkipped && (millis() - startFuncTime > 200)) {
        customKeypad.getKeys(); 
        for (int k=0; k<LIST_MAX; k++) {
           if (customKeypad.key[k].kstate == PRESSED && customKeypad.key[k].kchar == 'E') {
               delaySpeed = 0; hasSkipped = true;
           }
        }
    }

    waitAndPump(delaySpeed);
  }
  
  if (hasSkipped) waitAndPump(200);
}