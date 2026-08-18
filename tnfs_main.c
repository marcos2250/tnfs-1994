#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_audio.h>
#include <GL/gl.h>
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_files.h"
#include "tnfs_front.h"
#include "tnfs_gfx.h"
#include "tnfs_sfx.h"

static SDL_Event event;
static SDL_Window *window;
static SDL_GLContext glContext;
static SDL_AudioDeviceID audioDevice;

void sys_sdl_exit();
void tnfs_menu_pause();

int g_keybuffer[8];
int g_keybuffer_count = 0;
char isFrontEnd = 1;
char g_free_mode = 0;

/* Keyboard and Mouse inputs */

void keys_appendbuffer(int code) {
	if (g_keybuffer_count < 10 && code > 10) {
		g_keybuffer[g_keybuffer_count] = code;
		g_keybuffer_count++;
	}
}

int keys_getkey() {
	if (g_keybuffer_count > 0) {
		g_keybuffer_count--;
		return g_keybuffer[g_keybuffer_count];
	}
	return 0;
}

void handleKeys() {
	if (isFrontEnd) {
		if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
			keys_appendbuffer(event.key.keysym.sym);
		}
		return;
	}
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_LEFT:
			g_control_steer = -1;
			break;
		case SDLK_RIGHT:
			g_control_steer = 1;
			break;
		case SDLK_UP:
			g_control_throttle = 1;
			break;
		case SDLK_DOWN:
			g_control_brake = 1;
			break;
		case SDLK_SPACE:
			g_car_array[0].handbrake = 1;
			break;
		case SDLK_ESCAPE:
			tnfs_menu_pause();
			break;
		}
	}
	if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_a:
			tnfs_change_gear_up();
			break;
		case SDLK_z:
			tnfs_change_gear_down();
			break;
		case SDLK_r:
			tnfs_reset_car(g_car_ptr_array[0]);
			break;
		case SDLK_c:
			tnfs_change_camera();
			break;
		case SDLK_d:
			tnfs_crash_car();
			break;
		case SDLK_h:
			sfx_play_sound(5, 1, 0.25f, 1, 1);
			break;
		case SDLK_F9:
			tnfs_toggle_dash();
			break;
		case SDLK_F12:
			g_free_mode = g_free_mode ? 0 : 1;
			SDL_PauseAudioDevice(audioDevice, g_free_mode);
			break;
		default:
			break;
		}
	}
	if (event.type == SDL_KEYUP) {
		switch (event.key.keysym.sym) {
		case SDLK_UP:
			g_control_throttle = 0;
			break;
		case SDLK_DOWN:
			g_control_brake = 0;
			break;
		case SDLK_LEFT:
			g_control_steer = 0;
			break;
		case SDLK_RIGHT:
			g_control_steer = 0;
			break;
		case SDLK_SPACE:
			g_car_array[0].handbrake = 0;
			break;
		case SDLK_h:
			sfx_stop_sound(5);
			break;
		default:
			break;
		}
	}
}

void sys_sdl_exit() {
	file_clear_buffers();
	gfx_clear_buffers();
	sfx_clear_buffers();

	SDL_CloseAudioDevice(audioDevice);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	exit(0);
}

void sys_sdl_swapwindow() {
	SDL_GL_SwapWindow(window);
}

void gfx_update() {
	gfx_render_scene();
	SDL_GL_SwapWindow(window);
}

void renderGlFrontEnd() {
	glBindTexture(GL_TEXTURE_2D, 0);
	glPixelZoom(SCREEN_SCALE, SCREEN_SCALE);
	glDrawPixels(320, 240, GL_RGBA, GL_UNSIGNED_BYTE, &g_backbuffer);
	SDL_GL_SwapWindow(window);
}

void sys_sdl_loop_frontend() {
	renderGlFrontEnd();
	while (1) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			sys_sdl_exit();
		}
		handleKeys();
		if (event.type == SDL_KEYDOWN) {
			break;
		}
		SDL_Delay(30);
	}
}

void gfx_static_screen(char * file, char * label) {
	byte * data;
	shpm_image * image;
	int filesize;

	gfx_clear();
	data = openFileBuffer(file, &filesize);
	image = gfx_locateshape(data, label);
	gfx_draw_shpm(image, 0, 0);
	renderGlFrontEnd();
	SDL_Delay(1000);
}

void toggle_s(int *current, int max, int inc) {
	if (*current == 0 && inc < 0) {
		*current = max;
	} else {
		*current += inc;
		if (*current > max) {
			*current = 0;
		}
	}
}

