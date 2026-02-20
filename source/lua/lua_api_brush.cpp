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

#include "app/main.h"
#include "lua_api_brush.h"
#include "brushes/brush.h"
#include "ui/gui.h"

// Include brush headers for type checking
#include "brushes/raw/raw_brush.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/border/optional_border_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "brushes/flag/flag_brush.h"
// EraserBrush and TerrainBrush are defined in brush.h

namespace LuaAPI {

	// Helper to get brush type as string
	static std::string getBrushTypeName(Brush* brush) {
		if (!brush) {
			return "unknown";
		}

		if (brush->is<RAWBrush>()) {
			return "raw";
		}
		if (brush->is<DoodadBrush>()) {
			return "doodad";
		}
		if (brush->is<GroundBrush>()) {
			return "ground";
		}
		if (brush->is<WallBrush>()) {
			return "wall";
		}
		if (brush->is<TableBrush>()) {
			return "table";
		}
		if (brush->is<CarpetBrush>()) {
			return "carpet";
		}
		if (brush->is<DoorBrush>()) {
			return "door";
		}
		if (brush->is<OptionalBorderBrush>()) {
			return "optional_border";
		}
		if (brush->is<CreatureBrush>()) {
			return "creature";
		}
		if (brush->is<SpawnBrush>()) {
			return "spawn";
		}
		if (brush->is<HouseBrush>()) {
			return "house";
		}
		if (brush->is<HouseExitBrush>()) {
			return "house_exit";
		}
		if (brush->is<WaypointBrush>()) {
			return "waypoint";
		}
		if (brush->is<FlagBrush>()) {
			return "flag";
		}
		if (brush->is<EraserBrush>()) {
			return "eraser";
		}
		if (brush->is<TerrainBrush>()) {
			return "terrain";
		}

		return "unknown";
	}

	void registerBrush(sol::state& lua) {
		// Register Brush usertype (read-only)
		lua.new_usertype<Brush>("Brush", sol::no_constructor,

								// Properties (read-only)
								"id", sol::property([](Brush* b) -> uint32_t {
									return b ? b->getID() : 0;
								}),
								"name", sol::property([](Brush* b) -> std::string {
									return b ? b->getName() : "";
								}),
								"lookId", sol::property([](Brush* b) -> int {
									return b ? b->getLookID() : 0;
								}),
								"type", sol::property([](Brush* b) -> std::string {
									return getBrushTypeName(b);
								}),

								// Capabilities (read-only)
								"canDrag", sol::property([](Brush* b) -> bool {
									return b && b->canDrag();
								}),
								"canSmear", sol::property([](Brush* b) -> bool {
									return b && b->canSmear();
								}),
								"needsBorders", sol::property([](Brush* b) -> bool {
									return b && b->needBorders();
								}),
								"oneSizeFitsAll", sol::property([](Brush* b) -> bool {
									return b && b->oneSizeFitsAll();
								}),
								"maxVariation", sol::property([](Brush* b) -> int {
									return b ? b->getMaxVariation() : 0;
								}),
								"visibleInPalette", sol::property([](Brush* b) -> bool {
									return b && b->visibleInPalette();
								}),

								// Type checks
								"isRaw", sol::property([](Brush* b) { return b && b->is<RAWBrush>(); }),
								"isDoodad", sol::property([](Brush* b) { return b && b->is<DoodadBrush>(); }),
								"isTerrain", sol::property([](Brush* b) { return b && b->is<TerrainBrush>(); }),
								"isGround", sol::property([](Brush* b) { return b && b->is<GroundBrush>(); }),
								"isWall", sol::property([](Brush* b) { return b && b->is<WallBrush>(); }),
								"isTable", sol::property([](Brush* b) { return b && b->is<TableBrush>(); }),
								"isCarpet", sol::property([](Brush* b) { return b && b->is<CarpetBrush>(); }),
								"isDoor", sol::property([](Brush* b) { return b && b->is<DoorBrush>(); }),
								"isCreature", sol::property([](Brush* b) { return b && b->is<CreatureBrush>(); }),
								"isSpawn", sol::property([](Brush* b) { return b && b->is<SpawnBrush>(); }),
								"isHouse", sol::property([](Brush* b) { return b && b->is<HouseBrush>(); }),
								"isEraser", sol::property([](Brush* b) { return b && b->is<EraserBrush>(); }),

								// String representation
								sol::meta_function::to_string, [](Brush* b) -> std::string {
									if (!b) {
										return "Brush(invalid)";
									}
									return "Brush(\"" + b->getName() + "\", type=" + getBrushTypeName(b) + ")";
								});

		// Register Brushes namespace for brush lookup
		sol::table brushes = lua.create_named_table("Brushes");

		// Get brush by name
		brushes["get"] = [](const std::string& name) -> Brush* {
			return g_brushes.getBrush(name);
		};

		// Get all brush names (for iteration)
		brushes["getNames"] = [](sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table names = lua.create_table();

			int idx = 1;
			for (const auto& pair : g_brushes.getMap()) {
				names[idx++] = pair.first;
			}
			return names;
		};

		// Extend the existing 'app' table with brush-related functions
		sol::table app = lua["app"];

		// Current brush
		app.set_function("getBrush", []() -> Brush* {
			return g_gui.GetCurrentBrush();
		});

		// Set brush by name
		app.set_function("setBrush", [](const std::string& name) -> bool {
			Brush* brush = g_brushes.getBrush(name);
			if (brush) {
				g_gui.SelectBrush(brush);
				return true;
			}
			return false;
		});

		// Register BrushShape enum
		lua.new_enum("BrushShape", "SQUARE", BRUSHSHAPE_SQUARE, "CIRCLE", BRUSHSHAPE_CIRCLE);
	}

} // namespace LuaAPI
