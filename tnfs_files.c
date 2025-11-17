/*
 * tnfs_file.c
 * Readers for TNFS files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_files.h"
#include "tnfs_gfx.h"

struct file_assets g_file_assets[50];
int file_counter = 0;

int readFixed32(unsigned char *buffer, int pos) {
	return (int)(buffer[pos]) << 24 //
		| (int)(buffer[pos + 1]) << 16 //
		| (int)(buffer[pos + 2]) << 8 //
		| buffer[pos + 3];
}

short readSigned16(unsigned char *buffer, int pos) {
	return (((short) buffer[pos]) << 8) | buffer[pos + 1];
}

int readTxtLine(char *output, int size, FILE *fp) {
	char ch = 0;
	int count = 0;
	memset(output, 0, size);
	while(1) {
		ch = fgetc(fp);
		if (ch == '\r') ch = '\n';
		output[count] = ch;
		count++;
		if (ch == '\n') break;
		if (count == size) break;
	}
	return count;
}

int readTxtInt(FILE *ptr) {
  int iVar1;
  int uVar2;
  char local_dc[200];

  do {
    iVar1 = readTxtLine(local_dc, 99, ptr);
    if (local_dc[0] != '#' && local_dc[0] != '\n') {
      if (iVar1 == 0) {
        return 0;
      }
      uVar2 = atoi((char*)&local_dc);
      return uVar2;
    }
  } while (iVar1 != 0);
  return 0;
}

int readTxtDecimal(FILE *ptr) {
  int iVar1;
  float uVar2;
  char local_dc[200];

  do {
    iVar1 = readTxtLine(local_dc, 99, ptr);
    if (local_dc[0] != '#' && local_dc[0] != '\n') {
      if (iVar1 == 0) {
        return 0;
      }
      uVar2 = strtof((char*)&local_dc, NULL) * 0x10000;
      return (int)uVar2;
    }
  } while (iVar1 != 0);
  return 0;
}

void readTxtSkipLine(FILE *fp) {
	char c;
	int count = 100;
	do {
		count--;
		if (count <= 0) break;
		c = fgetc(fp);
	} while (c != '\n' && c != '\r' && c != EOF);
}

byte * read_wwww(byte *data, int path[], int depth) {
	int offset = 0;
	int i;
	for (i = 0; i < depth; i++) {
		if (data[0] != 'w' || data[1] != 'w' || data[2] != 'w' || data[3] != 'w') {
			break;
		}
		if (path[i] > data[7]) { // exceeded num entries
			return 0;
		}
		offset = readFixed32(data, (path[i] * 4) + 8);
		if (offset == 0 || offset > 10000000) {
			return 0;
		}
		data += offset;
	}
	return data;
}

int locate_wwww(byte *data, byte *pointer, int depth, int *path_result) {
	int offset = 0;
	int i;

	path_result[depth] = -1;

	if (data == pointer) {
		return 1;
	}
	if (data[0] != 'w' || data[1] != 'w' || data[2] != 'w' || data[3] != 'w') {
		return 0;
	}
	for (i = 0; i < data[7]; i++) {
		offset = readFixed32(data, i * 4 + 8);
		if (offset == 0 || offset > 10000000) {
			continue;
		}
		if (locate_wwww(data + offset, pointer, depth + 1, path_result)) {
			path_result[depth] = i;
			return 1;
		}
	}
	return 0;
}

int seekToken(byte * data, char * token) {
	int c = 0x3000;
	while(c--) {
		if (data[0] == token[0] && data[1] == token[1] && data[2] == token[2] && data[3] == token[3]) {
			return *(data + 11);
		}
		data++;
	}
	return 0;
}

byte * openFile(char * filename, int * fileSize) {
	FILE * fileptr;
	char fullFilename[128];
	byte * filedata = NULL;
	int _fileSize = 0;

	*fileSize = 0;

	sprintf(fullFilename, "assets/%s", filename);
	printf("Opening file %s \n", fullFilename);
	fileptr = fopen(fullFilename,"rb");
	if (fileptr == 0) {
		printf("File not found!\n");
		return 0;
	}
	fseek(fileptr, 0L, SEEK_END);
	_fileSize = ftell(fileptr);
	if (_fileSize == 0) {
		printf("Error - Empty File!\n");
		return 0;
	}
	fseek(fileptr, 0L, SEEK_SET);
	filedata = malloc(_fileSize);
	if (!fread(filedata, _fileSize, 1, fileptr)) {
		printf("Unable to read file!\n");
		return 0;
	};
	fclose(fileptr);
	printf("Read %d bytes.\n", _fileSize);
	*fileSize = _fileSize;
	return filedata;
}

byte * openFileBuffer(char * filename, int * fileSize) {
	byte * filedata = NULL;
	int _fileSize = 0;
	int i;

	for (i = 0; i < file_counter; i++) {
		if (strcmp(filename, g_file_assets[i].name) == 0) {
			//printf("Reloading %s from buffer...\n", filename);
			filedata = g_file_assets[i].content;
			*fileSize = g_file_assets[i].length;
			return filedata;
		}
	}

	filedata = openFile(filename, &_fileSize);
	if (filedata == 0) {
		return 0;
	}
	strcpy(g_file_assets[file_counter].name, filename);
	g_file_assets[file_counter].content = filedata;
	g_file_assets[file_counter].length = _fileSize;
	file_counter++;
	*fileSize = _fileSize;
	return filedata;
}

void clearFileBuffer() {
	int i;
	for (i = 0; i < file_counter; i++) {
		if (g_file_assets[i].content) {
			free(g_file_assets[i].content);
		}
		g_file_assets[i].content = 0;
		g_file_assets[i].length = 0;
	}
	file_counter = 0;
}

byte * seekNextCCB(byte *filedata) {
	byte *obj = filedata;
	int count = 10000;
	while(count--) {
		if (obj[0] == 'C' && obj[1] == 'C' && obj[2] == 'B' && obj[3] == ' ') {
			return obj;
		}
		obj++;
	}
	return 0;
}

void fileWrite(byte * data, int size) {
	FILE *ptr = fopen("output.bin","wb");
	fwrite(data, size, 1, ptr);
	fclose(ptr);
	printf("File saved: output.bin (%d bytes).\n", size);
}

/*
 * Import a TNFS 3DO TRK track file
 */
