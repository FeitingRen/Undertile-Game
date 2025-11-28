#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include "game_defs.h"
#include "background.h" 
#include "characters.h" 

// --- DEBUG SETTINGS ---
#define DEBUG_SKIP_INTRO true 

// --- DISPLAY SETUP ---
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- GLOBAL VARIABLES ---
GameState currentState = MENU;
DialogueState currentDialogueState = D_INTRO_1;
Inventory playerInventory = {true, true, true}; 

int storyProgress = 0; 
bool isStateFirstFrame = true;
unsigned long lastFrameTime = 0;
const int targetFPS = 60;
const int frameDelay = 1000 / targetFPS;

int playerChoiceYesNo = 0; 

// --- PROTOTYPES ---
void handleMenu();
void handleMap();
void handleDialogue();
void handleBattle();
void typeText(const char* text, int delaySpeed, bool shake = false);
void drawSpriteMixed(int x, int y, const uint16_t* sprite, int w, int h, const uint16_t* bgMap);

// --- GRAPHICS HELPER ---
void drawSpriteMixed(int x, int y, const uint16_t* sprite, int w, int h, const uint16_t* bgMap) {
    if (x < 0 || y < 0 || x + w > SCREEN_W || y + h > SCREEN_H) return; 

    uint16_t buffer[256]; 

    for (int row = 0; row < h; row++) {
        int screenY = y + row;
        int bgIndexStart = (screenY * SCREEN_W) + x;
        for (int col = 0; col < w; col++) {
             // Copy pixel from the huge background array
             buffer[row * w + col] = bgMap[bgIndexStart + col];
        }
    }

    for (int i = 0; i < w * h; i++) {
        uint16_t pixel = sprite[i];
        if (pixel != 0x07C0) { // Check for Lime Green Transparency
            buffer[i] = pixel; 
        }
    }
    tft.drawRGBBitmap(x, y, buffer, w, h);
}

// --- CLASSES ---
struct Rect {
    int x, y, w, h;
};

struct NPC {
  int x, y;
};

class Player {
public:
  float x, y, oldX, oldY;
  float speed = 2.0; 
  
  Rect* zones = nullptr;
  int zoneCount = 0;

  void init(int startX, int startY) {
    x = startX; y = startY; oldX = x; oldY = y;
  }

  void setZones(Rect* newZones, int count) {
      zones = newZones;
      zoneCount = count;
  }
  
  bool isWalkable(int px, int py) {
      if (zoneCount == 0) return true; 
      
      for(int i=0; i<zoneCount; i++) {
          if (px >= zones[i].x && px <= zones[i].x + zones[i].w &&
              py >= zones[i].y && py <= zones[i].y + zones[i].h) {
              return true;
          }
      }
      return false;
  }

  bool checkCollision(float newX, float newY, int objX, int objY, int objW, int objH) {
    return (newX < objX + objW && newX + PLAYER_W > objX && newY < objY + objH && newY + PLAYER_H > objY);
  }

  void update(NPC* enemy = nullptr) {
    oldX = x; oldY = y;
    float nextX = x, nextY = y;
    int joyX = analogRead(JOYSTICK_X);
    int joyY = analogRead(JOYSTICK_Y);
    
    if (joyX < 1500) nextX += speed; 
    if (joyX > 2500) nextX -= speed; 
    
    int feetX = (int)nextX + (PLAYER_W / 2);
    int feetY = (int)y + PLAYER_H;
    
    if (isWalkable(feetX, feetY)) {
        if (nextX >= 0 && nextX <= SCREEN_W - PLAYER_W) {
            x = nextX;
        }
    }
    
    nextY = y;
    if (joyY < 1500) nextY += speed; 
    if (joyY > 2500) nextY -= speed; 
    
    feetX = (int)x + (PLAYER_W / 2);
    feetY = (int)nextY + PLAYER_H;

    if (isWalkable(feetX, feetY)) {
        if (nextY >= 0 && nextY <= SCREEN_H - PLAYER_H) {
             y = nextY;
        }
    }

    if (currentState == MAP_WALK && enemy != nullptr) {
       if (checkCollision(x, y, enemy->x, enemy->y, NPC_SIZE, NPC_SIZE)) {
          x = oldX; y = oldY;
       }
    }
  }

