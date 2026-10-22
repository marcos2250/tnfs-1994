/*
 * tnfs_collision_2d.c
 * 2D road fence collision
 */
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_fiziks.h"
#include "tnfs_collision_3d.h"

int crash_pitch = 0;
int DAT_80111a40 = 0;

void tnfs_collision_rotate(tnfs_car_data *car_data, int angle, int lon_speed, int fence_side, int road_flag, int aux_angle) {
	int d_angle;
	int crash_speed_a;
	int crash_speed_b;
	int crash_roll;
	int crash_yaw;
	int collisionAngle;
	int rotAngle;
	int rotSide;

	if (fence_side <= 0)
		d_angle = aux_angle + angle;
	else
		d_angle = angle - aux_angle;

	rotAngle = d_angle - car_data->angle.y + (-(d_angle - car_data->angle.y < 0) & 0x1000000);
	if (rotAngle <= 0x800000)
		collisionAngle = d_angle - car_data->angle.y + (-(d_angle - car_data->angle.y < 0) & 0x1000000);
	else
		collisionAngle = 0x1000000 - rotAngle;
	if (collisionAngle > 0x400000) {
		rotAngle += 0x800000;
		collisionAngle = 0x1000000 - rotAngle;
	}
	if (rotAngle > 0x1000000)
		rotAngle -= 0x1000000;

	if (lon_speed >= 0x100000 && car_data->speed > 0x200000) {
		crash_speed_a = lon_speed - 0x60000;
		if (rotAngle >= 0x800000)
			rotSide = 0x1000000 - fix3(0x1000000 - rotAngle);
		else
			rotSide = fix3(rotAngle);

		if (rotSide > 0x800000)
			rotSide -= 0x1000000;

		if (crash_speed_a > 0xE0000)
			crash_speed_a = 0xE0000;

		crash_speed_b = fix6(crash_speed_a * (collisionAngle >> 16));

		if (rotSide <= 0) {
			crash_yaw = -crash_speed_a;
			crash_roll = -(crash_speed_a - crash_speed_b);
		} else {
			crash_yaw = crash_speed_a;
			crash_roll = crash_speed_a - crash_speed_b;
		}

		if (road_flag) {
			crash_pitch = -crash_speed_b;
		} else {
			crash_roll = -crash_roll;
			crash_pitch = fix6(crash_speed_a * (collisionAngle >> 16));
		}
		if (crash_yaw != 0) {
			tnfs_collision_rollover_start(car_data, fix2(crash_yaw), fix2(3 * crash_roll), fix2(3 * crash_pitch));
		}
	} else {
		if (abs(collisionAngle) < 0x130000 && car_data->speed_local_lon > 0) {
			if (fence_side <= 0) {
				if (rotAngle < 0x130000) {
					car_data->angle.y += rotAngle;
					car_data->angular_speed = 0x9FFFC + 4;
				}
			} else {
				if (rotAngle > 0xed0001) {
					car_data->angle.y += rotAngle;
					car_data->angular_speed -= 0x9FFFC + 4;
				}
			}
		}
	}
}


int tnfs_collision_car_size(tnfs_car_data *car_data, int fence_angle) {
	int x;

	// fast cosine
	x = abs(fence_angle - car_data->angle.y) >> 0x10;
	if (x > 0xc1) {
		x = 0x100 - x;
	} else if (x > 0x80) {
		x = x - 0x80;
	} else if (x > 0x40) {
		x = 0x80 - x;
	}

	return ((((car_data->car_length - car_data->car_width) * x) >> 6) + car_data->car_width) >> 1;
}

