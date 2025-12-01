#ifndef AUDIOSYS_H
#define AUDIOSYS_H

#include <Arduino.h>

void setupAudio();

// Plays the currently loaded "voice" sound (e.g., text.wav)
// fast and optimized for repetitive typing.
void playVoice();

// Loads and plays a specific sound file once (Blocking).
// Use this for events like the dialup sounds.
void playSFX(const char* filename);

// Allows changing the typing sound (e.g. to a different voice)
void setVoice(const char* filename);

#endif