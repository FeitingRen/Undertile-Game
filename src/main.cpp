#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <Keypad.h> 
#include <SD.h>             // NEW: SD Card
#include <driver/i2s.h>     // NEW: Audio Driver
#include "game_defs.h"
#include "background.h" 
#include "characters.h" 

// --- DEBUG SETTINGS ---
#define DEBUG_SKIP_INTRO false 

// --- DISPLAY SETUP ---
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- KEYPAD SETUP ---
const byte ROWS = 3; 
const byte COLS = 2; 

char hexaKeys[ROWS][COLS] = {
  {0, 'R'},   // Row 2
  {'U', 'D'}, // Row 3
  {'E', 'L'}  // Row 4
};

byte rowPins[ROWS] = {KEYPAD_R2, KEYPAD_R3, KEYPAD_R4}; 
byte colPins[COLS] = {KEYPAD_C3, KEYPAD_C4}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

char globalKey = 0;
unsigned long interactionCooldown = 0; 
unsigned long inputIgnoreTimer = 0; 
unsigned long menuMoveTimer = 0; 

// --- AUDIO GLOBALS ---
uint8_t* audioBuffer = nullptr;
size_t audioBufferSize = 0;
bool audioLoaded = false;
float soundVolume = 0.5; // Range: 0.0 (Mute) to 1.0 (Max)

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
void setupAudio(); // NEW
void playTextSound(); // NEW

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

// --- CLASSES ---
struct Rect { int x, y, w, h; };
struct NPC { int x, y; };

class Player {
public:
  float x, y, oldX, oldY;
  float speed = 2.0; 
  Rect* zones = nullptr;
  int zoneCount = 0;

  void init(int startX, int startY) { x = startX; y = startY; oldX = x; oldY = y; }
  void setZones(Rect* newZones, int count) { zones = newZones; zoneCount = count; }
  
  bool isWalkable(int px, int py) {
      if (zoneCount == 0) return true; 
      for(int i=0; i<zoneCount; i++) {
          if (px >= zones[i].x && px <= zones[i].x + zones[i].w &&
              py >= zones[i].y && py <= zones[i].y + zones[i].h) return true;
      }
      return false;
  }

  bool checkCollision(float newX, float newY, int objX, int objY, int objW, int objH) {
    return (newX < objX + objW && newX + PLAYER_W > objX && newY < objY + objH && newY + PLAYER_H > objY);
  }

  void update(NPC* enemy = nullptr) {
    oldX = x; oldY = y;
    float nextX = x, nextY = y;
    for (int i=0; i<LIST_MAX; i++) {
        if (customKeypad.key[i].kstate == PRESSED || customKeypad.key[i].kstate == HOLD) {
            char k = customKeypad.key[i].kchar;
            if (k == 'L') nextX -= speed;
            if (k == 'R') nextX += speed;
            if (k == 'U') nextY -= speed;
            if (k == 'D') nextY += speed;
        }
    }
    int feetX = (int)nextX + (PLAYER_W / 2); int feetY = (int)y + PLAYER_H;
    if (isWalkable(feetX, feetY)) { if (nextX >= 0 && nextX <= SCREEN_W - PLAYER_W) x = nextX; }
    nextY = (nextY == y) ? y : nextY; 
    feetX = (int)x + (PLAYER_W / 2); feetY = (int)nextY + PLAYER_H;
    if (isWalkable(feetX, feetY)) { if (nextY >= 0 && nextY <= SCREEN_H - PLAYER_H) y = nextY; }

    if (currentState == MAP_WALK && enemy != nullptr) {
       if (checkCollision(x, y, enemy->x, enemy->y, NPC_SIZE, NPC_SIZE)) { x = oldX; y = oldY; }
    }
  }

  void draw(const uint16_t* bgMap = nullptr) {
    if ((int)x != (int)oldX || (int)y != (int)oldY) {
      if (bgMap != nullptr) {
        int rx = (int)oldX; int ry = (int)oldY;
        for(int row = 0; row < PLAYER_H; row++) {
           int currentY = ry + row;
           if(currentY >= 0 && currentY < SCREEN_H) {
              tft.drawRGBBitmap(rx, currentY, (uint16_t*)(bgMap + (currentY * SCREEN_W) + rx), PLAYER_W, 1);
           }
        }
        drawSpriteMixed((int)x, (int)y, heart_sprite, PLAYER_W, PLAYER_H, bgMap);
      } else {
        if (x > oldX) tft.fillRect((int)oldX, (int)oldY, (int)x - (int)oldX, PLAYER_H, ST7735_BLACK);
        else if (x < oldX) tft.fillRect((int)x + PLAYER_W, (int)oldY, (int)oldX - (int)x, PLAYER_H, ST7735_BLACK);
        if (y > oldY) tft.fillRect((int)oldX, (int)oldY, PLAYER_W, (int)y - (int)oldY, ST7735_BLACK);
        else if (y < oldY) tft.fillRect((int)oldX, (int)y + PLAYER_H, PLAYER_W, (int)oldY - (int)y, ST7735_BLACK);
        tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite_blk, PLAYER_W, PLAYER_H);
      }
    }
  }
  
  void forceDraw(const uint16_t* bgMap = nullptr) {
    if (bgMap != nullptr) drawSpriteMixed((int)x, (int)y, heart_sprite, PLAYER_W, PLAYER_H, bgMap);
    else tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite_blk, PLAYER_W, PLAYER_H);
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
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); 
  tft.fillScreen(ST7735_BLACK);
  
  setupAudio(); // Initialize SD and I2S
  
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

