/*
 * ccb.c
 * 3DO CEL/CCB and TNFS SHPM image converter to RGBA format
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ccb.h"

// image attributes
byte * pdat;
byte * plut;
int width;
int height;
int bpp;
int isBgnd;
int isPacked;
int isShaded;
int buffer_width;
int buffer_height;
int buffer_size;
char isUpsideDown;

int isBlockLabel(char *label, byte *data) {
	return label[0] == data[0] && label[1] == data[1] && label[2] == data[2] && label[3] == data[3];
}

int readbits(unsigned char * data, int * idx_, byte size) {
    int val = 0;
    size_t i;
	int j;
    if (size == 0) {
    	return 0;
    }
    i = *idx_;
    for (j = 0; j < size; j++) {
        val = ((val << 1) | ((data[i >> 3] >> (7 - (i & 7))) & 1));
        i++;
    }
	*idx_ += size;
	return val;
}

int readInt32(unsigned char *buffer, int pos) {
	return (int) (buffer[pos]) << 24 //
		| (int)(buffer[pos + 1]) << 16 //
		| (int)(buffer[pos + 2]) << 8 //
		| buffer[pos + 3];
}

int bswap16(short in) {
	//return __builtin_bswap16(in);
	return ((in & 0xFF) << 8) | ((in & 0xFF00) >> 8);
}

int bswap32(int in) {
	//return __builtin_bswap32(in);
	return ((in & 0xFF) << 24) //
			| ((in & 0xFF00) << 8) //
			| ((in & 0xFF0000) >> 8) //
			| (in >> 24);
}

int getPaletteColor(int pixel) {
	pixel &= 0x1f; //limit to 32 color palette
	pixel *= 2; //palettes are 2-byte (16-bit) colors
	return plut[pixel + 1] | (plut[pixel] << 8);
}

void writepixel(byte * buffer, int *idx, int rgb, int shade) {
	if (*idx > buffer_size || *idx < 0) return;

	//if (!isBgnd && ((rgb & 0x7FFF) == 0)) {
	if (isBgnd == 0 && rgb == 0) {
		// transparent pixel
		*idx += 4;
		return;
	}

	//FIXME: 0xfc00 color -> transparent pixel?
	if (isBgnd == 0 && rgb == 0xfc00) {
		buffer[*idx] = shade << 5;
		*idx += 1;
		buffer[*idx] = shade << 5;
		*idx += 1;
		buffer[*idx] = shade << 5;
		*idx += 1;
		buffer[*idx] = shade < 4 ? 0 : 0xFF;
		*idx += 1;
		return;
	}

	if (shade) isShaded = 1; //FIXME detect if it's a shaded palette

	if (isShaded) {
		if (bpp == 6) {
			if (shade) {
				shade = 0xFF;
			} else {
				shade = 0x80;
			}
		} else if (bpp == 8) {
			shade <<= 5;
		} else {
			shade = 0xFF;
		}
		buffer[*idx] = (((rgb & 0x7c00) >> 7) * shade) >> 8;
		*idx += 1;
		buffer[*idx] = (((rgb & 0x3e0) >> 2) * shade) >> 8;
		*idx += 1;
		buffer[*idx] = (((rgb & 0x1f) << 3) * shade) >> 8;
		*idx += 1;
		buffer[*idx] = 0xFF;
		*idx += 1;

	} else {
		buffer[*idx] = (rgb & 0x7c00) >> 7;
		*idx += 1;
		buffer[*idx] = (rgb & 0x3e0) >> 2;
		*idx += 1;
		buffer[*idx] = (rgb & 0x1f) << 3;
		*idx += 1;
		buffer[*idx] = 0xFF;
		*idx += 1;
	}
}

int ccb_parse_header(ccb_chunk *ccb) {
	byte *cel = 0;
	int limit = 0x10000;
	pdat = 0;
	plut = 0;
	bpp = 0;
	isBgnd = 0;
	isShaded = 0;

	if (!isBlockLabel("CCB ", (byte*) ccb)) {
		printf("ccb_parse_header: Not a CCB!\n");
		return 0;
	}

	width = bswap16(ccb->ccb_Width);
	height = bswap16(ccb->ccb_Height);

	if (width < 1 || width > 2048 || height < 1 || height > 2048) {
		printf("ccb_parse_header: Invalid CCB data!\n");
		return 0;
	}

	switch ((ccb->ccb_PRE0 >> 24) & 7) {
	case 1:
		bpp = 1;
		break;
	case 2:
		bpp = 2;
		break;
	case 3:
		bpp = 4;
		break;
	case 4:
		bpp = 6;
		break;
	case 5:
		bpp = 8;
		break;
	case 6:
		bpp = 16;
		break;
	}

	isBgnd   = (ccb->ccb_Flags & 0x20000000) != 0; //CCB_BGND 0x00000020
	isPacked = (ccb->ccb_Flags & 0x00020000) != 0; //CCB_PACKED 0x00000200

	if (width == 4 && height == 4) isBgnd = 0; //FIXME transparent texture

	cel = (byte*) ccb;
	cel += bswap32(ccb->chunk_size);

	limit = 4;
	while (limit--) {
		if (isBlockLabel("PLUT", cel)) {
			plut = cel + 12; //'PLUT' + size + num_entries
			cel += readInt32(cel, 4);
		}
		if (isBlockLabel("PDAT", cel)) {
			if ((ccb->ccb_Flags & 0x4000) == 0) { //CCB_CCBPRE offset
				if (isPacked) {
					pdat = cel + 8 + 4;
				} else {
					pdat = cel + 8 + 8;
				}
			} else {
				pdat = cel + 8; //'PDAT' + size
			}
			cel += readInt32(cel, 4);
		}
		if (isBlockLabel("XTRA", cel) || isBlockLabel("HSPT", cel)) { //ignore tags
			cel += readInt32(cel, 4);
		}
	}

	return 1;
}

int shpm_parse_header(shpm_image * shape) {
	int plutOffset = 0;
	pdat = 0;
	plut = 0;
	bpp = 0;
	isShaded = 0;

	width = bswap16(shape->width_le);
	height = bswap16(shape->height_le);

	if (width < 1 || width > 2048 || height < 1 || height > 2048) {
		printf("shpm_parse_header: Invalid SHPM image!\n");
		return 0;
	}

	switch (shape->flags & 7) {
	case 1:
		bpp = 1;
		break;
	case 2:
		bpp = 2;
		break;
	case 3:
		bpp = 4;
		break;
	case 4:
		bpp = 6;
		break;
	case 5:
		bpp = 8;
		break;
	case 6:
		bpp = 16;
		break;
	}

	isBgnd = 0;
	isPacked = shape->flags & 0x80;

	plutOffset = (shape->plut_le[0] << 16) | (shape->plut_le[1] << 8) | shape->plut_le[2];
	if (plutOffset != 0) {
		if (plutOffset > 0xf00000) {
			plutOffset = plutOffset - 0x1000000;
		}
		plut = (byte*)shape + plutOffset + 0x10;
	} else {
		plut = 0;
	}

	pdat = shape->data;
	return 1;
}

void ccb_decode_linear_image(byte * output, int posX, int posY) {
	short bx, by;
	int pixel = 0;
	int shade = 0;
	int rgb = 0;
	int bit = 0;
	int outidx = 0;
	byte shadebits = 0;
	byte pixelbits = 0;

	pixelbits = bpp;
	//shaded palette colors
	if (plut) {
		if (bpp == 8) {
			pixelbits = 5;
			shadebits = 3;
		} else if (bpp == 6) {
			pixelbits = 5;
			shadebits = 1;
		}
	}

	for (by = 0; by < height; by++) {
		if (isUpsideDown) {
			outidx = ((buffer_height - by - posY - 1) * buffer_width + posX) * 4;
		} else {
			outidx = (((by + posY) * buffer_width) + posX) * 4;
		}

		for (bx = 0; bx < width; bx++) {
			shade = readbits(pdat, &bit, shadebits);
			pixel = readbits(pdat, &bit, pixelbits);

			if (plut) {
				rgb = getPaletteColor(pixel);
			} else {
				rgb = pixel;
			}
			writepixel(output, &outidx, rgb, shade);
		}

		// ???
		if (width < 5)
			bit += 32;

		//skip to 32bit boundary
		if (bit & 0x1F) {
			bit += 0x20 - (bit & 0x1F);
		}
	}
}

void ccb_decode_packed_image(byte * output, int posX, int posY) {
	byte chunkType;
	short chunkSize;
	short bx, by;
	int rgb = 0;
	int pixel = 0;
	int shade = 0;
	int bit = 0;
	int outidx = 0;
	int offset = 0;
	int offsetWidth = 0;
	byte shadebits = 0;
	byte pixelbits;

	pixelbits = bpp;
	if (pixelbits < 8) {
		offsetWidth = 8;
	} else {
		offsetWidth = 16;
	}

	//shaded palette colors
	if (plut) {
		if (bpp == 8) {
			pixelbits = 5;
			shadebits = 3;
		} else if (bpp == 6) {
			pixelbits = 5;
			shadebits = 1;
		}
	}

	for (by = 0; by < height; by++) {
		if (isUpsideDown) {
			outidx = ((buffer_height - by - posY - 1) * buffer_width + posX) * 4;
		} else {
			outidx = (((by + posY) * buffer_width) + posX) * 4;
		}

		bit = offset * 8;
		offset += ((readbits(pdat, &bit, offsetWidth) + 2) * 4);

		bx = width;
		while (bx > 0) {
			chunkType = readbits(pdat, &bit, 2);

			switch (chunkType){
				case 0: //eol
					bx = 0;
					break;
				case 1: //literal
					chunkSize = readbits(pdat, &bit, 6) + 1;
					bx -= chunkSize;
					while(chunkSize) {
						chunkSize--;
						shade = readbits(pdat, &bit, shadebits);
						pixel = readbits(pdat, &bit, pixelbits);
						if (plut) {
							rgb = getPaletteColor(pixel);
						} else {
							rgb = pixel;
						}
						writepixel(output, &outidx, rgb, shade);
					}
					break;
				case 2: //transparent
					chunkSize = readbits(pdat, &bit, 6) + 1;
					bx -= chunkSize;
					outidx += chunkSize * 4;
					break;
				case 3: //packed
					chunkSize = readbits(pdat, &bit, 6) + 1;
					bx -= chunkSize;
					shade = readbits(pdat, &bit, shadebits);
					pixel = readbits(pdat, &bit, pixelbits);
					if (plut) {
						rgb = getPaletteColor(pixel);
					} else {
						rgb = pixel;
					}
					while(chunkSize > 0) {
						chunkSize--;
						writepixel(output, &outidx, rgb, shade);
					}
					break;
			}
		}
	}
}

void ccb_draw_to_buffer(byte * output, int left, int top, int bufWidth, int bufHeight, char upsideDown) {
	// magic flag to center image on screen
	if (top == -990) {
		left = (320 - width) / 2;
		top = (240 - height) / 2;
	}
	isUpsideDown = upsideDown;

	buffer_width = bufWidth;
	buffer_height = bufHeight;
	buffer_size = bufWidth * bufHeight * 4;

	if (isPacked) {
		ccb_decode_packed_image(output, left, top);
	} else {
		ccb_decode_linear_image(output, left, top);
	}
}

image_data * shpm_image_convert(shpm_image * shpm, shpm_image * optional_plut) {
	image_data * output;
	int size = 0;
	if (!shpm_parse_header(shpm)) {
		return 0;
	}
	if (optional_plut != 0) {
		isShaded = 1; //FIXME
		plut = optional_plut->data;
	}

	size = width * height * 4;
	buffer_width = width;
	buffer_height = height;
	buffer_size = size;

	output = (image_data *) malloc(size + 32);
	if (output->rgba == 0) {
		printf("shpm_image_convert: Can't malloc size %d!\n", output->size);
		return 0;
	}
	memset(output->rgba, 0, size);
	output->width = width;
	output->height = height;
	output->size = size;
	isUpsideDown = 0;
	if (isPacked) {
		ccb_decode_packed_image(output->rgba, 0, 0);
	} else {
		ccb_decode_linear_image(output->rgba, 0, 0);
	}
	return output;
}

image_data * ccb_image_convert(ccb_chunk *ccb) {
	image_data * output;
	int size = 0;
	if (!ccb_parse_header(ccb)) {
		return 0;
	}

	size = width * height * 4;
	buffer_width = width;
	buffer_height = height;
	buffer_size = size;

	output = (image_data *) malloc(size + 32);
	if (output->rgba == 0) {
		printf("ccb_image_convert: Can't malloc size %d!\n", size);
		return 0;
	}
	memset(output->rgba, 0, size);
	output->width = width;
	output->height = height;
	output->size = size;
	isUpsideDown = 0;
	if (isPacked) {
		ccb_decode_packed_image(output->rgba, 0, 0);
	} else {
		ccb_decode_linear_image(output->rgba, 0, 0);
	}
	return output;
}
