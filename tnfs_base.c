/*
 * globals, structs, and common TNFS functions
 */
#include <stdio.h>
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_files.h"
#include "tnfs_fiziks.h"
#include "tnfs_collision_2d.h"
#include "tnfs_collision_3d.h"
#include "tnfs_ai.h"
#include "tnfs_camera.h"
#include "tnfs_front.h"
#include "tnfs_sfx.h"

const int g_gravity_const = 0x9CF5C;

struct tnfs_config g_config;
struct tnfs_game_stats g_game_stats[10];
struct tnfs_track_stats g_track_stats[4];

int g_player_car = 0;
int g_opp_car = 0;
int g_track_sel = 0;
int g_track_segment = 0;
int g_race_status = 0;

tnfs_track_data track_data[2400];
tnfs_surface_type road_surface_type_array[6];
tnfs_track_speed g_track_speed[600];

tnfs_car_specs car_specs;
tnfs_car_data g_car_array[8];
tnfs_car_data *g_car_ptr_array[8]; // 00153ba0/00153bec 8010c720/800f7e60
tnfs_car_data *player_car_ptr;
tnfs_car_data *g_cop_car_ptr = 0;
tnfs_ai_skill_cfg g_ai_skill_cfg;
tnfs_stats_data g_stats_data;

int g_total_cars_in_scene = 7;
int g_racer_cars_in_scene = 2; // (including player) 001670AB DAT_8010d1c8
int g_number_of_players = 1; //001670af 8010d1cc
int g_number_of_cops = 1; //001670B3 8010d1d0
int g_number_of_traffic_cars = 4; //001670BB

// settings/flags
char is_drifting;
int iSimTimeClock = 0;
int g_road_node_count = 0;
int g_road_finish_node = 0;
int sound_flag = 0;
int g_selected_cheat = 0;
int cheat_crashing_cars = 0;
int g_game_settings = 0;
char g_control_throttle;
char g_control_brake;
signed char g_control_steer;
int g_police_on_chase = 0; //000fdb90
int g_police_speeding_ticket = 0; //0016513C
int g_police_chase_time = 0; //0016533c
int g_police_ticket_time = 0;

tnfs_camera camera;
int selected_camera = 0;

int g_race_positions[8] = { 7, 6, 5, 4, 3, 2, 1, 0 }; // 00167179

char * g_track_files[4] = { "al", "cl", "cy", "test" };
char * g_Track_files[4] = { "Al", "Cl", "Cy", "Al" };

char * g_car_files[] = { "TSupra", "LDiablo", "P911", "CZR1", "F512TR", "DVIPER", "ANSX", "MRX7" }; //valid spec files
                       // "Porsche911", "TRUCK1", "FERRARI512TR", "CAR1" //older versions spec files

char * g_car_wrapfams[] = {
		"TSupra", "LDiablo", "P911", "CZR1", "F512TR", "DVIPER", "ANSX", "MRX7", //featured cars
		"CopMust", //"CopCamaro", "CopCaprice", "CopVic",
		"Wagon", "CRX", "Vandura", "BMW", "SunBird", "Jetta", "RODEO", "axxess", "PROBE", "Jeep",
		"Lemans", "Pickup", "SASCO", "PRELUDE", "GMCTRUCK", "Probe94", "Scooter", "Porsche" };

char * g_ai_tddyn[] = {
		"TSupra", "LDiablo", "P911", "CZR1", "F512TR", "DVIPER", "ANSX", "MRX7", //featured cars
		"copMust",
		"Wagon", "crx", "Vandura", "BMW", "SunBird", "Jetta", "RODEO", "axxess", "Probe", "Jeep",
		"lemans", "Pickup", "Sasco", "PRELUDE", "GMCTRUCK", "PROBE94", "CAR1", "CAR1" }; //"Inviso"?


// car models
struct tnfs_carmodel3d g_carmodels[32];
int g_carmodels_count = 0;
tnfs_vec3 g_shadow_points[4];
tnfs_vec9 g_shadow_matrix;

// track scenery
float g_terrain[99000];
char g_terrain_texId[6000];
int g_terrain_texPkt[256];
int g_horizon_texPkt[12];
int g_fences[600];

struct tnfs_object3d g_scenery_3d_objects[8];
struct tnfs_scenery_descriptor g_scenery_models[64];
struct tnfs_scenery_object g_scenery_object[1000];
int g_scenery_texPkt[256];
int g_scenery_models_count = 0;
int g_scenery_objects = 0;

// hud
int g_hud_texPkt[15];

// smoke
int g_smoke_texPkt[5];
int g_smoke_delay;
struct tnfs_smoke_puff g_smoke[SMOKE_PUFFS];

int DAT_800eb6a4 = 0; //800eb6a4
int DAT_8010d310 = 0; //8010d310

int DAT_000F9BB0 = 0;
int DAT_000f99e4 = 0x10000;
int DAT_000f99e8 = 0x34000;
int DAT_000f99ec = 10; //800eae14
int DAT_000f99f0 = 0x8000;
int DAT_000f9a74 = 0;
int DAT_000fae60 = 0;
int DAT_000FDB94 = 0;
int DAT_000FDCEC = 0;
int DAT_000FDCF0 = 0;
int DAT_000f9A70 = 0;
int g_lcg_random_mod = 0xFFFF; //800db6bc
int g_lcg_random_nbr = 0x12345678; //800db6c0 g_car_random_index
int g_lcg_random_seed = 0x12345679; // random value
int g_camera_node = 0; //144914
int g_slice_mask = 0xFFFF; // 14dccc
int g_track_slice = 0x76b; //00153b0c
int DAT_00153B20 = 0; // game over flag
int DAT_00153B24 = 0; // game over flag 2?
tnfs_car_data * DAT_00153BC4 = 0; //player car ptr 2
int DAT_00165148 = 0; // center lane distance/margin
int DAT_00165340 = 0;
int g_player_id = 0; //16707C
int g_cam_change_delay = 0; // 00143844


void setTerrainVertex(vector3f * vec, int chunk, int slice, int point) {
	int x = (chunk * 55) + (slice * 33) + (point * 3);
	g_terrain[x] = vec->x;
	g_terrain[x+1] = vec->y;
	g_terrain[x+2] = vec->z;
}

int defaultTexId[5] = { 3, 4, 18, 19, 20 };
int laneWidth[5] = { 10, 10, 20, 40, 100 };