int read_tri_file(char * file) {
	unsigned char buffer[0x800];
	FILE *ptr;
	int i, j, k;
	int pos;
	int texCount;

	ptr = fopen(file,"rb");
	if (!ptr) {
		printf("File not found: %s\n", file);
		return 0;
	}

	// header flags
	fread(buffer, 44, 1, ptr);
	if (buffer[3] != 0xE) {
		printf("Error:\tInitRoad - Incorrect version number.  Continuing anyway!\n");
		return 0;
	}

	g_road_node_count = readFixed32(buffer, 4) * 4;
	g_road_finish_node = g_road_node_count - 0xb5;
	g_slice_mask = -1;

	// 0x13B4 block: Virtual road / RoadSplinePoint
	for (i = 0; i < 2400; i++) {
		fseek(ptr, i * 36 + 0x13B4, SEEK_SET);
		fread(buffer, 36, 1, ptr);

		track_data[i].roadLeftMargin = buffer[0];
		track_data[i].roadRightMargin = buffer[1];
		track_data[i].roadLeftFence = buffer[2];
		track_data[i].roadRightFence = buffer[3];

		track_data[i].num_lanes = buffer[4];
		track_data[i].fence_flag = buffer[5];
		track_data[i].shoulder_surface_type = buffer[6];
		track_data[i].item_mode = buffer[7];

		track_data[i].pos.x = readFixed32(buffer, 8);
		track_data[i].pos.y = readFixed32(buffer, 12);
		track_data[i].pos.z = readFixed32(buffer, 16);

		// 14-bit angles: unsigned 0 to 0x4000 (360)
		track_data[i].slope = readSigned16(buffer, 20);
		track_data[i].slant = readSigned16(buffer, 22);
		track_data[i].heading = readSigned16(buffer, 24);

		// 16-bit side normal: signed -32768 to +32768
		track_data[i].side_normal_x = readSigned16(buffer, 28);
		track_data[i].side_normal_y = readSigned16(buffer, 30);
		track_data[i].side_normal_z = readSigned16(buffer, 32);
	}

	// 0x16534 block: Track node AI speed reference
	for (i = 0; i < 600; i++) {
		fseek(ptr, i * 3 + 0x16534, SEEK_SET);
		fread(buffer, 3, 1, ptr);
		g_track_speed[i].top_speed = buffer[0];
		g_track_speed[i].legal_speed = buffer[1];
		g_track_speed[i].safe_speed = buffer[2];
	}

	// 0x16C50 scenery descriptors block
	for (i = 0; i < 64; i++) {
		fseek(ptr, i * 16 + 0x16C50, SEEK_SET);
		fread(buffer, 16, 1, ptr);
		g_scenery_models[i].type = buffer[1];
		g_scenery_models[i].texture_1 = buffer[2];
		g_scenery_models[i].texture_2 = buffer[3];
		g_scenery_models[i].width = ((float) buffer[0x5]) / 2;
		g_scenery_models[i].height = ((float) buffer[0xD]);
	}

	// 0x17050 scenery objects block
	for (i = 0; i < 1000; i++) {
		fseek(ptr, i * 16 + 0x17050, SEEK_SET);
		fread(buffer, 16, 1, ptr);
		g_scenery_object[i].track_slice = k = readFixed32(buffer, 0);
		g_scenery_object[i].object_model_id = buffer[4];

		g_scenery_object[i].orientation = 360 - (((float) track_data[k].heading) / 45.5);
		g_scenery_object[i].orientation += ((float)buffer[5]) / 256 * -360;

		g_scenery_object[i].position.x = readSigned16(buffer, 0xA) * 0x100 + track_data[k].pos.x;
		g_scenery_object[i].position.y = readSigned16(buffer, 0xC) * 0x100 + track_data[k].pos.y;
		g_scenery_object[i].position.z = readSigned16(buffer, 0xE) * 0x100 + track_data[k].pos.z;
	}

	// 0x1B000 terrain mesh block
	pos = 0x1B000;
	texCount = 0;
	k = 0;
	for (i = 0; i < 600; i++) {
		fseek(ptr, pos, SEEK_SET);
		fread(buffer, 0x800, 1, ptr);
		pos += readFixed32(buffer, 4);

		// fence texture
		g_fences[i] = buffer[0xD];

		// read texture Id
		for (j = 0; j < 10; j++) {
			g_terrain_texId[texCount] = buffer[j + 0xE];
			texCount++;
		}
		// read 55 3D points: 3DO version has 5 sets of 11 points, absolute coords, for each TRKD group
		for (j = 0; j < 33; j++) {
			g_terrain[k] = ((float) readFixed32(buffer, j * 4 + 0x30)) / 0x10000;
			k++;
		}
		for (j = 0; j < 33; j++) {
			g_terrain[k] = ((float) readFixed32(buffer, j * 4 + 0xC0)) / 0x10000;
			k++;
		}
		for (j = 0; j < 33; j++) {
			g_terrain[k] = ((float) readFixed32(buffer, j * 4 + 0x150)) / 0x10000;
			k++;
		}
		for (j = 0; j < 33; j++) {
			g_terrain[k] = ((float) readFixed32(buffer, j * 4 + 0x1E0)) / 0x10000;
			k++;
		}
		for (j = 0; j < 33; j++) {
			g_terrain[k] = ((float) readFixed32(buffer, j * 4 + 0x288)) / 0x10000;
			k++;
		}
		// read to end of chunk
		fseek(ptr, pos, SEEK_SET);
		fread(buffer, 0x800, 1, ptr);
		for (j = 0; j < 0x800; j++) {
			if (buffer[j] == 'T') {
				break;
			}
			pos++;
		}
	}

	fclose(ptr);
	printf("Loaded track %s with %d nodes.\n", file, g_road_node_count);
	return 1;
}