void tnfs_track_fence_collision(tnfs_car_data *car_data) {
	int abs_speed;
	int distance;
	int aux;
	long fence_angle;
	int fenceSide;
	int fence_flag;
	int rebound_speed_x;
	int rebound_speed_z;
	int rebound_x;
	int rebound_z;
	int sfxA;
	int sfxB;
	int fence_sin;
	int fence_cos;
	int re_speed;
	int lm_speed;
	int roadPositionX;
	int roadPositionZ;
	int roadLeftMargin;
	int roadRightMargin;
	int roadLeftFence;
	int roadRightFence;
	int r12;

	roadPositionX = track_data[car_data->track_slice & g_slice_mask].pos.x;
	roadPositionZ = track_data[car_data->track_slice & g_slice_mask].pos.z;

	roadLeftMargin = track_data[car_data->track_slice & g_slice_mask].roadLeftMargin;
	roadRightMargin = track_data[car_data->track_slice & g_slice_mask].roadRightMargin;
	roadLeftFence = track_data[car_data->track_slice & g_slice_mask].roadLeftFence;
	roadRightFence = track_data[car_data->track_slice & g_slice_mask].roadRightFence;

	fence_angle = track_data[car_data->track_slice & g_slice_mask].heading * 0x400;
	fence_sin = math_sin_3(fence_angle);
	fence_cos = math_cos_3(fence_angle);
	fence_flag = 0;
	fenceSide = 0;
	distance = fixmul(fence_sin, roadPositionZ - car_data->position.z) - fixmul(fence_cos, roadPositionX - car_data->position.x);


    r12 = track_data[car_data->track_slice].item_mode;
	// ldr        r1,[PTR_DAT_00010a94]
	// ldr        r0,[PTR_DAT_00011af4]
	// mov        r3,#0x1
	// mov        r2,#0x0
	if ((r12 == 4)     // teq        r12,#0x4
	 || (r12 == 7)     // teqne      r12,#0x7
	 || (r12 == 9)) {  // teqne      r12,#0x9
		DAT_00010a94 = 1;  // streq      r3,[r1,#0x0]
		DAT_00011af4 = 1;  // streq      r3,[r0,#0x0]
	} else {           // beq        LAB_00011a60
		DAT_00010a94 = 0;  // str        r2,[r1,#0x0]
		// ldr        r1,[r4,#0x44]
		// add        r1,r1,r1, lsl #0x3
		// add        r1,r6,r1, lsl #0x2
		// ldrb       r1,[r1,#0x7]
		if (track_data[car_data->track_slice].item_mode != 8) { // teq r1,#0x8
			DAT_00011af4 = 0;  // strne r2,[r0,#0x0]
		} else {
			DAT_00011af4 = 1;  // streq r3,[r0,#0x0]
		}
	}

	// LAB_00011a60:
    // ldr        r0,[r4,#0x44]
    // add        r0,r0,r0, lsl #0x3
    // add        r0,r6,r0, lsl #0x2
    // ldrb       r0,[r0,#0x7]
    // ldr        r1,[car_data]
    if (track_data[car_data->track_slice].item_mode != 5) { // teq r0,#0x5
    	car_data->field_4a9 = 1; // strne r2,[r1,#0xc]
    } else {
    	car_data->field_4a9 = 0; // streq r3,[r1,#0xc]
    }

	rebound_speed_x = 0;
	if (distance < roadLeftMargin * -0x2000) {
		// left road side
		car_data->surface_type = track_data[car_data->track_slice & g_slice_mask].shoulder_surface_type >> 4;
		aux = roadLeftFence * -0x2000 + tnfs_collision_car_size(car_data, fence_angle);
		if (distance < aux) {
			fenceSide = -0x100;
			rebound_speed_x = aux - distance - 0x4ccc;
			fence_flag = track_data[car_data->track_slice & g_slice_mask].fence_flag >> 4;
		}

	} else if (distance > roadRightMargin * 0x2000) {
		// right road side
		car_data->surface_type = track_data[car_data->track_slice & g_slice_mask].shoulder_surface_type & 0xf;
		aux = roadRightFence * 0x2000 - tnfs_collision_car_size(car_data, fence_angle);
		if (distance > aux) {
			fenceSide = 0x100;
			rebound_speed_x = distance + 0x4ccc - aux;
			fence_flag = track_data[car_data->track_slice & g_slice_mask].fence_flag & 0xf;
		}
	} else {
		car_data->surface_type = 0;
	}
	if (fenceSide == 0) {
		return;
	}

	// reposition the car back off the fence
	math_rotate_2d(fix8(fenceSide *  abs(rebound_speed_x)), 0, fence_angle, &rebound_x, &rebound_z);
	car_data->position.x = car_data->position.x - rebound_x;
	car_data->position.z = car_data->position.z + rebound_z;

	// rotate speed vector to fence reference
	math_rotate_2d(-car_data->speed_x, car_data->speed_z, fence_angle, &rebound_speed_x, &rebound_speed_z);

	// play collision sound
	abs_speed = abs(rebound_speed_x);
	if (fence_flag && (abs_speed > 0x60000)) {
		if (sound_flag == 0) {
			if (car_data->car_id2 == 0) {
				if (selected_camera == 0) {
					if (distance <= 0)
						sfxA = 0x280000; //left channel
					else
						sfxA = 0xd70000; //right channel
					sfxB = 1;
				} else {
					tnfs_car_local_position_vector(car_data, &sfxA, &sfxB);
				}
			}
		} else {
			if (car_data->car_id2 == 0) {
				sfxA = 0x400000;
			} else {
				sfxA = 0xc00000;
			}
		}
		tnfs_sfx_play(-1, 2, 9, abs_speed, sfxB, sfxA);
		if (abs_speed > 0x140000) {
			tnfs_replay_highlight_record(0x32);
		}
	}

	// dampen collision rebound speed
	if (abs_speed > 0x180000) {
		if (rebound_speed_x > +0x20000)
			rebound_speed_x = +0x20000;
		if (rebound_speed_x < -0x20000)
			rebound_speed_x = -0x20000;
	} else {
		rebound_speed_x = fix2(rebound_speed_x);
	}
	if (rebound_speed_z < -0x30000) {
		if ((g_game_settings & 0x20) == 0) {
			rebound_speed_z = abs_speed + rebound_speed_z;
			lm_speed = fix2(abs_speed);
		} else {
			lm_speed = abs_speed / 2;
		}
		rebound_speed_z = rebound_speed_z + lm_speed;
		if (0 < rebound_speed_z) {
			rebound_speed_z = 0;
		}
	} else if (rebound_speed_z > 0x30000) {
		if ((g_game_settings & 0x20) == 0) {
			lm_speed = fix2(abs_speed);
			aux = abs_speed;
		} else {
			lm_speed = fix3(abs_speed);
			aux = abs_speed / 2;
		}
		rebound_speed_z = rebound_speed_z - lm_speed - aux;
		if (rebound_speed_z < 0) {
			rebound_speed_z = 0;
		}
	}

	// set collision angle side
	re_speed = fix2(abs_speed);
	if (re_speed > 0xa0000) {
		re_speed = 0xa0000;
	}
	if (fenceSide < 1) {
		fence_angle += 0x20000;
		car_data->collision_x = -re_speed;
	} else {
		fence_angle -= 0x20000;
		car_data->collision_x = re_speed;
	}

	car_data->collision_y = -re_speed;
	car_data->collision_a = 0x1e;
	car_data->collision_b = 0x1e;

	// reflect car speeds 180deg
	math_rotate_2d(rebound_speed_x, rebound_speed_z, fence_angle, &car_data->speed_x, &car_data->speed_z);

	// ???
	if (fence_flag == 0) {
		aux = 0x30000;
		fence_flag = 0;
	} else {
		aux = 0x18000;
	}
	tnfs_collision_rotate(car_data, fence_angle, abs_speed, fenceSide, fence_flag, aux);

	if (car_data->speed_y > 0) {
		car_data->speed_y = 0;
	}
}