void toggle(int *current, int max, int inc) {
	sfx_play_sound(2, 0, 0.5f, 1, 0);
	toggle_s(current, max, inc);
}

void tnfs_race_enter() {
	float a;

	/* Loading... */
	tnfs_ui_loading_screen(0);
	renderGlFrontEnd();

	gfx_clear_buffers();
	sfx_clear_buffers();
	tnfs_init_sim();
	sfx_init_sim(g_player_car);

	SDL_Delay(1000);
	tnfs_ui_loading_screen(1);
	SDL_Delay(1000);
	tnfs_ui_loading_screen(2);
	SDL_Delay(1000);

	SDL_PauseAudioDevice(audioDevice, 0);

	isFrontEnd = 0;
	g_quit_race = 0;

	/* game main loop */
	while(!g_quit_race) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				sys_sdl_exit();
			}
			handleKeys();
		}

		// free camera mode
		if (g_free_mode) {
			camera.orientation.x += (g_control_throttle - g_control_brake) * 0x10000;
			camera.orientation.y += g_control_steer * 0x10000;
			if (g_car_array[0].handbrake) {
				a = ((float) camera.orientation.y) / 2670179.113f;
				camera.position.x += sinf(a) * 0x10000;
				camera.position.z += cosf(a) * 0x10000;
				a = ((float) camera.orientation.x) / 2670179.113f;
				camera.position.y -= sinf(a) * 0x10000;
			}
			gfx_update();
			SDL_Delay(30);
			continue;
		}

		if (player_car_ptr->field_4c9 > 150) {
			if (player_car_ptr->track_slice < 0x10) {
				g_quit_race = 1; // player give up
			} else {
				g_quit_race = 0; // player finish track
			}
			break;
		}

		tnfs_update();
		gfx_update();
		SDL_Delay(30);
	}

	gfx_clear_buffers();
	sfx_clear_buffers();
	isFrontEnd = 1;
}

int tnfs_menu_credits() {
	CCB *bgnd;
	CCB *group;
	CCB *credits[3];
	CCB *display_bg;
	char filename[80];
	int time = 0;
	int crew = 0;
	int quit = 0;
	int scroll;
	int i;

	bgnd = LoadCel("frontend/display/credits/bgnd.cel", 0);
	//stamp = LoadCel("frontend/display/credits/stamp.cel", 0);
	group = LoadCel("frontend/display/credits/group.cel", 0);

	for (i = 0; i < 3; i++) {
		sprintf(filename, "frontend/display/credits/%d.cel", i + 1);
		credits[i] = LoadCel(filename, 0);
	}

	sfx_play_music(3);

	while(1) {
		if (time >= 2200 || quit) {
			break;
		}

		switch (keys_getkey()) {
		case SDLK_ESCAPE:
			quit = 1;
			break;
		case SDLK_SPACE:
			crew = crew ? 0 : 1;
			break;
		default:
			break;
		}
		while (SDL_PollEvent(&event)) {
			handleKeys();
		}

		scroll = 240 - time;
		if (crew) {
			display_bg = group;
		} else {
			display_bg = bgnd;
		}
		DrawScreenCels(0, display_bg);

		credits[0]->ccb_XPos = 35;
		credits[0]->ccb_YPos = scroll;
		DrawScreenCels(0, credits[0]);

		scroll += credits[0]->ccb_Height; //540;
		credits[1]->ccb_XPos = 35;
		credits[1]->ccb_YPos = scroll;
		DrawScreenCels(0, credits[1]);

		scroll += credits[1]->ccb_Height; //1310;
		credits[2]->ccb_XPos = 35;
		credits[2]->ccb_YPos = scroll;
		DrawScreenCels(0, credits[2]);

		DisplayScreen(0, 0);
		SDL_Delay(30);
		time++;
	}

	for (i = 0; i < 3; i++) {
		UnloadCel(credits[i]);
	}
	UnloadCel(bgnd);
	UnloadCel(group);
	//UnloadCel(stamp);
	return 1;
}