void auto_generate_track() {
	int pos_x = 0;
	int pos_y = 0;
	int pos_z = 0;
	int slope = 0;
	int slant = 0;
	int heading = 0;
	int i, j, k, v;
	int texCount = 0;
	long vx, vy, vz;

	g_road_node_count = 2395;
	g_road_finish_node = g_road_node_count - 0xb5;
	v = 0;

	for (i = 0; i < 2400; i++) {

		if (i > 20 && i % 20 == 0) {
			g_lcg_random_nbr = g_lcg_random_mod * g_lcg_random_seed;
			g_lcg_random_mod = g_lcg_random_nbr & 0xffff;
		}

		if (g_lcg_random_nbr & 128) {
			if (g_lcg_random_nbr & 64) {
				slope -= 10;
			} else {
				slope += 10;
			}
		} else {
			slope *= 0.9;
		}
		if (g_lcg_random_nbr & 32) {
			if (g_lcg_random_nbr & 16) {
				slant -= 20;
			} else {
				slant += 20;
			}
		} else {
			slant *= 0.9;
		}

		if (slope > 0x3FF) slope = 0x3FF;
		if (slope < -0x3FF) slope = -0x3FF;
		if (slant > 0x3FF) slant = 0x3FF;
		if (slant < -0x3FF) slant = -0x3FF;
		if (heading > 0xFFF) slant *= 0.9;
		if (heading < -0xFFF) slant *= 0.9;

		track_data[i].roadLeftFence = 0x50;
		track_data[i].roadRightFence = 0x50;
		track_data[i].roadLeftMargin = 0x25;
		track_data[i].roadRightMargin = 0x25;
		track_data[i].num_lanes = 0x11;
		track_data[i].fence_flag = 0;
		track_data[i].shoulder_surface_type = 0x22;
		track_data[i].item_mode = 0x3;

		track_data[i].slope = slope;
		track_data[i].heading = heading;
		track_data[i].slant = slant;
		track_data[i].pos.x = pos_x;
		track_data[i].pos.y = pos_y;
		track_data[i].pos.z = pos_z;

		track_data[i].side_normal_x = (short)(math_cos_3(track_data[i].heading * -0x400) / 2);
		track_data[i].side_normal_y = (short)(math_tan_3(track_data[i].slant * -0x400) / 2);
		track_data[i].side_normal_z = (short)(math_sin_3(track_data[i].heading * -0x400) / 2);

		// generate terrain mesh
		for (k = 0; k < 2; k++) {
			vx = pos_x;
			vy = pos_y;
			vz = pos_z;
			for (j = 0; j < 6; j++) {
				g_terrain[v++] = ((float) vx) / 0x10000;
				g_terrain[v++] = ((float) vy) / 0x10000;
				g_terrain[v++] = ((float) vz) / 0x10000;
				vx += track_data[i].side_normal_x * laneWidth[j % 5];
				vy += track_data[i].side_normal_y * laneWidth[j % 5];
				vz += track_data[i].side_normal_z * laneWidth[j % 5];
			}
			vx = pos_x;
			vy = pos_y;
			vz = pos_z;
			for (j = 6; j < 11; j++) {
				vx -= track_data[i].side_normal_x * laneWidth[j % 5];
				vy -= track_data[i].side_normal_y * laneWidth[j % 5];
				vz -= track_data[i].side_normal_z * laneWidth[j % 5];
				g_terrain[v++] = ((float) vx) / 0x10000;
				g_terrain[v++] = ((float) vy) / 0x10000;
				g_terrain[v++] = ((float) vz) / 0x10000;
			}
			if (i == 0 || (i % 4) != 0) {
				break;
			}
		}
		for (j = 0; j < 10; j++) {
			g_terrain_texId[texCount++] = defaultTexId[j % 5];
		}

		// next node
		pos_x += fixmul(math_sin_3(track_data[i].heading * 0x400), 0x80000);
		pos_y += fixmul(math_tan_3(track_data[i].slope * 0x400), 0x80000);
		pos_z += fixmul(math_cos_3(track_data[i].heading * 0x400), 0x80000);
		heading += slant >> 3;
	}

	for (i = 0; i < 600; i++) {
		// track section speed
		g_track_speed[i].top_speed = 0x42;
		g_track_speed[i].legal_speed = 0x1b;
		g_track_speed[i].safe_speed = 0x2c;
		//fence
		g_fences[i] = 0xC0 | 45;
	}
}

void tnfs_init_track(char *tri_file) {
	int i;
	int heading, s, c, t, dL, dR;
	int chunk, split, slice;
	vector3f *sliceptr;

	// try to read a TRI file if given, if not, generate a random track
	if (!read_tri_file(tri_file)) {
		auto_generate_track();
	}

	// invert Z axis
	for (i = 2; i < 99000; i+=3) {
		g_terrain[i] *= -1;
	}

	// model track for rendering
	for (i = 0; i < g_road_node_count; i++) {
		heading = track_data[i].heading * -0x400;
		s = math_sin_3(heading);
		c = math_cos_3(heading);
		t = math_tan_3(track_data[i].slant * -0x400);

		dL = (int)(track_data[i].roadLeftMargin) * -0x2000;
		dR = (int)track_data[i].roadRightMargin * 0x2000;

		track_data[i].vf_margin_L.x = (float) (track_data[i].pos.x + fixmul(c, dL)) / 0x10000;
		track_data[i].vf_margin_L.y = (float) (track_data[i].pos.y + fixmul(t, dL)) / 0x10000;
		track_data[i].vf_margin_L.z = -(float) (track_data[i].pos.z + fixmul(s, dL)) / 0x10000;
		track_data[i].vf_margin_R.x = (float) (track_data[i].pos.x + fixmul(c, dR)) / 0x10000;
		track_data[i].vf_margin_R.y = (float) (track_data[i].pos.y + fixmul(t, dR)) / 0x10000;
		track_data[i].vf_margin_R.z = -(float) (track_data[i].pos.z + fixmul(s, dR)) / 0x10000;

		dL = (int)(track_data[i].roadLeftFence) * -0x2000;
		dR = (int)(track_data[i].roadRightFence) * 0x2000;

		track_data[i].vf_fence_L.x = (float) (track_data[i].pos.x + fixmul(c, dL)) / 0x10000;
		track_data[i].vf_fence_L.y = (float) (track_data[i].pos.y + fixmul(t, dL)) / 0x10000;
		track_data[i].vf_fence_L.z = -(float) (track_data[i].pos.z + fixmul(s, dL)) / 0x10000;
		track_data[i].vf_fence_R.x = (float) (track_data[i].pos.x + fixmul(c, dR)) / 0x10000;
		track_data[i].vf_fence_R.y = (float) (track_data[i].pos.y + fixmul(t, dR)) / 0x10000;
		track_data[i].vf_fence_R.z = -(float) (track_data[i].pos.z + fixmul(s, dR)) / 0x10000;
	}

	// swap some points for lane splits/merges
	sliceptr = (vector3f*) g_terrain;
	for (chunk = 0; chunk < 600; chunk++) {
		i = chunk * 4;
		split = (track_data[i].num_lanes & 0x1f) - (track_data[i + 4].num_lanes & 0x1f);
		for (slice = 0; slice < 5; slice++) {
	    	if ((slice == 0 && split < 0) //lane split (1st row)
	    		|| (slice == 4 && split > 0)) { //lane merge (5th row)
	    		sliceptr[7] = sliceptr[6];
	    		sliceptr[6] = sliceptr[0];
	    		sliceptr[0] = sliceptr[1];
	    	}
	    	sliceptr += 11;
		}
	}

	// invisible polygons at tunnel entrances
	for (chunk = 0; chunk < 600; chunk++) {
		i = chunk * 4;
		if (track_data[i].item_mode == 3 && track_data[i + 4].item_mode == 5) {
			g_terrain_texId[chunk * 10 + 4] = 0; //rightmost strip
			g_terrain_texId[chunk * 10 + 9] = 0; //leftmost strip
		}
	}

}