/*
 * Read a TNFS 3DO .spec text file
 */
int read_carspecs_file(char * carname) {
	FILE *ptr;
	int i;
	char file[80];
	int iVar1, uVar2, iVar3;

	sprintf(file, "assets/DriveData/CarData/%s.spec", carname);
	ptr = fopen(file,"r");
	if (!ptr) {
		printf("File not found: %s\n", file);
		return 0;
	}

	car_specs.mass_total = readTxtDecimal(ptr);
	car_specs.body_length = readTxtDecimal(ptr);
	car_specs.body_width = readTxtDecimal(ptr);
	car_specs.wheelbase = readTxtDecimal(ptr);
	car_specs.wheelbase_inv = math_inverse_value(car_specs.wheelbase);
	car_specs.wheeltrack = readTxtDecimal(ptr);
	car_specs.wheeltrack_inv = math_inverse_value(car_specs.wheeltrack); //not used

	car_specs.rear_weight_percentage = readTxtDecimal(ptr);
	car_specs.rear_weight_percentage = 0x8000; //all cars are 50/50 on 3do!

	car_specs.mass_rear = math_mul(car_specs.rear_weight_percentage, car_specs.mass_total);
	car_specs.mass_front = car_specs.mass_total - car_specs.mass_rear;

	car_specs.inverse_mass_front = math_inverse_value(car_specs.mass_front);
	car_specs.inverse_mass_rear = math_inverse_value(car_specs.mass_rear);
	car_specs.inverse_mass = math_inverse_value(car_specs.mass_total);

	car_specs.front_drive_percentage = readTxtDecimal(ptr);
	car_specs.front_brake_percentage = readTxtDecimal(ptr);
	car_specs.rear_brake_percentage = 0x10000 - car_specs.front_brake_percentage;

	car_specs.inertia_factor = readTxtDecimal(ptr);
	car_specs.body_roll_factor = readTxtDecimal(ptr);
	car_specs.body_pitch_factor = readTxtDecimal(ptr);
	car_specs.front_friction_factor = readTxtDecimal(ptr);
	car_specs.rear_friction_factor = readTxtDecimal(ptr);

	iVar1 = readTxtDecimal(ptr);
	iVar1 = fix8(iVar1);
	car_specs.frontGripMult = iVar1;
	car_specs.frontGripMult_inv = math_div_int(0x10000, iVar1);

	iVar1 = readTxtDecimal(ptr);
	iVar1 = fix8(iVar1);
	car_specs.rearGripMult = iVar1;
	car_specs.rearGripMult_inv = math_div_int(0x10000, iVar1);

	iVar1 = readTxtDecimal(ptr);
	car_specs.burnOutDiv = fix8(iVar1); //not used

	car_specs.lateral_accel_cutoff = readTxtDecimal(ptr); //not used

	car_specs.abs_equipped = readTxtDecimal(ptr);
	car_specs.tcs_equipped = readTxtDecimal(ptr);

	car_specs.throttle_on_ramp = readTxtDecimal(ptr) >> 8;
	car_specs.throttle_off_ramp = readTxtDecimal(ptr) >> 8;
	car_specs.brake_on_ramp_1 = readTxtDecimal(ptr) >> 8;
	car_specs.brake_on_ramp_2 = readTxtDecimal(ptr) >> 8;
	car_specs.brake_off_ramp_1 = readTxtDecimal(ptr) >> 8;
	car_specs.brake_off_ramp_2 = readTxtDecimal(ptr) >> 8;

	car_specs.centre_of_gravity_height = readTxtDecimal(ptr);
	car_specs.roll_axis_height = readTxtDecimal(ptr);

	car_specs.front_roll_stiffness = readTxtDecimal(ptr); //never used
	car_specs.rear_roll_stiffness = readTxtDecimal(ptr);
	car_specs.rear_roll_stiffness_inv = math_inverse_value(car_specs.rear_roll_stiffness);
	car_specs.front_roll_stiffness_inv = 0x10000 - car_specs.rear_roll_stiffness_inv;

	car_specs.weight_transfer_factor = car_specs.centre_of_gravity_height - car_specs.roll_axis_height;

	car_specs.drag = readTxtDecimal(ptr);
	car_specs.top_speed = readTxtDecimal(ptr);
	car_specs.final_drive = readTxtDecimal(ptr);
	car_specs.efficiency = readTxtDecimal(ptr);
	car_specs.wheel_roll_radius = readTxtDecimal(ptr);
	car_specs.inverse_wheel_radius = math_inverse_value(car_specs.wheel_roll_radius);

	car_specs.mps_to_rpm_factor = readTxtDecimal(ptr);

    iVar1 = readTxtDecimal(ptr); //brake start speed
    iVar3 = readTxtDecimal(ptr); //brake factor
    iVar1 = math_mul(0x71c7, iVar1);
    iVar3 = math_mul(0x4e07, iVar3);
    iVar3 = math_div(iVar3, iVar1);
    uVar2 = math_div(iVar1, iVar3 << 1);
    iVar1 = math_div(uVar2, g_gravity_const);
    car_specs.max_tire_coeff = iVar1; //unused

	car_specs.max_brake_force = readTxtDecimal(ptr);

	car_specs.cutoff_slip_angle = readTxtDecimal(ptr);
	car_specs.normal_coeff_loss = readTxtDecimal(ptr); //not used
	car_specs.number_of_gears = readTxtInt(ptr);

	for (i = 0; i < car_specs.number_of_gears; i++) {
		car_specs.gear_ratio_table[i] = readTxtDecimal(ptr);
	}

	for (i = 0; i < car_specs.number_of_gears - 3; i++) {
		car_specs.gear_upshift_rpm[i] = readTxtInt(ptr);
	}

	car_specs.rpm_redline = readTxtInt(ptr);
	car_specs.rpm_idle = readTxtInt(ptr);
	car_specs.ride_height = readTxtDecimal(ptr); //not used
	car_specs.centre_y = readTxtInt(ptr); //not used

	car_specs.maxAutoSteerAngle = readTxtDecimal(ptr);
	car_specs.autoRampMultShift = readTxtDecimal(ptr);
	car_specs.autoRampDivShift = readTxtDecimal(ptr);
	car_specs.steerModel = readTxtDecimal(ptr);
	car_specs.vel1_AS2 = readTxtDecimal(ptr);
	car_specs.vel2_AS2 = readTxtDecimal(ptr);
	car_specs.vel3_AS2 = readTxtDecimal(ptr);
	car_specs.vel4_AS2 = readTxtDecimal(ptr);
	car_specs.velRamp_AS2 = readTxtDecimal(ptr);
	car_specs.velAttenuate_AS2 = readTxtDecimal(ptr);
	car_specs.autoRampMultShift_AS2 = readTxtDecimal(ptr);
	car_specs.autoRampDivShift_AS2 = readTxtDecimal(ptr);

	car_specs.shift_timer = readTxtInt(ptr);
	car_specs.noGasRpmDec = readTxtInt(ptr);
	car_specs.gasRpmInc = readTxtInt(ptr);
	car_specs.clutchDropRpmDec = readTxtInt(ptr);
	car_specs.clutchDropRpmInc = readTxtInt(ptr);
	car_specs.negTorque = readTxtDecimal(ptr); //<< 1

	car_specs.torque_table_entries = readTxtInt(ptr);

	for (i = 0; i < (car_specs.torque_table_entries * 2); i++) {
		car_specs.torque_table[i] = readTxtInt(ptr);
	}

	fclose(ptr);
	printf("Loaded car specs file %s.\n", file);

	// read the tire grip files
	// front
	sprintf(file, "assets/DriveData/CarData/%s.TireF", carname);
	ptr = fopen(file,"rb");
	if (ptr) {
		fread(car_specs.grip_table, 512, 1, ptr);
		fclose(ptr);
		printf("Loaded tire grip file %s.\n", file);
	} else {
		printf("File not found: %s\n", file);
	}

	// rear
	sprintf(file, "assets/DriveData/CarData/%s.TireR", carname);
	ptr = fopen(file,"rb");
	if (ptr) {
		fread(car_specs.grip_table + 512, 512, 1, ptr);
		fclose(ptr);
		printf("Loaded tire grip file %s.\n", file);
	} else {
		printf("File not found: %s\n", file);
	}

	return 1;
}

