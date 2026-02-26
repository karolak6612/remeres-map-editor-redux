#include "app/main.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/render_list.h"
#include "rendering/core/primitive_renderer.h"
#include "ui/gui.h"

#include "editor/editor.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "game/complexitem.h"

#include "rendering/core/drawing_options.h"
#include "rendering/core/render_view.h"
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
#include "rendering/systems/house_highlight_system.h"

TileRenderer::TileRenderer(ItemDrawer* id, SpriteDrawer* sd, CreatureDrawer* cd, CreatureNameDrawer* cnd, FloorDrawer* fd, MarkerDrawer* md) :
	item_drawer(id), sprite_drawer(sd), creature_drawer(cd), creature_name_drawer(cnd), floor_drawer(fd), marker_drawer(md) {
}

void TileRenderer::DrawTile(SpriteBatch& sprite_batch, TileLocation* location, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id, const MarkerFlags& marker_flags, int in_draw_x, int in_draw_y) {
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
	if (in_draw_x != -1 && in_draw_y != -1) {
		draw_x = in_draw_x;
		draw_y = in_draw_y;
	} else {
		// Early viewport culling - skip tiles that are completely off-screen
		if (!view.IsTileVisible(map_x, map_y, map_z, draw_x, draw_y)) {
			return;
		}
	}

	// Note: We don't check for tooltip logic here anymore, as it's been moved to TooltipDrawer / hover system

	bool as_minimap = options.show_as_minimap;
	bool only_colors = as_minimap || options.show_only_colors;

	uint8_t r = 255, g = 255, b = 255;

	// begin filters for ground tile
	if (!as_minimap) {
		TileColorCalculator::Calculate(tile, options, current_house_id, location->getSpawnCount(), r, g, b);
	}

	const ItemType* ground_it = nullptr;
	if (tile->ground) {
		ground_it = &g_items[tile->ground->getID()];
	}

	if (only_colors) {
		if (as_minimap) {
			TileColorCalculator::GetMinimapColor(tile, r, g, b);
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(r, g, b, 255));
		} else if (r != 255 || g != 255 || b != 255) {
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(r, g, b, 128));
		}
	} else {
		if (tile->ground && ground_it) {
			if (ground_it->sprite) {
				SpritePatterns patterns = PatternCalculator::Calculate(ground_it->sprite, *ground_it, tile->ground.get(), tile, location->getPosition());
				PreloadItem(tile, tile->ground.get(), *ground_it, &patterns);

				BlitItemParams params(tile, tile->ground.get(), options);
				params.red = r;
				params.green = g;
				params.blue = b;
				params.patterns = patterns;
				params.has_patterns = true;
				item_drawer->BlitItem(sprite_batch, draw_x, draw_y, params);
			}
		} else if (options.always_show_zones && (r != 255 || g != 255 || b != 255)) {
			ItemType* zoneItem = &g_items[SPRITE_ZONE];
			item_drawer->DrawRawBrush(sprite_batch, draw_x, draw_y, zoneItem, r, g, b, 60);
		}
	}

	// end filters for ground tile

	// Draw helper border for selected house tiles
	// Only draw on the current floor (grid)
	if (map_z == view.floor && tile->isHouseTile()) {
		HouseHighlightSystem::DrawBorder(sprite_batch, sprite_drawer, draw_x, draw_y, tile->getHouseID(), options);
	}

	if (!only_colors) {
		if (view.zoom < 10.0 || !options.hide_items_when_zoomed) {
			// Hoist house color calculation out of item loop
			uint8_t house_tint_r = 255, house_tint_g = 255, house_tint_b = 255;
			bool has_tint = HouseHighlightSystem::CalculateTint(tile, options, house_tint_r, house_tint_g, house_tint_b);

			bool process_tooltips = options.show_tooltips && map_z == view.floor;

			// items on tile
			for (const auto& item : tile->items) {
				const ItemType& it = g_items[item->getID()];

				if (it.sprite) {
					SpritePatterns patterns = PatternCalculator::Calculate(it.sprite, it, item.get(), tile, location->getPosition());
					PreloadItem(tile, item.get(), it, &patterns);

					BlitItemParams params(tile, item.get(), options);
					params.patterns = patterns;
					params.has_patterns = true;

					// item sprite
					if (item->isBorder()) {
						params.red = r;
						params.green = g;
						params.blue = b;
						item_drawer->BlitItem(sprite_batch, draw_x, draw_y, params);
					} else {
						params.red = house_tint_r;
						params.green = house_tint_g;
						params.blue = house_tint_b;
						item_drawer->BlitItem(sprite_batch, draw_x, draw_y, params);
					}
				}
			}
			// monster/npc on tile
			if (tile->creature && options.show_creatures) {
				creature_drawer->BlitCreature(sprite_batch, draw_x, draw_y, tile->creature.get());
				if (creature_name_drawer) {
					creature_name_drawer->addLabel(location->getPosition(), tile->creature->getName(), tile->creature.get());
				}
			}
		}

		if (view.zoom < 10.0) {
			// markers (waypoint, house exit, town temple, spawn)
			marker_drawer->draw(sprite_batch, sprite_drawer, draw_x, draw_y, marker_flags, options);
		}
	}
}