// --- UTILS ---
bool isEnterPressed() {
  if (globalKey == 'E') { globalKey = 0; return true; }
  return false;
}

// --- AUDIO FUNCTIONS ---
void setupAudio() {
    // 1. Initialize SD Card
    // Note: SD uses SPI. Ensure SD_CS (21) is correct.
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Start Failed");
        return;
    }

    // 2. Load "text.wav" into RAM
    File file = SD.open("/text.wav");
    if (!file) {
        Serial.println("text.wav not found!");
        return;
    }

    // Read WAV Header to get Sample Rate
    uint8_t header[44];
    file.read(header, 44);
    
    // Extract Sample Rate (Offset 24, 4 bytes, little endian)
    uint32_t sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    uint16_t channels = header[22] | (header[23] << 8);
    
    Serial.printf("WAV: %d Hz, %d Channels\n", sampleRate, channels);

    audioBufferSize = file.size() - 44; // Total size minus header
    audioBuffer = (uint8_t*)malloc(audioBufferSize); // Allocate RAM
    
    if (audioBuffer) {
        file.read(audioBuffer, audioBufferSize); // Copy file to RAM
        audioLoaded = true;
        Serial.println("Audio Loaded to RAM");
    } else {
        Serial.println("Not enough RAM for audio!");
    }
    file.close();

    // 3. Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = (channels == 2) ? I2S_CHANNEL_FMT_RIGHT_LEFT : I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

void playTextSound() {
    if (!audioLoaded || !audioBuffer) return;

    size_t bytes_written;
    
    // We process the audio in small chunks (256 bytes = 128 samples)
    // This keeps the memory usage low and the loop fast.
    uint8_t tempBuffer[256]; 
    size_t chunk_size = sizeof(tempBuffer);

    for (size_t i = 0; i < audioBufferSize; i += chunk_size) {
        // 1. Calculate how many bytes remain
        size_t bytes_to_process = (audioBufferSize - i) < chunk_size ? (audioBufferSize - i) : chunk_size;

        // 2. Copy the raw original audio into our temp buffer
        memcpy(tempBuffer, &audioBuffer[i], bytes_to_process);

        // 3. Apply Volume Scaling
        // We cast the buffer to int16_t* because your WAV is 16-bit
        int16_t* samples = (int16_t*)tempBuffer;
        size_t sample_count = bytes_to_process / 2; // 2 bytes per sample

        for (size_t s = 0; s < sample_count; s++) {
            // Multiply the sample by the volume factor
            samples[s] = (int16_t)(samples[s] * soundVolume);
        }

        // 4. Send the modified chunk to the I2S Driver
        i2s_write(I2S_NUM_0, tempBuffer, bytes_to_process, &bytes_written, portMAX_DELAY);
    }
}

