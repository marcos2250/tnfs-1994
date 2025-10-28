/*
 * tnfs_sfx.h
 */

#ifndef TNFS_SFX_H_
#define TNFS_SFX_H_

#include <stdint.h>

typedef struct sfx_assets {
	float length;
    float playback_pos;
    float volume;
    float pitch;
    char loop;
    char play;
    int16_t * data;
} sfx_assets;

extern sfx_assets g_sounds[32];


void sfx_init_frontend();
void sfx_init_sim(int carId);
void sfx_clear_buffers();

void sfx_play_sound(int id, char loop, float pitch, float volume);
void sfx_stop_sound(int id);

void sfx_play_music(int id);
void sfx_play_speech_car(int carId);
void sfx_play_speech_track(int trackId);

void sfx_sdl_audio_callback(void* userdata, int16_t* stream, int len);


#endif /* TNFS_SFX_H_ */
