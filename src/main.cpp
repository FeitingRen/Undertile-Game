#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include "game_defs.h"
#include "background.h" 
#include "characters.h" 
#include "AudioSys.h"
#include "Battle.h"
#include "Globals.h"
#include "Player.h"  
#include "Utils.h"

// --- DEBUG SETTINGS ---
#define DEBUG_SKIP_INTRO false 

char globalKey = 0;
unsigned long interactionCooldown = 0; 
unsigned long inputIgnoreTimer = 0; 
unsigned long menuMoveTimer = 0; 

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
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); 
  tft.fillScreen(ST7735_BLACK);
  
  setupAudio(); 
  
  #if DEBUG_SKIP_INTRO
    currentState = BATTLE;
    player.x = 80; player.y = 90; 
    static Rect battleZone[] = { {31, 77, 98, 38} }; 
    player.setZones(battleZone, 1);
  #else
    player.init(15, 60); 
  #endif
}

void loop() {
  unsigned long currentTime = millis();
  customKeypad.getKeys();
  globalKey = 0;
  for (int i=0; i<LIST_MAX; i++) {
      if (customKeypad.key[i].kstate == PRESSED) globalKey = customKeypad.key[i].kchar;
  }

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

bool isEnterPressed() {
  if (globalKey == 'E') { globalKey = 0; return true; }
  return false;
}

void handleMenu() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE); tft.setTextSize(1);
    tft.setCursor(30, 40); tft.print("UNDERTALE ESP32");
    tft.setCursor(20, 80); tft.print("Press Enter to Start");
    isStateFirstFrame = false; 
  }
  if (isEnterPressed()) {
    currentState = MAP_WALK; isStateFirstFrame = true;
    player.x = 25; player.y = 60; 
    static Rect walkableFloors[] = {
      { 21, 48, 122, 2 }, { 21, 51, 109, 43 }, { 10, 82, 11, 2 },
      { 11, 79, 9, 2 }, { 13, 74, 8, 4 }, { 14, 71, 7, 2 },
      { 15, 68, 6, 2 }, { 17, 66, 4, 1 }, { 18, 63, 3, 2 },
      { 18, 61, 2, 1 }, { 19, 58, 1, 2 }, { 130, 69, 7, 25 }, { 137, 82, 6, 12 }
    };
    player.setZones(walkableFloors, 13);
  }
}

void handleMap() {
  if (isStateFirstFrame) {
    tft.drawRGBBitmap(0, 0, bg_map, 160, 128);
    player.forceDraw(bg_map); isStateFirstFrame = false;
  }
  player.update(&enemy); 
  drawSpriteMixed(enemy.x, enemy.y, robot_npc, 16, 16, bg_map);
  player.draw(bg_map);

  float dist = sqrt(pow(player.x - enemy.x, 2) + pow(player.y - enemy.y, 2));
  if (dist < 20 && isEnterPressed() && millis() > interactionCooldown) {
      currentState = DIALOGUE;
      if (storyProgress == 0) currentDialogueState = D_INTRO_1; 
      else if (storyProgress == 1) currentDialogueState = D_REQUEST_FOOD_PART1; 
      else currentDialogueState = D_REQUEST_FOOD; 
      isStateFirstFrame = true;
  } 
}

