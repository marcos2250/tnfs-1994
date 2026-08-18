/*
 * tnfs_gfx.h
 */

#ifndef TNFS_GFX_H_
#define TNFS_GFX_H_

#include "ccb.h"

#define GL_CLAMP_TO_EDGE 0x812F

typedef int int32;
typedef unsigned int uint32;
typedef int Err;
typedef int Item;

extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const float SCREEN_SCALE;

extern unsigned char g_backbuffer[307200]; //320x240x4

int gfx_init_stuff();
void gfx_set_display_callback(void (*func)(void));
shpm_image * gfx_locateshape(byte *data, char *shapeid);
void gfx_clear();
void gfx_draw_text_9500(char *text, int x, int y);
void gfx_draw_shpm(shpm_image * data, int posX, int posY);
void gfx_draw_ccb(CCB * ccb);

// 3DO API
CCB * ParseCel(void *inBuf, int32 inBufSize);
CCB * LoadCel(char *filename, uint32 memTypeBits);
void UnloadCel(CCB * cel);
Err DrawScreenCels(Item screenItem, CCB *cel);
Err DisplayScreen(Item screenItem0, Item screenItem1);

// frontovl
byte * gfx_openfile_9594(char * filename, int mode);
shpm_image * gfx_readshape_9444(byte * a1, char * label);
void gfx_drawshape_94f4(shpm_image * shape, short left, short top);
void gfx_drawshape_94f4_at(shpm_image * shape, short left, short top);
void gfx_drawshape_950c(shpm_image * shape, short left, short top);
void gfx_drawshape_95a0(shpm_image * shape, short left, short top);
void gfx_write_alpha_channel(byte *data, int size, byte alpha);

// sim mode
int gfx_store_shpm_group(byte * shpm, unsigned int * texIdsGL);
unsigned int gfx_store_ccb(CCB *ccb, byte alpha);
unsigned int gfx_store_texture(image_data * image);
void gfx_clear_buffers();

void gfx_render_scene();

void fileView_drawImage(byte * file, int pos);

#endif
