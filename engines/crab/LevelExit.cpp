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
 * This code is based on the CRAB engine
 *
 * Copyright (c) Arvind Raja Yadav
 *
 * Licensed under MIT
 *
 */

#include "LevelExit.h"

using namespace pyrodactyl::level;

void Exit::Load(rapidxml::xml_node<char> *node) {
	dim.Load(node);
	LoadStr(name, "name", node);

	if (NodeValid("properties", node)) {
		rapidxml::xml_node<char> *propertynode = node->first_node("properties");
		for (auto n = propertynode->first_node("property"); n != NULL; n = n->next_sibling("property")) {
			std::string node_name;
			LoadStr(node_name, "name", n);
			if (node_name == "entry_x") {
				if (!LoadNum(entry.x, "value", n))
					entry.x = -1;
			} else if (node_name == "entry_y") {
				if (!LoadNum(entry.y, "value", n))
					entry.y = -1;
			}
		}
	}
}