void tnfs_init_surface_constants() {

	// Road Traction sections (surface constants)
	// Tarmac
	road_surface_type_array[0].roadFriction = 0x100;
	road_surface_type_array[0].velocity_drag = 0x100;
	road_surface_type_array[0].surface_drag = 0x3333;
	road_surface_type_array[0].is_unpaved = 0;
	// Shoulder
	road_surface_type_array[1].surface_drag = 0x8ccc;
	road_surface_type_array[1].velocity_drag = 0x1400;
	road_surface_type_array[1].roadFriction = 0xb3;
	road_surface_type_array[1].is_unpaved = 1;
	// Shoulder2
	road_surface_type_array[2].surface_drag = 0x8ccc;
	road_surface_type_array[2].velocity_drag = 0x1400;
	road_surface_type_array[2].roadFriction = 0xb3;
	road_surface_type_array[2].is_unpaved = 1;
	// ???
	road_surface_type_array[3].roadFriction = 0x100;
	road_surface_type_array[3].velocity_drag = 0x100;
	road_surface_type_array[3].surface_drag = 0x3333;
	// ???
	road_surface_type_array[4].surface_drag = 0x1999;
	road_surface_type_array[4].velocity_drag = 0x300;
	road_surface_type_array[4].roadFriction = 0x4c;
	// ???
	road_surface_type_array[5].surface_drag = 0x1999;
	road_surface_type_array[5].velocity_drag = 0x300;
	road_surface_type_array[5].roadFriction = 0x4c;
	//some easter egg
	/*
	if ((*(int*) PTR_DAT_00010c48 == 8) && (*(int*) &g_car_settings->field_0x4 != 0)) {
		road_surface_type_array[3].roadFriction = 0x66;
		road_surface_type_array[3].velocity_drag = 0x66;
		road_surface_type_array[3].surface_drag = 0x1999;
	}
	*/
}

void tnfs_reset_car(tnfs_car_data *car) {

	car->gear_selected = -1; //-2 Reverse, -1 Neutral, 0..8 Forward gears
	car->gear_auto_selected = 2; //0 Manual mode, 1 Reverse, 2 Neutral, 3 Drive
	car->gear_shift_current = -1;
	car->gear_shift_previous = -1;
	car->gear_shift_interval = 16;
	car->tire_skid_front = 0;
	car->tire_skid_rear = 0;
	car->is_gear_engaged = 0;
	car->handbrake = 0;
	car->is_engine_cutoff = 0;
	car->is_shifting_gears = -1;
	car->throttle_previous_pos = 0;
	car->throttle = 0;
	car->tcs_on = 0;
	//car->tcs_enabled = 0;
	car->brake = 0;
	car->abs_on = 0;
	//car->abs_enabled = 0;
	car->is_crashed = 0;
	car->is_wrecked = 0;
	car->time_off_ground = 0;
	car->slide_front = 0;
	car->slide_rear = 0;
	car->wheels_on_ground = 1;
	car->surface_type = 0;
	car->surface_type_b = 0;
	car->surface_type_2 = 0;
	car->slope_force_lat = 0;
	car->unknown_flag_3DD = 0;
	car->slope_force_lon = 0;

	car->position.x = track_data[car->track_slice].pos.x;
	car->position.y = track_data[car->track_slice].pos.y + 150;
	car->position.z = track_data[car->track_slice].pos.z;

	car->angle.x = track_data[car->track_slice].slope * 0x400;
	car->angle.y = track_data[car->track_slice].heading * 0x400;
	car->angle.z = track_data[car->track_slice].slant * 0x400;

	// convert slope/slant angles to signed values
	if (car->angle.x > 0x800000)
		car->angle.x -= 0x1000000;
	if (car->angle.z > 0x800000)
		car->angle.z -= 0x1000000;

	car->angle.x *= -1;
	car->angle.z *= -1;

	if (car->ai_state & 0x1000) {
		car->angle.y *= -1;
	}

	car->body_pitch = 0;
	car->body_roll = 0;
	car->angle_dy = 0;
	car->angular_speed = 0;
	car->speed_x = 0;
	car->speed_y = 0;
	car->speed_z = 0;
	car->speed = 0;
	car->car_road_speed = 0;
	car->speed_drivetrain = 0;
	car->speed_local_lat = 0;
	car->speed_local_vert = 0;
	car->speed_local_lon = 0;
	car->steer_angle = 0; //int32 -1769472 to +1769472
	car->tire_grip_loss = 0;
	car->accel_lat = 0;
	car->accel_lon = 0;
	car->road_grip_increment = 0;
	car->lap_number = 1;

	car->world_position = car->position;
	car->road_ground_position = car->position;

	car->rpm_vehicle = car_specs.rpm_idle;
	car->rpm_engine = car_specs.rpm_idle;
	car->rpm_redline = car_specs.rpm_redline;

	car->road_fence_normal.x = 0x10000;
	car->road_fence_normal.y = 0;
	car->road_fence_normal.z = 0;

	//surface normal (up)
	car->road_surface_normal.x = 0;
	car->road_surface_normal.y = 0x10000;
	car->road_surface_normal.z = 0;

	//track next node (north)
	car->road_heading.x = 0;
	car->road_heading.y = 0;
	car->road_heading.z = 0x10000;

	//surface position center
	car->road_position.x = 0;
	car->road_position.y = 0;
	car->road_position.z = 0;

	car->front_edge.x = 0x10000;
	car->front_edge.y = 0;
	car->front_edge.z = 0;

	car->side_edge.x = 0;
	car->side_edge.y = 0x10000;
	car->side_edge.z = 0;

	car->track_center_distance = 0;

	math_matrix_identity(&car->matrix);
	math_matrix_identity(&car->collision_data.matrix);

	car->collision_data.position.x = car->position.x;
	car->collision_data.position.y = car->position.y;
	car->collision_data.position.z = -car->position.z;
	car->collision_data.speed.x = 0;
	car->collision_data.speed.y = 0;
	car->collision_data.speed.z = 0;
	car->collision_data.gravity_vector.x = 0;
	car->collision_data.gravity_vector.y = -0x9cf5c;
	car->collision_data.gravity_vector.z = 0;
	car->collision_data.state_timer = 0;
	car->collision_data.angular_speed.x = 0;
	car->collision_data.angular_speed.y = 0;
	car->collision_data.angular_speed.z = 0;

	// ai car flags
	car->speed_target = 0;
	car->target_angle = 0;
	car->collision_data.field_084 = 0;
	car->collision_data.field_088 = 0;
	car->collision_data.field_08c = 0;
	car->collision_data.traffic_speed_factor = 0x10000;
	car->lane_slack = 0;
	car->crash_state = 3;
	car->field_461 = 0;
	car->field_4e9 = 7;
	car->field_158 = 0;

	car->field_4c5 = 0;
	car->field_4c9 = 0;
	car->field_4cd = 0;
	car->field_4d1 = 0;
	car->field_4d3 = 0;

	if (car == player_car_ptr) {
		// player car
		car->crash_state = 2;
		car->ai_state = 0x1e0;
		car->field_158 = 1;
		g_police_on_chase = 0;
		g_police_speeding_ticket = 0;
		g_police_ticket_time = 0;
		g_police_chase_time = 0;
	} else {
		// ai cars
		car->crash_state = 3;
		if (car->car_id == g_racer_cars_in_scene) {
			// police car
			car->ai_state = 0x1e8;
		}
	}
}