void tnfs_menu_pause() {
	int option = 0;
	isFrontEnd = 1;
	SDL_PauseAudioDevice(audioDevice, 1);
	while(1) {
		switch (keys_getkey()) {
		case SDLK_UP:
			toggle(&option, 3, -1);
			break;
		case SDLK_DOWN:
			toggle(&option, 3, +1);
			break;
		case SDLK_RETURN:
			if (option == 0) {
				SDL_PauseAudioDevice(audioDevice, 0);
				isFrontEnd = 0;
				return;
			}
			if (option == 3) {
				g_quit_race = 1;
				return;
			}
			break;
		case SDLK_ESCAPE:
			SDL_PauseAudioDevice(audioDevice, 0);
			isFrontEnd = 0;
			return;
		default:
			break;
		}
		tnfs_ui_pause(option);
		sys_sdl_loop_frontend();
	}
}

void tnfs_menu_wall() {
	while(1) {
		switch (keys_getkey()) {
		case SDLK_ESCAPE:
			return;
		case SDLK_SPACE:
			tnfs_menu_credits();
			break;
		default:
			break;
		}
		gfx_clear();
		tnfs_ui_wall(0);
		sys_sdl_loop_frontend();
	}
}

void tnfs_menu_checkopts() {
	int option = 0;
	while(1) {
		switch (keys_getkey()) {
		case SDLK_RIGHT:
			if (option == 0)
				toggle(&g_config.audio, 2, +1);
			if (option == 1)
				toggle(&g_config.audio_mode, 1, +1);
			if (option == 2)
				toggle(&g_config.opp_video, 1, +1);
			break;
		case SDLK_LEFT:
			if (option == 0)
				toggle(&g_config.audio, 2, -1);
			if (option == 1)
				toggle(&g_config.audio_mode, 1, -1);
			if (option == 2)
				toggle(&g_config.opp_video, 1, -1);
			 break;
		case SDLK_UP:
			toggle(&option, 2, -1);
			break;
		case SDLK_DOWN:
			toggle(&option, 2, +1);
			break;
		case SDLK_ESCAPE:
			return;
		default:
			break;
		}
		tnfs_ui_checkopts(option, &g_config);
		sys_sdl_loop_frontend();
	}
}

void tnfs_menu_finish() {
	int option = 0;
	while(1) {
		switch (keys_getkey()) {
		case SDLK_UP:
			toggle(&option, 4, -1);
			break;
		case SDLK_DOWN:
			toggle(&option, 4, +1);
			break;
		case SDLK_RETURN:
			if (option == 0)
				return;
			if (option == 3)
				tnfs_menu_checkopts();
			break;
		case SDLK_ESCAPE:
			return;
		default:
			break;
		}
		tnfs_ui_finish(option);
		sys_sdl_loop_frontend();
	}
}

int tnfs_menu_checkpoint() {
	int option = 0;
	while(1) {
		switch (keys_getkey()) {
		case SDLK_UP:
			toggle(&option, 5, -1);
			break;
		case SDLK_DOWN:
			toggle(&option, 5, +1);
			break;
		case SDLK_RETURN:
			if (option == 0)
				return 1;
			if (option == 3)
				tnfs_menu_checkopts();
			if (option == 5) {
				g_quit_race = 1;
				return 0;
			}
			break;
		case SDLK_ESCAPE:
			return 1;
		default:
			break;
		}
		tnfs_ui_checkpoint(option);
		sys_sdl_loop_frontend();
	}
	return 1;
}

void tnfs_menu_options() {
	int option = 0;
	gfx_clear();
	while(1) {
		switch (keys_getkey()) {
		case SDLK_RIGHT:
			if (option == 0)
				toggle(&g_config.skill_level, 5, +1);
			if (option == 1)
				toggle(&g_config.audio, 2, +1);
			if (option == 2)
				toggle(&g_config.audio_mode, 1, +1);
			if (option == 3)
				toggle(&g_config.opp_video, 1, +1);
			if (option == 4)
				toggle(&g_config.abs, 1, +1);
			if (option == 5)
				toggle(&g_config.tcs, 1, +1);
			if (option == 6)
				toggle(&g_config.control, 5, +1);
			break;
		case SDLK_LEFT:
			if (option == 0)
				toggle(&g_config.skill_level, 5, -1);
			if (option == 1)
				toggle(&g_config.audio, 2, -1);
			if (option == 2)
				toggle(&g_config.audio_mode, 1, -1);
			if (option == 3)
				toggle(&g_config.opp_video, 1, -1);
			if (option == 4)
				toggle(&g_config.abs, 1, -1);
			if (option == 5)
				toggle(&g_config.tcs, 1, -1);
			if (option == 6)
				toggle(&g_config.control, 5, -1);
			break;
		case SDLK_UP:
			toggle(&option, 7, -1);
			break;
		case SDLK_DOWN:
			toggle(&option, 7, +1);
			break;
		case SDLK_RETURN:
			if (option == 7)
				tnfs_menu_wall(0);
			break;	
		case SDLK_ESCAPE:
			return;
		default:
			break;
		}
		tnfs_ui_options(option, &g_config);
		sys_sdl_loop_frontend();
	}
}