/*
 * Import TNFS 3DO .TDDyn text file
 */
int read_tddyn_file(char *carname, tnfs_car_data *car) {
	char buffer[100];
	FILE *ptr;
	int i;
	int db[6];
	double dd[3];
	char file[80];

	sprintf(file, "assets/DriveData/CarData/%s.TDDyn", carname);
	ptr = fopen(file,"r");
	if (!ptr) {
		printf("File not found: %s\n", file);
		return 0;
	}

	car->collision_data.mass = readTxtDecimal(ptr);
	car->collision_data.moment_of_inertia = readTxtDecimal(ptr);

	readTxtLine(buffer, 99, ptr);
	readTxtLine(buffer, 99, ptr);
	sscanf(buffer, "%lf  %lf  %lf", &dd[0], &dd[1], &dd[2]);
	car->collision_data.size.x = dd[0] * 0x10000;
	car->collision_data.size.y = dd[1] * 0x10000;
	car->collision_data.size.z = dd[2] * 0x10000;

	readTxtLine(buffer, 99, ptr);
	readTxtLine(buffer, 99, ptr);
	sscanf(buffer, "%d %d %d %d %d %d", &db[0], &db[1], &db[2], &db[3], &db[4], &db[5]);
	for (i = 0; i < 6; i++) {
		car->top_speed_per_gear[i] = math_mul((int)db[i] * 0x10000, 0x71c4);
	}

	car->redline = readTxtDecimal(ptr);
	car->horn_freq = readTxtDecimal(ptr);

	car->handling_factor[0] = readTxtDecimal(ptr);
	car->handling_factor[1] = readTxtDecimal(ptr);
	car->handling_factor[2] = readTxtDecimal(ptr);

	car->speed_factor[0] = readTxtDecimal(ptr);
	car->speed_factor[1] = readTxtDecimal(ptr);
	car->speed_factor[2] = readTxtDecimal(ptr);

	for (i = 0; i < 100; i++) {
		car->power_curve[i] = readTxtDecimal(ptr);
	}

	car->field_168 = 0x120000;
	car->field_170 = 0x10000;
	car->pdn_max_rpm = 9000;
	car->pdn_number_of_gears = 6;

	fclose(ptr);
	printf("Loaded car specs file %s.\n", file);
	return 1;
}