void tnfs_Fiziks_InitCar(tnfs_car_data *car) {
	int iVar1;
	int aux;

	tnfs_init_surface_constants();

	iVar1 = math_mul(car->car_specs_ptr->normal_coeff_loss, car->car_specs_ptr->mass_front);
	iVar1 = car->car_specs_ptr->max_tire_coeff + iVar1 / 2;
	car->car_specs_ptr->max_tire_coeff = iVar1;
	printf("Adjusted max tire co = %d\n", iVar1);

	// weight distribution
	car->weight_distribution_front = math_mul(car->car_specs_ptr->mass_front, car->car_specs_ptr->inverse_mass);
	car->weight_distribution_rear = math_mul(car->car_specs_ptr->mass_rear, car->car_specs_ptr->inverse_mass);
	// more precise way of doing it:
	//car->weight_distribution_front = math_div(car->car_specs_ptr->mass_front, car->car_specs_ptr->mass_total);
	//car->weight_distribution_rear = 0x10000 - car->weight_distribution_front;

	// unused specs
	car->mass_front = math_mul(car->car_specs_ptr->mass_total, car->car_specs_ptr->inverse_mass_front);
	car->mass_rear = math_mul(car->car_specs_ptr->mass_total, car->car_specs_ptr->inverse_mass_rear);

	// drag coefficient to deccel factor
	car->car_specs_ptr->drag = math_mul(car->car_specs_ptr->drag, car->car_specs_ptr->inverse_mass);

	car->weight_transfer_factor = math_mul(car->car_specs_ptr->centre_of_gravity_height, car->car_specs_ptr->wheelbase_inv);
	car->front_friction_factor = math_mul(car->car_specs_ptr->front_friction_factor, math_mul(car->weight_distribution_front, g_gravity_const));
	car->rear_friction_factor = math_mul(car->car_specs_ptr->rear_friction_factor, math_mul(car->weight_distribution_rear, g_gravity_const));

	car->tire_grip_front = car->front_friction_factor;
	car->tire_grip_rear = car->rear_friction_factor;

	/*
	aux = math_mul(car->weight_distribution_rear, car->car_specs_ptr->wheelbase);
	aux = math_mul(aux, aux);
	math_mul(aux, car->weight_distribution_front);
	aux = math_mul(car->weight_distribution_front, car->car_specs_ptr->wheelbase);
	aux = math_mul(aux, aux);
	math_mul(aux, car->weight_distribution_rear);
	*/

	aux = car->car_specs_ptr->wheelbase;
	aux = math_mul(aux, aux);
	aux = math_mul(0x324, aux);
	car->wheel_base = math_div(aux, car->car_specs_ptr->wheelbase);
	car->moment_of_inertia = math_div(math_mul(aux, car->car_specs_ptr->inertia_factor), car->car_specs_ptr->wheelbase);
	car->front_yaw_factor = math_div(math_mul(car->weight_distribution_rear, car->car_specs_ptr->wheelbase), aux);
	car->rear_yaw_factor = math_div(math_mul(car->weight_distribution_front, car->car_specs_ptr->wheelbase), aux);

	//collision body specs
	car->collision_height_offset = 0x92f1;
	car->collision_data.mass = 0x10a1c;
	car->collision_data.moment_of_inertia = 0x10000;
	car->collision_data.linear_acc_factor = 0xf646;
	car->collision_data.angular_acc_factor = 0x7dd4;
	car->collision_data.size.x = car_specs.body_width / 2;
	car->collision_data.size.y = 0x92f1;
	car->collision_data.size.z = car_specs.body_length / 2;
	car->collision_data.edge_length = math_vec3_length(&car->collision_data.size);
}


void tnfs_init_car() {
	int i;

	tnfs_car_data *car = &g_car_array[0];
	car->car_data_ptr = car;
	car->car_specs_ptr = &car_specs;
	car->car_model_id = g_player_car;
	g_car_ptr_array[0] = car;
	player_car_ptr = &g_car_array[0];

	// load car specs
	//tnfs_create_car_specs();
	read_carspecs_file(g_car_files[g_player_car]);

	car->crash_state = 2;
	car->car_id = 0;
	car->field_4e9 = 7;
	car->position.z = 0; //0x600000;
	car->track_slice = 0x10;
	car->track_slice_lap = car->track_slice;
	car->lap_number = 1;

	// net wheel torque values
	i = 1;
	do {
		car_specs.torque_table[i] = //
				math_mul(math_mul(math_mul(math_mul(
						car_specs.torque_table[i] << 0x10,
						car_specs.final_drive),
						car_specs.efficiency),
						car_specs.inverse_wheel_radius),
						car_specs.inverse_mass);
		i += 2;
	} while (i < car_specs.torque_table_entries * 2);

	car->car_length = car_specs.body_length;
	car->car_width = car_specs.body_width;

	if (g_config.abs)
		car->abs_enabled = car_specs.abs_equipped;
	if (g_config.tcs)
		car->tcs_enabled = car_specs.tcs_equipped;

	car->gear_auto_selected = 0;

	tnfs_Fiziks_InitCar(car);

	tnfs_reset_car(car);
}

void tnfs_initial_position(tnfs_car_data *car) {
	int iVar1;
	int iVar2;

	iVar1 = (track_data[10].num_lanes & 0xf) * 0x28 + (track_data[10].roadRightMargin << 3);

	iVar2 = g_race_positions[car->car_id2];

	(car->position).x = (iVar1 * -0x100) / 2 + (iVar2 % 2) * iVar1 * 0x100;

	if (((track_data[g_slice_mask & 0x14].num_lanes >> 4 == 2) //
		&& ((track_data[g_slice_mask & 0x14].num_lanes & 0xf) == 1)) //
		&& (iVar1 % 2 == 0)) {
		(car->position).x += iVar1 * -0x100;
	}

	if (g_racer_cars_in_scene < 3) {
		car->track_slice = (iVar2 / 2) * -4 + 0x20;
	} else {
		car->track_slice = iVar2 * -2 + 0x20;
	}

	car->track_center_distance = 0;
	(car->position).z = track_data[car->track_slice & g_slice_mask].pos.z;
}