void TileRenderer::DrawTile(RenderList& list, TileLocation* location, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id, const MarkerFlags& marker_flags, int in_draw_x, int in_draw_y) {
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
	if (in_draw_x != -1 && in_draw_y != -1) {
		draw_x = in_draw_x;
		draw_y = in_draw_y;
	} else {
		if (!view.IsTileVisible(map_x, map_y, map_z, draw_x, draw_y)) {
			return;
		}
	}

	bool as_minimap = options.show_as_minimap;
	bool only_colors = as_minimap || options.show_only_colors;

	uint8_t r = 255, g = 255, b = 255;

	if (!as_minimap) {
		TileColorCalculator::Calculate(tile, options, current_house_id, location->getSpawnCount(), r, g, b);
	}

	const ItemType* ground_it = nullptr;
	if (tile->ground) {
		ground_it = &g_items[tile->ground->getID()];
	}

	if (only_colors) {
		if (as_minimap) {
			TileColorCalculator::GetMinimapColor(tile, r, g, b);
			sprite_drawer->glBlitSquare(list, draw_x, draw_y, DrawColor(r, g, b, 255));
		} else if (r != 255 || g != 255 || b != 255) {
			sprite_drawer->glBlitSquare(list, draw_x, draw_y, DrawColor(r, g, b, 128));
		}
	} else {
		if (tile->ground && ground_it) {
			if (ground_it->sprite) {
				SpritePatterns patterns = PatternCalculator::Calculate(ground_it->sprite, *ground_it, tile->ground.get(), tile, location->getPosition());
				PreloadItem(tile, tile->ground.get(), *ground_it, &patterns);

				BlitItemParams params(tile, tile->ground.get(), options);
				params.red = r;
				params.green = g;
				params.blue = b;
				params.patterns = patterns;
				params.has_patterns = true;
				item_drawer->BlitItem(list, draw_x, draw_y, params);
			}
		} else if (options.always_show_zones && (r != 255 || g != 255 || b != 255)) {
			ItemType* zoneItem = &g_items[SPRITE_ZONE];
			item_drawer->DrawRawBrush(list, draw_x, draw_y, zoneItem, r, g, b, 60);
		}
	}

	if (map_z == view.floor && tile->isHouseTile()) {
		HouseHighlightSystem::DrawBorder(list, sprite_drawer, draw_x, draw_y, tile->getHouseID(), options);
	}

	if (!only_colors) {
		if (view.zoom < 10.0 || !options.hide_items_when_zoomed) {
			uint8_t house_tint_r = 255, house_tint_g = 255, house_tint_b = 255;
			bool has_tint = HouseHighlightSystem::CalculateTint(tile, options, house_tint_r, house_tint_g, house_tint_b);

			bool process_tooltips = options.show_tooltips && map_z == view.floor;

			for (const auto& item : tile->items) {
				const ItemType& it = g_items[item->getID()];

				if (it.sprite) {
					SpritePatterns patterns = PatternCalculator::Calculate(it.sprite, it, item.get(), tile, location->getPosition());
					PreloadItem(tile, item.get(), it, &patterns);

					BlitItemParams params(tile, item.get(), options);
					params.patterns = patterns;
					params.has_patterns = true;

					if (item->isBorder()) {
						params.red = r;
						params.green = g;
						params.blue = b;
						item_drawer->BlitItem(list, draw_x, draw_y, params);
					} else {
						params.red = house_tint_r;
						params.green = house_tint_g;
						params.blue = house_tint_b;
						item_drawer->BlitItem(list, draw_x, draw_y, params);
					}
				}
			}
			if (tile->creature && options.show_creatures) {
				creature_drawer->BlitCreature(list, draw_x, draw_y, tile->creature.get());
				if (creature_name_drawer) {
					creature_name_drawer->addLabel(location->getPosition(), tile->creature->getName(), tile->creature.get());
				}
			}
		}

		if (view.zoom < 10.0) {
			marker_drawer->draw(list, sprite_drawer, draw_x, draw_y, marker_flags, options);
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

void TileRenderer::AddLight(TileLocation* location, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	if (!options.isDrawLight() || !location) {
		return;
	}

	auto tile = location->get();
	if (!tile || !tile->hasLight()) {
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
		for (const auto& item : tile->items) {
			if (item->hasLight()) {
				light_buffer.AddLight(position.x, position.y, position.z, item->getLight());
			}
		}
	}
}
