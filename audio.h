#pragma once

#include <Arduino.h>

// Initializes SPIFFS and leaves the GPIO 26 DAC at its unsigned-PCM midpoint.
// Call once from setup(). Returns false if synchronization objects or SPIFFS
// cannot be initialized.
bool audioInit();

// Loads and validates an 8-bit, mono, uncompressed PCM WAV from SPIFFS, then
// starts asynchronous playback. A new call cancels and joins prior playback.
// The playback task owns the sample buffer until it finishes or is stopped.
bool audioPlayWav(const char *filename);

bool audioIsPlaying();
void audioStop();

// Scales samples around unsigned-PCM silence (128), rather than toward zero.
// Values over 100 are clamped.
void audioSetVolume(uint8_t percent);