/*
 * Import TNFS 3DO skills.* text file
 */
int read_skill_file(int skill) {
	FILE *ptr;
	char file[80];
	int i;

	sprintf(file, "assets/DriveData/skills.%d", skill);
	ptr = fopen(file,"r");
	if (!ptr) {
		printf("File not found: %s\n", file);
		return 0;
	}

	g_ai_skill_cfg.echo_stats = readTxtInt(ptr);

	g_ai_skill_cfg.opp_block_look_behind = readTxtInt(ptr);
	g_ai_skill_cfg.opp_lane_change_speeds  = readTxtDecimal(ptr);
	g_ai_skill_cfg.opp_oncoming_look_ahead = readTxtInt(ptr);
	g_ai_skill_cfg.opp_oncoming_corner_swerve = readTxtInt(ptr);
	g_ai_skill_cfg.opp_cut_corners = readTxtInt(ptr);

	g_ai_skill_cfg.opp_desired_speed_c = math_mul(0x10000, readTxtDecimal(ptr)); //m/s
	g_ai_skill_cfg.opp_desired_ahead = math_mul(0x10000, readTxtDecimal(ptr)) << 1; //m/s
	g_ai_skill_cfg.cop_warning_time = readTxtInt(ptr);
	g_ai_skill_cfg.max_player_runways = readTxtInt(ptr);

	g_ai_skill_cfg.penalty_for_warning = readTxtDecimal(ptr);
	g_ai_skill_cfg.penalty_for_ticket = readTxtDecimal(ptr);
	g_ai_skill_cfg.number_of_player_cars = readTxtInt(ptr);
	g_ai_skill_cfg.traffic_density = readTxtInt(ptr);
	g_ai_skill_cfg.number_of_traffic_cars = readTxtInt(ptr);

	g_ai_skill_cfg.traffic_speed_factors[0] = readTxtDecimal(ptr);
	g_ai_skill_cfg.traffic_speed_factors[1] = readTxtDecimal(ptr);
	g_ai_skill_cfg.traffic_speed_factors[2] = readTxtDecimal(ptr);
	g_ai_skill_cfg.traffic_speed_factors[3] = readTxtDecimal(ptr);

	g_ai_skill_cfg.traffic_base_speed = math_mul(0x471c, readTxtDecimal(ptr)); //convert km/h to m/s

	g_ai_skill_cfg.lane_slack[0] = readTxtDecimal(ptr);
	g_ai_skill_cfg.lane_slack[1] = readTxtDecimal(ptr);
	g_ai_skill_cfg.lane_slack[2] = readTxtDecimal(ptr);
	g_ai_skill_cfg.lane_slack[3] = readTxtDecimal(ptr);

	for (i = 0; i < 20; i++) {
		g_ai_skill_cfg.opponent_glue_0[i] = readTxtDecimal(ptr);
	}

	//constants
	g_ai_skill_cfg.opp_block_behind_distance = 0x50000;
	g_ai_skill_cfg.penalty_count = 3;

	fclose(ptr);
	printf("Loaded car specs file %s.\n", file);
	return 1;
}