void handleDialogue() {
  int boxY = 88; int boxH = 40; int textY = 94;
  if (isStateFirstFrame && currentDialogueState != D_COFFEE_EVENT) {
    tft.fillRect(2, boxY, 156, boxH, ST7735_BLACK); 
    tft.drawRect(0, boxY-2, 160, boxH+2, ST7735_WHITE); 
    tft.setTextColor(ST7735_WHITE); tft.setTextSize(1); tft.setCursor(5, textY); 
  }
  auto clearText = [&]() { tft.fillRect(4, boxY+2, 152, boxH-4, ST7735_BLACK); tft.setCursor(5, textY); };

  // Helper to ensure we don't skip accidentally
  // It checks if Enter is pressed AND if the input cooldown has passed
  auto canProceed = [&]() { return isEnterPressed() && millis() > inputIgnoreTimer; };

  switch (currentDialogueState) {
    case D_INTRO_1:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* WHAT!!?", 30); 
          isStateFirstFrame = false; 
          inputIgnoreTimer = millis() + 300; // Wait 300ms before accepting next input
      }
      if (canProceed()) { currentDialogueState = D_INTRO_2; isStateFirstFrame = true; }
      break;
    case D_INTRO_2:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* ...", 30); 
          isStateFirstFrame = false; 
          inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentDialogueState = D_INTRO_4; isStateFirstFrame = true; }
      break;
    case D_INTRO_4:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* Sorry, I've been here\n* alone for so long.", 30); 
          isStateFirstFrame = false; 
          inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentDialogueState = D_INTRO_5; isStateFirstFrame = true; }
      break;
    case D_INTRO_5:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* I'm actually a\n* nonchalant robot.", 30); 
          isStateFirstFrame = false; 
          inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentDialogueState = D_INTRO_6; isStateFirstFrame = true; }
      break;
    case D_INTRO_6:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* Are you a human?", 30); 
          isStateFirstFrame = false; 
          inputIgnoreTimer = millis() + 500; // Slightly longer wait before choice
      }
      if (canProceed()) { 
          currentDialogueState = D_HUMAN_CHOICE; menuSelection = 0; lastDrawnSelection = -1; 
          isStateFirstFrame = true; 
      }
      break;
    case D_HUMAN_CHOICE:
      if (isStateFirstFrame) {
        clearText();
        tft.setCursor(30, textY + 10); tft.print("YES"); tft.setCursor(100, textY + 10); tft.print("NO");
        isStateFirstFrame = false;
        inputIgnoreTimer = millis() + 300; // Safety delay
      }
      if (millis() > menuMoveTimer) {
        if (globalKey == 'R') { menuSelection = 1; menuMoveTimer = millis() + 200; globalKey = 0; }
        if (globalKey == 'L') { menuSelection = 0; menuMoveTimer = millis() + 200; globalKey = 0; }
      }
      if (menuSelection != lastDrawnSelection) {
        tft.fillRect(15, textY + 8, 14, 14, ST7735_BLACK); tft.fillRect(85, textY + 8, 14, 14, ST7735_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(15, textY + 8, heart_sprite_blk, 12, 12);
        else tft.drawRGBBitmap(85, textY + 8, heart_sprite_blk, 12, 12);
        lastDrawnSelection = menuSelection;
      }
      if (canProceed()) {
        playerChoiceYesNo = menuSelection; currentDialogueState = D_HUMAN_RESULT_1; isStateFirstFrame = true;
      }
      break;
    case D_HUMAN_RESULT_1:
      if (isStateFirstFrame) {
        clearText();
        if (playerChoiceYesNo == 0) typeText("* First human friend!", 30);
        else typeText("Then you are the 1,025th\n* rock I've met today.", 30);
        isStateFirstFrame = false;
        inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentDialogueState = D_HUMAN_RESULT_2; isStateFirstFrame = true; }
      break;
    case D_HUMAN_RESULT_2:
      if (isStateFirstFrame) {
        clearText();
        if (playerChoiceYesNo == 0) typeText("* I mean. Cool. Whatever.", 30);
        else typeText("* The other rocks were\n* less talkative.", 30);
        storyProgress = 1; isStateFirstFrame = false;
        inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentDialogueState = D_REQUEST_FOOD_PART1; isStateFirstFrame = true; }
      break;
    case D_REQUEST_FOOD_PART1:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* My battery is low.", 30); 
          isStateFirstFrame = false;
          inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true; }
      break;
    case D_REQUEST_FOOD:
      if (isStateFirstFrame) {
        clearText();
        if (storyProgress == 1) typeText("* Do you have any food?", 30); 
        else if (storyProgress == 2) typeText("* Can I have one more?", 30);
        else if (storyProgress == 3) typeText("* Just one last byte?", 30);
        tft.setCursor(30, textY + 15); tft.print("GIVE"); tft.setCursor(100, textY + 15); tft.print("REFUSE");
        menuSelection = 0; lastDrawnSelection = -1; isStateFirstFrame = false;
        inputIgnoreTimer = millis() + 300;
      }
      if (millis() > menuMoveTimer) {
        if (globalKey == 'R') { menuSelection = 1; menuMoveTimer = millis() + 200; globalKey = 0; }
        if (globalKey == 'L') { menuSelection = 0; menuMoveTimer = millis() + 200; globalKey = 0; }
      }
      if (menuSelection != lastDrawnSelection) {
        tft.fillRect(15, textY + 13, 14, 14, ST7735_BLACK); tft.fillRect(85, textY + 13, 14, 14, ST7735_BLACK);
        if (menuSelection == 0) tft.drawRGBBitmap(15, textY + 13, heart_sprite_blk, 12, 12);
        else tft.drawRGBBitmap(85, textY + 13, heart_sprite_blk, 12, 12);
        lastDrawnSelection = menuSelection;
      }
      if (canProceed()) {
        if (menuSelection == 0) { currentDialogueState = D_SELECT_ITEM; }
        else currentDialogueState = D_REFUSAL;
        isStateFirstFrame = true;
      }
      break;
    case D_SELECT_ITEM:
      if (isStateFirstFrame) {
        clearText(); tft.print("Give what?"); availableCount = 0;
        if (playerInventory.hasCoffee) inventoryOptions[availableCount++] = 0;
        if (playerInventory.hasGas)    inventoryOptions[availableCount++] = 1;
        if (playerInventory.hasBattery) inventoryOptions[availableCount++] = 2;
        int currentX = 20; int gap = 20; 
        for(int i=0; i<availableCount; i++) {
            itemXPositions[i] = currentX; tft.setCursor(currentX, textY + 15); 
            int itemType = inventoryOptions[i]; int textWidth = 0;
            if(itemType == 0) { tft.print("Coffee"); textWidth = 36; } 
            if(itemType == 1) { tft.print("Gas");    textWidth = 18; } 
            if(itemType == 2) { tft.print("Bat.");   textWidth = 24; } 
            currentX += textWidth + gap;
        }
        menuSelection = 0; lastDrawnSelection = -1; isStateFirstFrame = false;
        inputIgnoreTimer = millis() + 300;
      }
      if (millis() > menuMoveTimer) {
          if (globalKey == 'R' && menuSelection < availableCount-1) { menuSelection++; menuMoveTimer = millis() + 200; globalKey = 0; }
          if (globalKey == 'L' && menuSelection > 0) { menuSelection--; menuMoveTimer = millis() + 200; globalKey = 0; }
      }
      if (menuSelection != lastDrawnSelection) {
        if (lastDrawnSelection != -1) tft.fillRect(itemXPositions[lastDrawnSelection] - 14, textY+13, 12, 12, ST7735_BLACK);
        tft.drawRGBBitmap(itemXPositions[menuSelection] - 14, textY+13, heart_sprite_blk, 12, 12);
        lastDrawnSelection = menuSelection;
      }
      if (canProceed()) {
        int chosenItem = inventoryOptions[menuSelection];
        if (chosenItem == 0) { playerInventory.hasCoffee = false; currentDialogueState = D_COFFEE_EVENT; } 
        else {
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
          inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) {
        storyProgress++; if (storyProgress > 3) storyProgress = 3; 
        currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true;
      }
      break;
    case D_REFUSAL:
      if (isStateFirstFrame) { 
          clearText(); 
          typeText("* Oh... okay.\n* I'll just go into Sleep\n* Mode FOREVER.", 40); 
          isStateFirstFrame = false; 
          inputIgnoreTimer = millis() + 300;
      }
      if (canProceed()) { currentState = MAP_WALK; isStateFirstFrame = true; interactionCooldown = millis() + 1000; }
      break;
    case D_COFFEE_EVENT:
       if (isStateFirstFrame) {
        tft.fillScreen(ST7735_BLACK); tft.setTextColor(ST7735_WHITE); tft.setTextSize(1);
        tft.setCursor(10, 30); typeText("THANKS! SLURP...", 50); delay(300);
        tft.setCursor(10, 50); typeText("Analyzing...", 50); delay(1000);
        tft.setCursor(10, 70); typeText("Is this C8H10N4O2?", 50); delay(1000);
        tft.setTextColor(ST7735_RED);
        tft.setCursor(10, 90); typeText("Was that... COFFEE?", 100, true); delay(1500);
        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(5, 50); typeText("NO OVERCLOCKING!!!", 30, true); delay(1000);
        tft.setCursor(5, 70); typeText("I CAN TASTE MATH!", 20, true); delay(1000);
        tft.fillScreen(ST7735_RED); delay(100); tft.fillScreen(ST7735_BLACK);
        tft.setCursor(20, 60); typeText("CTRL+ALT+DELETE ME!", 10, true); delay(1000);
        currentState = BATTLE; 
        isStateFirstFrame = true;
        player.x = 80; player.y = 90;
        static Rect battleZone[] = { {25, 65, 110, 50} }; player.setZones(battleZone, 1);
       }
       break;
  }
}

void handleBattle() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK); tft.drawRect(23, 63, 113, 53, ST7735_WHITE); 
    player.forceDraw();
    tft.setCursor(30, 20); tft.setTextColor(ST7735_RED); tft.setTextSize(1);
    tft.print("OVERCLOCKED ROBOT"); isStateFirstFrame = false;
  }
  player.update(); player.draw();
}