void tnfs_menu_showcase() {
	int scroll = 0;
	sfx_play_speech_car(g_player_car);
	while(1) {
		switch (keys_getkey()) {
		case SDLK_RIGHT:
			toggle(&g_player_car, 7, +1);
			sfx_play_speech_car(g_player_car);
			break;
		case SDLK_LEFT:
			toggle(&g_player_car, 7, -1);
			sfx_play_speech_car(g_player_car);
			break;
		case SDLK_UP:
			scroll -= 0x10;
			break;
		case SDLK_DOWN:
			scroll += 0x10;
			break;
		case SDLK_ESCAPE:
			sfx_stop_sound(3);
			return;
		default:
			break;
		}
		gfx_clear();
		tnfs_ui_showcase(scroll, 0);
		sys_sdl_loop_frontend();
	}
}

// insert new record into an ordered array list
int tnfs_insert_record_score(int score) {
	int i, c, n;
	n = -1;
	c = 10;
	while (c--) {
		i = c;
		if (score > g_hiscores[i].score || g_hiscores[i].score == 0) {
			n = i;
			if (c < 9)
				g_hiscores[i + 1] = g_hiscores[i]; // shift records to the right
		} else {
			break;
		}
	}
	// place new record
	if (n != -1) {
		g_hiscores[n].track_id = g_track_sel;
		g_hiscores[n].car_id = g_player_car;
		g_hiscores[n].time = g_stats_data.route_time;
		g_hiscores[n].max_speed = g_stats_data.top_speed;
		g_hiscores[n].skill = g_config.skill_level;
		g_hiscores[n].score = score;
	}
	return n;
}

int tnfs_insert_record_time() {
	int i, c, n;
	n = -1;
	c = 3;
	while (c--) {
		i = (g_track_sel * 3) + c;
		if (g_stats_data.route_time < g_best_times[i].time || g_best_times[i].time == 0) {
			n = i;
			if (c < 2)
				g_best_times[i + 1] = g_best_times[i]; // shift records to the right
		} else {
			break;
		}
	}
	// place new record
	if (n != -1) {
		g_best_times[n].track_id = g_track_sel;
		g_best_times[n].car_id = g_player_car;
		g_best_times[n].time = g_stats_data.route_time;
		g_best_times[n].max_speed = g_stats_data.top_speed;
		g_best_times[n].skill = g_config.skill_level;
		g_best_times[n].score = 0;
	}
	return n;
}

int tnfs_insert_record_speed() {
	int result;
	if (g_stats_data.top_speed > g_best_times[g_track_sel * 3].max_speed) {
		result = 0; // 1st, best top speed
	} else {
		result = -1; // no speed record
	}
	return result;
}

char * g_input_lcase = "abcdefghijklmnopqrstuvwxyz0123456789'-.   ";
char * g_input_ucase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\"/()&*";
char g_username[12];

void tnfs_input_records() {
	int idTime = 0;
	int idScore = 0;
	int idSpeed = 0;
	int score = 0;
	int uppercase = 0;
	int cx = 0;
	int cy = 0;
	int len = 0;

	if (g_config.skill_level > 3) return; // invalid score
	if (g_quit_race) return; // player gave up

	// not sure if scores are calculated this way
	score = ( 40000 - g_stats_data.route_time )
			+ ( g_stats_data.cars_crashed * -3000 )
			+ ( g_stats_data.warning_count * -1000 )
			+ ( g_stats_data.penalty_count * -5000 );

	if (score <= 0) return;

	idTime = tnfs_insert_record_time();
	idScore = tnfs_insert_record_score(score);
	idSpeed = tnfs_insert_record_speed();

	if (idTime < 0 && idScore < 0) {
		return; //no record to save
	}

	memset(g_username, '\0', 12);

	while(1) {
		switch (keys_getkey()) {
		case SDLK_RIGHT:
			toggle(&cx, 6, +1);
			break;
		case SDLK_LEFT:
			toggle(&cx, 6, -1);
			break;
		case SDLK_UP:
			toggle(&cy, 5, -1);
			break;
		case SDLK_DOWN:
			toggle(&cy, 5, +1);
			break;
		case SDLK_a:
			uppercase = 1;
			break;
		case SDLK_z:
			uppercase = 0;
			break;
		case SDLK_SPACE:
			if (uppercase == 0 && cy == 5 && cx == 6) { //end
				g_username[len] = '\0';
				if (idTime >= 0) {
					strcpy(g_best_times[idTime].name, g_username);
				}
				if (idScore >= 0) {
					strcpy(g_hiscores[idScore].name, g_username);
				}
				file_highscores_write();
				return;
			} else if (uppercase == 0 && cy == 5 && cx == 5) { //backspace
				if (len > 0) len--;
			} else if (len < 10) {
				if (uppercase) {
					g_username[len] = g_input_ucase[cy * 7 + cx];
				} else {
					g_username[len] = g_input_lcase[cy * 7 + cx];
				}
				len++;
			}
			g_username[len] = '_';
			g_username[len+1] = '\0';
			break;
		case SDLK_ESCAPE:
			return;
		default:
			break;
		}
		gfx_clear();
		tnfs_ui_userinput(idTime, idScore, idSpeed, cx, cy, uppercase, (char*)&g_username);
		sys_sdl_loop_frontend();
	}
}