/* 2adf0 */
void tnfs_track_fence_collision_ai(tnfs_car_data *car) {
	//FIXME stub
}


/*
 * Is the road object at (x, y, z) inside the car's footprint; plays the hit sound when close to the camera.
 * DOS tnfs_collision_sound_00047d9f (0x47d9f), PSX tnfs_collision_scenery (0x8001f0f0), Win95 SE
 * tnfs_collision_road_object (0x40fef0). y is unused in all three. SE also calls the joystick force feedback
 * (_Jolt, 0x43cc10) on a hit when it is enabled (DAT_0052b320).
 */
int tnfs_collision_road_object(tnfs_car_data *car, int x, int y, int z) {
	int dx;
	int dz;
	int cos;
	int sin;
	int lat;
	int lon;
	int distance;
	int direction;

	dx = x - car->position.x;
	dz = z - car->position.z;
	cos = math_cos_2(car->angle.y >> 8);
	sin = math_sin_2(car->angle.y >> 8);

	// The original compares with half the width / length of the car's 3D model (DOS width 0x61d / length 0x619,
	// SE 0x64e / 0x64a, PSX spec_collision_car_width/length, all computed from the model's vertex extents in the
	// renderer). The port does not load the model, so the PDN half extents in collision_data.size are used instead.
	lat = math_mul(cos, dx) - math_mul(sin, dz);
	if (abs(lat) > car->collision_data.size.x) {
		return 0;
	}
	lon = math_mul(sin, dx) + math_mul(cos, dz);
	if (abs(lon) > car->collision_data.size.z) {
		return 0;
	}

	if ((car->track_slice - g_camera_node) < 10 && (car->track_slice - g_camera_node) > -9) {
		tnfs_car_local_position_vector(car, &direction, &distance);
		tnfs_sfx_play(-1, 2, 10, 0x10000, distance, direction);
	}
	return 1;
}