  void draw(const uint16_t* bgMap = nullptr) {
    if ((int)x != (int)oldX || (int)y != (int)oldY) {
      if (bgMap != nullptr) {
        // MAP MODE: Restore background pixels
        int rx = (int)oldX; 
        int ry = (int)oldY;
        for(int row = 0; row < PLAYER_H; row++) {
           int currentY = ry + row;
           if(currentY >= 0 && currentY < SCREEN_H) {
              tft.drawRGBBitmap(rx, currentY, (uint16_t*)(bgMap + (currentY * SCREEN_W) + rx), PLAYER_W, 1);
           }
        }
        drawSpriteMixed((int)x, (int)y, heart_sprite, PLAYER_W, PLAYER_H, bgMap);
      } else {
        // BATTLE MODE (No bgMap, Black Background)
        
        // 1. Erase trail with Black rectangles
        if (x > oldX) tft.fillRect((int)oldX, (int)oldY, (int)x - (int)oldX, PLAYER_H, ST7735_BLACK);
        else if (x < oldX) tft.fillRect((int)x + PLAYER_W, (int)oldY, (int)oldX - (int)x, PLAYER_H, ST7735_BLACK);
        if (y > oldY) tft.fillRect((int)oldX, (int)oldY, PLAYER_W, (int)y - (int)oldY, ST7735_BLACK);
        else if (y < oldY) tft.fillRect((int)oldX, (int)y + PLAYER_H, PLAYER_W, (int)oldY - (int)y, ST7735_BLACK);
        
        // 2. Draw Sprite directly using the black-background version
        tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite_blk, PLAYER_W, PLAYER_H);
      }
    }
  }
  
  // UPDATED: Now allows passing bgMap for transparent initial draw
  void forceDraw(const uint16_t* bgMap = nullptr) {
    if (bgMap != nullptr) {
        drawSpriteMixed((int)x, (int)y, heart_sprite, PLAYER_W, PLAYER_H, bgMap);
    } else {
        // For battle mode (no map), force draw using the black background sprite
        tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite_blk, PLAYER_W, PLAYER_H);
    }
  }
};

Player player;
NPC enemy = {85,56};

// --- DIALOGUE VARIABLES ---
int menuSelection = 0; 
int inventoryOptions[3]; 
int itemXPositions[3]; 
int availableCount = 0;
int lastDrawnSelection = -1; 
int textCursorX = 10; 
int textCursorY = 95; 

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); 
  tft.fillScreen(ST7735_BLACK);
  
  #if DEBUG_SKIP_INTRO
    currentState = BATTLE;
    player.x = 80; player.y = 90; 
    
    // UPDATED: Correct Battle Box Bounds
    // Box is drawn at (24, 64) with w=112, h=52.
    static Rect battleZone[] = { {31, 77, 98, 38} }; 
    player.setZones(battleZone, 1);
    
  #else
    player.init(15, 60); 
  #endif
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastFrameTime >= frameDelay) {
    lastFrameTime = currentTime;
    switch(currentState) {
      case MENU: handleMenu(); break;
      case MAP_WALK: handleMap(); break;
      case DIALOGUE: handleDialogue(); break;
      case BATTLE: handleBattle(); break;
      case GAME_OVER: break;
    }
  }
}

// --- UTILS ---
bool isButtonPressed() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    while(digitalRead(BUTTON_PIN) == LOW) delay(10); 
    return true;
  }
  return false;
}

