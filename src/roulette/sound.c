/**
 * @file sound.c
 * @brief Simple procedural sound generation for roulette
 */

#include "roulette/sound.h"

#ifdef HAVE_SDL_MIXER

#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265359

// Generate a simple tick sound (short beep)
Mix_Chunk* roulette_sound_generate_tick(void) {
    int duration_ms = 30;  // Very short click
    int samples = (SAMPLE_RATE * duration_ms) / 1000;
    
    Sint16 *buffer = malloc(samples * sizeof(Sint16));
    if (!buffer) return NULL;
    
    // Generate a short, high-pitched tick
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float freq = 1200.0f; // High frequency for tick
        float envelope = 1.0f - ((float)i / samples); // Quick decay
        
        // Simple sine wave with envelope
        float sample = sinf(2.0f * PI * freq * t) * envelope;
        buffer[i] = (Sint16)(sample * 8000.0f); // Lower volume
    }
    
    Mix_Chunk *chunk = malloc(sizeof(Mix_Chunk));
    if (!chunk) {
        free(buffer);
        return NULL;
    }
    
    chunk->allocated = 1;
    chunk->abuf = (Uint8*)buffer;
    chunk->alen = samples * sizeof(Sint16);
    chunk->volume = MIX_MAX_VOLUME / 2; // 50% volume
    
    return chunk;
}

// Generate a selection sound (triumphant beep)
Mix_Chunk* roulette_sound_generate_select(void) {
    int duration_ms = 400;  // Longer, more satisfying sound
    int samples = (SAMPLE_RATE * duration_ms) / 1000;
    
    Sint16 *buffer = malloc(samples * sizeof(Sint16));
    if (!buffer) return NULL;
    
    // Generate a rising tone with harmonics
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = (float)i / samples;
        
        // Rising pitch from 400Hz to 800Hz
        float freq = 400.0f + (400.0f * progress);
        
        // Envelope: quick attack, sustained, slow release
        float envelope;
        if (progress < 0.1f) {
            envelope = progress / 0.1f; // Attack
        } else if (progress < 0.7f) {
            envelope = 1.0f; // Sustain
        } else {
            envelope = 1.0f - ((progress - 0.7f) / 0.3f); // Release
        }
        
        // Fundamental + harmonics for richer sound
        float fundamental = sinf(2.0f * PI * freq * t);
        float harmonic2 = 0.5f * sinf(2.0f * PI * freq * 2.0f * t);
        float harmonic3 = 0.25f * sinf(2.0f * PI * freq * 3.0f * t);
        
        float sample = (fundamental + harmonic2 + harmonic3) * envelope;
        buffer[i] = (Sint16)(sample * 12000.0f);
    }
    
    Mix_Chunk *chunk = malloc(sizeof(Mix_Chunk));
    if (!chunk) {
        free(buffer);
        return NULL;
    }
    
    chunk->allocated = 1;
    chunk->abuf = (Uint8*)buffer;
    chunk->alen = samples * sizeof(Sint16);
    chunk->volume = MIX_MAX_VOLUME;
    
    return chunk;
}

void roulette_sound_free(Mix_Chunk *chunk) {
    if (!chunk) return;
    if (chunk->abuf) {
        free(chunk->abuf);
    }
    free(chunk);
}

#endif /* HAVE_SDL_MIXER */