void tnfs_menu_route() {
	gfx_clear();
	sfx_play_speech_track(g_track_sel);
	while(1) {
		switch (keys_getkey()) {
		case SDLK_RIGHT:
			toggle(&g_track_sel, 3, +1);
			sfx_play_speech_track(g_track_sel);
			break;
		case SDLK_LEFT:
			toggle(&g_track_sel, 3, -1);
			sfx_play_speech_track(g_track_sel);
			break;
		case SDLK_UP:
			toggle(&g_track_segment, 2, +1);
			break;
		case SDLK_DOWN:
			toggle(&g_track_segment, 2, -1);
			break;
		case SDLK_ESCAPE:
			sfx_stop_sound(3);
			return;
		default:
			break;
		}
		tnfs_ui_route(g_track_sel, 0, 0, g_track_segment);
		sys_sdl_loop_frontend();
	}
}

void tnfs_menu_drive_start() {
	SDL_PauseAudioDevice(audioDevice, 1);

	g_stats_data.segment_time = 0;
	g_stats_data.route_time = 0;
	g_stats_data.top_speed = 0;
	g_stats_data.top_speed_2 = 0;
	g_stats_data.cars_crashed = 0;
	g_stats_data.cars_remaining = 2;
	g_stats_data.penalty_count = 0;
	g_stats_data.warning_count = 0;

	g_quit_race = 0;
	g_track_segment = 0;
	tnfs_race_enter();
	if (!g_quit_race) {
		g_stats_data.segment_time = iSimTimeClock;
		g_stats_data.route_time += g_stats_data.segment_time;
		tnfs_menu_checkpoint();
	}

	if (!g_quit_race) {
		g_track_segment = 1;
		tnfs_race_enter();
	}
	if (!g_quit_race) {
		g_stats_data.segment_time = iSimTimeClock;
		g_stats_data.route_time += g_stats_data.segment_time;
		tnfs_menu_checkpoint();
	}

	if (!g_quit_race) {
		g_track_segment = 2;
		tnfs_race_enter();
	}
	if (!g_quit_race) {
		g_stats_data.segment_time = iSimTimeClock;
		g_stats_data.route_time += g_stats_data.segment_time;
		tnfs_menu_finish();
		tnfs_input_records();
	}

	sfx_init_frontend();
	sfx_play_sound(0, 1, 1, 1, 0);
	SDL_PauseAudioDevice(audioDevice, 0);
}

int g_cc_menu_up[5]    = { 1, 0, 4, 1, 2 };
int g_cc_menu_down[5]  = { 1, 0, 4, 0, 2 };
int g_cc_menu_left[5]  = { 3, 2, 1, 4, 0 };
int g_cc_menu_right[5] = { 4, 2, 1, 0, 3 };

