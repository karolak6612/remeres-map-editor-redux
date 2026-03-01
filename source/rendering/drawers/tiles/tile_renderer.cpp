#include "app/main.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "ui/gui.h"

#include "editor/editor.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "game/complexitem.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/core/drawing_options.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"
#include "app/definitions.h"
#include "game/sprites.h"

#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_preloader.h"
#include "rendering/utilities/pattern_calculator.h"

TileRenderer::TileRenderer(ItemDrawer* id, SpriteDrawer* sd, CreatureDrawer* cd, CreatureNameDrawer* cnd, FloorDrawer* fd, MarkerDrawer* md, TooltipDrawer* td, Editor* ed) :
	item_drawer(id), sprite_drawer(sd), creature_drawer(cd), floor_drawer(fd), marker_drawer(md), tooltip_drawer(td), creature_name_drawer(cnd), editor(ed) {
}

// Helper function to populate tooltip data from an item (in-place)
static bool FillItemTooltipData(TooltipData& data, Item* item, const ItemType& it, const Position& pos, bool isHouseTile, float zoom) {
	if (!item) {
		return false;
	}

	const uint16_t id = item->getID();
	if (id < 100) {
		return false;
	}

	uint16_t unique = 0;
	uint16_t action = 0;
	std::string_view text;
	std::string_view description;
	uint8_t doorId = 0;
	Position destination;
	bool hasContent = false;

	bool is_complex = item->isComplex();
	// Early exit for simple items
	// isTooltipable is cached (isContainer || isDoor || isTeleport)
	if (!is_complex && !it.isTooltipable()) {
		return false;
	}

	bool is_container = it.isContainer();
	bool is_door = isHouseTile && item->isDoor();
	bool is_teleport = item->isTeleport();

	if (is_complex) {
		unique = item->getUniqueID();
		action = item->getActionID();
		text = item->getText();
		description = item->getDescription();
	}

	// Check if it's a door
	if (is_door) {
		if (const Door* door = item->asDoor()) {
			if (door->isRealDoor()) {
				doorId = door->getDoorID();
			}
		}
	}

	// Check if it's a teleport
	if (is_teleport) {
		Teleport* tp = static_cast<Teleport*>(item);
		if (tp->hasDestination()) {
			destination = tp->getDestination();
		}
	}

	// Check if container has content
	if (is_container) {
		if (const Container* container = item->asContainer()) {
			hasContent = container->getItemCount() > 0;
		}
	}

	// Only create tooltip if there's something to show
	if (unique == 0 && action == 0 && doorId == 0 && text.empty() && description.empty() && destination.x == 0 && !hasContent) {
		return false;
	}

	// Get item name from database
	std::string_view itemName = it.name;
	if (itemName.empty()) {
		itemName = "Item";
	}

	data.pos = pos;
	data.itemId = id;
	data.itemName = itemName; // Assign string_view to string_view (no copy)

	data.actionId = action;
	data.uniqueId = unique;
	data.doorId = doorId;
	data.text = text;
	data.description = description;
	data.destination = destination;

	// Populate container items
	if (it.isContainer() && zoom <= 1.5f) {
		if (const Container* container = item->asContainer()) {
			// Set capacity for rendering empty slots
			data.containerCapacity = static_cast<uint8_t>(container->getVolume());

			const auto& items = container->getVector();
			data.containerItems.clear();
			// Reserve only what we need (capped at 32)
			data.containerItems.reserve(std::min(items.size(), size_t(32)));
			for (const auto& subItem : items) {
				if (subItem) {
					ContainerItem ci;
					ci.id = subItem->getID();
					ci.subtype = subItem->getSubtype();
					ci.count = subItem->getCount();
					// Sanity check for count
					if (ci.count == 0) {
						ci.count = 1;
					}

					data.containerItems.push_back(ci);

					// Limit preview items to avoid massive tooltips
					if (data.containerItems.size() >= 32) {
						break;
					}
				}
			}
		}
	}

	data.updateCategory();

	return true;
}

