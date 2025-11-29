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
#include <sys/stat.h> // For stat, though access is used now
#include <unistd.h>   // For access
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265359

// Generate a simple tick sound (short beep)
MIX_Audio* roulette_sound_generate_tick(MIX_Mixer *mixer) {
    // Create a simple sine wave for tick sound
    // In SDL3_mixer, we'd typically create raw audio data
    // For now, return NULL and rely on file loading
    // TODO: Implement procedural generation with SDL3_mixer API
    return NULL;
}

// Generate a selection sound (triumphant beep)
MIX_Audio* roulette_sound_generate_select(MIX_Mixer *mixer) {
    // Create a simple sine wave for selection sound
    // In SDL3_mixer, we'd typically create raw audio data
    // For now, return NULL and rely on file loading
    // TODO: Implement procedural generation with SDL3_mixer API
    return NULL;
}

// Check if file exists
static bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// Load tick sound from file or generate procedurally
MIX_Audio* roulette_sound_load_tick(MIX_Mixer *mixer, const char *audio_dir) {
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
                MIX_Audio *audio = MIX_LoadAudio(mixer, path, false);
                if (audio) {
                    printf("Loaded tick sound from: %s\n", path);
                    return audio;
                }
            }
        }

        printf("No tick sound file found in %s, generating procedurally\n", audio_dir);
    }

    // Fall back to procedural generation
    return roulette_sound_generate_tick(mixer);
}

// Load selection sound from file or generate procedurally
MIX_Audio* roulette_sound_load_select(MIX_Mixer *mixer, const char *audio_dir) {
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
                MIX_Audio *audio = MIX_LoadAudio(mixer, path, false);
                if (audio) {
                    printf("Loaded selection sound from: %s\n", path);
                    return audio;
                }
            }
        }

        printf("No selection sound file found in %s, generating procedurally\n", audio_dir);
    }

    // Fall back to procedural generation
    return roulette_sound_generate_select(mixer);
}

void roulette_sound_free(MIX_Audio *audio) {
    if (!audio) return;

    // Free the audio resource
    MIX_DestroyAudio(audio);
}

#endif /* HAVE_SDL_MIXER */
