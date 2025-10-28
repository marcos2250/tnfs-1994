#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_audio.h>
#include <GL/gl.h>
#include <GL/glu.h>
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
char quit = 0;
char isFrontEnd = 1;

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
			sfx_play_sound(5, 1, 0.25f, 1);
			break;
		case SDLK_F1:
			tnfs_abs();
			break;
		case SDLK_F2:
			tnfs_tcs();
			break;
		case SDLK_F3:
			tnfs_change_traction();
			break;
		case SDLK_F4:
			tnfs_change_transmission_type();
			break;
		case SDLK_F5:
			tnfs_cheat_mode();
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

/* System events */

void sys_sdl_exit() {
	clearFileBuffer();
	gfx_clear_buffers();
	sfx_clear_buffers();

	SDL_CloseAudioDevice(audioDevice);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	exit(0);
}

/* Sim Mode */
void gfx_update() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, 1.38, 0.1, 1000);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gfx_render_scene();
	SDL_GL_SwapWindow(window);
}

void renderGlFrontEnd() {
	glPixelZoom(2.5, 2.5);
	glDrawPixels(320, 240, GL_RGBA, GL_UNSIGNED_BYTE, &g_backbuffer);
	SDL_GL_SwapWindow(window);
}

void sys_sdl_loop_frontend() {
	quit = 0;
	renderGlFrontEnd();
	while (1) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			quit = 1;
			sys_sdl_exit();
		}
		handleKeys();
		if (event.type == SDL_KEYDOWN) {
			break;
		}
		SDL_Delay(30);
	}
}

void tnfs_race_enter() {

	gfx_clear_buffers();
	sfx_clear_buffers();
	tnfs_init_sim();
	isFrontEnd = 0;
	quit = 0;

	sfx_init_sim(g_player_car);
	SDL_PauseAudioDevice(audioDevice, 0);

	/* game main loop */
	while(!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = 1;
				sys_sdl_exit();
			}
			handleKeys();
		}

		if (player_car_ptr->field_4c9 > 150) {
			if (player_car_ptr->track_slice < 0x10) {
				quit = 1; // player give up
			} else {
				quit = 0; // player finish track
			}
			break;
		}

		if (g_police_ticket_time) {
			g_police_ticket_time--;
			tnfs_ui_cop_ticket(g_police_speeding_ticket);
			renderGlFrontEnd();
			if (g_police_ticket_time == 0) {
				//just reset player car
				tnfs_reset_car(player_car_ptr);
			}
		} else {
			tnfs_update();
			gfx_update();
		}

		SDL_Delay(30);
	}

	gfx_clear_buffers();
	sfx_clear_buffers();
	isFrontEnd = 1;
}

