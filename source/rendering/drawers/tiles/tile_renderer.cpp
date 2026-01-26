#include "app/main.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"

#include "editor/editor.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"
#include "brushes/waypoint_brush.h"
#include "game/complexitem.h"

#include "rendering/core/drawing_options.h"
#include "rendering/core/render_view.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"
#include "app/definitions.h"
#include "game/sprites.h"

#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/core/light_buffer.h"

TileRenderer::TileRenderer(ItemDrawer* id, SpriteDrawer* sd, CreatureDrawer* cd, FloorDrawer* fd, MarkerDrawer* md, TooltipDrawer* td, Editor* ed) :
	item_drawer(id), sprite_drawer(sd), creature_drawer(cd), floor_drawer(fd), marker_drawer(md), tooltip_drawer(td), editor(ed) {
}

// Helper function to create tooltip data from an item
static TooltipData CreateItemTooltipData(Item* item, const Position& pos, bool isHouseTile) {
	if (!item) {
		return TooltipData();
	}

	const uint16_t id = item->getID();
	if (id < 100) {
		return TooltipData();
	}

	const uint16_t unique = item->getUniqueID();
	const uint16_t action = item->getActionID();
	const std::string& text = item->getText();
	const std::string& description = item->getDescription();
	uint8_t doorId = 0;
	Position destination;

	// Check if it's a door
	if (isHouseTile && item->isDoor()) {
		if (Door* door = dynamic_cast<Door*>(item)) {
			if (door->isRealDoor()) {
				doorId = door->getDoorID();
			}
		}
	}

	// Check if it's a teleport
	Teleport* tp = dynamic_cast<Teleport*>(item);
	if (tp && tp->hasDestination()) {
		destination = tp->getDestination();
	}

	// Only create tooltip if there's something to show
	if (unique == 0 && action == 0 && doorId == 0 && text.empty() && description.empty() && destination.x == 0) {
		return TooltipData();
	}

	// Get item name from database
	std::string itemName = g_items[id].name;
	if (itemName.empty()) {
		itemName = "Item";
	}

	TooltipData data(pos, id, itemName);
	data.actionId = action;
	data.uniqueId = unique;
	data.doorId = doorId;
	data.text = text;
	data.description = description;
	data.destination = destination;
	data.updateCategory();

	return data;
}

