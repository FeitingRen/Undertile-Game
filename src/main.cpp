#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include "game_defs.h"

// --- DISPLAY SETUP ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// --- GLOBAL VARIABLES ---
GameState currentState = MENU;
DialogueState currentDialogueState = D_INTRO_1;
Inventory playerInventory = {true, true, true}; // Start with all 3 items

// Story Progress: 0 = Just Met, 1 = Round 1 (Hungry), 2 = Round 2, 3 = Round 3
int storyProgress = 0; 
bool isStateFirstFrame = true;
unsigned long lastFrameTime = 0;
const int targetFPS = 60;
const int frameDelay = 1000 / targetFPS;

// Stores the player's Yes(0)/No(1) choice to remember it across split dialogue states
int playerChoiceYesNo = 0; 

// --- PROTOTYPES ---
void handleMenu();
void handleMap();
void handleDialogue();
void handleBattle();
void typeText(const char* text, int delaySpeed, bool shake = false);

// --- ASSETS ---
// 12x12 Heart Sprite (144 pixels)
const uint16_t heart_sprite[144] = {
0x0000, 0x0000, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0x0000, 0x0000, 
0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 
0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 
0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 
0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

const uint16_t robot_npc[256] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0xcd67, 0xd5a9, 0xd5a9, 0xd5a9, 0xcd67, 0xd5a9, 0xd5a9, 0xcd67, 0xd5a9, 0xd5a9, 0xd5a9, 0xcd67, 0x0000, 0x0000, 
  0x0000, 0x0000, 0xcd67, 0x0000, 0x2f28, 0x0000, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0x0000, 0x2f28, 0x0000, 0xcd67, 0x0000, 0x0000,
  0x0000, 0x0000, 0xcd67, 0x0000, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0x0000, 0xcd67, 0x0000, 0x0000, 
  0x0000, 0x0000, 0xcd67, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0x0000, 0x0000, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xcd67, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xcd67, 0xcd67, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0xcd67, 0xcd67, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xcd67, 0xcd67, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x6320, 0x0000, 0xcd67, 0xd5a9, 0xd5a9, 0xd5a9, 0x2f28, 0xcd67, 0x0000, 0x6320, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x6320, 0x0000, 0x6320, 0x6320, 0xd5a9, 0xd5a9, 0xd5a9, 0xd5a9, 0x6320, 0x6320, 0x0000, 0x6320, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x6320, 0x6320, 0xcd67, 0xcd67, 0xcd67, 0xcd67, 0x6320, 0x6320, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x6320, 0x6320, 0x0000, 0x0000, 0x0000, 0x0000, 0x6320, 0x6320, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

// --- CLASSES ---
struct NPC {
  int x, y;
};

class Player {
public:
  float x, y, oldX, oldY;
  float speed = 3.0;
  int minX, maxX, minY, maxY;

  void init(int startX, int startY) {
    x = startX; y = startY; oldX = x; oldY = y;
    setBounds(0, 240, 0, 320);
  }

  void setBounds(int x1, int x2, int y1, int y2) {
    minX = x1; maxX = x2; minY = y1; maxY = y2;
  }

  bool checkCollision(float newX, float newY, int objX, int objY, int objW, int objH) {
    return (newX < objX + objW && newX + PLAYER_W > objX && newY < objY + objH && newY + PLAYER_H > objY);
  }

  void update(NPC* enemy = nullptr) {
    oldX = x; oldY = y;
    float nextX = x, nextY = y;
    int joyX = analogRead(JOYSTICK_X);
    int joyY = analogRead(JOYSTICK_Y);
    
    if (joyX < 1800) nextX += speed; 
    if (joyX > 2200) nextX -= speed; 
    if (joyY < 1800) nextY += speed; 
    if (joyY > 2200) nextY -= speed; 
    
    // Bounds
    if (nextX < minX) nextX = minX;
    if (nextX > maxX - PLAYER_W) nextX = maxX - PLAYER_W;
    if (nextY < minY) nextY = minY;
    if (nextY > maxY - PLAYER_H) nextY = maxY - PLAYER_H;

    // Collision
    if (currentState == MAP_WALK && enemy != nullptr) {
       if (!checkCollision(nextX, nextY, enemy->x, enemy->y, NPC_SIZE, NPC_SIZE)) {
          x = nextX; y = nextY;
       }
    } else {
       x = nextX; y = nextY;
    }
  }

  void draw() {
    if ((int)x != (int)oldX || (int)y != (int)oldY) {
      tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite, PLAYER_W, PLAYER_H);
      if (x > oldX) tft.fillRect((int)oldX, (int)oldY, (int)x - (int)oldX, PLAYER_H, ILI9341_BLACK);
      else if (x < oldX) tft.fillRect((int)x + PLAYER_W, (int)oldY, (int)oldX - (int)x, PLAYER_H, ILI9341_BLACK);
      if (y > oldY) tft.fillRect((int)oldX, (int)oldY, PLAYER_W, (int)y - (int)oldY, ILI9341_BLACK);
      else if (y < oldY) tft.fillRect((int)oldX, (int)y + PLAYER_H, PLAYER_W, (int)oldY - (int)y, ILI9341_BLACK);
    }
  }
  
  void forceDraw() {
    tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite, PLAYER_W, PLAYER_H);
  }
};

