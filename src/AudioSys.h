#ifndef AUDIOSYS_H
#define AUDIOSYS_H

#include <Arduino.h>

void setupAudio();
void playVoice(); // Plays the "blip" sound (blocking-ish but fast)

// --- NEW NON-BLOCKING API ---
void startSFX(const char* filename); // Starts playing a file
void updateSFX();                    // Call this in your loop to keep music playing
void stopSFX();                      // Stops music manually
bool isSFXPlaying();                 // Check if music is still going

// A replacement for delay() that keeps music playing
void waitAndPump(int ms);

#endif