void tnfs_menu_control() {
	int option = 0;
	sfx_init_frontend();
	sfx_play_sound(0, 1, 1, 1, 0);
	SDL_PauseAudioDevice(audioDevice, 0);
	while(1) {
		switch (keys_getkey()) {
		case SDLK_UP:
			sfx_play_sound(2, 0, 0.5f, 1, 0);
			option = g_cc_menu_up[option];
			break;
		case SDLK_RIGHT:
			sfx_play_sound(2, 0, 0.5f, 1, 0);
			option = g_cc_menu_right[option];
			break;
		case SDLK_DOWN:
			sfx_play_sound(2, 0, 0.5f, 1, 0);
			option = g_cc_menu_down[option];
			break;
		case SDLK_LEFT:
			sfx_play_sound(2, 0, 0.5f, 1, 0);
			option = g_cc_menu_left[option];
			break;
		case SDLK_RETURN:
			sfx_play_sound(1, 0, 0.5f, 1, 0);
			if (option == 0)
				tnfs_menu_drive_start();
			if (option == 1)
				tnfs_menu_showcase();
			if (option == 2)
				tnfs_menu_route();
			if (option == 4)
				tnfs_menu_options();
			break;
		case SDLK_a:
			if (option == 1)
				toggle(&g_player_car, 7, -1);
			if (option == 2)
				toggle(&g_track_sel, 3, -1);
			if (option == 3)
				toggle(&g_opp_car, 8, -1);
			break;
		case SDLK_z:
		case SDLK_SPACE: 
			if (option == 1)
				toggle(&g_player_car, 7, 1);
			if (option == 2)
				toggle(&g_track_sel, 3, 1);
			if (option == 3)
				toggle(&g_opp_car, 8, 1);
			break;
		default:
			break;
		}
		tnfs_ui_control(option);
		sys_sdl_loop_frontend();
	}
}

void tnfs_init_config() {
	int i;

    g_config.audio = 0;
    g_config.audio_mode = 0;
    g_config.abs = 1;
    g_config.tcs = 1;
    g_config.skill_level = 1;
    g_config.opp_video = 1;
    g_config.control = 0;

	for (i = 0; i < 10; i++) {
		g_hiscores[i].id = i;
		g_hiscores[i].name[0] = 0;
		g_hiscores[i].car_id = 0;
		g_hiscores[i].track_id = 0;
		g_hiscores[i].score = 0;
		g_hiscores[i].skill = 0;
		g_hiscores[i].time = 0;
		g_hiscores[i].max_speed = 0;
	}
	for (i = 0; i < 12; i++) {
		g_best_times[i].id = i;
		g_best_times[i].name[0] = 0;
		g_best_times[i].car_id = 0;
		g_best_times[i].track_id = 0;
		g_best_times[i].score = 0;
		g_best_times[i].skill = 0;
		g_best_times[i].time = 0;
		g_best_times[i].max_speed = 0;
	}

	strcpy(g_best_times[0].name, "Daredevil");
	g_best_times[0].time = 26619;
	g_best_times[0].max_speed = 5103575; //174,2 mph
	g_best_times[0].car_id = 4;
	g_best_times[0].skill = 2;
	strcpy(g_best_times[3].name, "Daredevil");
	g_best_times[3].time = 24498;
	g_best_times[3].max_speed = 4142626; //141,4 mph
	g_best_times[3].car_id = 4;
	g_best_times[3].skill = 2;
	strcpy(g_best_times[6].name, "Daredevil");
	g_best_times[6].time = 20112;
	g_best_times[6].max_speed = 5481509; //187,1 mph
	g_best_times[6].car_id = 4;
	g_best_times[6].skill = 2;
	strcpy(g_best_times[9].name, "Daredevil");
	g_best_times[9].time = 20000;
	g_best_times[9].max_speed = 6000000;
	g_best_times[9].car_id = 4;
	g_best_times[9].skill = 2;

	strcpy(g_hiscores[0].name, "Daredevil");
	g_hiscores[0].score = 19490;
	g_hiscores[0].car_id = 4;
	g_hiscores[0].skill = 2;
	strcpy(g_hiscores[1].name, "Daredevil");
	g_hiscores[1].score = 18741;
	g_hiscores[1].car_id = 2;
	g_hiscores[1].skill = 2;
	strcpy(g_hiscores[2].name, "Daredevil");
	g_hiscores[2].score = 18469;
	g_hiscores[2].car_id = 4;
	g_hiscores[2].skill = 2;

	file_highscores_read();
}

void tnfs_game_main() {
	gfx_static_screen("frontend/display/3do.3sh", "3do ");
	//gfx_static_screen("frontend/display/EALOGO.3SH", "eal2");
	//gfx_static_screen("frontend/display/pioneer.3sh", "shot");
	gfx_static_screen("frontend/display/TITLE.3SH", "ndtl");
	tnfs_menu_control();
}

/***** File Viewer *****/
int objectIds[512];
int texCount = 0;
int objectSel = 0;
byte * fileView_data = 0;
byte testpath[] = {3, 4};
int g_wpath_result[8];
char g_fv_msg[80];