void parse_ori3_data(tnfs_object3d * model, byte * ori3, byte * shpm, byte invertZ) {
	int verticesCount;
	int polyCount;
	float vertices[512];
	int texIndex[128];
	int texIdsGL[128];
	int value, i, k;

	unsigned char * obj;
	unsigned char * verticesBlock;
	unsigned char * polyBlock;

	if (ori3[0] != 'O' || shpm[0] != 'S') {
		printf("parse_ori3_data: invalid ori3/SHPM files!\n");
		return;
	}

	verticesCount = readFixed32(ori3, 0x10);
	verticesBlock = ori3 + readFixed32(ori3, 0x14);

	polyCount = readFixed32(ori3, 0x20);
	polyBlock = ori3 + readFixed32(ori3, 0x24);

	//texture block
	gfx_store_shpm_group(shpm, texIdsGL);

	//texture id data (shpm -> !ori)
	obj = (byte*) gfx_locateshape(shpm, "!ori");
	if (obj != 0) {
		obj += 0x40;
		for (i = 0; i < polyCount; i++) {
			value = obj[i * 8 + 7];
			texIndex[i] = texIdsGL[value];
		}
	}

	// vertices block
	obj = verticesBlock;
	for (i = 0; i < verticesCount; i++) {
		vertices[i * 3] = ((float) readFixed32(obj, 0)) / 0x80;
		vertices[i * 3 + 1] = ((float) readFixed32(obj, 4)) / 0x80;
		vertices[i * 3 + 2] = ((float) readFixed32(obj, 8)) / 0x80;
		if (invertZ) vertices[i * 3 + 2] *= -1;
		obj += 12;
	}

	// polygon block
	obj = polyBlock;
	k = 0;
	for (i = 0; i < polyCount; i++) {
		model->mesh[k].polyId = i;

		value = readFixed32(obj, 8);
		model->mesh[k].points[0] = vertices[value * 3];
		model->mesh[k].points[1] = vertices[value * 3 + 1];
		model->mesh[k].points[2] = vertices[value * 3 + 2];

		value = readFixed32(obj, 0xC);
		model->mesh[k].points[3] = vertices[value * 3];
		model->mesh[k].points[4] = vertices[value * 3 + 1];
		model->mesh[k].points[5] = vertices[value * 3 + 2];

		value = readFixed32(obj, 0x10);
		model->mesh[k].points[6] = vertices[value * 3];
		model->mesh[k].points[7] = vertices[value * 3 + 1];
		model->mesh[k].points[8] = vertices[value * 3 + 2];

		model->mesh[k].textureId = texIndex[i];
		model->mesh[k].texUv[0] = 0; model->mesh[k].texUv[1] = 0;
		model->mesh[k].texUv[2] = 1; model->mesh[k].texUv[3] = 0;
		model->mesh[k].texUv[4] = 1; model->mesh[k].texUv[5] = 1;
		k++;

		// convert quadrangle to 2 triangles
		if (obj[0] == 0 || obj[0] == 4) {
			model->mesh[k].polyId = i;

			value = readFixed32(obj, 8);
			model->mesh[k].points[0] = vertices[value * 3];
			model->mesh[k].points[1] = vertices[value * 3 + 1];
			model->mesh[k].points[2] = vertices[value * 3 + 2];

			value = readFixed32(obj, 0x10);
			model->mesh[k].points[3] = vertices[value * 3];
			model->mesh[k].points[4] = vertices[value * 3 + 1];
			model->mesh[k].points[5] = vertices[value * 3 + 2];

			value = readFixed32(obj, 0x14);
			model->mesh[k].points[6] = vertices[value * 3];
			model->mesh[k].points[7] = vertices[value * 3 + 1];
			model->mesh[k].points[8] = vertices[value * 3 + 2];

			model->mesh[k].textureId = texIndex[i];
			model->mesh[k].texUv[0] = 0; model->mesh[k].texUv[1] = 0;
			model->mesh[k].texUv[2] = 1; model->mesh[k].texUv[3] = 1;
			model->mesh[k].texUv[4] = 0; model->mesh[k].texUv[5] = 1;
			k++;
		}
		obj += 24;
	}
	model->numPolys = k;
}

/*
 * Read .WrapFam files
 */
