#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// --- WIRING DEFINITIONS (ESP32 Standard VSPI) ---
#define TFT_CS        5
#define TFT_RST       4
#define TFT_DC        2
#define JOYSTICK_X    34
#define JOYSTICK_Y    35
#define BUTTON_PIN    0 

// --- DISPLAY SETUP ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// --- FORWARD DECLARATIONS ---
void handleMenu();
void handleMap();
void handleDialogue();
void handleBattle();
void typeText(const char*, int);

// --- GAME CONSTANTS ---
const int PLAYER_W = 16;
const int PLAYER_H = 16;
const int NPC_SIZE = 20; // Size of the red square NPC

// 16x16 Heart Sprite
const uint16_t heart_sprite[256] = {
  0x0000, 0x0000, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0x0000, 0x0000, 
  0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 
  0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 
  0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

enum GameState { MENU, MAP_WALK, DIALOGUE, BATTLE, GAME_OVER };
GameState currentState = MENU;

// --- DIALOGUE VARIABLES ---
int dialoguePhase = 0;   // 0=Intro, 1=Choice, 2=Response, 3=Merged
int selectedOption = 0;  // 0 = Left (Option A), 1 = Right (Option B)

// --- SCRIPT DEFINITIONS ---
const char* text_Intro     = "* ARE YOU A ROBOT?";
const char* text_OptionA   = "YES";
const char* text_OptionB   = "NO";

const char* text_ReplyA    = "* I KNEW IT! BROTHER!";       // If they picked Yes
const char* text_ReplyB    = "* LIES! YOU SMELL LIKE MEAT."; // If they picked No

const char* text_Merged    = "* ANYWAY... PREPARE TO DIE!";  // Combined path

bool isStateFirstFrame = true;
unsigned long lastFrameTime = 0;
const int targetFPS = 60;
const int frameDelay = 1000 / targetFPS;

// --- MOVED UP: NPC DEFINITION (So Player can see it) ---
struct NPC {
  int x, y;
  bool active;
};

NPC enemy = {140, 2, true}; 

// --- Player Class ---
class Player {
public:
  float x, y;
  float oldX, oldY; 
  float speed = 3.0;
  
  // Movement Boundaries
  int minX, maxX, minY, maxY;

  const int w = 16;
  const int h = 16;

  void init(int startX, int startY) {
    x = startX;
    y = startY;
    oldX = x;
    oldY = y;
    setBounds(0, 240, 0, 320);
  }

  void setBounds(int x1, int x2, int y1, int y2) {
    minX = x1;
    maxX = x2;
    minY = y1;
    maxY = y2;
  }

  // AABB Collision Check
  bool checkCollision(float newX, float newY, int objX, int objY, int objW, int objH) {
    return (newX < objX + objW &&
            newX + w > objX &&
            newY < objY + objH &&
            newY + h > objY);
  }

  void update() {
    oldX = x;
    oldY = y;

    float nextX = x;
    float nextY = y;

    int joyX = analogRead(JOYSTICK_X);
    int joyY = analogRead(JOYSTICK_Y);
    
    // Joystick Logic
    if (joyX < 1800) nextX += speed; 
    if (joyX > 2200) nextX -= speed; 
    if (joyY < 1800) nextY += speed; 
    if (joyY > 2200) nextY -= speed; 
    
    // Boundary Checks for next position
    if (nextX < minX) nextX = minX;
    if (nextX > maxX - w) nextX = maxX - w;
    if (nextY < minY) nextY = minY;
    if (nextY > maxY - h) nextY = maxY - h;

    // COLLISION CHECK: Only update if we don't hit the NPC in Map Mode
    if (currentState == MAP_WALK) {
       if (!checkCollision(nextX, nextY, enemy.x, enemy.y, NPC_SIZE, NPC_SIZE)) {
          x = nextX;
          y = nextY;
       }
    } else {
       x = nextX;
       y = nextY;
    }
  }

  void draw() {
    // Only redraw if position changed
    if ((int)x != (int)oldX || (int)y != (int)oldY) {
      
      // --- FIX 1: FLICKER-FREE RENDERING ---
      // Instead of erasing the whole block (which causes a black flash),
      // we draw the NEW sprite first, then only erase the thin strips
      // left behind by the movement.
      
      // 1. Draw the new sprite (overwriting the overlap area)
      tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite, w, h);

      // 2. Erase the TRAIL (Horizontal)
      if (x > oldX) {
        // Moved Right: Erase strip on the Left
        tft.fillRect((int)oldX, (int)oldY, (int)x - (int)oldX, h, ILI9341_BLACK);
      } else if (x < oldX) {
        // Moved Left: Erase strip on the Right
        tft.fillRect((int)x + w, (int)oldY, (int)oldX - (int)x, h, ILI9341_BLACK);
      }

      // 3. Erase the TRAIL (Vertical)
      if (y > oldY) {
        // Moved Down: Erase strip on Top
        tft.fillRect((int)oldX, (int)oldY, w, (int)y - (int)oldY, ILI9341_BLACK);
      } else if (y < oldY) {
        // Moved Up: Erase strip on Bottom
        tft.fillRect((int)oldX, (int)y + h, w, (int)oldY - (int)y, ILI9341_BLACK);
      }
    }
  }
  
  void forceDraw() {
    tft.drawRGBBitmap((int)x, (int)y, (uint16_t*)heart_sprite, w, h);
  }
};