/*
 * Walks the road objects of the car's slice (cursor road_object_index/slice kept per car) and knocks down the first
 * sign the car runs over. DOS tnfs_scenery_collision (0x47a7d), PSX tnfs_collision_scenery_detect (0x8001ed38),
 * Win95 SE tnfs_collision_road_objects (0x40fc64). Every road object is tested, 3D models (prop type 1) too.
 * The status stores the time of the hit in replay keyframes + 1 (DOS ticks / 0x4b0, PSX and SE / 0x708): a replay
 * rewind clears the signs knocked at or after the keyframe it restarts from (DOS 0x7085f, SE 0x4501a8).
 * On a hit, all binaries then invalidate the renderer's cached track vertex lists (DOS 0x62f46, SE 0x43f51c,
 * PSX 0x80038614 for both views) so the sign is rebuilt lying flat; the port has no renderer.
 */
int tnfs_collision_road_objects(tnfs_car_data *car) {
	int slice;
	int index;
	int x;
	int y;
	int z;

	slice = car->track_slice;

	// rewind to the first object of this slice
	while (car->road_object_slice == slice && car->road_object_index != 0) {
		car->road_object_index--;
		car->road_object_slice = g_road_objects[car->road_object_index].slice;
	}

	if (slice < car->road_object_slice || g_road_objects[car->road_object_index + 1].slice == -1) {
		if (slice < car->road_object_slice) {
			// objects behind the cursor
			index = car->road_object_index;
			while (index != 0) {
				if (car->road_object_slice == slice && g_sign_status[car->road_object_index * 2] == 0) {
					x = track_data[slice].pos.x + (g_road_objects[car->road_object_index].pos_x << 8);
					y = track_data[slice].pos.y + (g_road_objects[car->road_object_index].pos_y << 8);
					z = track_data[slice].pos.z + (g_road_objects[car->road_object_index].pos_z << 8);
					if (tnfs_collision_road_object(car, x, y, z)) {
						g_sign_status[car->road_object_index * 2] = (char)(iSimTimeClock / 0x4b0) + 1; // PSX, SE: / 0x708
						return 1;
					}
				}
				car->road_object_index--;
				car->road_object_slice = g_road_objects[car->road_object_index].slice;
				if (car->road_object_slice < slice) {
					return 0;
				}
				index = car->road_object_index;
			}
		}
	} else {
		// objects ahead of the cursor
		do {
			if (car->road_object_slice == slice && g_sign_status[car->road_object_index * 2] == 0) {
				x = track_data[slice].pos.x + (g_road_objects[car->road_object_index].pos_x << 8);
				y = track_data[slice].pos.y + (g_road_objects[car->road_object_index].pos_y << 8);
				z = track_data[slice].pos.z + (g_road_objects[car->road_object_index].pos_z << 8);
				if (tnfs_collision_road_object(car, x, y, z)) {
					g_sign_status[car->road_object_index * 2] = (char)(iSimTimeClock / 0x4b0) + 1; // PSX, SE: / 0x708
					return 1;
				}
			}
			car->road_object_index++;
			car->road_object_slice = g_road_objects[car->road_object_index].slice;
		} while (car->road_object_slice <= slice && g_road_objects[car->road_object_index + 1].slice != -1);
	}
	return 0;
}
