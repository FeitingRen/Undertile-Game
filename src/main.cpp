#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Changed from ILI9341
#include <SPI.h>
#include "game_defs.h"

// --- DISPLAY SETUP ---
// Use ST7735 class instead of ILI9341
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
  float speed = 2.0; // Slightly reduced speed for smaller screen
  int minX, maxX, minY, maxY;

  void init(int startX, int startY) {
    x = startX; y = startY; oldX = x; oldY = y;
    setBounds(0, SCREEN_W, 0, SCREEN_H);
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
    
    // Adjusted thresholds for ESP32 ADC (0-4095)
    // Center is approx 1850-2000 usually
    if (joyX < 1500) nextX += speed; 
    if (joyX > 2500) nextX -= speed; 
    if (joyY < 1500) nextY += speed; 
    if (joyY > 2500) nextY -= speed; 
    
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
      // Erase trails
      if (x > oldX) tft.fillRect((int)oldX, (int)oldY, (int)x - (int)oldX, PLAYER_H, ST7735_BLACK);
      else if (x < oldX) tft.fillRect((int)x + PLAYER_W, (int)oldY, (int)oldX - (int)x, PLAYER_H, ST7735_BLACK);
      if (y > oldY) tft.fillRect((int)oldX, (int)oldY, PLAYER_W, (int)y - (int)oldY, ST7735_BLACK);
      else if (y < oldY) tft.fillRect((int)oldX, (int)y + PLAYER_H, PLAYER_W, (int)oldY - (int)y, ST7735_BLACK);
    }
  }
  
  void forceDraw() {
    tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite, PLAYER_W, PLAYER_H);
  }
};

Player player;
NPC enemy = {70, 25}; // Scaled enemy position

// --- DIALOGUE VARIABLES ---
int menuSelection = 0; 
int inventoryOptions[3]; 
int availableCount = 0;
int lastDrawnSelection = -1; 
int textCursorX = 10; 
int textCursorY = 95; 

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // ST7735 Initialization
  // Use INITR_BLACKTAB for 1.8" typically. If colors are wrong, try INITR_GREENTAB.
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); // Landscape
  tft.fillScreen(ST7735_BLACK);
  
  player.init(15, 60); // Scaled start pos
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
    tft.setTextSize(1); // Size 2 is too big for 160px width with long text
    tft.setCursor(30, 40);
    tft.print("UNDERTALE ESP32");
    
    tft.setCursor(20, 80);
    tft.print("Press Button to Start");
    isStateFirstFrame = false; 
  }
  
  if (isButtonPressed()) {
    currentState = MAP_WALK;
    isStateFirstFrame = true;
    player.x = 10; player.y = 60;
    player.setBounds(0, SCREEN_W, 0, SCREEN_H);
  }
}

void handleMap() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK); 
    player.forceDraw();
    isStateFirstFrame = false;
  }

  player.update(&enemy);
  player.draw();
  
  // Draw NPC
  tft.drawRGBBitmap(enemy.x, enemy.y, (uint16_t*)robot_npc, 16, 16);

  float dist = sqrt(pow(player.x - enemy.x, 2) + pow(player.y - enemy.y, 2));
  if (dist < 20 && digitalRead(BUTTON_PIN) == LOW) {
      currentState = DIALOGUE;
      
      if (storyProgress == 0) {
        currentDialogueState = D_INTRO_1; 
      } else if (storyProgress == 1) {
        currentDialogueState = D_REQUEST_FOOD_PART1; 
      } else {
        currentDialogueState = D_REQUEST_FOOD; 
      }
      
      isStateFirstFrame = true;
      while(digitalRead(BUTTON_PIN) == LOW) delay(10);
  } 
}

void handleDialogue() {
  // Scaled Dialogue Box for 128x160
  // Box at bottom ~40px high
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

  // Helper lambda to clear text area
  auto clearText = [&]() {
     tft.fillRect(4, boxY+2, 152, boxH-4, ST7735_BLACK);
     tft.setCursor(5, textY);
  };

  switch (currentDialogueState) {
    
    case D_INTRO_1:
      if (isStateFirstFrame) {
        clearText();
        typeText("* WHAT!!?", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_2; isStateFirstFrame = true; }
      break;

    case D_INTRO_2:
      if (isStateFirstFrame) {
        clearText();
        typeText("* ...", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_4; isStateFirstFrame = true; }
      break;

    case D_INTRO_4:
      if (isStateFirstFrame) {
        clearText();
        typeText("* Sorry, I've been here\n* alone for so long.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_5; isStateFirstFrame = true; }
      break;

    case D_INTRO_5:
      if (isStateFirstFrame) {
        clearText();
        typeText("* I'm actually a\n* nonchalant robot.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_INTRO_6; isStateFirstFrame = true; }
      break;

    case D_INTRO_6:
      if (isStateFirstFrame) {
        clearText();
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
        tft.fillRect(15, textY + 10, 14, 14, ST7735_BLACK);
        tft.fillRect(85, textY + 10, 14, 14, ST7735_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(15, textY + 10, heart_sprite, 12, 12);
        else tft.drawRGBBitmap(85, textY + 10, heart_sprite, 12, 12);
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
        else typeText("* Then you are the 1,025th\n* rock I've met today.", 30);
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
      if (isStateFirstFrame) {
        clearText();
        typeText("* My battery is low.", 30);
        isStateFirstFrame = false;
      }
      if (isButtonPressed()) { currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true; }
      break;

    case D_REQUEST_FOOD:
      if (isStateFirstFrame) {
        clearText();
        if (storyProgress == 1) typeText("* Do you have any food?", 30); 
        else if (storyProgress == 2) typeText("* Can I have one more unit?", 30);
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
        tft.fillRect(15, textY + 15, 14, 14, ST7735_BLACK);
        tft.fillRect(85, textY + 15, 14, 14, ST7735_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(15, textY + 15, heart_sprite, 12, 12);
        else tft.drawRGBBitmap(85, textY + 15, heart_sprite, 12, 12);
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
        
        for(int i=0; i<availableCount; i++) {
            tft.setCursor(20 + (i*50), textY + 15); // Compressed X spacing
            int itemType = inventoryOptions[i];
            if(itemType == 0) tft.print("Coffee");
            if(itemType == 1) tft.print("Gas");
            if(itemType == 2) tft.print("Bat.");
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
            tft.fillRect(5 + (lastDrawnSelection*50), textY+15, 12, 12, ST7735_BLACK);
        }
        tft.drawRGBBitmap(5 + (menuSelection*50), textY+15, heart_sprite, 12, 12);
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
        clearText();
        typeText("* CRUNCH CRUNCH.\n* That flavor!", 30);
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
        clearText();
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
        player.x = 80; player.y = 100;
        player.setBounds(20, 140, 50, 120);
       }
       break;
  }
}

void handleBattle() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK);
    // Scaled box for battle
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
      startY += 10; // Reduced line height for smaller screen
      startX = originalX; 
      tft.setCursor(startX, startY);
      continue;
    }

    if (shake) {
       int ox = random(-1, 2); // Reduced shake
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