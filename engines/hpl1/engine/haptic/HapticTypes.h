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

/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 */

#ifndef HPL_HAPTIC_TYPES_H
#define HPL_HAPTIC_TYPES_H

#include <vector>
#include <map>
#include "hpl1/engine/system/SystemTypes.h"
#include "common/list.h"

namespace hpl {

//----------------------------------------------

enum eHapticShapeType {
	eHapticShapeType_Box,
	eHapticShapeType_Sphere,
	eHapticShapeType_Cylinder,
	eHapticShapeType_Capsule,
	eHapticShapeType_Mesh,
	eHapticShapeType_Compund,
	eHapticShapeType_LastEnum
};

//----------------------------------------------

enum eHapticForceType {
	eHapticForceType_Impulse,
	eHapticForceType_Spring,
	eHapticForceType_SineWave,
	eHapticForceType_SawWave,
	eHapticForceType_Viscosity,
	eHapticForceType_LastEnum
};

//----------------------------------------------

enum eHapticSurfaceType {
	eHapticSurfaceType_Simple,
	eHapticSurfaceType_Rough,
	eHapticSurfaceType_Frictional,
	eHapticSurfaceType_Sticky,
	eHapticSurfaceType_LastEnum
};

//----------------------------------------------

class iHapticShape;
class iHapticForce;
class iHapticSurface;

//----------------------------------------------

typedef std::vector<iHapticShape *> tHapticShapeVec;
typedef tHapticShapeVec::iterator tHapticShapeVecIt;

typedef Common::List<iHapticShape *> tHapticShapeList;
typedef tHapticShapeList::iterator tHapticShapeListIt;

typedef Common::List<iHapticForce *> tHapticForceList;
typedef tHapticForceList::iterator tHapticForceListIt;

typedef Common::List<iHapticSurface *> tHapticSurfaceList;
typedef tHapticSurfaceList::iterator tHapticSurfaceListIt;

typedef std::map<tString, iHapticSurface *> tHapticSurfaceMap;
typedef tHapticSurfaceMap::iterator tHapticSurfaceMapIt;

typedef cSTLIterator<iHapticShape *, tHapticShapeList, tHapticShapeListIt> cHapticShapeIterator;

//----------------------------------------------

}     // namespace hpl

#endif // HPL_HAPTIC_TYPES_H