void TileRenderer::DrawTile(SpriteBatch& sprite_batch, PrimitiveRenderer& primitive_renderer, TileLocation* location, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id, std::ostringstream& tooltip_stream) {
	if (!location) {
		return;
	}
	Tile* tile = location->get();

	if (!tile) {
		return;
	}

	if (options.show_only_modified && !tile->isModified()) {
		return;
	}

	int map_x = location->getX();
	int map_y = location->getY();
	int map_z = location->getZ();

	int draw_x, draw_y;
	view.getScreenPosition(map_x, map_y, map_z, draw_x, draw_y);

	// Early viewport culling using pre-calculated screen coordinates
	// Margin account for large sprites (TileSize * 3)
	int margin = TileSize * 3;
	int screen_width = int(view.screensize_x * view.zoom);
	int screen_height = int(view.screensize_y * view.zoom);

	if (draw_x < -margin || draw_x > screen_width + margin || draw_y < -margin || draw_y > screen_height + margin) {
		return;
	}

	Waypoint* waypoint = editor->map.waypoints.getWaypoint(location);

	bool as_minimap = options.show_as_minimap;
	bool only_colors = as_minimap || options.show_only_colors;

	uint8_t r = 255, g = 255, b = 255;

	// begin filters for ground tile
	if (!as_minimap) {
		TileColorCalculator::Calculate(tile, options, current_house_id, location->getSpawnCount(), r, g, b);
	}

	if (only_colors) {
		if (as_minimap) {
			TileColorCalculator::GetMinimapColor(tile, r, g, b);
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, r, g, b, 255);
		} else if (r != 255 || g != 255 || b != 255) {
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, r, g, b, 128);
		}
	} else {
		if (tile->ground) {
			if (options.show_preview && view.zoom <= 2.0) {
				tile->ground->animate();
			}

			item_drawer->BlitItem(sprite_batch, primitive_renderer, sprite_drawer, creature_drawer, draw_x, draw_y, tile, tile->ground, options, false, r, g, b);
		} else if (options.always_show_zones && (r != 255 || g != 255 || b != 255)) {
			ItemType* zoneItem = &g_items[SPRITE_ZONE];
			item_drawer->DrawRawBrush(sprite_batch, sprite_drawer, draw_x, draw_y, zoneItem, r, g, b, 60);
		}
	}

	// end filters for ground tile

	if (!only_colors) {
		if (view.zoom < 10.0 || !options.hide_items_when_zoomed) {
			// items on tile
			for (ItemVector::iterator it = tile->items.begin(); it != tile->items.end(); it++) {
				// item animation
				if (options.show_preview && view.zoom <= 2.0) {
					(*it)->animate();
				}

				// item sprite
				if ((*it)->isBorder()) {
					item_drawer->BlitItem(sprite_batch, primitive_renderer, sprite_drawer, creature_drawer, draw_x, draw_y, tile, *it, options, false, r, g, b);
				} else {
					uint8_t ir = 255, ig = 255, ib = 255;

					if (options.extended_house_shader && options.show_houses && tile->isHouseTile()) {
						if ((int)tile->getHouseID() == current_house_id) {
							ir /= 2;
						} else {
							ir /= 2;
							ig /= 2;
						}
					}
					item_drawer->BlitItem(sprite_batch, primitive_renderer, sprite_drawer, creature_drawer, draw_x, draw_y, tile, *it, options, false, ir, ig, ib);
				}
			}
			// monster/npc on tile
			if (tile->creature && options.show_creatures) {
				creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, tile->creature);
			}
		}

		if (view.zoom < 10.0) {
			// markers (waypoint, house exit, town temple, spawn)
			marker_drawer->draw(sprite_batch, sprite_drawer, draw_x, draw_y, tile, waypoint, current_house_id, *editor, options);
		}
	}
}

void TileRenderer::CaptureTooltips(TileLocation* location, const RenderView& view, const DrawingOptions& options) {
	if (!location || !options.show_tooltips) {
		return;
	}

	int map_z = location->getZ();

	// Only show tooltips for current floor
	if (map_z != view.floor) {
		return;
	}

	Tile* tile = location->get();
	if (!tile) {
		return;
	}

	const Position& pos = location->getPosition();

	// Waypoint tooltip
	Waypoint* waypoint = editor->map.waypoints.getWaypoint(location);
	if (location->getWaypointCount() > 0 && waypoint) {
		tooltip_drawer->addWaypointTooltip(pos, waypoint->name);
	}

	// Ground tooltip
	if (tile->ground) {
		TooltipData groundData = CreateItemTooltipData(tile->ground, pos, tile->isHouseTile());
		if (groundData.hasVisibleFields()) {
			tooltip_drawer->addItemTooltip(groundData);
		}
	}

	// Items tooltip
	for (auto item : tile->items) {
		TooltipData itemData = CreateItemTooltipData(item, pos, tile->isHouseTile());
		if (itemData.hasVisibleFields()) {
			tooltip_drawer->addItemTooltip(itemData);
		}
	}
}

void TileRenderer::AddLight(TileLocation* location, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	if (!options.isDrawLight() || !location) {
		return;
	}

	auto tile = location->get();
	if (!tile) {
		return;
	}

	const auto& position = location->getPosition();

	if (tile->ground) {
		if (tile->ground->hasLight()) {
			light_buffer.AddLight(position.x, position.y, position.z, tile->ground->getLight());
		}
	}

	bool hidden = options.hide_items_when_zoomed && view.zoom > 10.f;
	if (!hidden && !tile->items.empty()) {
		for (auto item : tile->items) {
			if (item->hasLight()) {
				light_buffer.AddLight(position.x, position.y, position.z, item->getLight());
			}
		}
	}
}
