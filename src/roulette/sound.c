/**
 * @file sound.c
 * @brief Simple procedural sound generation for roulette
 */

#include "roulette/sound.h"

#ifdef HAVE_SDL_MIXER

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265359

// Helper function to check if file exists
static int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

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
        float harmonic2 = 0.7f * sinf(2.0f * PI * freq * 2.0f * t);
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

// Load tick sound from file or generate procedurally
Mix_Chunk* roulette_sound_load_tick(const char *audio_dir) {
    if (audio_dir) {
        // Try to load from files - support multiple formats
        const char *filenames[] = {
            "tick.wav",
            "tick.mp3",
            "tick.ogg",
            "roulette_tick.wav",
            "roulette_tick.mp3",
            "roulette_tick.ogg"
        };
        
        char path[1024];
        for (int i = 0; i < 6; i++) {
            snprintf(path, sizeof(path), "%s/%s", audio_dir, filenames[i]);
            if (file_exists(path)) {
                Mix_Chunk *chunk = Mix_LoadWAV(path);
                if (chunk) {
                    printf("Loaded tick sound from: %s\n", path);
                    return chunk;
                }
            }
        }
        
        printf("No tick sound file found in %s, generating procedurally\n", audio_dir);
    }
    
    // Fall back to procedural generation
    return roulette_sound_generate_tick();
}

// Load selection sound from file or generate procedurally
Mix_Chunk* roulette_sound_load_select(const char *audio_dir) {
    if (audio_dir) {
        // Try to load from files - support multiple formats
        const char *filenames[] = {
            "select.wav",
            "select.mp3",
            "select.ogg",
            "roulette_select.wav",
            "roulette_select.mp3",
            "roulette_select.ogg",
            "win.wav",
            "win.mp3",
            "win.ogg"
        };
        
        char path[1024];
        for (int i = 0; i < 9; i++) {
            snprintf(path, sizeof(path), "%s/%s", audio_dir, filenames[i]);
            if (file_exists(path)) {
                Mix_Chunk *chunk = Mix_LoadWAV(path);
                if (chunk) {
                    printf("Loaded selection sound from: %s\n", path);
                    return chunk;
                }
            }
        }
        
        printf("No selection sound file found in %s, generating procedurally\n", audio_dir);
    }
    
    // Fall back to procedural generation
    return roulette_sound_generate_select();
}

void roulette_sound_free(Mix_Chunk *chunk) {
    if (!chunk) return;
    
    // Check if this is a procedurally generated sound (we allocated the buffer)
    // vs a loaded sound (SDL_mixer allocated it)
    // For safety, we'll just use Mix_FreeChunk which handles both cases
    Mix_FreeChunk(chunk);
}

#endif /* HAVE_SDL_MIXER */
