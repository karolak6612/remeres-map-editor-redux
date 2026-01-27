//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_AUTO_BORDER_H_
#define RME_AUTO_BORDER_H_

#include "app/main.h"

class GroundBrush;

class AutoBorder {
public:
	AutoBorder(int _id) :
		id(_id), group(0), ground(false) {
		for (int i = 0; i < 13; i++) {
			tiles[i] = 0;
		}
	}
	~AutoBorder() { }

	static int edgeNameToID(const std::string& edgename);
	bool load(pugi::xml_node node, wxArrayString& warnings, GroundBrush* owner = nullptr, uint16_t ground_equivalent = 0);

	uint32_t tiles[13];
	uint32_t id;
	uint16_t group;
	bool ground;
};

#endif
