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

/*
 * @file position.h
 * @brief Defines the 3D coordinate system for the map.
 *
 * The Position class represents a specific location in the game world
 * using (x, y, z) coordinates, where z typically denotes the floor/layer.
 */

#ifndef __POSITION_HPP__
#define __POSITION_HPP__

#include <ostream>
#include <istream>
#include <cstdint>
#include <vector>
#include <list>

#if __has_include("app/definitions.h")
	#include "app/definitions.h"
#elif __has_include("../app/definitions.h")
	#include "../app/definitions.h"
#endif

#ifndef MAP_MAX_WIDTH
	#define MAP_MAX_WIDTH 65000
#endif
#ifndef MAP_MAX_HEIGHT
	#define MAP_MAX_HEIGHT 65000
#endif
#ifndef MAP_MAX_LAYER
	#define MAP_MAX_LAYER 15
#endif

class SmallPosition;

/*
 * @brief Represents a 3D coordinate in the map world.
 *
 * Uses integer coordinates (x, y, z).
 * Typically:
 * - X: Horizontal position (West -> East)
 * - Y: Vertical position (North -> South)
 * - Z: Layer/Floor (0 is highest/surface, 7 is ground level, 15 is deep underground)
 */
class Position {
public:
	// We use int since it's the native machine type and can be several times faster than
	// the other integer types in most cases, also, the position may be negative in some
	// cases
	int x, y, z;

	/*
	 * @brief Default constructor. Initializes to (0, 0, 0).
	 */
	Position() :
		x(0), y(0), z(0) { }

	/*
	 * @brief Constructs a position with specific coordinates.
	 * @param _x X coordinate.
	 * @param _y Y coordinate.
	 * @param _z Z coordinate.
	 */
	Position(int _x, int _y, int _z) :
		x(_x), y(_y), z(_z) { }

	/*
	 * @brief Less-than operator for sorting.
	 *
	 * Orders primarily by Z, then Y, then X.
	 *
	 * @param p The other position.
	 * @return true if this position is "less than" p.
	 */
	bool operator<(const Position& p) const {
		if (z < p.z) {
			return true;
		}
		if (z > p.z) {
			return false;
		}

		if (y < p.y) {
			return true;
		}
		if (y > p.y) {
			return false;
		}

		if (x < p.x) {
			return true;
		}
		// if(x > p.x)
		//	return false;

		return false;
	}

	/*
	 * @brief Greater-than operator.
	 * @param p The other position.
	 * @return true if this position is "greater than" p.
	 */
	bool operator>(const Position& p) const {
		return !(*this < p);
	}

	/*
	 * @brief Subtraction operator.
	 * @param p The position to subtract.
	 * @return A new Position representing the difference (vector).
	 */
	Position operator-(const Position& p) const {
		Position newpos;
		newpos.x = x - p.x;
		newpos.y = y - p.y;
		newpos.z = z - p.z;
		return newpos;
	}

	/*
	 * @brief Addition operator.
	 * @param p The position to add.
	 * @return A new Position representing the sum.
	 */
	Position operator+(const Position& p) const {
		Position newpos;
		newpos.x = x + p.x;
		newpos.y = y + p.y;
		newpos.z = z + p.z;
		return newpos;
	}

	/*
	 * @brief In-place addition.
	 * @param p The position to add.
	 * @return Reference to self.
	 */
	Position& operator+=(const Position& p) {
		*this = *this + p;
		return *this;
	}

	/*
	 * @brief Equality operator.
	 * @param p The other position.
	 * @return true if coordinates match exactly.
	 */
	bool operator==(const Position& p) const {
		return p.x == x && p.y == y && p.z == z;
	}

	/*
	 * @brief Inequality operator.
	 * @param p The other position.
	 * @return true if coordinates differ.
	 */
	bool operator!=(const Position& p) const {
		return !(*this == p);
	}

	/*
	 * @brief Checks if the position is within valid map bounds.
	 * @return true if valid.
	 */
	bool isValid() const;
};

/*
 * @brief Stream output operator for Position.
 *
 * Formats as "x:y:z".
 *
 * @param os Output stream.
 * @param pos The position.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& os, const Position& pos) {
	os << pos.x << ':' << pos.y << ':' << pos.z;
	return os;
}

/*
 * @brief Stream input operator for Position.
 *
 * Parses format "x:y:z".
 *
 * @param is Input stream.
 * @param pos The position to populate.
 * @return The stream.
 */
inline std::istream& operator>>(std::istream& is, Position& pos) {
	char a, b;
	int x, y, z;
	is >> x;
	if (!is) {
		return is;
	}
	is >> a;
	if (!is || a != ':') {
		return is;
	}
	is >> y;
	if (!is) {
		return is;
	}
	is >> b;
	if (!is || b != ':') {
		return is;
	}
	is >> z;
	if (!is) {
		return is;
	}

	pos.x = x;
	pos.y = y;
	pos.z = z;

	return is;
}

inline bool Position::isValid() const {
	return x >= 0 && x <= MAP_MAX_WIDTH && y >= 0 && y <= MAP_MAX_HEIGHT && z >= 0 && z <= MAP_MAX_LAYER;
}

/*
 * @brief Calculates the absolute values of a position's coordinates.
 * @param position The input position.
 * @return New Position with absolute coordinates.
 */
inline Position abs(const Position& position) {
	return Position(
		std::abs(position.x),
		std::abs(position.y),
		std::abs(position.z)
	);
}

using PositionVector = std::vector<Position>;
using PositionList = std::list<Position>;

#endif
