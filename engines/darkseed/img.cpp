/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/debug.h"
#include "common/file.h"
#include "darkseed/darkseed.h"
#include "darkseed/img.h"

namespace Darkseed {

bool Img::load(const Common::Path &filename) {
	Common::File file;
	if (!file.open(filename)) {
		return false;
	}
	bool ret = load(file);
	file.close();
	if (ret) {
		debug("Loaded %s (%d,%d) (%d,%d) %x", filename.toString().c_str(), x, y, width, height, mode);
	}
	return ret;
}

bool Img::load(Common::SeekableReadStream &readStream) {
	Common::Array<uint8> unpackedData;
	unpackRLE(readStream, unpackedData);
	x = READ_UINT16(&unpackedData.data()[0]);
	y = READ_UINT16(&unpackedData.data()[2]);
	unpackPlanarData(unpackedData, 4);
	return true;
}

bool Img::loadWithoutPosition(Common::SeekableReadStream &readStream) {
	Common::Array<uint8> unpackedData;
	unpackRLE(readStream, unpackedData);
	x = 0;
	y = 0;
	unpackPlanarData(unpackedData, 0);
	return false;
}

bool Img::unpackRLE(Common::SeekableReadStream &readStream, Common::Array<byte> &buf) {
	uint16 size = readStream.readUint16LE();
	uint16 idx = 0;
	buf.resize(size + 1);

	while (idx <= size) {
		uint8 byte = readStream.readByte();
		assert(!readStream.err());
		if (byte & 0x80) {
			uint8 count = byte & 0x7f;
			count++;
			byte = readStream.readByte();
			for (int i = 0; i < count && idx + i < size; i++) {
				buf[idx + i] = byte;
			}
			idx += count;
		} else {
			uint8 count = byte + 1;
			for (int i = 0; i < count && idx + i < size; i++) {
				buf[idx + i] = readStream.readByte();
			}
			idx += count;
		}
	}

	return true;
}

void Img::unpackPlanarData(Common::Array<uint8> &planarData, uint16 headerOffset) {
	height = READ_UINT16(&planarData.data()[headerOffset]);
	width = READ_UINT16(&planarData.data()[headerOffset + 2]) * 8;
	mode = planarData.data()[headerOffset + 4];
//	assert(mode == 0xff);
	pixels.resize(width * height, 0);
	for (int py = 0; py < height; py++) {
		for (int plane = 0; plane < 4; plane++) {
			for (int px = 0; px < width; px++) {
				int bitPos = (7 - (px % 8));
				int planeBit = (planarData[(headerOffset + 5) + (px / 8) + (width / 8) * plane + py * (width / 8) * 4] & (1 << bitPos)) >> bitPos;
				pixels[px + py * width] |= planeBit << (3 - plane);
			}
		}
	}
}

Common::Array<uint8> &Img::getPixels() {
	return pixels;
}

void Img::draw(int drawMode) {
	drawAt(x, y, drawMode);
}

void Img::drawAt(uint16 xPos, uint16 yPos, int drawMode, int drawWidth) {
	if (drawMode != 0) {
		uint8 *screen = (uint8 *)g_engine->_screen->getBasePtr(xPos, yPos);
		uint8 *imgPixels = pixels.data();
		for (int sy = 0; sy < height; sy++) {
			int w = drawWidth != 0 ? drawWidth : width;
			for (int sx = 0; sx < w; sx++) {
				if (drawMode == 1 && imgPixels[sx] != 0) {
					screen[sx] ^= imgPixels[sx];
				} else if (drawMode == 2 && imgPixels[sx] != 15) {
					screen[sx] &= imgPixels[sx];
				} else if (drawMode == 3 && imgPixels[sx] != 0) {
					screen[sx] |= imgPixels[sx];
				}
			}
			imgPixels += width;
			screen += g_engine->_screen->pitch;
		}
	} else {
		g_engine->_screen->copyRectToSurface(pixels.data(), width, xPos, yPos, width, height);
	}
	g_engine->_screen->addDirtyRect({{(int16)xPos, (int16)yPos}, (int16)width, (int16)height});
}

} // namespace Darkseed
