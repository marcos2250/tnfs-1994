/*
 * tnfs_sfx.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tnfs_sfx.h"
#include "tnfs_files.h"

char * g_sfx_music_files[4] = {
		"DriveData/aiff/FunkinA.44K.sw.aifc",
		"DriveData/aiff/FunkinB.44K.sw.aifc",
		"DriveData/aiff/FunkinC.44K.sw.aifc",
		"DriveData/aiff/Credits.44K.sw.aifc"
};

char * g_sfx_speech_track_files[4] = { "ALPINE", "COASTAL", "CITY", "ALPINE" };
char * g_sfx_speech_car_files[8] = { "SUPRA", "DIABLO", "911", "ZR1", "512TR", "VIPER", "NSX", "RX7" };
char * g_sfx_car_files[8] = { "Supra", "Diablo", "911", "ZR1", "TR512", "Viper", "NSX", "RX7" };

// sound files cache
struct sfx_assets g_sounds[32];
int g_sound_counter = 0;

int music_rnd = 0;

void sfx_clear_buffers() {
	int i;
	for (i = 0; i < 32; i++) {
		if (g_sounds[i].data != 0) {
			free(g_sounds[i].data);
		}
		g_sounds[i].data = 0;
		g_sounds[i].length = 0;
		g_sounds[i].playback_pos = 0;
		g_sounds[i].volume = 1;
		g_sounds[i].pitch = 1;
		g_sounds[i].loop = 0;
		g_sounds[i].play = 0;
	}
	g_sound_counter = 0;
}

int16_t * sfx_resample_aiff(byte * input, int inLength, int * outLength) {
	int16_t sample;
    int16_t * out;
    int16_t * out_buffer;

    *outLength = inLength * 2; // convert mono to stereo

	out_buffer = malloc(*outLength);
	if (out_buffer == 0)
		return 0;

	out = out_buffer;
	for (int i = 0; i < inLength; i += 2) {
		sample = (input[i] << 8) | input[i + 1];
		*out++ = sample;
		*out++ = sample;
	}
	return out_buffer;
}

/**
 * SDX2_DPCM decoder (adapted from FFmpeg)
 */
int16_t * sfx_decompress_aifc(byte * input, int inLength, int * outLength) {
	int i;
	int ch = 0;
	int s_sample[2];
    int16_t * out;
    int16_t * out_buffer;

    *outLength = inLength * 2; // decompression doubles the size

	out_buffer = malloc(*outLength);
	if (out_buffer == 0) {
		return 0;
	}

	out = out_buffer;
	for (i = 0; i < inLength; i++) {
        int8_t n = input[i];

        if (!(n & 1))
            s_sample[ch] = 0;

        if (n < 0) {
        	s_sample[ch] += n * n * 2;
        } else {
        	s_sample[ch] -= n * n * 2;
        }

        //Clip a signed integer value into the -32768,32767 range.
        if ((s_sample[ch] + 0x8000U) & ~0xFFFF) {
        	s_sample[ch] = (s_sample[ch] >> 31) ^ 0x7FFF;
        }

        *out++ = s_sample[ch];
        ch ^= 1; //stereo;
    }
	return out_buffer;
}

int16_t * sfx_read_aiff(byte * aiff, int * outLength) {
	int length = 0;
	char is_aifc = 0;
	byte * data = aiff;

	length = aiff[7] | ( aiff[6] << 8 ) | ( aiff[5] << 16 ) | ( aiff[4] << 24 );
	*outLength = length;

	for (int i = 0; i < length; i++) {
		if (data[0] == 'A' && data[1] == 'I' && data[2] == 'F' && data[3] == 'C') {
			is_aifc = 1;
		}
		if (data[0] == 'S' && data[1] == 'S' && data[2] == 'N' && data[3] == 'D') {
			break;
		}
		data++;
	}

	if (data == 0)
		return 0;

	//'SSND' + block length
	if (data[6] == 0 && data[7] == 0) {
		length = data[5] | ( data[4] << 8 );
	} else {
		length = data[7] | ( data[6] << 8 ) | ( data[5] << 16 ) | ( data[4] << 24 );
	}
	length -= 8;
	data += 8;

	if (is_aifc) {
		return sfx_decompress_aifc(data, length, outLength);
	}
	return sfx_resample_aiff(data, length, outLength);
}

void sfx_load_audio_file(char * filename) {
	int fileSize = 0;
	int out_length = 0;
	int16_t * out_buffer;

	unsigned char * file = openFile(filename, &fileSize);
	if (fileSize < 1)
		return;

	out_buffer = sfx_read_aiff(file, &out_length);
	if (out_buffer == 0)
		return;

	g_sounds[g_sound_counter].data = out_buffer;
	g_sounds[g_sound_counter].length = out_length;
	g_sound_counter++;

	free(file);
}

void sfx_load_audio_bank(char * filename) {
	int numEntries = 0;
	int fileSize = 0;
	int wpath[3];
	int out_length = 0;
	unsigned char * aiff;

	unsigned char * filedata = openFile(filename, &fileSize);
	if (filedata == 0)
		return;

	numEntries = filedata[7];
	for (int i = 0; i < numEntries; i++) {
		wpath[0] = i;
		aiff = read_wwww(filedata, wpath, 1);
		g_sounds[g_sound_counter].data = sfx_read_aiff(aiff, &out_length);
		if (g_sounds[g_sound_counter].data == 0)
			continue;
		g_sounds[g_sound_counter].length = out_length;
		g_sound_counter++;
	}
}