/* basic game controls */

void tnfs_controls_update() {
	// steer ramp
	if (g_control_steer > 0) {
		g_car_array[0].steer_angle += 0x40000;
		if (g_car_array[0].steer_angle > 0x1B0000)
			g_car_array[0].steer_angle = 0x1B0000;
	} else if (g_control_steer < 0) {
		g_car_array[0].steer_angle -= 0x40000;
		if (g_car_array[0].steer_angle < -0x1B0000)
			g_car_array[0].steer_angle = -0x1B0000;
	} else {
		g_car_array[0].steer_angle >>= 1;//*= 0.8;
	}
	// throttle ramp
	if (g_control_throttle) {
		g_car_array[0].throttle += 0x11;
		if (g_car_array[0].throttle > 0xFF)
			g_car_array[0].throttle = 0xFF;
	} else {
		g_car_array[0].throttle -= 0xC;
		if (g_car_array[0].throttle < 0)
			g_car_array[0].throttle = 0;
	}
	// brake ramp
	if (g_control_brake) {
		g_car_array[0].brake += g_car_array[0].brake < 140 ? 0xC : 2;
		if (g_car_array[0].brake > 0xFF)
			g_car_array[0].brake = 0xFF;
	} else {
		g_car_array[0].brake -= 0x33;
		if (g_car_array[0].brake < 0)
			g_car_array[0].brake = 0;
	}

	//checkpoint flick
	if (tnfs_racer_crossed_finish_line(&g_car_array[0])) {
		tnfs_driving_checkpoint_flick(&g_car_array[0]);
	} else {
		g_car_array[0].field_4cd = 0;
	}
}

void tnfs_change_camera() {
	selected_camera++;
	if (selected_camera > 4)
		selected_camera = 0;

	camera.id_user = selected_camera;

	if (selected_camera == 0) {
		tnfs_camera_set(&camera, 9);
	} else {
		tnfs_camera_set(&camera, selected_camera);
	}
}

void tnfs_change_gear_automatic(int shift) {
	g_car_array[0].gear_auto_selected += shift;

	switch (g_car_array[0].gear_auto_selected) {
	case 1:
		g_car_array[0].gear_selected = -2;
		g_car_array[0].is_gear_engaged = 1;
		printf("Gear: Reverse\n");
		break;
	case 2:
		g_car_array[0].gear_selected = -1;
		g_car_array[0].is_gear_engaged = 0;
		printf("Gear: Neutral\n");
		break;
	case 3:
		g_car_array[0].gear_selected = 0;
		g_car_array[0].is_gear_engaged = 1;
		printf("Gear: Drive\n");
		break;
	}
}

void tnfs_change_gear_manual(int shift) {
	g_car_array[0].gear_selected += shift;

	switch (g_car_array[0].gear_selected) {
	case -2:
		g_car_array[0].is_gear_engaged = 1;
		printf("Gear: Reverse\n");
		break;
	case -1:
		g_car_array[0].is_gear_engaged = 0;
		printf("Gear: Neutral\n");
		break;
	default:
		g_car_array[0].is_gear_engaged = 1;
		printf("Gear: %d\n", g_car_array[0].gear_selected + 1);
		break;
	}
}

void tnfs_change_gear_up() {
	if (g_car_array[0].gear_auto_selected == 0) {
		if (g_car_array[0].gear_selected < car_specs.number_of_gears - 1)
			tnfs_change_gear_manual(+1);
	} else {
		if (g_car_array[0].gear_auto_selected < 3)
			tnfs_change_gear_automatic(+1);
	}
}

void tnfs_change_gear_down() {
	if (g_car_array[0].gear_auto_selected == 0) {
		if (g_car_array[0].gear_selected > -2)
			tnfs_change_gear_manual(-1);
	} else {
		if (g_car_array[0].gear_auto_selected > 1)
			tnfs_change_gear_automatic(-1);
	}
}

/* additional features */

void tnfs_abs() {
	if (g_car_array[0].abs_enabled) {
		g_car_array[0].abs_enabled = 0;
		printf("ABS brakes off\n");
	} else {
		g_car_array[0].abs_enabled = 1;
		printf("ABS brakes on\n");
	}
}

void tnfs_tcs() {
	if (g_car_array[0].tcs_enabled) {
		g_car_array[0].tcs_enabled = 0;
		printf("Traction control off\n");
	} else {
		g_car_array[0].tcs_enabled = 1;
		printf("Traction control on\n");
	}
}

void tnfs_change_transmission_type() {
	if (g_car_array[0].gear_auto_selected == 0) {
		printf("Automatic Transmission mode\n");
		g_car_array[0].gear_auto_selected = 2;
		tnfs_change_gear_automatic(0);
	} else {
		printf("Manual Transmission mode\n");
		g_car_array[0].gear_auto_selected = 0;
		tnfs_change_gear_manual(0);
	}
}

void tnfs_change_traction() {
	if (g_car_array[0].car_specs_ptr->front_drive_percentage == 0x8000) {
		g_car_array[0].car_specs_ptr->front_drive_percentage = 0x10000;
		printf("Traction: FWD\n");
	} else if (g_car_array[0].car_specs_ptr->front_drive_percentage == 0) {
		g_car_array[0].car_specs_ptr->front_drive_percentage = 0x8000;
		printf("Traction: AWD\n");
	} else {
		g_car_array[0].car_specs_ptr->front_drive_percentage = 0;
		printf("Traction: RWD\n");
	}
}

void tnfs_cheat_mode() {
	g_selected_cheat++;
	cheat_crashing_cars = 0;
	g_game_settings = 0;

	if (g_selected_cheat > 2) {
		g_selected_cheat = 0;
		printf("No easter egg active.\n");
	}
	if (g_selected_cheat == 1) {
		cheat_crashing_cars = 4;
		printf("Cheat mode: Crashing cars - Press handbrake to crash\n");
	}
	if (g_selected_cheat == 2) {
		printf("Cheat mode: Rally Mode\n");
		g_game_settings = 0x20;
	}

	tnfs_init_car();
	tnfs_reset_car(g_car_ptr_array[0]);
}

void tnfs_crash_car() {
	int i;
	for (i = 1; i < g_total_cars_in_scene; i++) {
		tnfs_collision_rollover_start(g_car_ptr_array[i], -0xa0000, -0xa0000, -0xa0000);
	}
}