int read_carmodel_file(char * carname, tnfs_carmodel3d * carmodel) {
	FILE * fileptr;
	char filename[80];
	int size = 0;
	int wpath[3];
	int i;
	int j;
	tnfs_polygon * poly;

	unsigned char * filedata;
	unsigned char * ori3;
	unsigned char * shpm;
	shpm_image * tex1;

	printf("Loading car WrapFam file - %s\n", carname);
	sprintf(filename, "assets/DriveData/CarData/%s.WrapFam", carname);
	fileptr = fopen(filename,"rb");
	if (fileptr == 0) {
		printf("File not found %s!\n", filename);
		return 0;
	}
	fseek(fileptr, 0L, SEEK_END);
	size = ftell(fileptr);
	fseek(fileptr, 0L, SEEK_SET);
	filedata = malloc(size);
	fread(filedata, size, 1, fileptr);
	fclose(fileptr);

	//'wwww'
	wpath[0] = 2;
	wpath[1] = 0;
	ori3 = read_wwww(filedata, wpath, 2);
	wpath[1] = 1;
	shpm = read_wwww(filedata, wpath, 2);
	parse_ori3_data(&carmodel->model, ori3, shpm, 0);

	// brake light texture
	tex1 = gfx_locateshape(shpm, "bkl1");
	if (tex1) {
		carmodel->brakeLightTexId = gfx_store_texture(shpm_image_convert(tex1, 0));
		carmodel->bkll = seekToken(ori3, "bkll");
		carmodel->bklr = seekToken(ori3, "bklr");
		carmodel->brakeTexId = carmodel->brakeLightTexId - 24;
	}

	// wheel texture
	tex1 = gfx_locateshape(shpm, "whl0");
	if (tex1) {
		carmodel->wheelTexId[0] = gfx_store_texture(shpm_image_convert(tex1, 0));
		carmodel->rt_rear = seekToken(ori3, "rt_r");
		carmodel->lt_rear = seekToken(ori3, "lt_r");
		carmodel->rt_frnt = seekToken(ori3, "rt_f");
		carmodel->lt_frnt = seekToken(ori3, "lt_f");

		tex1 = gfx_locateshape(shpm, "whl1");
		if (tex1) carmodel->wheelTexId[1] = gfx_store_texture(shpm_image_convert(tex1, 0));
		tex1 = gfx_locateshape(shpm, "whl2");
		if (tex1) carmodel->wheelTexId[2] = gfx_store_texture(shpm_image_convert(tex1, 0));

		// get fast spinning wheel texture
		for (i = 0; i < carmodel->model.numPolys; i++) {
			if (carmodel->rt_rear == carmodel->model.mesh[i].polyId) {
				carmodel->wheelTexId[3] = carmodel->model.mesh[i].textureId;
				break;
			}
		}
	}

	//cop siren lights
	if (strcmp("CopMust", carname) == 0) {
		tex1 = gfx_locateshape(shpm, "lrl1");
		if (tex1) {
			carmodel->copSirenLights[0] = gfx_store_texture(shpm_image_convert(tex1, 0));
			tex1 = gfx_locateshape(shpm, "lrr1");
			carmodel->copSirenLights[1] = gfx_store_texture(shpm_image_convert(tex1, 0));
			carmodel->lrl0 = seekToken(ori3, "lrl0");
			carmodel->lrr0 = seekToken(ori3, "lrr0");
		}
	}

	// FIX: scale up F512TR model
	if (strcmp("F512TR", carname) == 0) {
		for (i = 0; i < carmodel->model.numPolys; i++) {
			poly = &carmodel->model.mesh[i];
			for (j = 0; j < 3; j++) {
				poly->points[j * 3] *= 1.2f;
				poly->points[j * 3 + 1] *= 1.15f;
				poly->points[j * 3 + 2] *= 1.1f;
				poly->points[j * 3 + 2] += 0.2f;
			}
		}
	}

	free(filedata);
	printf("Loaded car model file %s.\n", filename);
	return 1;
}


int g_lod_sequence[12] = { 4, 8, 16, 20,   0, 12, 24, 32,  0, 12, 24, 32 };

/*
 * Read track PKT files
 */
int read_track_pkt_file(char * trackname) {
	FILE * fileptr;
	char filename[80];
	int size = 0;
	unsigned char * filedata;
	unsigned char * obj;
	unsigned char * ori3;
	unsigned char * shpm;
	int i, j, k;
	int wpath[4] = { 0, 0, 0, 0 };

	printf("Loading track PKT file - %s\n", trackname);
	sprintf(filename, "assets/DriveData/DriveArt/%s_PKT_000", trackname);
	fileptr = fopen(filename,"rb");
	if (fileptr == 0) {
		printf("File not found %s!\n", filename);
		return 0;
	}
	fseek(fileptr, 0L, SEEK_END);
	size = ftell(fileptr);
	fseek(fileptr, 0L, SEEK_SET);
	filedata = malloc(size);
	fread(filedata, size, 1, fileptr);
	fclose(fileptr);

	// Group 0: Terrain textures
	for (i = 0; i < 256; i++) {
		g_terrain_texPkt[i] = 0;
	}
	for (i = 0; i < filedata[0x23]; i++) {
		if (readFixed32(filedata, i * 4 + 0x24)) {
			wpath[1] = i;
		}
		k = 0;
		for (j = 0; j < 12; j++) {
			if (k == 4) break;
			wpath[2] = g_lod_sequence[j];
			obj = read_wwww(filedata, wpath, 3);
			if (obj == 0) continue;
			g_terrain_texPkt[i * 4 + k] = gfx_store_ccb((ccb_chunk *) obj, 0xFF);
			if (g_terrain_texPkt[i * 4 + k]) {
				k++;
			}
		}
	}

	// Group 1: Scenery textures
	for (i = 0; i < 256; i+=4) {
		wpath[0] = 1;
		wpath[1] = i;
		obj = read_wwww(filedata, wpath, 2);
		if (obj == 0) continue;
		obj = seekNextCCB(obj);
		g_scenery_texPkt[i] = gfx_store_ccb((ccb_chunk *) obj, 0xFF);
	}

	// Group 2: Complex 3D scenery models
	g_scenery_models_count = 0;
	for (i = 0; i < 1; i++) { //its just one
		wpath[0] = 2;
		wpath[1] = i;
		wpath[2] = 2; //highest quality
		wpath[3] = 0;
		ori3 = read_wwww(filedata, wpath, 4);
		if (ori3 == 0) continue;
		wpath[3] = 1;
		shpm = read_wwww(filedata, wpath, 4);
		parse_ori3_data(&g_scenery_3d_objects[i], ori3, shpm, 1);
		g_scenery_models_count++;
	}

	// Group 3: Horizon textures (lower part)
	for (i = 0; i < 6; i++) {
		wpath[0] = 3;
		wpath[1] = i;
		obj = read_wwww(filedata, wpath, 2);
		g_horizon_texPkt[i] = gfx_store_ccb((ccb_chunk *) obj, 0xFF);
	}

	// Group 4: Horizon textures (upper part)
	for (i = 0; i < 6; i++) {
		wpath[0] = 4;
		wpath[1] = i;
		obj = read_wwww(filedata, wpath, 2);
		g_horizon_texPkt[i + 6] = gfx_store_ccb((ccb_chunk *) obj, 0xFF);
	}

	printf("Loaded track PKT file %s.\n", filename);
	free(filedata);
	return 1;
}


