/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */


#include "common/system.h"
#include "common/endian.h"

#include "cruise/cruise_main.h"

namespace Cruise {

uint8 page00[320 * 200];
uint8 page10[320 * 200];

char screen[320 * 200];
palEntry lpalette[256];

int palDirtyMin = 256;
int palDirtyMax = -1;

gfxModuleDataStruct gfxModuleData = {
	0,			// field_1
	0,			// use Tandy
	0,			// use EGA
	1,			// use VGA

	page00,			// pPage00
	page10,			// pPage10
};

void gfxModuleData_gfxClearFrameBuffer(uint8 *ptr) {
	memset(ptr, 0, 64000);
}

void gfxModuleData_gfxCopyScreen(char *sourcePtr, char *destPtr) {
	memcpy(destPtr, sourcePtr, 64000);
}

void outputBit(char *buffer, int bitPlaneNumber, uint8 data) {
	*(buffer + (8000 * bitPlaneNumber)) = data;
}

void convertGfxFromMode4(uint8 *sourcePtr, int width, int height, uint8 *destPtr) {

	for (int y = 0; y < (height/16); ++y) {
		for (int x = 0; x < width; ++x) {
			for (int bit = 0; bit < 16; ++bit) {
				uint8 color = 0;
				for (int p = 0; p < 4; ++p) {
					if (READ_BE_UINT16(sourcePtr + p * 2) & (1 << (15 - bit))) {
						color |= 1 << p;
					}
				}
				*destPtr++ = color;
			}
			sourcePtr += 8;
		}
	}
}

void convertGfxFromMode5(uint8 *sourcePtr, int width, int height, uint8 *destPtr) {
	int range = (width/8) * height;

	for(int line = 0; line < 200; line++) {
		uint8 p0;
		uint8 p1;
		uint8 p2;
		uint8 p3;
		uint8 p4;

		for(int col = 0; col < 40; col++) {
			for(int bit = 0; bit <8; bit++ ) {
				p0 = (sourcePtr[line*40 + col + range * 0] >> bit) & 1;
				p1 = (sourcePtr[line*40 + col + range * 1] >> bit) & 1;
				p2 = (sourcePtr[line*40 + col + range * 2] >> bit) & 1;
				p3 = (sourcePtr[line*40 + col + range * 3] >> bit) & 1;
				p4 = (sourcePtr[line*40 + col + range * 4] >> bit) & 1;

				destPtr[line * width + col * 8 + (7-bit)] = p0 | (p1 << 1) | (p2 << 2) | (p3 << 3) | (p4 << 4);
			}
		}
	}
}

void gfxModuleData_setDirtyColors(int min, int max) {
	if (min < palDirtyMin)
		palDirtyMin = min;
	if (max > palDirtyMax)
		palDirtyMax = max;
}

void gfxModuleData_setPalColor(int idx, int r, int g, int b) {
	lpalette[idx].R = r;
	lpalette[idx].G = g;
	lpalette[idx].B = b;
	gfxModuleData_setDirtyColors(idx, idx);
}

void gfxModuleData_setPal256(uint8 *ptr) {
	int R;
	int G;
	int B;
	int i;

	for (i = 0; i < 256; i++) {
		R = *(ptr++);
		G = *(ptr++);
		B = *(ptr++);

		lpalette[i].R = R;
		lpalette[i].G = G;
		lpalette[i].B = B;
		lpalette[i].A = 255;
	}

	gfxModuleData_setDirtyColors(0, 255);
}

/*void gfxModuleData_setPal(uint8 *ptr) {
	int i;
	int R;
	int G;
	int B;

	for (i = 0; i < 256; i++) {
#define convertRatio 36.571428571428571428571428571429
		uint16 atariColor = *ptr;
		//flipShort(&atariColor);
		ptr ++;

		R = (int)(convertRatio * ((atariColor & 0x700) >> 8));
		G = (int)(convertRatio * ((atariColor & 0x070) >> 4));
		B = (int)(convertRatio * ((atariColor & 0x007)));

		if (R > 0xFF)
			R = 0xFF;
		if (G > 0xFF)
			G = 0xFF;
		if (B > 0xFF)
			B = 0xFF;

		lpalette[i].R = R;
		lpalette[i].G = G;
		lpalette[i].B = B;
		lpalette[i].A = 255;
	}

	gfxModuleData_setDirtyColors(0, 16);
}*/

void gfxModuleData_convertOldPalColor(uint16 oldColor, uint8* pOutput) {
	int R;
	int G;
	int B;

#define convertRatio 36.571428571428571428571428571429

	R = (int)(convertRatio * ((oldColor & 0x700) >> 8));
	G = (int)(convertRatio * ((oldColor & 0x070) >> 4));
	B = (int)(convertRatio * ((oldColor & 0x007)));

	if (R > 0xFF)
		R = 0xFF;
	if (G > 0xFF)
		G = 0xFF;
	if (B > 0xFF)
		B = 0xFF;

	*(pOutput++) = R;
	*(pOutput++) = G;
	*(pOutput++) = B;
}

void gfxModuleData_field_90(void) {
}

void gfxModuleData_gfxWaitVSync(void) {
}

void gfxModuleData_flip(void) {
}

void gfxModuleData_field_64(char *sourceBuffer, int width, int height,
	    char *dest, int x, int y, int color) {
	int i;
	int j;

	x = 0;
	y = 0;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			dest[(y + i) * 320 / 4 + x + j] =
			    sourceBuffer[i * width + j];
		}
	}
}

void gfxModuleData_flipScreen(void) {
	memcpy(globalScreen, gfxModuleData.pPage00, 320 * 200);

	flip();
}

void flip() {
	int i;
	byte paletteRGBA[256 * 4];
	//uint8* outPtr = scaledScreen;
	//uint8* inPtr  = globalScreen;

	if (palDirtyMax != -1) {
		for (i = palDirtyMin; i <= palDirtyMax; i++) {
			paletteRGBA[i * 4 + 0] = lpalette[i].R;
			paletteRGBA[i * 4 + 1] = lpalette[i].G;
			paletteRGBA[i * 4 + 2] = lpalette[i].B;
			paletteRGBA[i * 4 + 3] = 0xFF;
		}
		g_system->setPalette(paletteRGBA+palDirtyMin*4, palDirtyMin, palDirtyMax - palDirtyMin + 1);
		palDirtyMin = 256;
		palDirtyMax = -1;
	}

	g_system->copyRectToScreen(globalScreen, 320, 0, 0, 320, 200);
	g_system->updateScreen();

}

} // End of namespace Cruise
