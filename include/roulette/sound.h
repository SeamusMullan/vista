/**
 * @file sound.h
 * @brief Sound generation for roulette animation
 */

#ifndef ROULETTE_SOUND_H
#define ROULETTE_SOUND_H

#ifdef HAVE_SDL_MIXER
#include <SDL2/SDL_mixer.h>

/**
 * @brief Generate tick sound for passing items
 * @return Generated sound chunk or NULL on error
 */
Mix_Chunk* roulette_sound_generate_tick(void);

/**
 * @brief Generate selection sound for final choice
 * @return Generated sound chunk or NULL on error
 */
Mix_Chunk* roulette_sound_generate_select(void);

/**
 * @brief Free a generated sound chunk
 * @param chunk Sound chunk to free
 */
void roulette_sound_free(Mix_Chunk *chunk);

#endif /* HAVE_SDL_MIXER */

#endif /* ROULETTE_SOUND_H */
