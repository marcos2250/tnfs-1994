/*
 * tnfs_gfx.h
 */

#ifndef TNFS_GFX_H_
#define TNFS_GFX_H_

#include "ccb.h"

#define GL_CLAMP_TO_EDGE 0x812F
#define byte unsigned char

extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int BUFFER_PIXELS;

extern unsigned char g_backbuffer[307200]; //320x240x4

int gfx_init_stuff();
void gfx_set_filedata(byte * ptr);
shpm_image * gfx_locateshape(byte *data, char *shapeid);
void gfx_clear();
void gfx_draw_text_9500(char *text, int x, int y);
void gfx_draw_shpm(shpm_image * data, int posX, int posY);
void gfx_draw_ccb(ccb_chunk * ccb, int left, int top);
void gfx_draw_cel(char * file);
void gfx_draw_3sh(char * file, char * label);

// frontovl
byte * gfx_openfile_9594(char * filename, int mode);
void gfx_readshape_9444(byte * a1, char * label);
void gfx_drawshape_94f4();
void gfx_drawshape_94f4_at(int left, int top);
void gfx_drawshape_950c();
void gfx_drawshape_95a0();
byte * gfx_openfile_96ec(char * filename, int mode);
void gfx_drawshape_96f8(shpm_image * cel);
void gfx_write_alpha_channel(byte *data, int size, byte alpha);

// sim mode
int gfx_store_shpm_group(byte * shpm, int * texIdsGL);
int gfx_store_ccb(ccb_chunk *ccb, byte alpha);
int gfx_store_texture(image_data * image);
void gfx_clear_buffers();

void gfx_render_scene();

void fileView_drawImage(byte * file, int pos);

#endif