Player player;

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
      case MENU:
        handleMenu();
        break;
      case MAP_WALK:
        handleMap();
        break;
      case DIALOGUE:
        handleDialogue();
        break;
      case BATTLE:
        handleBattle();
        break;
      case GAME_OVER:
        break;
    }
  }
}

// --- SCENE FUNCTIONS ---

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
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    currentState = MAP_WALK;
    isStateFirstFrame = true;
    player.x = 20; 
    player.y = 120;
    player.setBounds(0, 320, 0, 240);
    delay(200); 
  }
}

void handleMap() {
  if (isStateFirstFrame) {
    tft.fillScreen(ILI9341_BLACK); 
    player.forceDraw();
    isStateFirstFrame = false;
  }

  // Update Player Position (Logic handles collision now)
  player.update();
  
  // Draw Player
  player.draw();

  // Draw NPC (Always redraw to ensure it stays on top of any erased trails)
  tft.fillRect(enemy.x, enemy.y, NPC_SIZE, NPC_SIZE, ILI9341_RED);

  // Check Distance to NPC for Interaction
  float dist = sqrt(pow(player.x - enemy.x, 2) + pow(player.y - enemy.y, 2));

  // If close enough (30px) AND Button Pressed -> Dialogue
  if (dist < 30 && digitalRead(BUTTON_PIN) == LOW) {
      currentState = DIALOGUE;
      isStateFirstFrame = true;
      delay(200); 
  } 
}

