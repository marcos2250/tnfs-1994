#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "tnfs_gfx.h"
#include "tnfs_base.h"
#include "tnfs_files.h"
#include "ccb.h"

const int SCREEN_WIDTH = 1280; //320;
const int SCREEN_HEIGHT = 960; //240;
const float SCREEN_SCALE = 4; //1;

byte g_backbuffer[307200];
byte g_fontdata[4464];
int g_filesize = 0;

unsigned int g_tex_count_lo = 0;
unsigned int g_tex_count_hi = 0;

void (*_sdl_display_callback)();

GLfloat matrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
vector3f cam_orientation = { 0, 0, 0 };
vector3f cam_position = { 0, 0, 0 };

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

void gfx_set_display_callback(void (*func)(void)) {
	_sdl_display_callback = func;
}

void gfx_clear_buffers() {
	unsigned int i;
	unsigned int texId;
	glBindTexture(GL_TEXTURE_2D, 0);
	for (i = g_tex_count_lo; i <= g_tex_count_hi; i++) {
		if (glIsTexture(i)) {
			texId = i;
			glDeleteTextures(1, &texId);
		}
	}
	g_tex_count_lo = (unsigned int)-1;
	g_tex_count_hi = 0;
	glFlush();
}

unsigned int gfx_store_texture(image_data * image) {
	GLuint texId = 0;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->rgba);
	if (texId > g_tex_count_hi) g_tex_count_hi = texId;
	if (texId < g_tex_count_lo) g_tex_count_lo = texId;
	return texId;
}

/* 3DO Library functions */

CCB * ParseCel(void *inBuf, int32 inBufSize) {
	unsigned int id;
	image_data * image;
	CCB * input;
	CCB * output;

	input = (CCB*)inBuf;
	image = ccb_image_convert(input);
	id = gfx_store_texture(image);

	output = malloc(sizeof(CCB));
	output->ccb_version = id; //use field to store GL's texture id
	output->ccb_Width = image->width;
	output->ccb_Height = image->height;
	output->ccb_XPos = bswap16(input->ccb_XPos);
	output->ccb_YPos = bswap16(input->ccb_YPos);
	output->ccb_HDX = bswap16(input->ccb_HDX);
	output->ccb_HDY = bswap16(input->ccb_HDY);
	output->ccb_VDX = bswap16(input->ccb_VDX);
	output->ccb_VDY = bswap16(input->ccb_VDY);
	output->ccb_DDX = bswap16(input->ccb_DDX);
	output->ccb_DDY = bswap16(input->ccb_DDY);

	return output;
}

CCB * LoadCel (char *filename, uint32 memTypeBits) {
	int size;
	CCB * input;
	CCB * output;

	input = (CCB*)openFileBuffer(filename, &size);
	output = ParseCel(input, 0);

	return output;
}

void UnloadCel(CCB * cel) {
	unsigned int id = cel->ccb_version;
	glBindTexture(GL_TEXTURE_2D, 0);
	if (glIsTexture(id)) {
		glDeleteTextures(1, &id);
	}
	glFlush();
	free(cel);
}

