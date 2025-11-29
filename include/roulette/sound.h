/**
 * @file sound.h
 * @brief Sound generation for roulette animation
 */

#ifndef ROULETTE_SOUND_H
#define ROULETTE_SOUND_H

#ifdef HAVE_SDL_MIXER
#include <SDL3_mixer/SDL_mixer.h>

/**
 * @brief Load or generate tick sound for passing items
 * @param mixer The mixer to create audio for
 * @param audio_dir Optional directory containing audio files (can be NULL)
 * @return Sound audio or NULL on error
 */
MIX_Audio* roulette_sound_load_tick(MIX_Mixer *mixer, const char *audio_dir);

/**
 * @brief Load or generate selection sound for final choice
 * @param mixer The mixer to create audio for
 * @param audio_dir Optional directory containing audio files (can be NULL)
 * @return Sound audio or NULL on error
 */
MIX_Audio* roulette_sound_load_select(MIX_Mixer *mixer, const char *audio_dir);

/**
 * @brief Generate tick sound for passing items (procedural)
 * @param mixer The mixer to create audio for
 * @return Generated sound audio or NULL on error
 */
MIX_Audio* roulette_sound_generate_tick(MIX_Mixer *mixer);

/**
 * @brief Generate selection sound for final choice (procedural)
 * @param mixer The mixer to create audio for
 * @return Generated sound audio or NULL on error
 */
MIX_Audio* roulette_sound_generate_select(MIX_Mixer *mixer);

/**
 * @brief Free a generated sound audio
 * @param audio Sound audio to free
 */
void roulette_sound_free(MIX_Audio *audio);

#endif /* HAVE_SDL_MIXER */

#endif /* ROULETTE_SOUND_H */
