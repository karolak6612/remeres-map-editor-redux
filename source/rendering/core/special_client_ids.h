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

#ifndef RME_RENDERING_CORE_SPECIAL_CLIENT_IDS_H_
#define RME_RENDERING_CORE_SPECIAL_CLIENT_IDS_H_

#include <cstdint>

// Named constants for special client IDs used in the rendering pipeline.
// These items have no visible sprites but are shown with colored indicators
// when "Show Technical Items" is enabled.
namespace SpecialClientId {

// Invisible stairs tile — rendered as yellow indicator
inline constexpr uint16_t INVISIBLE_STAIRS = 469;

// Invisible walkable tiles — rendered as red indicator
inline constexpr uint16_t INVISIBLE_WALKABLE_470 = 470;
inline constexpr uint16_t INVISIBLE_WALKABLE_17970 = 17970;
inline constexpr uint16_t INVISIBLE_WALKABLE_20028 = 20028;
inline constexpr uint16_t INVISIBLE_WALKABLE_34168 = 34168;

// Invisible wall — rendered as cyan indicator
inline constexpr uint16_t INVISIBLE_WALL = 2187;

// Primal light sources — rendered with light source sprite
inline constexpr uint16_t PRIMAL_LIGHT_RANGE_START = 39092;
inline constexpr uint16_t PRIMAL_LIGHT_RANGE_END = 39100;
inline constexpr uint16_t PRIMAL_LIGHT_39236 = 39236;
inline constexpr uint16_t PRIMAL_LIGHT_39367 = 39367;
inline constexpr uint16_t PRIMAL_LIGHT_39368 = 39368;

[[nodiscard]] inline constexpr bool isInvisibleWalkable(uint16_t cid) {
	return cid == INVISIBLE_WALKABLE_470
		|| cid == INVISIBLE_WALKABLE_17970
		|| cid == INVISIBLE_WALKABLE_20028
		|| cid == INVISIBLE_WALKABLE_34168;
}

[[nodiscard]] inline constexpr bool isPrimalLight(uint16_t cid) {
	return (cid >= PRIMAL_LIGHT_RANGE_START && cid <= PRIMAL_LIGHT_RANGE_END)
		|| cid == PRIMAL_LIGHT_39236
		|| cid == PRIMAL_LIGHT_39367
		|| cid == PRIMAL_LIGHT_39368;
}

} // namespace SpecialClientId

#endif
