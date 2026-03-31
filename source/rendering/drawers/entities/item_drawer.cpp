//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include <algorithm>
#undef min
#undef max

#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/core/graphics.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/drawing_options.h"
#include "rendering/utilities/pattern_calculator.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/complexitem.h"
#include "game/sprites.h"
#include "ui/gui.h"
#include "app/visuals.h"
#include "rendering/drawers/overlays/visual_overlay_drawer.h"

namespace {
	GameSprite* resolveSprite(const ItemDefinitionView& definition) {
		if (!definition) {
			return nullptr;
		}
		return dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(definition.clientId()));
	}

	GameSprite* resolveSprite(ServerItemId item_id) {
		return resolveSprite(g_item_definitions.get(item_id));
	}

	GameSprite* resolveAppearanceSprite(const VisualAppearance& appearance) {
		switch (appearance.type) {
			case VisualAppearanceType::SpriteId:
				return dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(appearance.sprite_id));
			case VisualAppearanceType::OtherItemVisual:
				return resolveSprite(appearance.item_id);
			case VisualAppearanceType::Rgba:
			case VisualAppearanceType::Png:
			case VisualAppearanceType::Svg:
				return nullptr;
		}
		return nullptr;
	}

	GameSprite* resolveResourceSprite(const ResolvedVisualResource& resource) {
		switch (resource.kind) {
			case VisualResourceKind::NativeSpriteId:
				return dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(resource.sprite_id));
			case VisualResourceKind::NativeItemVisual:
				return resolveSprite(resource.item_id);
			case VisualResourceKind::None:
			case VisualResourceKind::FlatColor:
			case VisualResourceKind::AtlasSprite:
				return nullptr;
		}
		return nullptr;
	}

	bool enqueueVisualAtlas(VisualOverlayDrawer* drawer, const Position& pos, const wxColour& color, const ResolvedVisualResource& resource, VisualOverlayPlacement placement = VisualOverlayPlacement::TileInset) {
		if (!drawer || resource.kind != VisualResourceKind::AtlasSprite || resource.atlas_sprite_id == 0) {
			return false;
		}

		drawer->add(VisualOverlayRequest {
			.pos = pos,
			.atlas_sprite_id = resource.atlas_sprite_id,
			.color = color,
			.placement = placement
		});
		return true;
	}
}

BlitItemParams::BlitItemParams(const Tile* t, Item* i, const DrawingOptions& o) : tile(t), item(i), options(&o) {
	if (t) {
		pos = t->getPosition();
	}
}

BlitItemParams::BlitItemParams(const Position& p, Item* i, const DrawingOptions& o) : pos(p), item(i), options(&o) {
}

ItemDrawer::ItemDrawer() {
}

ItemDrawer::~ItemDrawer() {
}