// --- SCENES ---
void handleMenu() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.setCursor(30, 40);
    tft.print("UNDERTALE ESP32");
    
    tft.setCursor(20, 80);
    tft.print("Press Button to Start");
    isStateFirstFrame = false; 
  }
  
  if (isButtonPressed()) {
    currentState = MAP_WALK;
    isStateFirstFrame = true;
    player.x = 25; player.y = 60; 
    
    static Rect walkableFloors[] = {
      { 21, 48, 122, 2 },
      { 21, 51, 109, 43 },
      { 10, 82, 11, 2 },
      { 11, 79, 9, 2 },
      { 13, 74, 8, 4 },
      { 14, 71, 7, 2 },
      { 15, 68, 6, 2 },
      { 17, 66, 4, 1 },
      { 18, 63, 3, 2 },
      { 18, 61, 2, 1 },
      { 19, 58, 1, 2 },
      { 130, 69, 7, 25 },
      { 137, 82, 6, 12 }
    };
    player.setZones(walkableFloors, 13);
  }
}

void handleMap() {
  if (isStateFirstFrame) {
    tft.drawRGBBitmap(0, 0, bg_map, 160, 128);
    // Passing bg_map ensures transparency on frame 1
    player.forceDraw(bg_map); 
    isStateFirstFrame = false;
  }

  player.update(&enemy);
  drawSpriteMixed(enemy.x, enemy.y, robot_npc, 16, 16, bg_map);
  player.draw(bg_map);

  float dist = sqrt(pow(player.x - enemy.x, 2) + pow(player.y - enemy.y, 2));
  if (dist < 20 && digitalRead(BUTTON_PIN) == LOW) {
      currentState = DIALOGUE;
      if (storyProgress == 0) currentDialogueState = D_INTRO_1; 
      else if (storyProgress == 1) currentDialogueState = D_REQUEST_FOOD_PART1; 
      else currentDialogueState = D_REQUEST_FOOD; 
      
      isStateFirstFrame = true;
      while(digitalRead(BUTTON_PIN) == LOW) delay(10);
  } 
}