void TileRenderer::DrawTile(const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x, int in_draw_y, bool draw_lights) {
	if (!location) {
		return;
	}
	Tile* tile = location->get();

	if (!tile) {
		return;
	}

	if (ctx.options.show_only_modified && !tile->isModified()) {
		return;
	}

	int map_x = location->getX();
	int map_y = location->getY();
	int map_z = location->getZ();

	int draw_x, draw_y;
	if (in_draw_x != -1 && in_draw_y != -1) {
		draw_x = in_draw_x;
		draw_y = in_draw_y;
	} else {
		// Early viewport culling - skip tiles that are completely off-screen
		if (!ctx.view.IsTileVisible(map_x, map_y, map_z, draw_x, draw_y)) {
			return;
		}
	}

	const auto& position = location->getPosition();

	// Light Processing (Ground)
	if (draw_lights && tile->hasLight()) {
		if (tile->ground && tile->ground->hasLight()) {
			ctx.light_buffer.AddLight(position.x, position.y, position.z, tile->ground->getLight());
		}
	}

	Waypoint* waypoint = nullptr;
	if (location->getWaypointCount() > 0) {
		waypoint = editor->map.waypoints.getWaypoint(location);
	}

	// Waypoint tooltip (one per waypoint)
	if (ctx.options.show_tooltips && waypoint && map_z == ctx.view.floor) {
		tooltip_drawer->addWaypointTooltip(position, waypoint->name);
	}

	bool as_minimap = ctx.options.show_as_minimap;
	bool only_colors = as_minimap || ctx.options.show_only_colors;

	uint8_t r = 255, g = 255, b = 255;

	// begin filters for ground tile
	if (!as_minimap) {
		TileColorCalculator::Calculate(tile, ctx.options, current_house_id, location->getSpawnCount(), r, g, b);
	}

	const ItemType* ground_it = nullptr;
	if (tile->ground) {
		ground_it = &tile->ground->getType();
	}

	if (only_colors) {
		if (as_minimap) {
			TileColorCalculator::GetMinimapColor(tile, r, g, b);
			sprite_drawer->glBlitSquare(ctx, draw_x, draw_y, DrawColor(r, g, b, 255));
		} else if (r != 255 || g != 255 || b != 255) {
			sprite_drawer->glBlitSquare(ctx, draw_x, draw_y, DrawColor(r, g, b, 128));
		}
	} else {
		if (tile->ground && ground_it) {
			if (ground_it->sprite) {
				SpritePatterns patterns = PatternCalculator::Calculate(ground_it->sprite, *ground_it, tile->ground.get(), tile, position);

				// Inline preload check — skip function call when sprite is simple and loaded (95%+ case)
				if (!ground_it->sprite->isSimpleAndLoaded()) {
					rme::collectTileSprites(ground_it->sprite, patterns.x, patterns.y, patterns.z, patterns.frame);
				}

				BlitItemParams params(position, tile->ground.get(), ctx.options);
				params.tile = tile;
				params.item_type = ground_it;
				params.red = r;
				params.green = g;
				params.blue = b;
				params.patterns = &patterns;
				item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
			}
		} else if (ctx.options.always_show_zones && (r != 255 || g != 255 || b != 255)) {
			ItemType* zoneItem = &g_items[SPRITE_ZONE];
			item_drawer->DrawRawBrush(ctx, sprite_drawer, draw_x, draw_y, zoneItem, r, g, b, 60);
		}
	}

	// Cache isHouseTile — used multiple times below
	const bool is_house_tile = tile->isHouseTile();

	// Ground tooltip (one per item)
	if (ctx.options.show_tooltips && map_z == ctx.view.floor && tile->ground && ground_it) {
		TooltipData& groundData = tooltip_drawer->requestTooltipData();
		if (FillItemTooltipData(groundData, tile->ground.get(), *ground_it, position, is_house_tile, ctx.view.zoom)) {
			if (groundData.hasVisibleFields()) {
				tooltip_drawer->commitTooltip();
			}
		}
	}

	// end filters for ground tile

	// Draw helper border for selected house tiles
	// Only draw on the current floor (grid)
	if (ctx.options.show_houses && is_house_tile && static_cast<int>(tile->getHouseID()) == current_house_id && map_z == ctx.view.floor) {

		uint8_t hr, hg, hb;
		TileColorCalculator::GetHouseColor(tile->getHouseID(), hr, hg, hb);

		float intensity = 0.5f + (0.5f * ctx.options.highlight_pulse);
		// Optimization: Use integer math for border color to avoid vec4 construction and casting
		int ba = static_cast<int>(intensity * 255.0f);
		// hr, hg, hb are already uint8_t
		sprite_drawer->glDrawBox(ctx, draw_x, draw_y, 32, 32, DrawColor(hr, hg, hb, ba));
	}

	if (!only_colors) {
		if (ctx.view.zoom < 10.0 || !ctx.options.hide_items_when_zoomed) {
			// Hoist house color calculation out of item loop
			uint8_t house_r = 255, house_g = 255, house_b = 255;
			bool calculate_house_color = ctx.options.extended_house_shader && ctx.options.show_houses && is_house_tile;
			bool should_pulse = calculate_house_color && (static_cast<int>(tile->getHouseID()) == current_house_id) && (ctx.options.highlight_pulse > 0.0f);
			float boost = 0.0f;

			if (calculate_house_color) {
				TileColorCalculator::GetHouseColor(tile->getHouseID(), house_r, house_g, house_b);
				if (should_pulse) {
					boost = ctx.options.highlight_pulse * 0.6f;
				}
			}

			bool process_tooltips = ctx.options.show_tooltips && map_z == ctx.view.floor;

			for (const auto& item : tile->items) {
				if (draw_lights && item->hasLight()) {
					ctx.light_buffer.AddLight(position.x, position.y, position.z, item->getLight());
				}

				const ItemType& it = item->getType();

				if (process_tooltips) {
					TooltipData& itemData = tooltip_drawer->requestTooltipData();
					if (FillItemTooltipData(itemData, item.get(), it, position, is_house_tile, ctx.view.zoom)) {
						if (itemData.hasVisibleFields()) {
							tooltip_drawer->commitTooltip();
						}
					}
				}

				if (it.sprite) {
					SpritePatterns patterns = PatternCalculator::Calculate(it.sprite, it, item.get(), tile, position);

					// Inline preload check — skip function call when sprite is simple and loaded
					if (!it.sprite->isSimpleAndLoaded()) {
						rme::collectTileSprites(it.sprite, patterns.x, patterns.y, patterns.z, patterns.frame);
					}

					BlitItemParams params(position, item.get(), ctx.options);
					params.tile = tile;
					params.item_type = &it;
					params.patterns = &patterns;

					// item sprite
					if (item->isBorder()) {
						params.red = r;
						params.green = g;
						params.blue = b;
						item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					} else {
						uint8_t ir = 255, ig = 255, ib = 255;

						if (calculate_house_color) {
							// Apply house color tint
							ir = static_cast<uint8_t>(ir * house_r / 255);
							ig = static_cast<uint8_t>(ig * house_g / 255);
							ib = static_cast<uint8_t>(ib * house_b / 255);

							if (should_pulse) {
								// Pulse effect matching the tile pulse
								ir = static_cast<uint8_t>(std::min(255, static_cast<int>(ir + (255 - ir) * boost)));
								ig = static_cast<uint8_t>(std::min(255, static_cast<int>(ig + (255 - ig) * boost)));
								ib = static_cast<uint8_t>(std::min(255, static_cast<int>(ib + (255 - ib) * boost)));
							}
						}
						params.red = ir;
						params.green = ig;
						params.blue = ib;
						item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					}
				}
			}
			// monster/npc on tile
			if (tile->creature && ctx.options.show_creatures) {
				creature_drawer->BlitCreature(ctx, sprite_drawer, draw_x, draw_y, tile->creature.get(), CreatureDrawOptions { .map_pos = position, .transient_selection_bounds = ctx.options.transient_selection_bounds });
				if (creature_name_drawer) {
					creature_name_drawer->addLabel(position, tile->creature->getName(), tile->creature.get());
				}
			}
		}

		if (ctx.view.zoom < 10.0) {
			// markers (waypoint, house exit, town temple, spawn)
			marker_drawer->draw(ctx, sprite_drawer, draw_x, draw_y, tile, waypoint, current_house_id, *editor);
		}
	}
}

void TileRenderer::PreloadItem(const Tile* tile, Item* item, const ItemType& it, const SpritePatterns* cached_patterns) {
	if (!item) {
		return;
	}

	GameSprite* spr = it.sprite;
	if (spr && !spr->isSimpleAndLoaded()) {
		SpritePatterns patterns;
		if (cached_patterns) {
			patterns = *cached_patterns;
		} else {
			patterns = PatternCalculator::Calculate(spr, it, item, tile, tile->getPosition());
		}
		rme::collectTileSprites(spr, patterns.x, patterns.y, patterns.z, patterns.frame);
	}
}