void gfx_static_screen(char * file, char * label) { 
	gfx_clear();
	gfx_draw_3sh(file, label);
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
	sfx_play_sound(2, 0, 0.5f, 1);
	toggle_s(current, max, inc);
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
				quit = 1;
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

void tnfs_menu_credits() {
	int time = 0;
	int crew = 0;

	sfx_play_music(3);

	while(1) {
		if (time == 0x897) {
			return;
		}
		time++;

		switch (keys_getkey()) {
		case SDLK_ESCAPE:
			return;
		case SDLK_SPACE:
			toggle(&crew, 1, 1);
			break;
		default:
			break;
		}
		tnfs_ui_credits(time, crew);

		while (SDL_PollEvent(&event)) {
			handleKeys();
		}
		renderGlFrontEnd();
		SDL_Delay(30);
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
				quit = 1;
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

void tnfs_loading_screen() {
	int i;
	for (i = 0; i < 3; i++) {
		tnfs_ui_loading_screen(i);
		renderGlFrontEnd();
		SDL_Delay(1000);
	}
}

void tnfs_menu_drive_start() {
	quit = 0;
	SDL_PauseAudioDevice(audioDevice, 1);

	g_track_segment = 0;
	tnfs_loading_screen();
	tnfs_race_enter();
	if (!quit) {
		tnfs_menu_checkpoint();
	}

	if (!quit) {
		g_track_segment = 1;
		tnfs_loading_screen();
		tnfs_race_enter();
	}
	if (!quit) {
		tnfs_menu_checkpoint();
	}

	if (!quit) {
		g_track_segment = 2;
		tnfs_loading_screen();
		tnfs_race_enter();
	}
	if (!quit) {
		tnfs_menu_finish();
	}

	sfx_init_frontend();
	sfx_play_sound(0, 1, 1, 1);
	SDL_PauseAudioDevice(audioDevice, 0);
}

void tnfs_menu_control() {
	int option = 0;
	sfx_init_frontend();
	sfx_play_sound(0, 1, 1, 1);
	SDL_PauseAudioDevice(audioDevice, 0);
	while(1) {
		switch (keys_getkey()) {
		case SDLK_UP:
			toggle(&option, 4, +1);
			break;
		case SDLK_RIGHT:
			toggle(&option, 4, +1);
			break;
		case SDLK_DOWN:
			toggle(&option, 4, -1);
			break;
		case SDLK_LEFT:
			toggle(&option, 4, -1);
			break;
		case SDLK_RETURN:
			sfx_play_sound(1, 0, 1, 1);
			if (option == 0)
				tnfs_menu_drive_start();
			if (option == 1)
				tnfs_menu_showcase();
			if (option == 2)
				tnfs_menu_route();
			if (option == 4)
				tnfs_menu_options();
			break;
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
		g_game_stats[i].id = i;
		strcpy(g_game_stats[i].name, "Racer");
		g_game_stats[i].car_id = (i * 3) & 4;
		g_game_stats[i].track_id = (i * 7) & 2;
		g_game_stats[i].score = 1000000 - (i * 7337);
		g_game_stats[i].skill = i & 2;
	}

	for (i = 0; i < 4; i++) {
		g_track_stats[i].id = i;
		strcpy(g_track_stats[i].name, "Racer");
		g_track_stats[i].car_id = (i * 3) & 4;
		g_track_stats[i].time = 1000000 - (i * 8239);
		g_track_stats[i].skill = i & 2;
		g_track_stats[i].max_speed = 240 - (i * 18);
	}
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
char g_wpath_result[12];
char g_fv_msg[80];

char * g_files[] = {
			"frontend/display/pioneer.3sh", //shpm linear
			"frontend/display/TITLE.3SH", //shpm packed
			"frontend/display/ctrlcars.3sh", //shpm packed
			"frontend/display/credits/group.cel", //ccb packed
			"frontend/display/credits/1.cel", //ccb packed palette
			"DriveData/CarData/LDIABLO.s1", //ccb linear
			"DriveData/CarData/TSupra.WrapFam", //shpm linear palette
			"DriveData/CarData/CopMust.WrapFam",
			"DriveData/CarData/MRX7.BigdashFam",
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
	fileView_data = openFile(g_files[id], &size);
	if (fileView_data == 0) {
		printf("File error %s\n", g_files[id]);
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
			gfx_set_filedata(obj);
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
}

void fileView_seekImage(int * pos, int direction) {
	objectSel += direction;
	if (objectSel < 0) objectSel = 0;
	if (objectSel >= texCount) objectSel = texCount - 1;
	*pos = objectIds[objectSel];
	if (fileView_data[0] == 'w') {
		locate_wwww(fileView_data, (fileView_data + *pos), 0, g_wpath_result);
	}
}

void fileView_printData() {
	if (fileView_data[0] == 'w') {
		gfx_draw_text_9500(g_wpath_result, 10, 10);
		g_wpath_result[0] = 0;
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
	int id = 11;
	int fileView_count = 20;
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
			sfx_play_sound(id, 0, 0.25f, 1);
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

    SDL_AudioSpec desiredSpec, obtainedSpec;
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

	if (!gfx_init_stuff()) {
		sys_sdl_exit();
		return 0;
	}

	tnfs_init_config();

	initial_menu();

	sys_sdl_exit();
	return 0;
}