void read_dash_constants(char * carname) {
	FILE *ptr;
	int i;
	char auxstr[120];

	g_dash_constants.num_panels = 0;

	sprintf(auxstr, "assets/DriveData/CarData/%s.dashConstants", carname);
	ptr = fopen(auxstr,"r");
	if (!ptr) {
		printf("File not found: %s\n", auxstr);
		return;
	}

	readTxtSkipLine(ptr);
	g_dash_constants.num_panels = readTxtInt(ptr);

	for (i = 0; i < g_dash_constants.num_panels; i++) {
		readTxtSkipLine(ptr);
		g_dash_constants.position[i].x = readTxtInt(ptr);
		readTxtSkipLine(ptr);
		g_dash_constants.position[i].y = readTxtInt(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
		readTxtSkipLine(ptr);
	}

	for (i = 0; i < 100; i++) {
		readTxtLine((char*)&auxstr, 120, ptr);
		if (strncmp((char*)&auxstr, "TACHO", 5) == 0) {
			g_dash_constants.tacho_pos_x = (160 + readTxtInt(ptr)) * 2.5;
			readTxtLine((char*)&auxstr, 120, ptr);
			g_dash_constants.tacho_pos_y = (240 + readTxtInt(ptr)) * 2.5;
			break;
		}
	}

	fclose(ptr);
}

int read_hud_dash_file(char * carname) {
	int i;
	char filename[80];
	int wpath[2] = { 3, 3 };
	ccb_chunk * ccb;
	image_data * image;
	int aux;

	sprintf((char*)&filename, "DriveData/CarData/%s.BigdashFam", carname);
	byte *dashfile = openFileBuffer(filename, &i);
	if (dashfile == 0) {
		return 0;
	}

	// tacho
	ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
	g_hud_texPkt[13] = gfx_store_ccb(ccb, 0xAA);
	wpath[1] = 4;
	ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
	g_hud_texPkt[14] = gfx_store_ccb(ccb, 0x44);

	// digits
	for (i = 0; i < 10; i++) {
		wpath[0] = 2;
		wpath[1] = i;
		ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
		g_hud_texPkt[i] = gfx_store_ccb(ccb, 0xFF);
	}

	// R, N
	wpath[0] = 4;
	wpath[1] = 10;
	ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
	g_hud_texPkt[10] = gfx_store_ccb(ccb, 0xFF);
	wpath[0] = 4;
	wpath[1] = 11;
	ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
	g_hud_texPkt[11] = gfx_store_ccb(ccb, 0xFF);

	// dash
	if (g_dash_constants.num_panels == 0) {
		return 1;
	}
	aux = 320*240*4;
	image = (image_data *) malloc(aux + 32);
	memset(image->rgba, 0, aux);
	image->size = aux;
	image->width = 320;
	image->height = 240;

	for (i = 0; i < g_dash_constants.num_panels; i++) {
		wpath[0] = 1;
		wpath[1] = i;
		ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
		ccb_parse_header(ccb);
		ccb_draw_to_buffer(image->rgba,
				160 + g_dash_constants.position[i].x,
				240 + g_dash_constants.position[i].y);
	}
	g_dash_texPkt[0] = gfx_store_texture(image);
	free(image);

	//steering wheel
	wpath[0] = 1;
	wpath[1] = g_dash_constants.num_panels + 2;
	ccb = (ccb_chunk*) read_wwww(dashfile, wpath, 2);
	g_dash_texPkt[1] = gfx_store_ccb(ccb, 0xFF);

	return 1;
}

int read_sim_common_art_file() {
	int i;
	int wpath[2] = { 0, 0 };
	ccb_chunk * img;

	byte *artfile = openFileBuffer("DriveData/DriveArt/SimCommonArt.Fam", &i);
	if (artfile == 0) {
		return 0;
	}

	// smoke puffs
	for (i = 0; i < 4; i++) {
		wpath[0] = i;
		img = (ccb_chunk*) read_wwww(artfile, wpath, 1);
		g_smoke_texPkt[i] = gfx_store_ccb(img, 0x22);
	}

	return 1;
}
