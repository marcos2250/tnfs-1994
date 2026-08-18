/*
 * ccb.h
 * 3DO CEL/CCB and TNFS SHPM image converter to RGBA format
 */
#ifndef CCB_H_
#define CCB_H_

typedef unsigned char byte;

/*
 * output image converted to RGBA8888
 */
typedef struct image_data {
    int size;
    int width;
    int height;
    byte rgba[];
} image_data;

/*
 * SHPM files contains a set of images, this is a single image entry
 */
typedef struct shpm_image {
    byte flags; //0x0 - 4 bits/unknown; 1 bit/is_packed; 3 bits/bpp.
    byte plut_le[3]; //0x1
    short width_le; //0x4
    short height_le; //0x6
    short unk1; //0x8
    short unk2; //0xA
    short left_le; //0xC
    short top_le; //0xE
    byte data[]; //0x10
} shpm_image;

/**
 * CEL/CCB image
 */
typedef struct CCB {
	char id[4]; // 0x0 'CCB '
	int chunk_size; //0x4
	int ccb_version; //0x8
	int ccb_Flags; //0xC
	int ccb_NextPtr; //0x10
	int ccb_CelData; //0x14
	short * ccb_PLUTPtr; //0x18
	int ccb_XPos; //0x1C
	int ccb_YPos;  //0x20
	int ccb_HDX; //0x24
	int ccb_HDY; //0x28
	int ccb_VDX; //0x2C
	int ccb_VDY; //0x30
	int ccb_DDX; //0x34
	int ccb_DDY;  //0x38
	int ccb_PPMPC; //0x3C
	int ccb_PRE0; //0x40 Cel Preamble 0
	int ccb_PRE1; //0x44 Cel Preamble 1
	short field_48; //0x48
	short ccb_Width; //0x4A
	short field_4C; //0x4C
	short ccb_Height; //0x4E
	byte data[]; //0x50
} CCB;

int ccb_parse_header(CCB *ccb);
int shpm_parse_header(shpm_image * shape);
void ccb_draw_to_buffer(byte * output, int left, int top, int bufWidth, int bufHeight, char upsideDown);
int bswap16(unsigned short in);
int bswap32(unsigned int in);

image_data * ccb_image_convert(CCB *ccb);
image_data * shpm_image_convert(shpm_image * shpm, shpm_image * optional_plut);

#endif /* CCB_H_ */
