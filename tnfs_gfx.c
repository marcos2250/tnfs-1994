#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include "tnfs_gfx.h"
#include "tnfs_base.h"
#include "tnfs_files.h"
#include "ccb.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

byte g_backbuffer[307200];
shpm_image * g_shape; 
byte g_fontdata[4464];
byte * g_filedata;
int g_filesize = 0;

GLfloat matrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
vector3f cam_orientation = { 0, 0, 0 };

int gfx_init_stuff() {
	FILE * fileptr;
	// initialize Trixie font
	fileptr = fopen("assets/frontend/display/Trixie.3fn","rb");
	if (fileptr == 0) {
		printf("File not found: assets/frontend/display/Trixie.3fn\n");
		printf("In order to play, you should unpack TNFS CD contents into 'assets' folder!\n");
		return 0;
	}
	if (!fread(&g_fontdata, 4464, 1, fileptr)) {
		printf("Unable to read file!\n");
		return 0;
	};
	fclose(fileptr);
	return 1;
}

shpm_image * gfx_locateshape(byte *data, char *shapeid) {
	byte *ptr;
	byte *label;
	int count;
	int offset;

	ptr = data;
	count = ptr[11] + (ptr[10] << 8) + (ptr[9] << 16) + (ptr[8] << 24);
	ptr += 0x10;

	while (count) {
		count--;
		label = ptr;
		offset = ptr[7] + (ptr[6] << 8) + (ptr[5] << 16) + (ptr[4] << 24);
		if (*(int*)shapeid == *(int*)label) {
			ptr = data + offset;
			return (shpm_image *)(ptr);
		}
		ptr += 8;
	}

	printf("locateshape - \'%-4.4s\' SHAPE NOT FOUND\r\n",shapeid);
	return 0;
}

void gfx_readshape_9444(byte * data, char * label) {
	g_shape = gfx_locateshape(data, label);
}