char * g_viewer_files[] = {
			"frontend/display/pioneer.3sh", //shpm linear
			"frontend/display/TITLE.3SH", //shpm packed
			"frontend/display/ctrlcars.3sh", //shpm packed
			"frontend/display/options.3sh", //
			"frontend/display/credits/group.cel", //ccb packed
			"frontend/display/credits/1.cel", //ccb packed palette
			"DriveData/CarData/LDIABLO.s1", //ccb linear
			"DriveData/CarData/MRX7.WrapFam", //shpm linear palette
			"DriveData/CarData/ANSX.WrapFam",
			"DriveData/CarData/CopMust.WrapFam",
			"DriveData/CarData/CZR1.BigdashFam",
			"DriveData/DriveArt/SimCommonArt.Fam", //ccb packed palette no shade
			"DriveData/DriveArt/warningt0.celFam", //ccb packed palette shade
			"DriveData/DriveArt/Al1_PKT_000",
			"DriveData/DriveArt/Al2_PKT_000",
			"DriveData/DriveArt/Al3_PKT_000",
			"DriveData/DriveArt/Cy1_PKT_000",
			"DriveData/DriveArt/Cy2_PKT_000",
			"DriveData/DriveArt/Cy3_PKT_000",
			"DriveData/DriveArt/Cl1_PKT_000",
			"DriveData/DriveArt/Cl2_PKT_000",
			"DriveData/DriveArt/Cl3_PKT_000"
		};

void fileView_scan_file(int id) {
	int size = 0;
	int pos = 0;
	byte * obj = 0;
	int numShapes = 0;
	int j;

	if (fileView_data != 0) {
		free(fileView_data);
	}
	fileView_data = openFile(g_viewer_files[id], &size);
	if (fileView_data == 0) {
		printf("File error %s\n", g_viewer_files[id]);
		return;
	}
	printf("Scanning file with %d bytes for images...\n", size);
	texCount = 0;
	while (1) {
		obj = fileView_data + pos;
		if (obj[0] == 'C' && obj[1] == 'C' && obj[2] == 'B' && obj[3] == ' ') {
			objectIds[texCount] = pos;
			texCount++;
		}
		if (obj[0] == 'S' && obj[1] == 'H' && obj[2] == 'P' && obj[3] == 'M') {
			numShapes = obj[11] + (obj[10] << 8);
			obj += 0x10;

			for (j = 0; j < numShapes; ++j) {
				objectIds[texCount] = obj[7] + (obj[6] << 8) + (obj[5] << 16) + (obj[4] << 24) + pos;
				texCount++;
				obj += 8;
			}
		}
		pos++;
		if (pos > size) break;
	}

	pos = 0;
	if (texCount == 0) {
		printf("no objects found!\n");
		return;
	}
	printf("found %d objects.\n", texCount);
	objectSel = 0;
	fileView_drawImage(fileView_data, objectIds[0]);
	gfx_draw_text_9500(g_viewer_files[id], 10, 10);
}

void fileView_seekImage(int * pos, int direction) {
	byte * seek;
	objectSel += direction;
	if (objectSel < 0) objectSel = 0;
	if (objectSel >= texCount) objectSel = texCount - 1;
	*pos = objectIds[objectSel];
	seek = fileView_data + objectIds[objectSel];
	if (fileView_data[0] == 'w') {
		locate_wwww(fileView_data, seek, 0, (int*)&g_wpath_result);
	}
}

void fileView_printData() {
	int i = 0;
	char auxstr[80];
	char text[80] = "wwww ";
	if (fileView_data[0] == 'w') {
		while (g_wpath_result[i] >= 0) {
			sprintf(auxstr, " / %d ", g_wpath_result[i]);
			strcat(text, auxstr);
			i++;
		}
		gfx_draw_text_9500((char*)&text, 10, 10);
	}
}

void fileView_dumpFile() {
	int size = objectIds[objectSel + 1] - objectIds[objectSel];
	if (size > 0 && size < 0x100000) {
		fileWrite(fileView_data + objectIds[objectSel], size);
	} else {
		printf("Error - can't write file!\n");
	}
}

void fileView_sfx_screen(int id) {
	char text[80];
	gfx_clear();
	sprintf((char*)&text, "Sound bank: %d", id);
	gfx_draw_text_9500(text, 10, 20);
	gfx_draw_text_9500("Up/Dn:change Space:play Esc:back", 10, 210);
}