void handleDialogue() {
  int boxY = 88;
  int boxH = 40;
  int textY = 94;

  if (isStateFirstFrame && currentDialogueState != D_COFFEE_EVENT) {
    tft.fillRect(2, boxY, 156, boxH, ST7735_BLACK); 
    tft.drawRect(0, boxY-2, 160, boxH+2, ST7735_WHITE); 
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, textY); 
  }

  auto clearText = [&]() {
     tft.fillRect(4, boxY+2, 152, boxH-4, ST7735_BLACK);
     tft.setCursor(5, textY);
  };

  switch (currentDialogueState) {
    case D_INTRO_1:
      if (isStateFirstFrame) { clearText(); typeText("* WHAT!!?", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_2; isStateFirstFrame = true; }
      break;
    case D_INTRO_2:
      if (isStateFirstFrame) { clearText(); typeText("* ...", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_4; isStateFirstFrame = true; }
      break;
    case D_INTRO_4:
      if (isStateFirstFrame) { clearText(); typeText("* Sorry, I've been here\n* alone for so long.", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_5; isStateFirstFrame = true; }
      break;
    case D_INTRO_5:
      if (isStateFirstFrame) { clearText(); typeText("* I'm actually a\n* nonchalant robot.", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_6; isStateFirstFrame = true; }
      break;
    case D_INTRO_6:
      if (isStateFirstFrame) { clearText(); typeText("* Are you a human?", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentDialogueState = D_HUMAN_CHOICE; menuSelection = 0; lastDrawnSelection = -1; isStateFirstFrame = true; }
      break;
    case D_HUMAN_CHOICE:
      if (isStateFirstFrame) {
        clearText();
        tft.setCursor(30, textY + 10); tft.print("YES");
        tft.setCursor(100, textY + 10); tft.print("NO");
        isStateFirstFrame = false;
      }
      {
        int joyX = analogRead(JOYSTICK_X);
        if (joyX > 2500) menuSelection = 0;
        if (joyX < 1500) menuSelection = 1;
      }
      if (menuSelection != lastDrawnSelection) {
        tft.fillRect(15, textY + 8, 14, 14, ST7735_BLACK);
        tft.fillRect(85, textY + 8, 14, 14, ST7735_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(15, textY + 8, heart_sprite_blk, 12, 12);
        else tft.drawRGBBitmap(85, textY + 8, heart_sprite_blk, 12, 12);
        lastDrawnSelection = menuSelection;
      }
      if (isButtonPressed()) {
        playerChoiceYesNo = menuSelection;
        currentDialogueState = D_HUMAN_RESULT_1; 
        isStateFirstFrame = true;
      }
      break;
    case D_HUMAN_RESULT_1:
      if (isStateFirstFrame) {
        clearText();
        if (playerChoiceYesNo == 0) typeText("* First human friend!", 30);
        else typeText("Then you are the 1,025th\n* rock I've met today.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_HUMAN_RESULT_2; isStateFirstFrame = true; }
      break;
    case D_HUMAN_RESULT_2:
      if (isStateFirstFrame) {
        clearText();
        if (playerChoiceYesNo == 0) typeText("* I mean. Cool. Whatever.", 30);
        else typeText("* The other rocks were\n* less talkative.", 30);
        storyProgress = 1; 
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_REQUEST_FOOD_PART1; isStateFirstFrame = true; }
      break;
    case D_REQUEST_FOOD_PART1:
      if (isStateFirstFrame) { clearText(); typeText("* My battery is low.", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true; }
      break;
    case D_REQUEST_FOOD:
      if (isStateFirstFrame) {
        clearText();
        if (storyProgress == 1) typeText("* Do you have any food?", 30); 
        else if (storyProgress == 2) typeText("* Can I have one more?", 30);
        else if (storyProgress == 3) typeText("* Just one last byte?", 30);
        
        tft.setCursor(30, textY + 15); tft.print("GIVE");
        tft.setCursor(100, textY + 15); tft.print("REFUSE");
        
        menuSelection = 0; 
        lastDrawnSelection = -1;
        isStateFirstFrame = false;
      }
      {
        int joyX = analogRead(JOYSTICK_X);
        if (joyX > 2500) menuSelection = 0;
        if (joyX < 1500) menuSelection = 1;
      }
      if (menuSelection != lastDrawnSelection) {
        tft.fillRect(15, textY + 13, 14, 14, ST7735_BLACK);
        tft.fillRect(85, textY + 13, 14, 14, ST7735_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(15, textY + 13, heart_sprite_blk, 12, 12);
        else tft.drawRGBBitmap(85, textY + 13, heart_sprite_blk, 12, 12);
        lastDrawnSelection = menuSelection;
      }
      if (isButtonPressed()) {
        if (menuSelection == 0) currentDialogueState = D_SELECT_ITEM;
        else currentDialogueState = D_REFUSAL;
        isStateFirstFrame = true;
      }
      break;
    case D_SELECT_ITEM:
      if (isStateFirstFrame) {
        clearText();
        tft.print("Give what?");
        availableCount = 0;
        if (playerInventory.hasCoffee) inventoryOptions[availableCount++] = 0;
        if (playerInventory.hasGas)    inventoryOptions[availableCount++] = 1;
        if (playerInventory.hasBattery) inventoryOptions[availableCount++] = 2;
        
        int currentX = 20; 
        int gap = 20; 

        for(int i=0; i<availableCount; i++) {
            itemXPositions[i] = currentX; 
            tft.setCursor(currentX, textY + 15); 
            int itemType = inventoryOptions[i];
            int textWidth = 0;

            if(itemType == 0) { tft.print("Coffee"); textWidth = 6 * 6; } 
            if(itemType == 1) { tft.print("Gas");    textWidth = 3 * 6; } 
            if(itemType == 2) { tft.print("Bat.");   textWidth = 4 * 6; } 
            
            currentX += textWidth + gap;
        }

        menuSelection = 0;
        lastDrawnSelection = -1;
        isStateFirstFrame = false;
      }
      {
        int joyX = analogRead(JOYSTICK_X);
        if (joyX > 2500 && menuSelection > 0) { menuSelection--; delay(150); }
        if (joyX < 1500 && menuSelection < availableCount-1) { menuSelection++; delay(150); }
      }
      if (menuSelection != lastDrawnSelection) {
        if (lastDrawnSelection != -1) {
            tft.fillRect(itemXPositions[lastDrawnSelection] - 14, textY+13, 12, 12, ST7735_BLACK);
        }
        tft.drawRGBBitmap(itemXPositions[menuSelection] - 14, textY+13, heart_sprite_blk, 12, 12);
        lastDrawnSelection = menuSelection;
      }
      if (isButtonPressed()) {
        int chosenItem = inventoryOptions[menuSelection];
        if (chosenItem == 0) {
          playerInventory.hasCoffee = false;
          currentDialogueState = D_COFFEE_EVENT;
        } else {
          if (chosenItem == 1) playerInventory.hasGas = false;
          if (chosenItem == 2) playerInventory.hasBattery = false;
          currentDialogueState = D_EATING;
        }
        isStateFirstFrame = true;
      }
      break;
    case D_EATING:
      if (isStateFirstFrame) { clearText(); typeText("* CRUNCH CRUNCH.\n* That flavor!", 30); isStateFirstFrame = false; }
      if (isButtonPressed()) {
        storyProgress++;
        if (storyProgress > 3) storyProgress = 3; 
        currentDialogueState = D_REQUEST_FOOD; 
        isStateFirstFrame = true;
      }
      break;
    case D_REFUSAL:
      if (isStateFirstFrame) { clearText(); typeText("* Oh... okay.\n* I'll just go into Sleep\n* Mode FOREVER.", 40); isStateFirstFrame = false; }
      if (isButtonPressed()) { currentState = MAP_WALK; isStateFirstFrame = true; }
      break;
    case D_COFFEE_EVENT:
       if (isStateFirstFrame) {
        tft.fillScreen(ST7735_BLACK); 
        tft.setTextColor(ST7735_WHITE);
        tft.setTextSize(1);
        tft.setCursor(10, 30); typeText("THANKS! SLURP...", 50);
        delay(300);
        tft.setCursor(10, 50); typeText("Analyzing...", 50);
        delay(1000);
        tft.setCursor(10, 70); typeText("Is this C8H10N4O2?", 50);
        delay(1000);
        tft.setTextColor(ST7735_RED);
        tft.setCursor(10, 90); typeText("Was that... COFFEE?", 100, true); 
        delay(1500);
        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(5, 50); typeText("NO OVERCLOCKING!!!", 30, true);
        delay(1000);
        tft.setCursor(5, 70); typeText("I CAN TASTE MATH!", 20, true);
        delay(2000);
        tft.fillScreen(ST7735_RED);
        delay(100);
        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(20, 60); typeText("CTRL+ALT+DELETE ME!", 10, true);
        delay(1000);
        
        currentState = BATTLE;
        isStateFirstFrame = true;
        player.x = 80; player.y = 90;
        
        // UPDATED: Correct Battle Box Bounds here as well
        static Rect battleZone[] = { {25, 65, 110, 50} }; 
        player.setZones(battleZone, 1);
       }
       break;
  }
}

void handleBattle() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK);
    tft.drawRect(24, 64, 112, 52, ST7735_WHITE); 
    player.forceDraw();
    tft.setCursor(30, 20);
    tft.setTextColor(ST7735_RED);
    tft.setTextSize(1);
    tft.print("OVERCLOCKED ROBOT");
    isStateFirstFrame = false;
  }
  player.update();
  player.draw();
}

void typeText(const char* text, int delaySpeed, bool shake) {
  int startX = tft.getCursorX();
  int startY = tft.getCursorY();
  int originalX = startX; 
  bool hasSkipped = false;

  for (int i = 0; i < strlen(text); i++) {
    if(text[i] == '\n') {
      startY += 10; 
      startX = originalX; 
      tft.setCursor(startX, startY);
      continue;
    }
    if (shake) {
       int ox = random(-1, 2); 
       int oy = random(-1, 2);
       tft.setCursor(startX + ox, startY + oy);
       tft.print(text[i]);
       startX = tft.getCursorX() - ox;
    } else {
       tft.setCursor(startX, startY);
       tft.print(text[i]);
       startX = tft.getCursorX();
       startY = tft.getCursorY();
    }
    if (digitalRead(BUTTON_PIN) == LOW) {
      delaySpeed = 0; 
      hasSkipped = true; 
    }
    delay(delaySpeed);
  }
  if (hasSkipped) {
    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
  }
}