void gfx_clear() {
	int i = 0;
	while (i < 307200) {
		g_backbuffer[i++] = 4;
		g_backbuffer[i++] = 4;
		g_backbuffer[i++] = 4;
		g_backbuffer[i++] = 0xFF;
	}
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void gfx_write_alpha_channel(byte *data, int size, byte alpha) {
	int i = 0;
	while (i < size) {
		data[i + 3] = alpha;
		if (data[i] < 4 && data[i + 1] < 4 && data[i + 2] < 4) {
			data[i + 3] = 0;
		}
		i += 4;
	}
}

byte * gfx_openfile_9594(char * filename, int mode) {
	g_filedata = openFileBuffer(filename, &g_filesize);
	return g_filedata;
}

byte * gfx_openfile_96ec(char * filename, int mode) {
	g_filedata = openFileBuffer(filename, &g_filesize);
	return g_filedata;
}

void gfx_set_filedata(byte * ptr) {
	g_filedata = ptr;
}

/*** FRONTEND UI ***/

void gfx_draw_shpm(shpm_image * shpm, int posX, int posY) {
	if (!shpm_parse_header(shpm)) {
		return;
	}
	ccb_draw_to_buffer((byte*) &g_backbuffer, posX, posY);
}

void gfx_draw_ccb(ccb_chunk *ccb, int left, int top) {
	if (!ccb_parse_header(ccb)) {
		return;
	}
	ccb_draw_to_buffer((byte*) &g_backbuffer, left, top);
}

void gfx_draw_3sh(char * file, char * label) {
	g_filedata = openFileBuffer(file, &g_filesize);
	g_shape = gfx_locateshape(g_filedata, label);
	gfx_draw_shpm(g_shape, 0, 0);
}

void gfx_draw_cel(char * file) { 
	int filesize;
	byte * filedata;
	filedata = openFileBuffer(file, &filesize);
	gfx_draw_ccb((ccb_chunk*) filedata, 0, 0);
}

void gfx_drawshape_94f4() {
	short top, left;
	left = __builtin_bswap16(g_shape->left_le);
	top = __builtin_bswap16(g_shape->top_le);
	gfx_draw_shpm(g_shape, left, top);
}

//void gfx_drawshape_94f4_at(cel_bitmap * cel, int left, int top) {
void gfx_drawshape_94f4_at(int left, int top) {
	gfx_draw_shpm(g_shape, left, top);
}

void gfx_drawshape_950c() {
	short top, left;
	left = __builtin_bswap16(g_shape->left_le);
	top = __builtin_bswap16(g_shape->top_le);
	gfx_draw_shpm(g_shape, left, top);
}

void gfx_drawshape_95a0() {
	gfx_draw_shpm(g_shape, 0, 0);
}

void gfx_drawshape_96f8(shpm_image * shape) {
	gfx_draw_shpm(shape, 0, 0);
}

void fileView_drawImage(byte * file, int pos) {
	shpm_image * shape;
	ccb_chunk * ccb;
	byte * obj = file;
	obj += pos;

	gfx_clear();
	gfx_write_alpha_channel(g_backbuffer, 307200, 0xFF);
	glClearColor(0, 0, 0, 0xFF);
	glClear(GL_COLOR_BUFFER_BIT);

	if (obj[0] == 'C' && obj[1] == 'C' && obj[2] == 'B') {
		printf("CCB at 0x%x\n", pos);
		ccb = (ccb_chunk*) obj;
		gfx_draw_ccb(ccb, 0, -990);
	} else if (obj[0] == 'S' && obj[1] == 'H' && obj[2] == 'P' && obj[3] == 'M') {
		printf("SHPM at 0x%x\n", pos);
		shape = (shpm_image*) obj;
		gfx_draw_shpm(shape, 0, -990);
	} else {
		printf("fileView_drawImage: unknown token at 0x%x\n", pos);
	}
}


/*** FRONTEND TEXT ***/

void gfx_print_char(byte * data, int ix, short x, short y) {
	int bx,by,n,p;
	byte * output;

	n = 0;
	for (by = 0; by < 11; by++) {
		n = by * 320 + ix;
		output = (((240 - by - y + 1) * 320) + x) * 4 + g_backbuffer;
		for (bx = 0; bx < 3; bx++) {
			p = data[n] & 0xF0;
			*output |= p;
			output++;
			*output |= p;
			output++;
			*output |= p;
			output++;
			*output |= 0;
			output++;

			p = (data[n] & 0xF) << 4;
			*output |= p;
			output++;
			*output |= p;
			output++;
			*output |= p;
			output++;
			*output |= 0;
			output++;
			
			n++;
		}	
	}
}

void gfx_draw_text_9500(char *text, int x, int y) {
	int number;
	int ix;
	byte code;
	byte * data = (byte*)&g_fontdata;

	number = 0;
	while(1) {
		code = text[number];
		if (code == 0) break;

		ix = (code - 33) * 4 + 0x24;
		ix = data[ix + 3] | (data[ix + 2] << 8);
		ix >>= 1;
		ix += 624;

		if (code != 32)
			gfx_print_char(data, ix, x, y);

		x += 7;
		number++; 
	}	
}

/*** SIM MODE ***/

int gfx_store_texture(image_data * image) {
	unsigned int texId = 0;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->rgba);
	return texId;
}

int gfx_store_shpm_group(byte * shpm, int * texIdsGL) {
	byte * obj;
	byte * tex = 0;
	image_data * data = 0;
	int numEntries = 0;
	byte * plt0 = 0;
	int numPluts = 0;
	int i;

	if (shpm == 0 || shpm[0] != 'S' || shpm[1] != 'H' || shpm[2] != 'P' || shpm[3] != 'M') {
		printf("gfx_store_shpm_group: Not a SHPM chunk!\n");
		return 0;
	}

	plt0 = 0;

	numEntries = shpm[11] + (shpm[10] << 8) - 1;

	// locate alternate palettes
	obj = shpm + 16;
	for (i = 0; i < numEntries; i++) {
		if (obj[0] == 'p' && obj[1] == 'l' && obj[2] == 't') {
			plt0 = obj[7] + (obj[6] << 8) + (obj[5] << 16) + (obj[4] << 24) + shpm;
			break;
		}
		obj += 8;
	}

	// load all shapes, for each palette
	if (numPluts < 1) numPluts = 1;
	obj = shpm + 16;
	//for (j = 0; j < numPluts; j++) {
		for (i = 0; i < numEntries; i++) {
			tex = obj[7] + (obj[6] << 8) + (obj[5] << 16) + (obj[4] << 24) + shpm;
			data = shpm_image_convert((shpm_image*) tex, (shpm_image*) plt0);
			if (!data) {
				continue;
			}
			texIdsGL[i] = gfx_store_texture(data);
			obj += 8;
			free(data);
		}
	//}

	return numEntries;
}