Err DrawScreenCels(Item screenItem, CCB *cel) {
	int top, left;
	float hdx, hdy, vdx, vdy, ddx, ddy, aux_d;
	int x0, y0, x1, y1, x2, y2, x3, y3;

	top = cel->ccb_YPos;
	if (cel->ccb_YPos > 0x10000) {
		top >>= 16;
	}

	left = cel->ccb_XPos;
	if (cel->ccb_XPos > 0x10000) { //if 16.16
		left >>= 16;
	}

	if (cel->ccb_HDX <= 0x10) {
		hdx = 1;
	} else {
		hdx = ((float)cel->ccb_HDX) / 0x100000;
	}

	hdy = ((float)cel->ccb_HDY) / 0x100000;
	vdx = ((float)cel->ccb_VDX) / 0x10000;

	if (cel->ccb_VDY <= 0x10) {
		vdy = 1;
	} else {
		vdy = ((float)cel->ccb_VDY) / 0x10000;
	}

	ddx = ((float)cel->ccb_DDX) / 0x10000;
	ddy = ((float)cel->ccb_DDY) / 0x10000;

	hdx *= cel->ccb_Width;
	hdy *= cel->ccb_Width;
	vdx *= cel->ccb_Height;
	vdy *= cel->ccb_Height;
	aux_d = cel->ccb_Height * cel->ccb_Width;
	ddx *= aux_d;
	ddy *= aux_d;

	x0 = left;
	y0 = top;

	x1 = left + hdx;
	y1 = top + hdy;

	x2 = left + vdx + hdx + ddx;
	y2 = top + vdy + hdy + ddy;

	x3 = left + vdx;
	y3 = top + vdy;

	glEnable(GL_BLEND);

	// draw quad
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glOrtho(0.0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, -1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPolygonMode(GL_FRONT, GL_FILL);
	glBindTexture(GL_TEXTURE_2D, cel->ccb_version); //texId
	glColor3f(1,1,1);

	x0*=SCREEN_SCALE; y0*=SCREEN_SCALE;
	x1*=SCREEN_SCALE; y1*=SCREEN_SCALE;
	x2*=SCREEN_SCALE; y2*=SCREEN_SCALE;
	x3*=SCREEN_SCALE; y3*=SCREEN_SCALE;

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(0, 0);
	glVertex3f(x0, y0, 0);
	glTexCoord2d(1, 0);
	glVertex3f(x1, y1, 0);
	glTexCoord2d(0, 1);
	glVertex3f(x3, y3, 0);
	glTexCoord2d(1, 1);
	glVertex3f(x2, y2, 0);
	glEnd();

	glDisable(GL_BLEND);
	return 1;
}

Err DisplayScreen(Item screenItem0, Item screenItem1) {
	_sdl_display_callback();
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

shpm_image * gfx_readshape_9444(byte * data, char * label) {
	return gfx_locateshape(data, label);
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
	return openFileBuffer(filename, &g_filesize);
}

/*** FRONTEND UI ***/

void gfx_draw_ccb(CCB *ccb) {
	if (!ccb_parse_header(ccb)) {
		return;
	}
	ccb_draw_to_buffer((byte*) &g_backbuffer, ccb->ccb_XPos, ccb->ccb_YPos, 320, 240, 1);
}

void gfx_draw_shpm(shpm_image * shpm, int posX, int posY) {
	if (!shpm_parse_header(shpm)) {
		return;
	}
	ccb_draw_to_buffer((byte*) &g_backbuffer, posX, posY, 320, 240, 1);
}

void gfx_drawshape_94f4(shpm_image * shape, short x, short y) {
	short top, left;
	left = bswap16(x);
	top = bswap16(y);
	gfx_draw_shpm(shape, left, top);
}

void gfx_drawshape_94f4_at(shpm_image * shape, short x, short y) {
	gfx_draw_shpm(shape, x, y);
}

void gfx_drawshape_950c(shpm_image * shape, short x, short y) {
	short top, left;
	left = bswap16(x);
	top = bswap16(y);
	gfx_draw_shpm(shape, left, top);
}

void gfx_drawshape_95a0(shpm_image * shape, short x, short y) {
	short top, left;
	left = bswap16(x);
	top = bswap16(y);
	gfx_draw_shpm(shape, left, top);
}

void fileView_drawImage(byte * file, int pos) {
	short width, height, top, left;
	shpm_image * shape;
	CCB * ccb;
	byte * obj = file;
	obj += pos;

	gfx_clear();
	gfx_write_alpha_channel(g_backbuffer, 307200, 0xFF);
	glClearColor(0, 0, 0, 0xFF);
	glClear(GL_COLOR_BUFFER_BIT);

	if (obj[0] == 'C' && obj[1] == 'C' && obj[2] == 'B') {
		printf("CCB at 0x%x:\n", pos);
		ccb = (CCB*) obj;

		width = bswap16(ccb->ccb_Width);
		height = bswap16(ccb->ccb_Height);
		ccb->ccb_XPos = (320 - width) / 2;
		ccb->ccb_YPos = (240 - height) / 2;

		printf("image %d x %d:\n", width, height);
		gfx_draw_ccb(ccb);
	} else {
		printf("SHPM at 0x%x\n", pos);
		shape = (shpm_image*) obj;

		width = bswap16(shape->width_le);
		height = bswap16(shape->height_le);
		left = (320 - width) / 2;
		top = (240 - height) / 2;

		gfx_draw_shpm(shape, left, top);
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

int gfx_store_shpm_group(byte * shpm, unsigned int * texIdsGL) {
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

unsigned int gfx_store_ccb(CCB *ccb, byte alpha) {
	image_data * data = 0;
	unsigned int id = 0;
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
	int i;

	tnfs_smoke_puff * smoke;
	glEnable(GL_BLEND);
	//glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	for (i = 0; i < SMOKE_PUFFS; i++) {
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
		glPolygonMode(GL_FRONT, GL_FILL);

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
	int i;

	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0.0f, 0.0f, 0.0, 0.3f);
	glPolygonMode(GL_FRONT, GL_FILL);

	for (i = 0; i < g_total_cars_in_scene; i++) {
		car = &g_car_array[i];
		if (i == 0 && camera.id == 0) {
			continue;
		}
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
		glTranslatef(cam_position.x, cam_position.y, cam_position.z);
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

unsigned int gfx_getWheelTexture(tnfs_car_data * car, tnfs_carmodel3d * carModel, int isFront) {
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
	} else if (car->car_model_id == 4 && isFront) { //also F512
		pX = 1.0f;
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
	unsigned int textureId;
	int i;
	float a;

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
	glTranslatef(cam_position.x, cam_position.y, cam_position.z);
	glMultMatrixf(matrix);

	carModel = &g_carmodels[car->car_model_id];

	if (track_data[car->track_slice].item_mode & 4) {
		// dimmer lighting inside tunnels
		a = 0.5;
	} else if (track_data[car->track_slice].item_mode & 8) {
		// tree tunnel
		a = (float)((car->position.z & 0x3FFFF) - 0x1FFFF);
		if (a < 0) a = -a;
		a /= 0x80000;
		a = 0.7 - a;
	} else {
		a = 1;
	}
	glColor3f(a, a, a);

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT, GL_FILL);

	for (i = 0; i < carModel->model.numPolys; i++) {
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
			if (carModel->lrl0 == poly->polyId && (iSimTimeClock & 16) == 0) {
				textureId = carModel->copSirenLights[0];
			} else if (carModel->lrr0 == poly->polyId  && (iSimTimeClock & 16) != 0) {
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
	unsigned int texture;
	int i, d;
	float a, x1, x2, y1, y2, z1, z2;

	if (g_track_sel == 2) {
		glClearColor(0.5f, 0.7f, 0.9f, 1.0f); //city = cyan
	} else {
		glClearColor(0.05f, 0.2f, 0.5f, 1.0f); //blue
	}
	glClear(GL_COLOR_BUFFER_BIT);

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

	layer = 2;
	while (layer--) {
		d = cam_orientation.y / 30;
		a = (d - 2) * 0.52359f;
		for (i = 0; i < 5; i++) {
			x1 = sinf(a);
			z1 = -cosf(a);
			a += 0.52359f;
			x2 = sinf(a);
			z2 = -cosf(a);
			texture = d % 6;
			d++;

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
	float w, h;
	unsigned int t;
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
	int i;

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

	for (i = 0; i < model->numPolys; i++) {
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

void gfx_drawRoad(int isMirror) {
	int x;
	int p1, p2;
	int chunk, strip, slice;
	unsigned int texture;
	int chunk_increment;
	int count;
	int i;
	float h;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (cam_orientation.x != 0) {
		glRotatef(cam_orientation.x, 1, 0, 0);
	}
	if (cam_orientation.z != 0) {
		glRotatef(cam_orientation.z, 0, 0, 1);
	}
	glRotatef(cam_orientation.y, 0, 1, 0);
	glTranslatef(cam_position.x, cam_position.y, cam_position.z);

	glColor3f(1.0f, 1.0f, 1.0);
	glPolygonMode(GL_FRONT, GL_FILL);

	// terrain
	if (isMirror) {
		chunk = (camera.track_slice >> 2) - 4;
		count = 5;
		chunk_increment = +1;
	} else {
		chunk = (camera.track_slice >> 2) + 25;
		count = 26;
		chunk_increment = -1;
	}
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

		    for (slice = 0; slice < 4; slice++) {
		    	glBindTexture(GL_TEXTURE_2D, g_terrain_texPkt[texture * 4 + slice]);
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
		chunk += chunk_increment;
	}

	// fences/tunnel wall
	if (isMirror) {
		chunk = (camera.track_slice >> 2) - 4;
		count = 5;
	} else {
		chunk = (camera.track_slice >> 2) + 14;
		count = 15;
	}
	while (count--) {
		p1 = chunk * 4;
		p2 = p1 + 1;

		if (g_fences[chunk] != 0) {
			texture = (g_fences[chunk] & 0x1F) + 0x20;
			for (slice = 0; slice < 4; slice++) {
				glBindTexture(GL_TEXTURE_2D, g_terrain_texPkt[texture * 4 + slice]);

				// fence/wall height
				if (track_data[p1].item_mode == 7 || track_data[p1].item_mode == 9) {
					h = 8; // tunnel wall
				} else {
					if (g_fences[chunk] & 0x20) {
						h = 0.9f; // guard rail
					} else {
						h = 1.8f; // fence
					}
				}

				// left fence
				if (g_fences[chunk] & 0x80) {
					glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2d(0, 1);
					glVertex3f(track_data[p1].vf_fence_L.x, track_data[p1].vf_fence_L.y, track_data[p1].vf_fence_L.z);
					glTexCoord2d(0, 0);
					glVertex3f(track_data[p1].vf_fence_L.x, track_data[p1].vf_fence_L.y + h, track_data[p1].vf_fence_L.z);
					glTexCoord2d(1, 1);
					glVertex3f(track_data[p2].vf_fence_L.x, track_data[p2].vf_fence_L.y, track_data[p2].vf_fence_L.z);
					glTexCoord2d(1, 0);
					glVertex3f(track_data[p2].vf_fence_L.x, track_data[p2].vf_fence_L.y + h, track_data[p2].vf_fence_L.z);
					glEnd();
				}

				// right fence
				if (g_fences[chunk] & 0x40) {
					glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2d(0, 1);
					glVertex3f(track_data[p2].vf_fence_R.x, track_data[p2].vf_fence_R.y, track_data[p2].vf_fence_R.z);
					glTexCoord2d(0, 0);
					glVertex3f(track_data[p2].vf_fence_R.x, track_data[p2].vf_fence_R.y + h, track_data[p2].vf_fence_R.z);
					glTexCoord2d(1, 1);
					glVertex3f(track_data[p1].vf_fence_R.x, track_data[p1].vf_fence_R.y, track_data[p1].vf_fence_R.z);
					glTexCoord2d(1, 0);
					glVertex3f(track_data[p1].vf_fence_R.x, track_data[p1].vf_fence_R.y + h, track_data[p1].vf_fence_R.z);
					glEnd();
				}
				p1++;
				p2++;
			}
		}
		chunk += chunk_increment;
	}

	if (g_track_sel == 3) return;

	//objects
	for (i = 0; i < 1000; i++) {
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

void gfx_drawSprite(int x1, int y1, int x2, int y2, unsigned int texId) {
	x1 *= SCREEN_SCALE;
	x2 *= SCREEN_SCALE;
	y1 *= SCREEN_SCALE;
	y2 *= SCREEN_SCALE;
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
	int x2 = x + 3;
	int y2 = y + 5;
	gfx_drawSprite(x, y, x2, y2, g_hud_texPkt[n]);
}

void gfx_draw_hud() {
	float c,s,r;
	int v;

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	gluPerspective(90.0, 1.0, 0.1, 10);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, -1.0, 10.0);
	glPolygonMode(GL_FRONT, GL_FILL);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor4f(1, 1, 1, 1);

	// tachometer
	glEnable( GL_BLEND );
	glColor3f(1.0f, 1.0f, 1.0);
	gfx_drawSprite(32, 174, 83, 196, g_hud_texPkt[14]);
	gfx_drawSprite(26, 164, 90, 220, g_hud_texPkt[13]);

	// gear
	int gear = player_car_ptr->gear_selected + 1;
	if (gear == -1) gear = 11;
	if (gear == 0) gear = 10;
	gfx_drawHudDigit(82, 200, gear);

	// speed
	v = math_mul(player_car_ptr->speed, 0x23CA8) >> 16; //to MPH
	gfx_drawHudDigit(76, 211, v % 10);
	if (v > 9)  gfx_drawHudDigit(72, 211, (v / 10) % 10);
	if (v > 99) gfx_drawHudDigit(68, 211, (v / 100) % 10);

	/*
	// track slice
	v = player_car_ptr->track_slice;
	gfx_drawHudDigit(19, 4, v % 10);
	gfx_drawHudDigit(14, 4, (v / 10) % 10);
	gfx_drawHudDigit( 9, 4, (v / 100) % 10);
	gfx_drawHudDigit( 4, 4, (v / 1000) % 10);
	*/

	// time
	v = (iSimTimeClock / 6) % 600;
	gfx_drawHudDigit(56, 211, v % 10);
	gfx_drawHudDigit(50, 211, (v / 10) % 10);
	gfx_drawHudDigit(46, 211, (v / 100) % 10);
	v = iSimTimeClock / 3600;
	gfx_drawHudDigit(40, 211, v % 10);
	gfx_drawHudDigit(36, 211, (v / 10) % 10);

	// RPM needle
	r = ((float) g_car_array[0].rpm_engine / (float) g_car_array[0].rpm_redline);
	if (r > 1) r = 1;
	r = r * 2.5 - 1.56;
	c = -cosf(r);
	s = sinf(r);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = 58 * SCREEN_SCALE; matrix[13] = 200 * SCREEN_SCALE; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);
	gfx_drawSprite(-2, 30, +2, 0, g_hud_texPkt[16]);

	// steer indicator
	glBindTexture(GL_TEXTURE_2D, 0);
	r = ((float) g_car_array[0].steer_angle) / 0x280000;
	c = -cosf(r);
	s = sinf(r);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = 58 * SCREEN_SCALE; matrix[13] = 200 * SCREEN_SCALE; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);
	gfx_drawSprite(-2, 40, +2, 0, g_hud_texPkt[15]);

	// reset matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable( GL_BLEND );
}

void gfx_rear_view_mirror(int x, int y) {
	int i = 0;
	float x1, y1, x2, y2;

	x1 = (g_dash_constants.rear_view[0] + x) * SCREEN_SCALE;
	y1 = (g_dash_constants.rear_view[1] - y) * SCREEN_SCALE;
	x2 = g_dash_constants.rear_view[2] * SCREEN_SCALE;
	y2 = g_dash_constants.rear_view[3] * SCREEN_SCALE;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, -1.38, 0.1, 1000);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(x1, y1, x2, y2);
	glScissor(x1, y1, x2, y2);
	glEnable(GL_SCISSOR_TEST);

	cam_orientation.y += 180;

	glDisable(GL_DEPTH_TEST);
	gfx_drawHorizon();
	gfx_drawRoad(1);
	gfx_drawSmoke();

	for (i = 0; i < g_total_cars_in_scene; i++) {
		if ((g_car_ptr_array[i]->field_4e9 & 4) == 0) {
			continue; //disabled car
		}
		if ((camera.id == 0) && (i == 0)) {
			continue; // player's in car camera
		}
		gfx_drawVehicle(g_car_ptr_array[i]);
	}

	glDisable(GL_SCISSOR_TEST);
}

// gear shift animation vars
int shift_pos_x[8] = { 1, 0, -1, -1,  0, 0,  1, 1 };
int shift_pos_y[8] = { 1, 0, -1, +1, -1, 1, -1, 1 };
int shift_pos_x_dogleg[8] = { -1, 0, -1,  0, 0,  1,  1, 2 };
int shift_pos_y_dogleg[8] = { -1, 0, +1, -1, 1, -1, +1, -1 };
int knobX = 280;
int knobY = 200;

void gfx_draw_dashboard() {
	float c,s,r;
	int x,y,i;

	if (g_dash_constants.num_panels == 0) {
		return;
	}

	x = (player_car_ptr->body_roll >> 15) - 10;
	y = (player_car_ptr->body_pitch >> 15) - 10;

	// rear view mirror
	gfx_rear_view_mirror(x, y);

	// reset projection
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	gluPerspective(90.0, 1.0, 0.1, 10);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, -1.0, 10.0);
	glPolygonMode(GL_FRONT, GL_FILL);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor4f(1, 1, 1, 1);

	// dash
	gfx_drawSprite(x, y, 340+x, 260+y, g_dash_texPkt[0]);

	// speedo needle
	if (player_car_ptr->car_model_id != 3) { // CZR1 digital speedo
		glBindTexture(GL_TEXTURE_2D, 0);
		r = ((float) player_car_ptr->speed) / 0x100000 + g_dash_constants.gauge_idle_angle;
		c = -cosf(r);
		s = sinf(r);
		glMatrixMode(GL_MODELVIEW);
		matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
		matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
		matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
		matrix[12] = (g_dash_constants.speedo_pos_x+x) * SCREEN_SCALE; matrix[13] = (g_dash_constants.speedo_pos_y+y) * SCREEN_SCALE; matrix[14] = 0; matrix[15] = 1;
		glLoadMatrixf(matrix);

		glColor3f(0.9f, 0.3f, 0.1f);
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-SCREEN_SCALE, 0, 0);
		glVertex3f(+SCREEN_SCALE, 0, 0);
		glVertex3f(-SCREEN_SCALE, g_dash_constants.gauge_needle_length * SCREEN_SCALE, 0);
		glVertex3f(+SCREEN_SCALE, g_dash_constants.gauge_needle_length * SCREEN_SCALE, 0);
		glEnd();
	}

	// RPM needle
	glBindTexture(GL_TEXTURE_2D, 0);
	r = ((float) player_car_ptr->rpm_engine / (float) player_car_ptr->rpm_redline);
	if (r > 1) r = 1;
	r = r * g_dash_constants.tacho_rotate_factor + g_dash_constants.gauge_idle_angle;
	c = -cosf(r);
	s = sinf(r);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = (g_dash_constants.tacho_pos_x+x) * SCREEN_SCALE; matrix[13] = (g_dash_constants.tacho_pos_y+y) * SCREEN_SCALE; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);

	glColor3f(0.9f, 0.3f, 0.1f);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(-SCREEN_SCALE, 0, 0);
	glVertex3f(+SCREEN_SCALE, 0, 0);
	glVertex3f(-SCREEN_SCALE, g_dash_constants.gauge_needle_length * SCREEN_SCALE, 0);
	glVertex3f(+SCREEN_SCALE, g_dash_constants.gauge_needle_length * SCREEN_SCALE, 0);
	glEnd();

	//steering wheel
	glColor3f(1.0f, 1.0f, 1.0);
	r = ((float) player_car_ptr->steer_angle) / 0x280000;
	c = -cosf(r);
	s = sinf(r);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = (g_dash_constants.steer_pos_x+x) * SCREEN_SCALE; matrix[13] = (g_dash_constants.steer_pos_y+y) * SCREEN_SCALE; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);
	gfx_drawSprite(-g_dash_constants.steer_size, g_dash_constants.steer_size, g_dash_constants.steer_size, -g_dash_constants.steer_size, g_dash_texPkt[1]);
	glBindTexture(GL_TEXTURE_2D, 0);

	// reset matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// gear shift animation
	if (player_car_ptr->gear_shift_interval) {
		if (player_car_ptr->car_model_id == 4) { // F512TR dogleg
			x = shift_pos_x_dogleg[player_car_ptr->gear_selected + 2];
			y = shift_pos_y_dogleg[player_car_ptr->gear_selected + 2];
		} else {
			x = shift_pos_x[player_car_ptr->gear_selected + 2];
			y = shift_pos_y[player_car_ptr->gear_selected + 2];
		}

		if (player_car_ptr->car_model_id == 1 || player_car_ptr->car_model_id == 4) {
			// gated shifters
			gfx_drawSprite(270, 190, 310, 230, g_dash_texPkt[3]);
			knobX -= (knobX - (x * 10 + 280)) >> 1;
			knobY -= (knobY - (y * 10 + 200)) >> 1;
			gfx_drawSprite(knobX, knobY, knobX + 20, knobY + 20, g_dash_texPkt[2]);
		} else {
			// leather covered lever
			gfx_drawSprite(270, 190, 310, 230, g_dash_texPkt[6]);
			for (i = 0; i < 4; i++) {
				knobX -= (knobX - (x * i * 4 + 280)) >> 1;
				knobY -= (knobY - (y * i * 4 + 200)) >> 1;
				gfx_drawSprite(knobX, knobY, knobX + 20, knobY + 20, g_dash_texPkt[5 - i]);
			}
		}
	}
}

CCB * g_ticket_cel;

void gfx_cop_ticket_screen() {
	byte* data;

	if (g_police_ticket_time == 100) {
		if (g_ticket_cel) {
			UnloadCel(g_ticket_cel);
			g_ticket_cel = 0;
		}
		if (g_police_speeding_ticket) {
			data = gfx_openfile_9594("DriveData/DriveArt/speedingt0.celFam", 0);
		} else {
			data = gfx_openfile_9594("DriveData/DriveArt/warningt0.celFam", 0);
		}
		data += 0xC; //jump wwww header
		g_ticket_cel = ParseCel((CCB*)data, 0);
	}

	DrawScreenCels(0, g_ticket_cel);
}

// "Crashed!" banner
void gfx_toast_crashed() {
	g_toast_crash_time--;
	gfx_drawSprite(92, 50, 135 + 92, 50 + 18, g_hud_texPkt[18]);
	// number of cars left
	gfx_drawSprite(212, 53, 212 + 10, 53 + 12, g_hud_texPkt[(g_stats_data.cars_remaining % 10) + 20]);
}

// "Bonus car!" banner
void gfx_toast_bonus_car() {
	g_toast_bonus_time--;
	gfx_drawSprite(80, 50, 160 + 80, 50 + 18, g_hud_texPkt[17]);
	// number of cars left
	gfx_drawSprite(225, 53, 225 + 10, 53 + 12, g_hud_texPkt[(g_stats_data.cars_remaining % 10) + 20]);
}

void gfx_render_scene() {
	int i;

	cam_orientation.x = ((float) camera.orientation.x) * 0.0000214576733981; //(360/0xFFFFFF)
	cam_orientation.y = ((float) camera.orientation.y) * 0.0000214576733981; //(360/0xFFFFFF)
	cam_orientation.z = -((float) camera.orientation.z) * 0.0000214576733981; //(360/0xFFFFFF)
	// inverted axis camera position
	cam_position.x = ((float) -camera.position.x) / 0x10000;
	cam_position.y = ((float) -camera.position.y) / 0x10000;
	cam_position.z = ((float) camera.position.z) / 0x10000;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	gluPerspective(50.0, 1.38, 0.1, 1000);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gfx_drawHorizon();
	glEnable(GL_DEPTH_TEST);
	gfx_drawRoad(0);
	glDisable(GL_DEPTH_TEST);
	gfx_drawShadows();

	glEnable(GL_DEPTH_TEST);
	for (i = 0; i < g_total_cars_in_scene; i++) {
		if ((g_car_ptr_array[i]->field_4e9 & 4) == 0) {
			continue; //disabled car
		}
		if ((camera.id == 0) && (i == 0)) {
			continue; // player's in car camera
		}
		gfx_drawVehicle(g_car_ptr_array[i]);
	}

	gfx_drawSmoke();

	if (camera.id == 0 && g_dash_enabled) {
		gfx_draw_dashboard();
	} else {
		gfx_draw_hud();
	}
	if (g_police_ticket_time) {
		gfx_cop_ticket_screen();
		return;
	}
	if (g_toast_crash_time) {
		gfx_toast_crashed();
	}
	if (g_toast_bonus_time) {
		gfx_toast_bonus_car();
	}
}