// --- SCENES ---
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

  switch (currentDialogueState) {
    case D_INTRO_1:
      if (isStateFirstFrame) { clearText(); typeText("* WHAT!!?", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) { currentDialogueState = D_INTRO_2; isStateFirstFrame = true; }
      break;
    case D_INTRO_2:
      if (isStateFirstFrame) { clearText(); typeText("* ...", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) { currentDialogueState = D_INTRO_4; isStateFirstFrame = true; }
      break;
    case D_INTRO_4:
      if (isStateFirstFrame) { clearText(); typeText("* Sorry, I've been here\n* alone for so long.", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) { currentDialogueState = D_INTRO_5; isStateFirstFrame = true; }
      break;
    case D_INTRO_5:
      if (isStateFirstFrame) { clearText(); typeText("* I'm actually a\n* nonchalant robot.", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) { currentDialogueState = D_INTRO_6; isStateFirstFrame = true; }
      break;
    case D_INTRO_6:
      if (isStateFirstFrame) { clearText(); typeText("* Are you a human?", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) { 
          currentDialogueState = D_HUMAN_CHOICE; menuSelection = 0; lastDrawnSelection = -1; 
          isStateFirstFrame = true; inputIgnoreTimer = millis() + 500; 
      }
      break;
    case D_HUMAN_CHOICE:
      if (isStateFirstFrame) {
        clearText();
        tft.setCursor(30, textY + 10); tft.print("YES"); tft.setCursor(100, textY + 10); tft.print("NO");
        isStateFirstFrame = false;
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
      if (isEnterPressed() && millis() > inputIgnoreTimer) {
        playerChoiceYesNo = menuSelection; currentDialogueState = D_HUMAN_RESULT_1; isStateFirstFrame = true;
      }
      break;
    case D_HUMAN_RESULT_1:
      if (isStateFirstFrame) {
        clearText();
        if (playerChoiceYesNo == 0) typeText("* First human friend!", 30);
        else typeText("Then you are the 1,025th\n* rock I've met today.", 30);
        isStateFirstFrame = false;
      }
      if (isEnterPressed()) { currentDialogueState = D_HUMAN_RESULT_2; isStateFirstFrame = true; }
      break;
    case D_HUMAN_RESULT_2:
      if (isStateFirstFrame) {
        clearText();
        if (playerChoiceYesNo == 0) typeText("* I mean. Cool. Whatever.", 30);
        else typeText("* The other rocks were\n* less talkative.", 30);
        storyProgress = 1; isStateFirstFrame = false;
      }
      if (isEnterPressed()) { currentDialogueState = D_REQUEST_FOOD_PART1; isStateFirstFrame = true; }
      break;
    case D_REQUEST_FOOD_PART1:
      if (isStateFirstFrame) { clearText(); typeText("* My battery is low.", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) { currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true; inputIgnoreTimer = millis() + 500; }
      break;
    case D_REQUEST_FOOD:
      if (isStateFirstFrame) {
        clearText();
        if (storyProgress == 1) typeText("* Do you have any food?", 30); 
        else if (storyProgress == 2) typeText("* Can I have one more?", 30);
        else if (storyProgress == 3) typeText("* Just one last byte?", 30);
        tft.setCursor(30, textY + 15); tft.print("GIVE"); tft.setCursor(100, textY + 15); tft.print("REFUSE");
        menuSelection = 0; lastDrawnSelection = -1; isStateFirstFrame = false;
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
      if (isEnterPressed() && millis() > inputIgnoreTimer) {
        if (menuSelection == 0) { currentDialogueState = D_SELECT_ITEM; inputIgnoreTimer = millis() + 500; }
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
      if (isEnterPressed() && millis() > inputIgnoreTimer) {
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
      if (isStateFirstFrame) { clearText(); typeText("* CRUNCH CRUNCH.\n* That flavor!", 30); isStateFirstFrame = false; }
      if (isEnterPressed()) {
        storyProgress++; if (storyProgress > 3) storyProgress = 3; 
        currentDialogueState = D_REQUEST_FOOD; isStateFirstFrame = true;
      }
      break;
    case D_REFUSAL:
      if (isStateFirstFrame) { clearText(); typeText("* Oh... okay.\n* I'll just go into Sleep\n* Mode FOREVER.", 40); isStateFirstFrame = false; }
      if (isEnterPressed()) { currentState = MAP_WALK; isStateFirstFrame = true; interactionCooldown = millis() + 1000; }
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
        currentState = BATTLE; isStateFirstFrame = true;
        player.x = 80; player.y = 90;
        static Rect battleZone[] = { {25, 65, 110, 50} }; player.setZones(battleZone, 1);
       }
       break;
  }
}

void handleBattle() {
  if (isStateFirstFrame) {
    tft.fillScreen(ST7735_BLACK); tft.drawRect(24, 64, 112, 52, ST7735_WHITE); 
    player.forceDraw();
    tft.setCursor(30, 20); tft.setTextColor(ST7735_RED); tft.setTextSize(1);
    tft.print("OVERCLOCKED ROBOT"); isStateFirstFrame = false;
  }
  player.update(); player.draw();
}

void typeText(const char* text, int delaySpeed, bool shake) {
  int startX = tft.getCursorX(); int startY = tft.getCursorY(); int originalX = startX; 
  bool hasSkipped = false;

  for (int i = 0; i < strlen(text); i++) {
    if(text[i] == '\n') { startY += 10; startX = originalX; tft.setCursor(startX, startY); continue; }
    
    // --- PLAY AUDIO HERE ---
    if (text[i] != ' ') { // Don't play sound for spaces
        playTextSound();
    }

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
    delay(delaySpeed);
  }
  
  if (hasSkipped) delay(200);
  globalKey = 0; 
}