void ItemDrawer::BlitItem(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params) {
	const Position& pos = params.pos;
	Item* item = params.item;
	const Tile* tile = params.tile;
	const DrawingOptions& options = *params.options;
	bool ephemeral = params.ephemeral;
	int red = params.red;
	int green = params.green;
	int blue = params.blue;
	int alpha = params.alpha;
	const SpritePatterns* cached_patterns = params.patterns;

	const ItemDefinitionView it = params.item_definition ? params.item_definition : item->getDefinition();
	ItemDefinitionView draw_definition = it;

	// Locked door indicator
	if (!options.ingame && options.highlight_locked_doors && it.isDoor()) {
		bool locked = item->isLocked();

		// Door orientation: horizontal wall -> West border (south=true), vertical wall -> North border (east=true)
		if (static_cast<BorderType>(it.attribute(ItemAttributeKey::BorderAlignment)) == WALL_HORIZONTAL) {
			DrawDoorIndicator(locked, pos, true, false);
		} else if (static_cast<BorderType>(it.attribute(ItemAttributeKey::BorderAlignment)) == WALL_VERTICAL) {
			DrawDoorIndicator(locked, pos, false, true);
		} else {
			// Center case for non-aligned doors
			DrawDoorIndicator(locked, pos, false, false);
		}
	}

	bool is_transient_selected = !ephemeral && options.transient_selection_bounds && options.transient_selection_bounds->contains(pos.x, pos.y);
	if (!options.ingame && (item->isSelected() || is_transient_selected)) {
		red /= 2;
		blue /= 2;
		green /= 2;
	}

	// item sprite
	GameSprite* spr = resolveSprite(draw_definition);

	if (!options.ingame) {
		const wxColour base_color(
			static_cast<unsigned char>(red),
			static_cast<unsigned char>(green),
			static_cast<unsigned char>(blue),
			static_cast<unsigned char>(alpha)
		);
		if (const ResolvedVisualResource* visual_resource = g_visuals.ResolveItemResource(item->getID()); visual_resource) {
			const wxColour final_color = Visuals::CombineColor(base_color, visual_resource->color);
			switch (visual_resource->kind) {
				case VisualResourceKind::FlatColor:
					sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(final_color.Red(), final_color.Green(), final_color.Blue(), final_color.Alpha()));
					return;
				case VisualResourceKind::NativeSpriteId: {
					if (GameSprite* override_sprite = resolveResourceSprite(*visual_resource)) {
						spr = override_sprite;
					}
					red = final_color.Red();
					green = final_color.Green();
					blue = final_color.Blue();
					alpha = final_color.Alpha();
					break;
				}
				case VisualResourceKind::NativeItemVisual:
					draw_definition = g_item_definitions.get(visual_resource->item_id);
					spr = resolveSprite(draw_definition);
					red = final_color.Red();
					green = final_color.Green();
					blue = final_color.Blue();
					alpha = final_color.Alpha();
					break;
				case VisualResourceKind::AtlasSprite:
					if (enqueueVisualAtlas(visual_overlay_drawer, pos, final_color, *visual_resource)) {
						return;
					}
					break;
				case VisualResourceKind::None:
					break;
			}
		}
	}

	// Invalid items still need a visible fallback even if no visual rule exists.
	if (!options.ingame && options.show_tech_items && !g_visuals.ResolveItem(item->getID())) {
		if (!it) {
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, 0, 0, alpha));
			return;
		}
	}

	// metaItem, sprite not found or not hidden
	if (draw_definition.isMetaItem() || spr == nullptr || !ephemeral && draw_definition.hasFlag(ItemFlag::Pickupable) && !options.show_items) {
		return;
	}

	// int screenx = draw_x - spr->getDrawOffset().first;
	// int screeny = draw_y - spr->getDrawOffset().second;
	// The original code modified draw_x/draw_y AFTER calculating screenx/screeny using the original draw_x/draw_y
	// screenx use input draw_x
	int screenx = draw_x - spr->drawoffset_x;
	int screeny = draw_y - spr->drawoffset_y;

	// Set the newd drawing height accordingly
	draw_x -= spr->draw_height;
	draw_y -= spr->draw_height;

	SpritePatterns patterns;
	if (cached_patterns && spr == resolveSprite(draw_definition)) {
		patterns = *cached_patterns;
	} else {
		patterns = PatternCalculator::Calculate(spr, draw_definition, item, tile, pos);
	}

	int subtype = patterns.subtype;
	int pattern_x = patterns.x;
	int pattern_y = patterns.y;
	int pattern_z = patterns.z;
	int frame = patterns.frame;

	if (!ephemeral && options.transparent_items && (!draw_definition.isGroundTile() || spr->width > 1 || spr->height > 1) && !draw_definition.isSplash() && (!draw_definition.hasFlag(ItemFlag::IsBorder) || spr->width > 1 || spr->height > 1)) {
		alpha /= 2;
	}

	if (draw_definition.isPodium()) {
		Podium* podium = static_cast<Podium*>(item);
		if (!podium->hasShowPlatform() && !options.ingame) {
			if (options.show_tech_items) {
				alpha /= 2;
			} else {
				alpha = 0;
			}
		}
	}

	// Atlas-only rendering
	// g_gui.gfx.ensureAtlasManager();
	// BatchRenderer::SetAtlasManager(g_gui.gfx.getAtlasManager());

	if (spr->width == 1 && spr->height == 1 && spr->layers == 1) {
		const AtlasRegion* region;
		if (subtype == -1 && pattern_x == 0 && pattern_y == 0 && pattern_z == 0 && frame == 0) {
			region = spr->getAtlasRegion(0, 0, 0, -1, 0, 0, 0, 0);
		} else {
			region = spr->getAtlasRegion(0, 0, 0, subtype, pattern_x, pattern_y, pattern_z, frame);
		}

		if (region) {
#ifdef DEBUG
			// DEBUG: Check for mismatch on Item 369 using PRECISE sub-sprite ID
			if (item->getID() == 369) {
				// Use 0,0 as pattern coordinates for 1x1 items
				uint32_t precise_expected_id = spr->getSpriteId(frame, 0, 0);
				if (region->debug_sprite_id != 0 && precise_expected_id != 0 && region->debug_sprite_id != precise_expected_id) {
					spdlog::error("SPRITE MISMATCH DETECTED: Item 369 (Expected Sprite ID {}, Actual Region Owner {})", precise_expected_id, region->debug_sprite_id);
				}
			}
#endif
			sprite_drawer->glBlitAtlasQuad(sprite_batch, screenx, screeny, region, DrawColor(red, green, blue, alpha));
		}
	} else {
		for (int cx = 0; cx != spr->width; cx++) {
			for (int cy = 0; cy != spr->height; cy++) {
				for (int cf = 0; cf != spr->layers; cf++) {
					const AtlasRegion* region = spr->getAtlasRegion(cx, cy, cf, subtype, pattern_x, pattern_y, pattern_z, frame);
					if (region) {
						sprite_drawer->glBlitAtlasQuad(sprite_batch, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, DrawColor(red, green, blue, alpha));
					}
				}
			}
		}
	}

	if (draw_definition.isPodium()) {
		Podium* podium = static_cast<Podium*>(item);
		Outfit outfit = podium->getOutfit();
		if (!podium->hasShowOutfit()) {
			if (podium->hasShowMount()) {
				outfit.lookType = outfit.lookMount;
				outfit.lookHead = outfit.lookMountHead;
				outfit.lookBody = outfit.lookMountBody;
				outfit.lookLegs = outfit.lookMountLegs;
				outfit.lookFeet = outfit.lookMountFeet;
				outfit.lookAddon = 0;
				outfit.lookMount = 0;
			} else {
				outfit.lookType = 0;
			}
		}
		if (!podium->hasShowMount()) {
			outfit.lookMount = 0;
		}

		creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, outfit, static_cast<Direction>(podium->getDirection()), CreatureDrawOptions { .color = DrawColor(red, green, blue, alpha) });
	}

	// draw wall hook
	if (!options.ingame && options.show_hooks && (draw_definition.hasFlag(ItemFlag::HookSouth) || draw_definition.hasFlag(ItemFlag::HookEast))) {
		DrawHookIndicator(draw_definition, pos);
	}

	// draw light color indicator
	if (!options.ingame && options.show_light_str) {
		const SpriteLight& light = item->getLight();
		if (light.intensity > 0) {
			wxColor lightColor = colorFromEightBit(light.color);
			uint8_t byteR = lightColor.Red();
			uint8_t byteG = lightColor.Green();
			uint8_t byteB = lightColor.Blue();
			uint8_t byteA = 255;

			int startOffset = std::max<int>(16, 32 - light.intensity);
			int sqSize = TILE_SIZE - startOffset;
			const ResolvedVisualResource* light_resource = g_visuals.ResolveOverlayResource(OverlayVisualKind::LightIndicator);
			if (!light_resource) {
				sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 2, draw_y + startOffset - 2, DrawColor(0, 0, 0, byteA), sqSize + 2);
				sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 1, draw_y + startOffset - 1, DrawColor(byteR, byteG, byteB, byteA), sqSize);
			} else {
				const wxColour final_color = Visuals::CombineColor(wxColour(byteR, byteG, byteB, byteA), light_resource->color);
				switch (light_resource->kind) {
					case VisualResourceKind::FlatColor:
						sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 2, draw_y + startOffset - 2, DrawColor(0, 0, 0, byteA), sqSize + 2);
						sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 1, draw_y + startOffset - 1, DrawColor(final_color.Red(), final_color.Green(), final_color.Blue(), final_color.Alpha()), sqSize);
						break;
					case VisualResourceKind::NativeSpriteId:
					case VisualResourceKind::NativeItemVisual:
						if (GameSprite* sprite = resolveResourceSprite(*light_resource)) {
							sprite_drawer->BlitSprite(sprite_batch, draw_x, draw_y, sprite, DrawColor(final_color.Red(), final_color.Green(), final_color.Blue(), final_color.Alpha()));
						}
						break;
					case VisualResourceKind::AtlasSprite:
						if (!enqueueVisualAtlas(visual_overlay_drawer, pos, final_color, *light_resource, VisualOverlayPlacement::TileCenter)) {
							sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 2, draw_y + startOffset - 2, DrawColor(0, 0, 0, byteA), sqSize + 2);
							sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 1, draw_y + startOffset - 1, DrawColor(final_color.Red(), final_color.Green(), final_color.Blue(), final_color.Alpha()), sqSize);
						}
						break;
					case VisualResourceKind::None:
						break;
				}
			}
		}
	}
}