void tnfs_sfx_play(int a, int id1, int id2, int volume, int distance, int direction) {
	float vol;

	if (distance < 0x80000) {
		vol = 1;
	} else if (distance < 0x100000) {
		vol = 0.5;
	} else if (distance < 0x800000) {
		vol = 0.25;
	} else {
		return;
	}

	if (id1 == 2) {
		//collisions
		if (id2 == 0) {
			// car on car
			sfx_play_sound(9, 0, 0.25f, vol);
		} else if (id2 == 1) {
			// rollover crash
			sfx_play_sound(7, 0, 0.25f, vol);
		} else if (id2 == 9) {
			//fence collision
			sfx_play_sound(14, 0, 0.25f, vol);
		}

	} else if (id1 == 4) {
		// car jump hit
		sfx_play_sound(13, 0, 0.25f, 1);

	} else if (id1 == 13) {
		// gear shift
		sfx_play_sound(4, 0, 0.25f, 1);
	}
}

void sfx_update() {
	float f = 0;
	tnfs_car_data * car;

	car = camera.car_ptr_1;
	if (!car) {
		car = player_car_ptr;
	}

	// engine sound
	if (car->is_crashed) {
		sfx_play_sound(2, 1, 0, 0);
	} else {
		f = ((float)car->rpm_engine) / 14000;
		sfx_play_sound(2, 1, f, f);
	}

	// cop siren
	if (g_cop_car_ptr && ((g_cop_car_ptr->ai_state & 0x408) == 0x408)) {
		f = abs(camera.track_slice - g_cop_car_ptr->track_slice);
		f = (100 / f) / 100;
	} else {
		f = 0;
	}
	sfx_play_sound(10, 1, 0.25f, f);

	// air drag/rolling tires sound
	if (car->time_off_ground > 0 || car->is_crashed) {
		f = 0;
	} else {
		f = ((float)car->speed_local_lon) / 0x600000;
	}
	sfx_play_sound(17, 1, f, f);

	// tire screeching
	if (car->time_off_ground > 0 || car->is_crashed) {
		f = 0;
	} else {
		if (is_drifting) {
			f = 0.5;
		} else {
			if (car->tire_skid_front || car->tire_skid_rear) {
				f = 0.25f;
			} else {
				f = 0;
			}
		}
	}
	sfx_play_sound(18, 1, f, f);
}

/* common stub functions */

void tnfs_replay_highlight_record(char a) {
	if (iSimTimeClock % 30 == 0)
		printf("replay highlight %i\n", a);
}

/* common original TNFS functions */

int tnfs_racer_crossed_finish_line(tnfs_car_data *car) {
	if ((g_number_of_players > 1) //
			&& (car->track_slice < 0xe) //
			&& (car->lap_number == 1) //
			&& (car->position.z < 0x510000)) {
		car->position.z = 0x510000;
		car->speed_local_lat = car->speed_local_lat / 2;
		car->speed_local_lon = car->speed_local_lon / 2;
		car->speed_z = car->speed_z / 2;
		car->speed_x = car->speed_x / 2;
	}
		if (car->track_slice < 0xc) {
			if (car->position.z < 0x120000) {
				car->position.z = 0x120000;
			}
			return 1;
		}
		if (car->track_slice > g_road_finish_node) {
			return 2;
		}
	return 0;
}

void tnfs_car_local_position_vector(tnfs_car_data *car, int *angle, int *length) {
	int x;
	int y;
	int z;
	int heading;

	x = car->position.x - camera.position.x;
	y = car->position.y - camera.position.y;
	z = car->position.z - camera.position.z;

	heading = camera.orientation.y;

	if (heading < 0) {
		heading = heading + 0x1000000;
	}
	*angle = heading - math_atan2(z, x);
	if (*angle < 0) {
		*angle += 0x1000000;
	}
	if (*angle > 0x1000000) {
		*angle -= 0x1000000;
	}

	if (x < 0) {
		x = -x;
	}
	if (y < 0) {
		y = -y;
	}
	if (z < 0) {
		z = -z;
	}
	if (z < x) {
		x = (z >> 2) + x;
	} else {
		x = (x >> 2) + z;
	}
	if (x < y) {
		*length = (x >> 2) + y;
	} else {
		*length = (y >> 2) + x;
	}
}

int tnfs_track_node_find(tnfs_vec3 *p_position, int *current) {
	int node;
	int dist1;
	int dist2;
	int changed;
	tnfs_vec3 position;
	struct tnfs_track_data *tracknode1;
	struct tnfs_track_data *tracknode2;

	changed = 0;

	if (*current != -1) {
		do {
			node = *current;

			tracknode1 = &track_data[node & g_slice_mask];
			tracknode2 = &track_data[(node + 1) & g_slice_mask];
			position.x = (tracknode1->pos.x + tracknode2->pos.x) >> 1;
			position.z = (tracknode1->pos.z + tracknode2->pos.z) >> 1;
			dist1 = math_vec3_distance_squared_XZ(&position, p_position);

			tracknode1 = &track_data[(node + 1) & g_slice_mask];
			tracknode2 = &track_data[(node + 2) & g_slice_mask];
			position.x = (tracknode1->pos.x + tracknode2->pos.x) >> 1;
			position.z = (tracknode1->pos.z + tracknode2->pos.z) >> 1;
			dist2 = math_vec3_distance_squared_XZ(&position, p_position);

			if (dist2 < dist1) {
				changed = 1;
				*current = *current + 1;
			} else if (0 < *current) {
				tracknode1 = &track_data[(node - 1) & g_slice_mask];
				tracknode2 = &track_data[node & g_slice_mask];
				position.x = (tracknode1->pos.x + tracknode2->pos.x) >> 1;
				position.z = (tracknode1->pos.z + tracknode2->pos.z) >> 1;
				dist2 = math_vec3_distance_squared_XZ(&position, p_position);

				if (dist2 < dist1) {
					node = *current;
					*current = node - 1;
					if (node - 1 < 0) {
						*current = 0;
					} else {
						changed = 1;
					}
				}
			}
		} while (node != *current);
	}
	return changed;
}

int tnfs_track_node_update(tnfs_car_data *car) {
	int changed;
	int node;
	node = car->track_slice;
	changed = tnfs_track_node_find(&car->position, &node);
	car->track_slice = node;
	car->track_slice_lap = node;
	return changed;
}

/*
 * #Traffic Speed Factors: Traffic Cars will select one of these 4 multipliers,
 * # and drive at the "Safe Speed" times this multiplier
 */
void tnfs_ai_get_speed_factor(tnfs_car_data *car) {
	int rnd;
	g_lcg_random_nbr = g_lcg_random_mod * g_lcg_random_seed;
	g_lcg_random_mod = g_lcg_random_nbr & 0xffff;
	rnd = (g_lcg_random_nbr & 0xffff00) >> 8 & 3;
	car->collision_data.traffic_speed_factor = g_ai_skill_cfg.traffic_speed_factors[rnd];
}

/*
 * #Lane Slack.  Determines how close the cars drive along the centre line.  Measured in units from the
 * # centre of a lane towards the centre of the road.
 */