int gfx_store_ccb(ccb_chunk *ccb, byte alpha) {
	image_data * data = 0;
	int id = 0;
	if (ccb == 0 || ccb->id[0] != 'C' || ccb->id[1] != 'C' || ccb->id[2] != 'B' || ccb->id[3] != ' ') {
		printf("gfx_store_ccb: Not a CCB file!\n");
		return 0;
	}
	data = ccb_image_convert(ccb);
	if (data == 0) {
		return 0;
	}
	if (alpha != 0xFF) {
		gfx_write_alpha_channel(data->rgba, data->size, alpha);
	}
	id = gfx_store_texture(data);
	free(data);
	return id;
}


void gfx_drawSmoke() {
	float w;
	tnfs_smoke_puff * smoke;
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	for (int i = 0; i < SMOKE_PUFFS; i++) {
		smoke = &g_smoke[i];
		if (smoke->time <= 0) continue;
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		if (cam_orientation.x != 0) {
			glRotatef(cam_orientation.x, 1, 0, 0);
		}
		if (cam_orientation.z != 0) {
			glRotatef(cam_orientation.z, 0, 0, 1);
		}
		glRotatef(cam_orientation.y, 0, 1, 0);
		glTranslatef(((float) (smoke->position.x - camera.position.x)) / 0x10000,
					 ((float) (smoke->position.y - camera.position.y)) / 0x10000,
					 ((float)(-smoke->position.z + camera.position.z)) / 0x10000);
		glRotatef(-cam_orientation.y, 0, 1, 0);

		w = (smoke->time / 256);
		glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glActiveTexture(GL_TEXTURE0);
		w = 3 - (w * 2);
		glBindTexture(GL_TEXTURE_2D, g_smoke_texPkt[smoke->texId]);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2d(0, 0);
		glVertex3d(-w, w, 0);
		glTexCoord2d(0, 1);
		glVertex3d(-w, 0, 0);
		glTexCoord2d(1, 0);
		glVertex3d(w, w, 0);
		glTexCoord2d(1, 1);
		glVertex3d(w, 0, 0);
		glEnd();
	}
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void gfx_drawShadows() {
	tnfs_car_data * car;
	float h, w, l;

	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0.0f, 0.0f, 0.0, 0.3f);
	glPolygonMode(GL_FRONT, GL_FILL);

	for (int i = 0; i < 8; i++) {
		car = &g_car_array[i];
		if ((car->field_4e9 & 4) == 0) {
			continue;
		}
		if (abs(car->track_slice - camera.track_slice) > 8) {
			continue;
		}
		tnfs_car_shadow_update(car, 0);

		glMatrixMode(GL_MODELVIEW);
		matrix[0] = (float) car->matrix.ax / 0x10000;
		matrix[1] = 0;
		matrix[2] = (float) -car->matrix.az / 0x10000;
		matrix[3] = 0;
		matrix[4] = 0;
		matrix[5] = 1;
		matrix[6] = 0;
		matrix[7] = 0;
		matrix[8] = (float) car->matrix.cx / 0x10000;
		matrix[9] = 0;
		matrix[10] = (float) -car->matrix.cz / 0x10000;
		matrix[11] = 0;
		matrix[12] = ((float) car->position.x) / 0x10000;
		matrix[13] = 0;
		matrix[14] = ((float) -car->position.z) / 0x10000;
		matrix[15] = 1;

		glLoadIdentity();
		if (cam_orientation.x != 0) {
			glRotatef(cam_orientation.x, 1, 0, 0);
		}
		if (cam_orientation.z != 0) {
			glRotatef(cam_orientation.z, 0, 0, 1);
		}
		glRotatef(cam_orientation.y, 0, 1, 0);
		glTranslatef(((float) -camera.position.x) / 0x10000, ((float) -camera.position.y) / 0x10000, ((float) camera.position.z) / 0x10000);
		glMultMatrixf(matrix);

		w = ((float)car->car_width) / 0x10000 / 2;
		l = ((float)car->car_length) / 0x10000 / 2;
		glBegin(GL_TRIANGLE_STRIP);
		h = ((float)g_shadow_points[3].y) / 0x10000;
		glVertex3f(-w, h, +l);
		h = ((float)g_shadow_points[0].y) / 0x10000;
		glVertex3f(+w, h, +l);
		h = ((float)g_shadow_points[2].y) / 0x10000;
		glVertex3f(-w, h, -l);
		h = ((float)g_shadow_points[1].y) / 0x10000;
		glVertex3f(+w, h, -l);
		glEnd();
	}
	glDisable(GL_BLEND);
}