void ItemDrawer::DrawRawBrush(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, ServerItemId item_id, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
	const auto definition = g_item_definitions.get(item_id);
	GameSprite* spr = resolveSprite(definition);

	if (const VisualRule* visual_rule = g_visuals.ResolveItem(item_id); visual_rule) {
		switch (visual_rule->appearance.type) {
			case VisualAppearanceType::Rgba:
				sprite_drawer->glBlitSquare(sprite_batch, screenx, screeny, DrawColor(visual_rule->appearance.color.Red(), visual_rule->appearance.color.Green(), visual_rule->appearance.color.Blue(), visual_rule->appearance.color.Alpha()));
				return;
			case VisualAppearanceType::SpriteId:
			case VisualAppearanceType::OtherItemVisual:
				if (GameSprite* override_sprite = resolveAppearanceSprite(visual_rule->appearance)) {
					sprite_drawer->BlitSprite(sprite_batch, screenx, screeny, override_sprite, DrawColor(visual_rule->appearance.color.Red(), visual_rule->appearance.color.Green(), visual_rule->appearance.color.Blue(), visual_rule->appearance.color.Alpha()));
					return;
				}
				break;
			case VisualAppearanceType::Png:
			case VisualAppearanceType::Svg:
				break;
		}
	}

	if (spr) {
		sprite_drawer->BlitSprite(sprite_batch, screenx, screeny, spr, DrawColor(r, g, b, alpha));
	}
}

void ItemDrawer::DrawHookIndicator(const ItemDefinitionView& definition, const Position& pos) {
	if (hook_indicator_drawer) {
		hook_indicator_drawer->addHook(pos, definition.hasFlag(ItemFlag::HookSouth), definition.hasFlag(ItemFlag::HookEast));
	}
}

void ItemDrawer::DrawDoorIndicator(bool locked, const Position& pos, bool south, bool east) {
	if (door_indicator_drawer) {
		door_indicator_drawer->addDoor(pos, locked, south, east);
	}
}