void tnfs_ai_get_lane_slack(tnfs_car_data *car) {
	int rnd;
	g_lcg_random_nbr = g_lcg_random_mod * g_lcg_random_seed;
	g_lcg_random_mod = g_lcg_random_nbr & 0xffff;
	rnd = (g_lcg_random_nbr & 0xffff00) >> 8 & 3;
	if ((car->ai_state & 4) == 0) {
		car->lane_slack = g_ai_skill_cfg.lane_slack[rnd];
	}
}

void tnfs_track_update_vectors(tnfs_car_data *car) {
	int node;
	tnfs_vec3 heading;
	tnfs_vec3 wall_normal;

	// current node
	node = car->track_slice & g_slice_mask;
	car->road_position.x = track_data[node].pos.x;
	car->road_position.y = track_data[node].pos.y;
	car->road_position.z = track_data[node].pos.z;

	wall_normal.x = track_data[node].side_normal_x << 1;
	wall_normal.y = track_data[node].side_normal_y << 1;
	wall_normal.z = track_data[node].side_normal_z << 1;

	// next node vector
	node = (car->track_slice + 1) & g_slice_mask;
	heading.x = track_data[node].pos.x - car->road_position.x;
	heading.y = track_data[node].pos.y - car->road_position.y;
	heading.z = track_data[node].pos.z - car->road_position.z;

	math_vec3_normalize_fast(&heading);

	// 0x10000, 0, 0 => points to right side of road
	car->road_fence_normal.x = wall_normal.x;
	car->road_fence_normal.y = wall_normal.y;
	car->road_fence_normal.z = wall_normal.z;

	// 0, 0x10000, 0 => up
	car->road_surface_normal.x = fixmul(heading.y, wall_normal.z) - fixmul(heading.z, wall_normal.y);
	car->road_surface_normal.y = fixmul(heading.z, wall_normal.x) - fixmul(heading.x, wall_normal.z);
	car->road_surface_normal.z = fixmul(heading.x, wall_normal.y) - fixmul(heading.y, wall_normal.x);

	// 0, 0, 0x10000 => north
	car->road_heading.x = heading.x;
	car->road_heading.y = heading.y;
	car->road_heading.z = heading.z;

	// unknown purpose
	if (car != &g_car_array[0]) {
		g_lcg_random_nbr = g_lcg_random_mod * g_lcg_random_seed;
		g_lcg_random_mod = g_lcg_random_nbr & 0xffff;
		if (DAT_800eb6a4 * ((g_lcg_random_nbr & 0xffff00) >> 8) >> 0x10 == 1) {
			tnfs_ai_get_lane_slack(car);
		}
	}
}


int tnfs_car_road_speed(tnfs_car_data *car) {
	return (-car->speed_x >> 8) * ((car->road_heading).x >> 8) //
			+ ((car->road_heading).y >> 8) * (car->speed_y >> 8) //
			+ ((car->road_heading).z >> 8) * (car->speed_z >> 8);
}

int tnfs_car_road_speed_2(tnfs_car_data *car) {
	return ((car->collision_data.speed).x >> 8) * ((car->road_heading).x >> 8) //
			+ ((car->road_heading).y >> 8) * ((car->collision_data.speed).y >> 8) //
			+ ((car->road_heading).z >> 8) * (-(car->collision_data.speed).z >> 8);
}

void tnfs_car_update_center_line(tnfs_car_data *car) {
	int iVar1;
	int iVar2;
	int iVar3;
	iVar3 = track_data[car->track_slice & g_slice_mask].heading;
	iVar1 = math_mul(math_sin_2(iVar3 << 2), track_data[car->track_slice & g_slice_mask].pos.x - (car->position).x);
	iVar2 = math_mul(math_cos_2(iVar3 << 2), track_data[car->track_slice & g_slice_mask].pos.z - (car->position).z);
	car->track_center_distance = -iVar2 - iVar1;
}

int tnfs_car_shadow_update(tnfs_car_data *car, int param_2) {
	int uVar1;
	int iVar2;
	int iVar4;
	int iVar5;
	int iVar6;
	//int iVar7;
	int iVar8;
	int iVar9;
	//tnfs_vec3 *ptVar3;
	//tnfs_vec3 tStack_68;
	//tnfs_vec3 local_5c;
	//tnfs_vec3 local_50;
	//tnfs_vec3 iStack_20;

	/*
	local_5c.x = (car->position).x - camera.position.x;
	local_5c.y = (car->position).y - camera.position.y;
	local_5c.z = (car->position).z - camera.position.z;
	math_matrix_0009d744(&local_5c, &g_shadow_matrix, &local_50);
	(car->carmodel_angle).x = local_50.x;
	(car->carmodel_angle).y = local_50.y;
	(car->carmodel_angle).z = local_50.z;
	*/

	//math_matrix_identity(&g_shadow_matrix);
	car->angle_dy = math_atan2(car->matrix.cz, car->matrix.cx);

	if (param_2 == 0) {
		/*
		if ((local_50.z < 0x20) || (0x1900 < local_50.z)) {
			return 0;
		}
		iVar2 = local_50.x;
		if (local_50.x < 1) {
			iVar2 = -local_50.x;
		}
		if (local_50.z < iVar2) {
			return 0;
		}
		*/
		tnfs_height_road_position(car, 1);
		/*
		local_5c.x = car->world_position.x;
		local_5c.y = car->world_position.y;
		local_5c.z = car->world_position.z;
		math_matrix_0009d744(&local_5c, &g_shadow_matrix, &local_50);
		*/

		iVar8 = (car->side_edge).x >> 1;
		iVar2 = (car->side_edge).y >> 1;
		iVar5 = (car->side_edge).z >> 1;
		iVar6 = (car->front_edge).x >> 1;
		iVar4 = (car->front_edge).y >> 1;
		iVar9 = (car->front_edge).z >> 1;

		g_shadow_points[3].x = iVar8 + iVar6;
		g_shadow_points[3].y = iVar2 + iVar4;
		g_shadow_points[3].z = iVar5 + iVar9;
		g_shadow_points[2].x = iVar8 - iVar6;
		g_shadow_points[2].y = iVar2 - iVar4;
		g_shadow_points[2].z = iVar5 - iVar9;
		g_shadow_points[1].x = -iVar8 - iVar6;
		g_shadow_points[1].y = -iVar2 - iVar4;
		g_shadow_points[1].z = -iVar5 - iVar9;
		g_shadow_points[0].x = -iVar8 + iVar6;
		g_shadow_points[0].y = -iVar2 + iVar4;
		g_shadow_points[0].z = -iVar5 + iVar9;

		//iVar7 = 0;
		for (int i = 0; i < 4; i++) {
			/*
			math_matrix_0009d744(&g_shadow_points[i], &g_shadow_matrix, &tStack_68);
			tStack_68.x = tStack_68.x + local_50.x;
			tStack_68.y = tStack_68.y + local_50.y;
			tStack_68.z = tStack_68.z + local_50.z;
			ptVar3 = (tnfs_vec3*)&g_shadow_matrix;
			ptVar3 += i * 12;
			//iVar7 = iVar7 + 8;
			tnfs_shadow_00475eac(&tStack_68, ptVar3);
			*/
			g_shadow_points[i].y += car->world_position.y;
		} //while (iVar7 != 0x20);

		//if (car == (&player_car_ptr)[g_player_id]) {
		//	FUN_004596a8(car, iVar6, iVar2, iVar8, iVar2, iVar5, iVar6, iVar4, iVar9, local_5c.x, local_5c.y, local_5c.z);
		//}
		uVar1 = 1;
	} else {
		uVar1 = 0;
	}
	return uVar1;
}