Player player;
NPC enemy = {140, 40};

// --- DIALOGUE VARIABLES ---
int menuSelection = 0; 
int inventoryOptions[3]; 
int availableCount = 0;
int lastDrawnSelection = -1; 
// To handle dynamic cursor positioning in typeText
int textCursorX = 20; 
int textCursorY = 185; 

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  player.init(30, 120);
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
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2); 
    tft.setCursor(60, 80);
    tft.print("UNDERTALE ESP32");
    tft.setTextSize(1);
    tft.setCursor(90, 120);
    tft.print("Press Button to Start");
    isStateFirstFrame = false; 
  }
  
  if (isButtonPressed()) {
    currentState = MAP_WALK;
    isStateFirstFrame = true;
    player.x = 20; player.y = 120;
    player.setBounds(0, 320, 0, 240);
  }
}

void handleMap() {
  if (isStateFirstFrame) {
    tft.fillScreen(ILI9341_BLACK); 
    player.forceDraw();
    isStateFirstFrame = false;
  }

  player.update(&enemy);
  player.draw();
  
  // NEW: Draw the robot sprite (16x16 size)
  tft.drawRGBBitmap(enemy.x, enemy.y, (uint16_t*)robot_npc, 16, 16);
  // --- CHANGED SECTION ENDS HERE ---

  float dist = sqrt(pow(player.x - enemy.x, 2) + pow(player.y - enemy.y, 2));
  if (dist < 30 && digitalRead(BUTTON_PIN) == LOW) {
      currentState = DIALOGUE;
      
      // Determine Start State based on progress
      if (storyProgress == 0) {
        currentDialogueState = D_INTRO_1; 
      } else if (storyProgress == 1) {
        currentDialogueState = D_REQUEST_FOOD_PART1; // Special split for round 1
      } else {
        currentDialogueState = D_REQUEST_FOOD; // Direct to request for round 2/3
      }
      
      isStateFirstFrame = true;
      while(digitalRead(BUTTON_PIN) == LOW) delay(10);
  } 
}