int wheelAnim = 0;

int gfx_getWheelTexture(tnfs_car_data * car, tnfs_carmodel3d * carModel, int isFront) {
	if (wheelAnim < -0x100000) wheelAnim = 0x100000;
	wheelAnim -= car->speed;
	if (isFront) {
		if (car->speed < 0x100000) {
			if (car->speed < 0x1000) {
				return carModel->wheelTexId[0];
			}
			return wheelAnim < 0 ? carModel->wheelTexId[2] : carModel->wheelTexId[1];
		}
	} else {
		if (!is_drifting && car->speed < 0x100000) {
			if (car->speed < 0x1000 || car->handbrake) {
				return carModel->wheelTexId[0];
			}
			return wheelAnim < 0 ? carModel->wheelTexId[2] : carModel->wheelTexId[1];
		}
	}
	return carModel->wheelTexId[3];
}

void gfx_drawCarWheel(tnfs_car_data * car, tnfs_carmodel3d * carModel, int isFront) {
	float steer, width, pX, pZ;
	int viewAngle = math_angle_wrap(car->angle.y - camera.orientation.y);

	if (car->car_model_id == 1 && isFront) {
		//FIX: diablo's narrow front wheel track
		pX = 0.9f;
	} else {
		pX = ((float) car->car_specs_ptr->body_width) / 0x10000 / 2;
	}
	pZ = ((float) car->car_specs_ptr->wheelbase) / 0x10000 / 2;

	if (isFront) {
		steer = ((float) (car->steer_angle >> 8)) / 80000;
	} else {
		steer = 0;
	}
	width = 0.35f;

	if (viewAngle > 0x800000) {
		pX *= -1;
		width *= -1;
		steer *= -1;
	}
	if (!isFront) {
		pZ *= -1;
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, gfx_getWheelTexture(car, carModel, isFront));
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(0, 0);
	glVertex3d(pX + steer, 0.7f, pZ + width);
	glTexCoord2d(0, 1);
	glVertex3d(pX - steer, 0.7f, pZ - width);
	glTexCoord2d(1, 0);
	glVertex3d(pX + steer, 0, pZ + width);
	glTexCoord2d(1, 1);
	glVertex3d(pX - steer, 0, pZ - width);
	glEnd();
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

void gfx_drawVehicle(tnfs_car_data * car) {
	tnfs_carmodel3d * carModel;
	tnfs_polygon * poly;
	int textureId;

	// TNFS uses LHS, convert to OpenGL's RHS
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = (float) car->matrix.ax / 0x10000;
	matrix[1] = (float) car->matrix.ay / 0x10000;
	matrix[2] = (float) -car->matrix.az / 0x10000;
	matrix[3] = 0;
	matrix[4] = (float) car->matrix.bx / 0x10000;
	matrix[5] = (float) car->matrix.by / 0x10000;
	matrix[6] = (float) -car->matrix.bz / 0x10000;
	matrix[7] = 0;
	matrix[8] = (float) car->matrix.cx / 0x10000;
	matrix[9] = (float) car->matrix.cy / 0x10000;
	matrix[10] = (float) -car->matrix.cz / 0x10000;
	matrix[11] = 0;
	matrix[12] = ((float) car->position.x) / 0x10000;
	matrix[13] = ((float) car->position.y) / 0x10000;
	matrix[14] = ((float) -car->position.z) / 0x10000;
	matrix[15] = 1;

	glLoadIdentity();
	if (cam_orientation.x != 0) {
		glRotatef(cam_orientation.x, 1, 0, 0);
	}
	if (cam_orientation.z != 0) {
		glRotatef(cam_orientation.z, 0, 0, 1);
	}
	glRotatef(cam_orientation.y, 0, 1, 0);
	glTranslatef(((float) -camera.position.x) / 0x10000, ((float) -camera.position.y) / 0x10000, ((float) camera.position.z) / 0x10000);
	glMultMatrixf(matrix);

	carModel = &g_carmodels[car->car_model_id];
	glColor3f(1.0f, 1.0f, 1.0);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < carModel->model.numPolys; i++) {
		poly = &carModel->model.mesh[i];
		textureId = poly->textureId;
		if (car->car_id == 0) {
			// brake lights on
			if (carModel->bkll == poly->polyId || carModel->bklr == poly->polyId) {
				if (g_control_brake) textureId = carModel->brakeLightTexId;
			}
			// render wheels later
			if (carModel->rt_frnt == poly->polyId || carModel->lt_frnt == poly->polyId) {
				continue;
			}
			if (carModel->rt_rear == poly->polyId || carModel->lt_rear == poly->polyId) {
				continue;
			}
		}
		// cop lights
		if ((car->car_model_id == 8) && ((car->ai_state & 0x408) == 0x408)) {
			if (carModel->lrl0 == poly->polyId && (iSimTimeClock & 8) == 0) {
				textureId = carModel->copSirenLights[0];
			} else if (carModel->lrr0 == poly->polyId  && (iSimTimeClock & 8) != 0) {
				textureId = carModel->copSirenLights[1];
			}
		}

		glBindTexture(GL_TEXTURE_2D, textureId);
		glBegin(GL_TRIANGLES);
		glTexCoord2d(poly->texUv[0], poly->texUv[1]);
		glVertex3d(poly->points[0], poly->points[1], poly->points[2]);
		glTexCoord2d(poly->texUv[2], poly->texUv[3]);
		glVertex3d(poly->points[3], poly->points[4], poly->points[5]);
		glTexCoord2d(poly->texUv[4], poly->texUv[5]);
		glVertex3d(poly->points[6], poly->points[7], poly->points[8]);
		glEnd();
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// draw wheels
	if (car->car_id == 0) {
		gfx_drawCarWheel(car, carModel, 0);
		gfx_drawCarWheel(car, carModel, 1);
	}
}

void gfx_drawHorizon() {
	int layer;
	int texture;
	int i;
	float x1, x2, y1, y2, z1, z2;

	if (g_track_sel == 2) {
		glClearColor(0.5f, 0.7f, 0.9f, 1.0f); //city = cyan
	} else {
		glClearColor(0.05f, 0.2f, 0.5f, 1.0f); //blue
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (cam_orientation.x != 0) {
		glRotatef(cam_orientation.x, 1, 0, 0);
	}
	if (cam_orientation.z != 0) {
		glRotatef(cam_orientation.z, 0, 0, 1);
	}
	glRotatef(cam_orientation.y, 0, 1, 0);

	glColor3f(1.0f, 1.0f, 1.0);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT, GL_FILL);
	glActiveTexture(GL_TEXTURE0);

	layer = 2;
	while (layer--) {
		for (i = 0; i < 12; i++) {
			x1 = cosf(i * 0.52359f);
			z1 = sinf(i * 0.52359f);
			x2 = cosf((i+1) * 0.52359f);
			z2 = sinf((i+1) * 0.52359f);
			texture = i % 6;

			if (layer) {
				texture = g_horizon_texPkt[texture+6];
				y1 = 0.55f;
				y2 = 0;
				x1 *= 2;
				x2 *= 2;
				z1 *= 2;
				z2 *= 2;
			} else {
				texture = g_horizon_texPkt[texture];
				y1 = 0.06f;
				y2 = -0.05f;
			}
			if (texture == 0) continue;
			glBindTexture(GL_TEXTURE_2D, texture);
			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2d(0, 1);
			glVertex3d(x2, y2, z2);
			glTexCoord2d(1, 1);
			glVertex3d(x2, y1, z2);
			glTexCoord2d(0, 0);
			glVertex3d(x1, y2, z1);
			glTexCoord2d(1, 0);
			glVertex3d(x1, y1, z1);
			glEnd();
		}
	}
}

void gfx_drawSimpleObject(tnfs_scenery_object * object) {
	tnfs_scenery_descriptor * model;
	float w, h, t;
	model = &g_scenery_models[object->object_model_id];

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (cam_orientation.x != 0) {
		glRotatef(cam_orientation.x, 1, 0, 0);
	}
	if (cam_orientation.z != 0) {
		glRotatef(cam_orientation.z, 0, 0, 1);
	}
	glRotatef(cam_orientation.y, 0, 1, 0);
	glTranslatef(((float) (object->position.x - camera.position.x)) / 0x10000,
				 ((float) (object->position.y - camera.position.y)) / 0x10000,
			     ((float)(-object->position.z + camera.position.z)) / 0x10000);
	glRotatef(object->orientation, 0, 1, 0);

	glColor3f(1.0f, 1.0f, 1.0);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glActiveTexture(GL_TEXTURE0);

	h = model->height;
	w = model->width;
	t = g_scenery_texPkt[model->texture_1];
	glBindTexture(GL_TEXTURE_2D, t);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(0, 0);
	glVertex3d(-w, h, 0);
	glTexCoord2d(0, 1);
	glVertex3d(-w, 0, 0);
	glTexCoord2d(1, 0);
	glVertex3d(w, h, 0);
	glTexCoord2d(1, 1);
	glVertex3d(w, 0, 0);
	glEnd();

	if (model->type == 6) {
		t = g_scenery_texPkt[model->texture_2];
		glBindTexture(GL_TEXTURE_2D, t);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2d(0, 0);
		glVertex3d(w, h, 0);
		glTexCoord2d(0, 1);
		glVertex3d(w, 0, 0);
		glTexCoord2d(1, 0);
		glVertex3d(w, h, -w);
		glTexCoord2d(1, 1);
		glVertex3d(w, 0, -w);
		glEnd();
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void gfx_drawObject(tnfs_scenery_object * object) {
	tnfs_polygon * poly;
	tnfs_object3d * model = &g_scenery_3d_objects[0];

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (cam_orientation.x != 0) {
		glRotatef(cam_orientation.x, 1, 0, 0);
	}
	if (cam_orientation.z != 0) {
		glRotatef(cam_orientation.z, 0, 0, 1);
	}
	glRotatef(cam_orientation.y, 0, 1, 0);
	glTranslatef(((float) (object->position.x - camera.position.x)) / 0x10000,
				 ((float) (object->position.y - camera.position.y)) / 0x10000,
			     ((float)(-object->position.z + camera.position.z)) / 0x10000);
	glRotatef(object->orientation, 0, 1, 0);
	glScalef(8, 8, 8);

	glColor3f(1.0f, 1.0f, 1.0);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < model->numPolys; i++) {
		poly = &model->mesh[i];
		glBindTexture(GL_TEXTURE_2D, poly->textureId);
		glBegin(GL_TRIANGLES);
		glTexCoord2d(poly->texUv[0], poly->texUv[1]);
		glVertex3d(poly->points[0], poly->points[1], poly->points[2]);
		glTexCoord2d(poly->texUv[2], poly->texUv[3]);
		glVertex3d(poly->points[3], poly->points[4], poly->points[5]);
		glTexCoord2d(poly->texUv[4], poly->texUv[5]);
		glVertex3d(poly->points[6], poly->points[7], poly->points[8]);
		glEnd();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void gfx_drawRoad() {
	int x;
	int p1, p2;
	int chunk, strip, slice, texture;
	int count;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (cam_orientation.x != 0) {
		glRotatef(cam_orientation.x, 1, 0, 0);
	}
	if (cam_orientation.z != 0) {
		glRotatef(cam_orientation.z, 0, 0, 1);
	}
	glRotatef(cam_orientation.y, 0, 1, 0);
	glTranslatef(((float) -camera.position.x) / 0x10000, ((float) -camera.position.y) / 0x10000, ((float) camera.position.z) / 0x10000);

	glColor3f(1.0f, 1.0f, 1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glActiveTexture(GL_TEXTURE0);

	// terrain
	chunk = (camera.track_slice >> 2) + 19;
	count = 20;
	while(count--) {
		for (strip = 0; strip < 10; strip++) {
			texture = g_terrain_texId[chunk * 10 + strip];
			if (texture == 0) {
				continue; // no texture, no render
			}

			x = chunk * 5 * 33;
			if (strip == 5) {
				p1 = 0; p2 = 6;
			} else {
				p1 = strip; p2 = strip + 1;
			}
			p1 *= 3; p2 *= 3;
			p1 += x; p2 += x;

			texture = g_terrain_texPkt[texture * 4];
		    for (slice = 0; slice < 4; slice++) {
		    	glBindTexture(GL_TEXTURE_2D, texture++);
				glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2d(0, 1);
				glVertex3f(g_terrain[p1], g_terrain[p1 + 1], g_terrain[p1 + 2]);
				glTexCoord2d(0, 0);
				glVertex3f(g_terrain[p2], g_terrain[p2 + 1], g_terrain[p2 + 2]);
				p1 += 33; p2 += 33;
				glTexCoord2d(1, 1);
				glVertex3f(g_terrain[p1], g_terrain[p1 + 1], g_terrain[p1 + 2]);
				glTexCoord2d(1, 0);
				glVertex3f(g_terrain[p2], g_terrain[p2 + 1], g_terrain[p2 + 2]);
				glEnd();
		    }
		}
		chunk--;
	}

	// fences
	chunk = (camera.track_slice >> 2) + 9;
	count = 10;
	while (count--) {
		p1 = chunk * 4;
		p2 = p1 + 1;
		texture = g_fences[chunk] & 0x3F;
		if (texture < 42) texture = 48; //FIXME wrong fence texture id?
		texture = g_terrain_texPkt[texture * 4];

		for (slice = 0; slice < 4; slice++) {
			glBindTexture(GL_TEXTURE_2D, texture++);

			// left fence
			if (g_fences[chunk] & 0x80) {
				glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2d(0, 1);
				glVertex3f(track_data[p1].vf_fence_L.x, track_data[p1].vf_fence_L.y, track_data[p1].vf_fence_L.z);
				glTexCoord2d(0, 0);
				glVertex3f(track_data[p1].vf_fence_L.x, track_data[p1].vf_fence_L.y + 1, track_data[p1].vf_fence_L.z);
				glTexCoord2d(1, 1);
				glVertex3f(track_data[p2].vf_fence_L.x, track_data[p2].vf_fence_L.y, track_data[p2].vf_fence_L.z);
				glTexCoord2d(1, 0);
				glVertex3f(track_data[p2].vf_fence_L.x, track_data[p2].vf_fence_L.y + 1, track_data[p2].vf_fence_L.z);
				glEnd();
			}

			// right fence
			if (g_fences[chunk] & 0x40) {
				glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2d(0, 1);
				glVertex3f(track_data[p2].vf_fence_R.x, track_data[p2].vf_fence_R.y, track_data[p2].vf_fence_R.z);
				glTexCoord2d(0, 0);
				glVertex3f(track_data[p2].vf_fence_R.x, track_data[p2].vf_fence_R.y + 1, track_data[p2].vf_fence_R.z);
				glTexCoord2d(1, 1);
				glVertex3f(track_data[p1].vf_fence_R.x, track_data[p1].vf_fence_R.y, track_data[p1].vf_fence_R.z);
				glTexCoord2d(1, 0);
				glVertex3f(track_data[p1].vf_fence_R.x, track_data[p1].vf_fence_R.y + 1, track_data[p1].vf_fence_R.z);
				glEnd();
			}
			p1++;
			p2++;
		}
		chunk--;
	}

	if (g_track_sel == 3) return;

	//objects
	for (int i = 0; i < 1000; i++) {
		slice = g_scenery_object[i].track_slice;
		x = slice - camera.track_slice;
		if (x < -1 || x > 80) continue;
		if (g_scenery_models[g_scenery_object[i].object_model_id].type == 1) {
			gfx_drawObject(&g_scenery_object[i]);
		} else {
			gfx_drawSimpleObject(&g_scenery_object[i]);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void gfx_drawSprite(int x1, int y1, int x2, int y2, int texId) {
	glBindTexture(GL_TEXTURE_2D, texId);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(0, 0);
	glVertex3f(x1, y1, 0);
	glTexCoord2d(0, 1);
	glVertex3f(x1, y2, 0);
	glTexCoord2d(1, 0);
	glVertex3f(x2, y1, 0);
	glTexCoord2d(1, 1);
	glVertex3f(x2, y2, 0);
	glEnd();
}

void gfx_drawHudDigit(int x, int y, int n) {
	int x2 = x + 10;
	int y2 = y + 12;
	gfx_drawSprite(x, y, x2, y2, g_hud_texPkt[n]);
}

void gfx_drawTach() {
	float c,s,r;
	int speed;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, -1.0, 10.0);
	glPolygonMode(GL_FRONT, GL_FILL);
	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// tachometer
	glEnable( GL_BLEND );
	glColor3f(1.0f, 1.0f, 1.0);
	gfx_drawSprite(80, 435, 208, 490, g_hud_texPkt[14]);
	gfx_drawSprite(65, 410, 225, 550, g_hud_texPkt[13]);
	glDisable( GL_BLEND );

	// gear
	int gear = player_car_ptr->gear_selected + 1;
	if (gear == -1) gear = 11;
	if (gear == 0) gear = 10;
	gfx_drawHudDigit(202, 500, gear);

	// speed
	speed = (player_car_ptr->speed >> 16) * 2.23694f; //to MPH
	gfx_drawHudDigit(191, 528, speed % 10);
	if (speed > 9)  gfx_drawHudDigit(178, 528, (speed / 10) % 10);
	if (speed > 99) gfx_drawHudDigit(166, 528, (speed / 100) % 10);

	// track slice
	speed = player_car_ptr->track_slice;
	gfx_drawHudDigit(55, 10, speed % 10);
	gfx_drawHudDigit(40, 10, (speed / 10) % 10);
	gfx_drawHudDigit(25, 10, (speed / 100) % 10);
	gfx_drawHudDigit(10, 10, (speed / 1000) % 10);

	// RPM needle
	glBindTexture(GL_TEXTURE_2D, 0);
	r = ((float) g_car_array[0].rpm_engine / (float) g_car_array[0].rpm_redline) * 2.5 - 1.56;
	c = -cosf(r);
	s = sinf(r);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = 145; matrix[13] = 500; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);

	glColor3f(1.0f, 0.0f, 0.0);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(-2, 0, 0);
	glVertex3f(+2, 0, 0);
	glVertex3f(-2, 60, 0);
	glVertex3f(+2, 60, 0);
	glEnd();

	// steer indicator
	r = ((float) g_car_array[0].steer_angle) / 0x280000;
	c = -cosf(r);
	s = sinf(r);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = 145; matrix[13] = 500; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);

	glColor3f(1.0f, 0.0f, 0.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(-6, 92, 0);
	glVertex3f(+6, 92, 0);
	glVertex3f(0, 80, 0);
	glEnd();
}

void gfx_render_scene() {
	cam_orientation.x = ((float) camera.orientation.x) * 0.0000214576733981; //(360/0xFFFFFF)
	cam_orientation.y = ((float) camera.orientation.y) * 0.0000214576733981; //(360/0xFFFFFF)
	cam_orientation.z = -((float) camera.orientation.z) * 0.0000214576733981; //(360/0xFFFFFF)

	gfx_drawHorizon();
	glEnable(GL_DEPTH_TEST);
	gfx_drawRoad();
	glDisable(GL_DEPTH_TEST);
	gfx_drawShadows();

	glEnable(GL_DEPTH_TEST);
	for (int i = 0; i < g_total_cars_in_scene; i++) {
		if ((g_car_ptr_array[i]->field_4e9 & 4) == 0) {
			continue; //disabled car
		}
		if ((camera.id == 0) && (i == 0)) {
			continue; // player's in car camera
		}
		gfx_drawVehicle(g_car_ptr_array[i]);
	}

	gfx_drawSmoke();

	glDisable(GL_DEPTH_TEST);
	gfx_drawTach();

}