void fileViewer_main() {
	int pos = 0;
	int id = 0;
	int fileView_count = sizeof(g_viewer_files);
	fileView_scan_file(id);

	gfx_draw_text_9500("PgUp/PgDn:chg.files L/R/Up/Dn:seek Esc:back", 10, 210);

	isFrontEnd = 1;
	while(1)  {
		switch (keys_getkey()) {
		case SDLK_PAGEUP:
			toggle_s(&id, fileView_count, -1);
			fileView_scan_file(id);
			break;
		case SDLK_PAGEDOWN:
			toggle_s(&id, fileView_count, +1);
			fileView_scan_file(id);
			break;
		case SDLK_RIGHT:
			fileView_seekImage(&pos, +1);
			fileView_drawImage(fileView_data, pos);
			fileView_printData();
			break;
		case SDLK_LEFT:
			fileView_seekImage(&pos, -1);
			fileView_drawImage(fileView_data, pos);
			fileView_printData();
			break;
		case SDLK_UP:
			fileView_seekImage(&pos, -20);
			fileView_drawImage(fileView_data, pos);
			fileView_printData();
			break;
		case SDLK_DOWN:
			fileView_seekImage(&pos, +20);
			fileView_drawImage(fileView_data, pos);
			fileView_printData();
			break;
		case SDLK_d:
			fileView_dumpFile();
			break;
		case SDLK_ESCAPE:
			return;
			break;
		default:
			break;
		}
		sys_sdl_loop_frontend();
	}
}

void audioPlayer_main() {
	int id = 0;
	int fileView_count = 17;
	gfx_clear();
	sfx_init_sim(0);
	SDL_PauseAudioDevice(audioDevice, 0);
	fileView_sfx_screen(id);

	isFrontEnd = 1;
	while(1)  {
		switch (keys_getkey()) {
		case SDLK_UP:
			toggle_s(&id, fileView_count, -1);
			fileView_sfx_screen(id);
			break;
		case SDLK_DOWN:
			toggle_s(&id, fileView_count, +1);
			fileView_sfx_screen(id);
			break;
		case SDLK_SPACE:
			sfx_play_sound(id, 0, 0.25f, 1, 0);
			break;
		case SDLK_ESCAPE:
			return;
			break;
		default:
			break;
		}
		sys_sdl_loop_frontend();
	}
}

void initial_menu() {
	int option = 0;
	while(1) {
		switch (keys_getkey()) {
		case SDLK_UP:
			toggle_s(&option, 2, -1);
			break;
		case SDLK_DOWN:
			toggle_s(&option, 2, +1);
			break;
		case SDLK_RETURN:
			if (option == 0)
				tnfs_game_main();
			if (option == 1)
				fileViewer_main();
			if (option == 2)
				audioPlayer_main();
			break;
		default:
			break;
		}
		gfx_clear();
		tnfs_ui_initial(option);
		sys_sdl_loop_frontend();
	}
}

int main(int argc, char **argv) {
    SDL_AudioSpec desiredSpec, obtainedSpec;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		printf("SDL could not be initialized! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}

#if defined linux && SDL_VERSION_ATLEAST(2, 0, 8)
	if (!SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0")) {
		printf("SDL can not disable compositor bypass!\n");
		return 0;
	}
#endif

	window = SDL_CreateWindow("SDL Window", //
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, //
			SCREEN_WIDTH, SCREEN_HEIGHT,
			SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (!window) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}

	glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		printf("GL Context could not be created! SDL_Error: %s\n", SDL_GetError());
	}

	desiredSpec.freq = 44100;
	desiredSpec.format = AUDIO_S16SYS; // Signed 16-bit audio, system endian
	desiredSpec.channels = 2; // 1 Mono/2 Stereo
	desiredSpec.samples = 0; // Buffer size (power of 2)
	desiredSpec.callback = (void*) sfx_sdl_audio_callback;
	desiredSpec.userdata = NULL;

	audioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &obtainedSpec, 0);
	if (audioDevice == 0) {
		printf("Audio device could not be created! SDL_Error: %s\n", SDL_GetError());
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glClearColor(1.f, 1.f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glAlphaFunc(GL_GREATER, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glColor3f(0.0f, 0.0f, 0.0f);

	gfx_set_display_callback(sys_sdl_swapwindow);

	if (gfx_init_stuff()) {
		tnfs_init_config();
		initial_menu();
	}

	sys_sdl_exit();
	return 0;
}