void handleDialogue() {
  // Common UI Setup
  if (isStateFirstFrame && currentDialogueState != D_COFFEE_EVENT) {
    tft.fillRect(10, 180, 300, 50, ILI9341_BLACK); 
    tft.drawRect(8, 178, 304, 54, ILI9341_WHITE); 
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    // Init Cursor for typeText
    tft.setCursor(20, 185); 
  }

  switch (currentDialogueState) {
    
    // --- SPLIT INTRO PHASE 1 ---
    case D_INTRO_1:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear text area
        tft.setCursor(20, 190);
        typeText("* WHAT!!?", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentDialogueState = D_INTRO_2;
        isStateFirstFrame = true;
      }
      break;

    // --- SPLIT INTRO PHASE 2 ---
    case D_INTRO_2:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear text area
        tft.setCursor(20, 190);
        typeText("* ...", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentDialogueState = D_INTRO_4;
        isStateFirstFrame = true;
      }
      break;

    // --- SPLIT INTRO PHASE 4 ---
    case D_INTRO_4:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear text area
        tft.setCursor(20, 190);
        typeText("* Sorry, I've been here alone for so long.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentDialogueState = D_INTRO_5;
        isStateFirstFrame = true;
      }
      break;

    // --- SPLIT INTRO PHASE 5 ---
    case D_INTRO_5:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear text area
        tft.setCursor(20, 190);
        typeText("* I'm actually a nonchalant robot.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentDialogueState = D_INTRO_6;
        isStateFirstFrame = true;
      }
      break;

    // --- SPLIT INTRO PHASE 6 ---
    case D_INTRO_6:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear text area
        tft.setCursor(20, 190);
        typeText("* Are you a human?", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentDialogueState = D_HUMAN_CHOICE;
        menuSelection = 0; 
        lastDrawnSelection = -1;
        isStateFirstFrame = true;
      }
      break;

    case D_HUMAN_CHOICE:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(60, 200); tft.print("YES");
        tft.setCursor(180, 200); tft.print("NO");
        isStateFirstFrame = false;
      }
      {
        int joyX = analogRead(JOYSTICK_X);
        if (joyX > 2500) menuSelection = 0;
        if (joyX < 1500) menuSelection = 1;
      }
      if (menuSelection != lastDrawnSelection) {
        tft.fillRect(40, 200, 16, 16, ILI9341_BLACK);
        tft.fillRect(160, 200, 16, 16, ILI9341_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(40, 200, heart_sprite, 12, 12);
        else tft.drawRGBBitmap(160, 200, heart_sprite, 12, 12);
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
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        if (playerChoiceYesNo == 0) typeText("* First human friend!", 30);
        else typeText("* Then you are the 1,025th rock\n* I have spoken to today.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_HUMAN_RESULT_2; isStateFirstFrame = true; }
      break;

    case D_HUMAN_RESULT_2:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        if (playerChoiceYesNo == 0) typeText("* I mean. Cool. Whatever.", 30);
        else typeText("* The other rocks were less talkative.", 30);
        storyProgress = 1; 
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { 
        currentDialogueState = D_REQUEST_FOOD_PART1; // Go to new part 1
        isStateFirstFrame = true; 
      }
      break;

    // --- NEW: PART 1 of Round 1 ---
    case D_REQUEST_FOOD_PART1:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        typeText("* My battery is low.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentDialogueState = D_REQUEST_FOOD; // Proceed to part 2
        isStateFirstFrame = true;
      }
      break;

    // --- REQUEST FOOD (Round 1 Part 2, or Round 2/3) ---
    case D_REQUEST_FOOD:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        
        if (storyProgress == 1) typeText("* Do you have any food?", 30); // Part 2 text
        else if (storyProgress == 2) typeText("* Can I have one more unit?", 30);
        else if (storyProgress == 3) typeText("* Just one last byte? I promise.", 30);
        
        tft.setCursor(60, 205); tft.print("GIVE");
        tft.setCursor(180, 205); tft.print("REFUSE");
        
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
        tft.fillRect(40, 205, 16, 16, ILI9341_BLACK);
        tft.fillRect(160, 205, 16, 16, ILI9341_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(40, 205, heart_sprite, 12, 12);
        else tft.drawRGBBitmap(160, 205, heart_sprite, 12, 12);
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
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        tft.print("Give what?");
        
        availableCount = 0;
        if (playerInventory.hasCoffee) inventoryOptions[availableCount++] = 0;
        if (playerInventory.hasGas)    inventoryOptions[availableCount++] = 1;
        if (playerInventory.hasBattery) inventoryOptions[availableCount++] = 2;
        
        for(int i=0; i<availableCount; i++) {
            tft.setCursor(50 + (i*80), 210);
            int itemType = inventoryOptions[i];
            if(itemType == 0) tft.print("Coffee");
            if(itemType == 1) tft.print("Gas");
            if(itemType == 2) tft.print("Battery");
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
        // Erase old cursor (if one was drawn)
        if (lastDrawnSelection != -1) {
            tft.fillRect(34 + (lastDrawnSelection*80), 210, 12, 12, ILI9341_BLACK);
        }
        // Draw new cursor
        tft.drawRGBBitmap(34 + (menuSelection*80), 210, heart_sprite, 12, 12);
        
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
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        typeText("* CRUNCH CRUNCH.\n* That texture! That flavor!", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        storyProgress++;
        if (storyProgress > 3) storyProgress = 3; 
        currentDialogueState = D_REQUEST_FOOD; 
        isStateFirstFrame = true;
      }
      break;

    case D_REFUSAL:
      if (isStateFirstFrame) {
        tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
        tft.setCursor(20, 185);
        typeText("* Oh... okay.\n* I'll just go into Sleep Mode.", 40);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) {
        currentState = MAP_WALK;
        isStateFirstFrame = true;
      }
      break;

    case D_COFFEE_EVENT:
       if (isStateFirstFrame) {
        tft.fillScreen(ILI9341_BLACK); 
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.setCursor(20, 50); typeText("THANKS! SLURP...", 50);
        delay(300);
        tft.setCursor(20, 80); typeText("Analyzing...", 50);
        delay(1000);
        tft.setCursor(20, 110); typeText("Is this C8H10N4O2?", 50);
        delay(1000);
        tft.setTextColor(ILI9341_RED);
        tft.setCursor(20, 140); typeText("Was that... COFFEE?", 100, true); 
        delay(1500);
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(10, 100); typeText("NO OVERLCOCKING!!!", 30, true);
        delay(1000);
        tft.setCursor(10, 140); typeText("I CAN TASTE MATH!", 20, true);
        delay(2000);
        tft.fillScreen(ILI9341_RED);
        delay(100);
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(40, 110); typeText("CTRL + ALT + DELETE ME!", 10, true);
        delay(1000);
        currentState = BATTLE;
        isStateFirstFrame = true;
        player.x = 160; player.y = 200;
        player.setBounds(50, 270, 130, 230);
       }
       break;
  }
}

void handleBattle() {
  if (isStateFirstFrame) {
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(48, 128, 224, 104, ILI9341_WHITE); 
    player.forceDraw();
    tft.setCursor(60, 40);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
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
      // Approximate line height as 10px * text size is a safer bet, 
      // but strictly following standard: 8px * size + spacing.
      // Since we can't easily get textsize, we rely on hardcoded average or logic.
      // 12 is good for Size 1. For Size 2 it might be tight, but readable.
      startY += 12; 
      startX = originalX; 
      tft.setCursor(startX, startY);
      continue;
    }

    if (shake) {
       int ox = random(-2, 3);
       int oy = random(-2, 3);
       tft.setCursor(startX + ox, startY + oy);
       tft.print(text[i]);
       
       // CRITICAL FIX:
       // The cursor has now moved forward by the character width.
       // We calculate the next startX by taking the current cursor and subtracting the jitter.
       // This guarantees correct spacing regardless of Text Size (1, 2, 3, etc).
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