void handleDialogue() {
  
  // --- PHASE 0: THE ROBOT ASKS A QUESTION ---
  if (dialoguePhase == 0) {
    if (isStateFirstFrame) {
      // Draw UI
      tft.fillRect(10, 180, 300, 50, ILI9341_BLACK); 
      tft.drawRect(8, 178, 304, 54, ILI9341_WHITE); 
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      
      // Type the Question
      tft.setCursor(20, 190);
      typeText(text_Intro, 40);
      
      isStateFirstFrame = false;
    }

    // Wait for Button to Proceed to Choice
    if (digitalRead(BUTTON_PIN) == LOW) {
      while(digitalRead(BUTTON_PIN) == LOW) delay(10); // Wait for release
      
      dialoguePhase = 1; // Move to Choice Phase
      isStateFirstFrame = true; // Trigger setup for next phase
    }
  }

  // --- PHASE 1: PLAYER CHOOSES (YES / NO) ---
  else if (dialoguePhase == 1) {
    if (isStateFirstFrame) {
      // Clear text area
      tft.fillRect(12, 182, 296, 46, ILI9341_BLACK);
      
      // Draw Options
      tft.setCursor(60, 200);
      tft.print(text_OptionA); // "YES"
      tft.setCursor(180, 200);
      tft.print(text_OptionB); // "NO"
      
      isStateFirstFrame = false;
    }

    // --- HANDLE JOYSTICK SELECTION ---
    int joyX = analogRead(JOYSTICK_X);
    int prevOption = selectedOption;

    if (joyX > 2500) selectedOption = 0; // Left (YES)
    if (joyX < 1500) selectedOption = 1; // Right (NO)

    // Only redraw the heart cursor if the selection CHANGED (prevents flickering)
    static int lastDrawnOption = -1;
    if (selectedOption != lastDrawnOption) {
       // Erase both possible heart positions
       tft.fillRect(40, 200, 16, 16, ILI9341_BLACK);
       tft.fillRect(160, 200, 16, 16, ILI9341_BLACK);

       // Draw Heart at new position
       if (selectedOption == 0){
        tft.drawRGBBitmap(40, 200, heart_sprite, 16, 16);
       } 
       else{
        tft.drawRGBBitmap(160, 200, heart_sprite, 16, 16);
       }
       
       lastDrawnOption = selectedOption;
    }

    // --- CONFIRM SELECTION ---
    if (digitalRead(BUTTON_PIN) == LOW) {
      while(digitalRead(BUTTON_PIN) == LOW) delay(10); // Wait for release
      
      dialoguePhase = 2; // Move to Reaction Phase
      isStateFirstFrame = true;
    }
  }

  // --- PHASE 2: ROBOT REACTS (BRANCHING) ---
  else if (dialoguePhase == 2) {
    if (isStateFirstFrame) {
      tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear Options
      tft.setCursor(20, 190);
      
      // BRANCHING LOGIC IS HERE
      if (selectedOption == 0) {
        typeText(text_ReplyA, 40); // User chose YES
      } else {
        typeText(text_ReplyB, 40); // User chose NO
      }
      
      isStateFirstFrame = false;
    }

    // Wait for Button to Merge
    if (digitalRead(BUTTON_PIN) == LOW) {
      while(digitalRead(BUTTON_PIN) == LOW) delay(10);
      
      dialoguePhase = 3; // Move to Merged Phase
      isStateFirstFrame = true;
    }
  }

  // --- PHASE 3: MERGED PATH (COMMON TEXT) ---
  else if (dialoguePhase == 3) {
    if (isStateFirstFrame) {
      tft.fillRect(12, 182, 296, 46, ILI9341_BLACK); // Clear Reaction
      tft.setCursor(20, 190);
      
      typeText(text_Merged, 40); // Everyone sees this
      
      isStateFirstFrame = false;
    }

    // Wait for Button to End Conversation / Start Battle
    if (digitalRead(BUTTON_PIN) == LOW) {
       // Reset dialogue for next time (optional)
       dialoguePhase = 0; 
       
       // Go to Battle
       currentState = BATTLE;
       isStateFirstFrame = true;
       player.x = 160; 
       player.y = 200;
       player.setBounds(50, 270, 130, 230); 
       delay(200);
    }
  }
}

void handleBattle() {
  if (isStateFirstFrame) {
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(48, 128, 224, 104, ILI9341_WHITE); 
    player.forceDraw();
    isStateFirstFrame = false;
  }

  player.update();
  player.draw();
}

void typeText(const char* text, int delaySpeed) {
  bool hasSkipped = false;

  for (int i = 0; i < strlen(text); i++) {
    tft.print(text[i]);

    // Check if player wants to skip
    if (digitalRead(BUTTON_PIN) == LOW) {
      delaySpeed = 0;    // Set delay to 0 (instant text)
      hasSkipped = true; // Remember that we skipped
    }

    delay(delaySpeed);
  }

  // SAFETY CHECK: 
  // If they skipped, they are currently holding the button.
  // We must wait for them to let go BEFORE returning, or they might
  // accidentally trigger the next action immediately.
  if (hasSkipped) {
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10); // Wait here until button is released
    }
    delay(100); // Small safety pause (debounce)
  }
}