void tnfs_smoke_update() {
	for (int i = 0; i < SMOKE_PUFFS; i++) {
		g_smoke[i].texId = i & 3;
		if (g_smoke[i].time < 0) {
			if (is_drifting && g_smoke_delay <= 0) {
				g_smoke_delay = 20;
				g_smoke[i].position.x = -fixmul(math_sin_3(player_car_ptr->angle.y), player_car_ptr->car_length / 2);
				g_smoke[i].position.x +=  player_car_ptr->position.x;
				g_smoke[i].position.z = -fixmul(math_cos_3(player_car_ptr->angle.y), player_car_ptr->car_length / 2);
				g_smoke[i].position.z += player_car_ptr->position.z;

				g_smoke[i].position.y = player_car_ptr->position.y;
				g_smoke[i].time = 0xFF;
			}
			g_smoke_delay--;
		} else {
			g_smoke[i].time -= 10;
			g_smoke[i].position.y += 0x100;
		}
	}
}


/*
 * setup everything
 */
void tnfs_init_sim() {
	iSimTimeClock = 200;
	cheat_crashing_cars = 0;
	g_game_settings = 0;
	sound_flag = 0;
	int i;

	g_race_status = 0;

	//init track
	char trkfile[80];
	sprintf(trkfile, "assets/DriveData/tracks/%s%d.trk", g_track_files[g_track_sel], g_track_segment + 1);
	tnfs_init_track(trkfile);

	//load track textures
	sprintf(trkfile, "%s%d", g_Track_files[g_track_sel], g_track_segment + 1);
	read_track_pkt_file(trkfile);

	//skill
	read_skill_file(g_config.skill_level);

	g_stats_data.best_accel_time_1 = 99999;
	g_stats_data.best_accel_time_2 = 99999;
	g_stats_data.best_brake_time_2 = 999;
	g_stats_data.best_brake_time_1 = 999;
	g_stats_data.quarter_mile_speed = 0;
	g_stats_data.quarter_mile_time = 99999;
	g_stats_data.penalty_count = 0;
	g_stats_data.warning_count = 0;
	g_stats_data.field_0x1b8 = 0;
	g_stats_data.prev_lap_time = 0;
	g_stats_data.lap_time_0x1c0 = 0;
	g_stats_data.top_speed = 0;
	g_stats_data.field_0x1c8 = 0;
	for (i = 0; i < 17; i++) {
		g_stats_data.lap_timer[i] = 0;
	}

	// load cars 3d models
	g_carmodels_count = 0;
	for (i = 0; i < 30; i++) {
		if (g_car_wrapfams[i] == 0)
			break;
		if (read_carmodel_file(g_car_wrapfams[i], &g_carmodels[i])) {
			g_carmodels_count++;
		}
	}

	// init player car
	tnfs_init_car();

	// create hud
	read_hud_dash_file();

	// common art
	read_sim_common_art_file();

	// create AI car(s)
	if (g_config.skill_level >= 3) { // single race
		g_number_of_cops = 0;
		g_number_of_traffic_cars = 0;
		g_total_cars_in_scene = 1;
	} else {
		g_number_of_cops = 1;
		g_number_of_traffic_cars = 4;
		g_total_cars_in_scene = 7;
	}

	if (g_opp_car == 8) { //race against the clock
		g_racer_cars_in_scene = 1;
	} else {
		g_racer_cars_in_scene = 2;
	}

	tnfs_ai_init(g_opp_car);

	// car positions
	if (g_racer_cars_in_scene == 1) {
		if (g_track_sel == 2) {
			g_car_array[0].position.x += 0x50000;
		} else {
			g_car_array[0].position.x = 0;
		}
	} else {
		if (g_track_sel == 2) { // City
			g_car_array[0].position.x += 0x30000;
			g_car_array[1].position.x += 0x78000;
		} else {
			g_car_array[0].position.x -= 0x20000;
			g_car_array[1].position.x += 0x20000;
		}
	}

	tnfs_camera_init();
}

/*
 * minimal basic main loop
 */
void tnfs_update() {
	int i;
	tnfs_car_data *car;
	int reset;

	iSimTimeClock++;

	if (g_race_status == 0 && player_car_ptr->car_road_speed > 0) {
		g_race_status = 1;
		tnfs_ai_respawn_00028cc4();
	}
	//FIX: disable cop if crashed or finished race
	if (player_car_ptr->is_crashed || player_car_ptr->field_4c9) {
		g_police_on_chase = 0;
		g_cop_car_ptr->ai_state = 0x1e8;
	}

	player_car_ptr->car_road_speed = tnfs_car_road_speed(player_car_ptr);

	tnfs_controls_update();
	tnfs_ai_collision_handler();

	// for each car
	for (i = 0; i < g_total_cars_in_scene; i++) {
		car = g_car_ptr_array[i];

		if ((car->field_4e9 & 4) == 0) {
			//disabled car
			tnfs_ai_hidden_traffic(car);
			continue;
		}

		if (car->crash_state != 4) {
			if (i < g_number_of_players) {
				tnfs_driving_main(car);
				math_matrix_from_pitch_yaw_roll(&car->matrix, car->angle.x + car->body_pitch, car->angle.y, car->angle.z + car->body_roll);
			} else {
				if (g_race_status > 0) {
					tnfs_ai_driving_main(car);
				}
			}
		} else {
			tnfs_collision_main(car);
		}

		// tweak to allow circuit track lap
		reset = 0;
		if ((car->track_slice > g_road_node_count - 2) && (car->car_road_speed > 0x1000)) {
			car->track_slice = 0;
			car->track_slice_lap = 0;
			reset = 1;
		} else if ((car->track_slice == 0) && (car->car_road_speed < -0x1000)) {
			car->track_slice = car->track_slice_lap = g_road_node_count - 2;
			reset = 1;
		}
		if (reset //
			&& ((abs(car->position.x - track_data[car->track_slice].pos.x) > 0x1000000)
			|| (abs(car->position.z - track_data[car->track_slice].pos.z) > 0x1000000))) {
			tnfs_reset_car(car);
		}

		tnfs_car_update_center_line(car);

		// set ground point for the collision engine
		car->road_ground_position = track_data[car->track_slice].pos;
	}

	tnfs_camera_auto_change(player_car_ptr);
	tnfs_camera_update(&camera);
	tnfs_smoke_update();
	sfx_update();
}
