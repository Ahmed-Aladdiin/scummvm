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
 */

#ifndef NEVERHOOD_PALETTE_H
#define NEVERHOOD_PALETTE_H

#include "neverhood/neverhood.h"
#include "neverhood/entity.h"

namespace Neverhood {

class Palette : public Entity {
public:
	// Default constructor with black palette
	Palette(NeverhoodEngine *vm);
	// Create from existing palette
	Palette(NeverhoodEngine *vm, byte *palette);
	// Create from resource with filename
	Palette(NeverhoodEngine *vm, const char *filename);
	// Create from resource with fileHash
	Palette(NeverhoodEngine *vm, uint32 fileHash);
	~Palette();
	void usePalette();
	void addPalette(const char *filename, int toIndex, int count, int fromIndex);
	void addPalette(uint32 fileHash, int toIndex, int count, int fromIndex);
	void startFadeToBlack(int counter);
	void startFadeToWhite(int counter);
protected:
	int _status;
	byte *_palette;
	int _palCounter;
	byte _fadeToR, _fadeToG, _fadeToB;
	int _fadeStep;
	void update();
	void fadeColor(byte *rgb, byte toR, byte toG, byte toB);
};

} // End of namespace Neverhood

#endif /* NEVERHOOD_PALETTE_H */