void sfx_load_file_into_channel(char * filename, int channelId) {
	int fileSize = 0;
	int out_length = 0;
	g_sounds[channelId].play = 0;

	if (g_sounds[channelId].data) {
		free(g_sounds[channelId].data);
	}

	unsigned char * file = openFile(filename, &fileSize);
	if (fileSize < 1)
		return;

	g_sounds[channelId].data = sfx_read_aiff(file, &out_length);
	free(file);
	g_sounds[channelId].length = out_length;
	g_sounds[channelId].loop = 1;
	g_sounds[channelId].playback_pos = 0;
	g_sounds[channelId].volume = 1;
	g_sounds[channelId].pitch = 1;

	if (g_sound_counter < channelId + 1)
		g_sound_counter = channelId + 1;
}

void sfx_init_frontend() {
	sfx_clear_buffers();

	music_rnd++;
	if (music_rnd > 3) music_rnd = 0;

	sfx_load_audio_file(g_sfx_music_files[music_rnd]);

	sfx_load_audio_file("DriveData/aiff/Select.22K.mw.aiff");
	g_sounds[1].pitch = 0.5f;

	sfx_load_audio_file("DriveData/aiff/Select2.22K.mw.aiff");
	g_sounds[2].pitch = 0.5f;
}

void sfx_init_sim(int carId) {
	char filename[80];
	sfx_clear_buffers();

	//car sounds: 0-1 low revs; 2-3 high revs; 4 shifter
	sprintf((char*)&filename, "DriveData/aiff/%sSound.AIFFfam", (char*)g_sfx_car_files[carId]);
	sfx_load_audio_bank((char*)&filename);

	//horn sounds: 0 and 1 -> 5, 6
	sprintf((char*)&filename, "DriveData/aiff/%sHorn.AIFFfam", (char*)g_sfx_car_files[carId]);
	sfx_load_audio_bank((char*)&filename); //2

	//collisions file: 12 sounds pack
	sfx_load_audio_bank("DriveData/aiff/Collisions.AIFFfam");
	/*
		 7. 0 hard bump hit
		 8. 1 car on car crash
		 9. 2 soft bump hit
		10. 3 cop siren
		11. 4 unpaved road loop
		12. 5 cop detector
		13. 6 jump
		14. 7 car wreck
		15. 8 car rollover
		16. 9 traffic horn
		17. 10 tunnel loop
		18. 11 tire screeching
	*/

	// most sounds are 22Khz
	for (int i = 0; i < g_sound_counter; i++) {
		g_sounds[i].pitch = 0.5f;
	}
}

/*
 * sample mixer
 */
void sfx_mix_stream(int16_t *outStream, int outLen, sfx_assets * sound) {

	int *inStreamPos = &sound->playback_pos;

	int value = 0;
	int tS = *inStreamPos;
	int ti = 0;
	float tf = 0;

	int16_t *in = sound->data;
	in += *inStreamPos / 2;

	int16_t *out = (int16_t*) outStream;
	outLen /= 2;

	for (int i = 0; i < outLen; i++) {
		// stream end and looping
		if (*inStreamPos >= sound->length) {
			*inStreamPos = tS = tf = ti = 0;
			sound->playback_pos = 0;
			if (sound->loop) {
				in = (int16_t*) sound->data;
			} else {
				sound->play = 0;
				return;
			}
		}

		// pitch //FIXME not interpolated
		if (sound->pitch != 1.0f) {
			value = in[ti];
			tf += sound->pitch;
			ti = (int)tf;
			*inStreamPos = ti * 2 + tS;
		} else {
			value = in[i];
			*inStreamPos += 2;
		}

		// volume
		if (sound->volume < 1.0f) {
			value *= sound->volume;
		}

		// sound output mix (just add and clamp value)
		value += out[i];
		if (value > 0x7FFF) value = 0x7FFF;
		if (value < -0x7FFF) value = -0x7FFF;
		out[i] = value;
	}
}

void sfx_play_music(int id) {
	sfx_load_file_into_channel(g_sfx_music_files[id], 0);
	g_sounds[0].play = 1;
	g_sounds[0].loop = 1;
}

void sfx_play_speech_car(int carId) {
	char filename[80];
	sprintf((char*)&filename, "DriveData/aiff/%s.22k.mw.aif", (char*)g_sfx_speech_car_files[carId]);
	sfx_load_file_into_channel((char*)&filename, 3);
	g_sounds[3].play = 1;
	g_sounds[3].loop = 0;
	g_sounds[3].pitch = 0.5f;
}

void sfx_play_speech_track(int trackId) {
	char filename[80];
	sprintf((char*)&filename, "DriveData/aiff/%s.22k.mw.aif", (char*)g_sfx_speech_track_files[trackId]);
	sfx_load_file_into_channel((char*)&filename, 3);
	g_sounds[3].play = 1;
	g_sounds[3].loop = 0;
	g_sounds[3].pitch = 0.5f;
}

void sfx_play_sound(int id, char loop, float pitch, float volume) {
	g_sounds[id].volume = volume;
	g_sounds[id].pitch = pitch;
	g_sounds[id].play = volume > 0 ? 1 : 0;
	g_sounds[id].loop = loop;
}

void sfx_stop_sound(int id) {
	g_sounds[id].play = 0;
	g_sounds[id].playback_pos = 0;
	g_sounds[id].loop = 0;
}

/**
 * callback for SDL Audio system
 */
void sfx_sdl_audio_callback(void* userdata, int16_t* stream, int len) {
	sfx_assets * channel;
	memset(stream, 0, len);
	for (int i = 0; i < g_sound_counter; i++) {
		channel = &g_sounds[i];
		if (channel->play && channel->volume > 0) {
			sfx_mix_stream(stream, len, channel);
		}
	}
}
