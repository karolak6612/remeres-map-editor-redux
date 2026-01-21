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

#include "../../main.h"
#include "tile_renderer.h"

// Include core game types
#include "tile.h"
#include "item.h"
#include "creature.h"
#include "map.h"
#include "editor.h"
#include "rendering/graphics.h"
#include "rendering/sprites.h"
#include "gui.h"
#include "items.h"
#include "rendering/types/render_types.h"
#include <algorithm>

namespace rme {
	namespace render {

		namespace {
			// kGroundLayer and kTileSize come from render_types.h

			// Convert fluid subtype to pattern index
			[[nodiscard]] constexpr int fluidToPattern(int subtype) noexcept {
				switch (subtype) {
					case 0:
						return 0; // empty
					case 1:
						return 1; // water
					case 2:
						return 2; // blood
					case 3:
						return 3; // beer
					case 4:
						return 4; // slime
					case 5:
						return 5; // lemonade
					case 6:
						return 6; // milk
					case 7:
						return 7; // mana fluid
					case 8:
						return 8; // life fluid
					case 9:
						return 9; // oil
					case 10:
						return 10; // urine
					case 11:
						return 11; // coconut milk
					case 12:
						return 12; // wine
					case 13:
						return 13; // mud
					case 14:
						return 14; // fruit juice
					case 15:
						return 15; // lava
					case 16:
						return 16; // rum
					default:
						return 0;
				}
			}
		}

		// Calculate stack display index (0-7) based on item count
		int TileRenderer::getStackIndex(int count) noexcept {
			if (count <= 1) {
				return 0;
			}
			if (count <= 2) {
				return 1;
			}
			if (count <= 3) {
				return 2;
			}
			if (count <= 4) {
				return 3;
			}
			if (count < 10) {
				return 4;
			}
			if (count < 25) {
				return 5;
			}
			if (count < 50) {
				return 6;
			}
			return 7;
		}

		void TileRenderer::bindTexture(GLuint textureId) {
			if (lastBoundTexture_ != textureId) {
				glBindTexture(GL_TEXTURE_2D, textureId);
				lastBoundTexture_ = textureId;
			}
		}

		void TileRenderer::glBlitTexture(int sx, int sy, int textureNum, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
			bindTexture(textureNum);

			float rF = r / 255.0f;
			float gF = g / 255.0f;
			float bF = b / 255.0f;
			float aF = alpha / 255.0f;

			glColor4f(rF, gF, bF, aF);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2i(sx, sy);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(sx + kTileSize, sy);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(sx + kTileSize, sy + kTileSize);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(sx, sy + kTileSize);
			glEnd();
		}

		void TileRenderer::glBlitSquare(int sx, int sy, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, int size) {
			glDisable(GL_TEXTURE_2D);

			float rF = r / 255.0f;
			float gF = g / 255.0f;
			float bF = b / 255.0f;
			float aF = alpha / 255.0f;

			glColor4f(rF, gF, bF, aF);
			glBegin(GL_QUADS);
			glVertex2i(sx, sy);
			glVertex2i(sx + size, sy);
			glVertex2i(sx + size, sy + size);
			glVertex2i(sx, sy + size);
			glEnd();

			glEnable(GL_TEXTURE_2D);
		}

		Color TileRenderer::calculateTileColor(const Tile* tile, const RenderOptions& opts, int currentHouseId) {
			Color color { 255, 255, 255, 255 };

			if (opts.showBlocking && tile->isBlocking() && tile->size() > 0) {
				color.g = color.g / 3 * 2;
				color.b = color.b / 3 * 2;
			}

			if (opts.highlightItems && !tile->items.empty() && !tile->items.back()->isBorder()) {
				static constexpr float kFactors[5] = { 0.75f, 0.6f, 0.48f, 0.40f, 0.33f };
				int idx = std::min<int>(static_cast<int>(tile->items.size()), 5) - 1;
				color.g = static_cast<uint8_t>(color.g * kFactors[idx]);
				color.r = static_cast<uint8_t>(color.r * kFactors[idx]);
			}

			if (opts.showHouses && tile->isHouseTile()) {
				if (static_cast<int>(tile->getHouseID()) == currentHouseId) {
					color.r /= 2;
				} else {
					color.r /= 2;
					color.g /= 2;
				}
			} else if ((opts.showOnlyColors || opts.showSpecialTiles) && tile->isPZ()) {
				color.r /= 2;
				color.b /= 2;
			}

			if ((opts.showOnlyColors || opts.showSpecialTiles) && (tile->getMapFlags() & TILESTATE_PVPZONE)) {
				color.g = color.r / 4;
				color.b = color.b / 3 * 2;
			}

			if ((opts.showOnlyColors || opts.showSpecialTiles) && (tile->getMapFlags() & TILESTATE_NOLOGOUT)) {
				color.b /= 2;
			}

			if ((opts.showOnlyColors || opts.showSpecialTiles) && (tile->getMapFlags() & TILESTATE_NOPVP)) {
				color.g /= 2;
			}

			return color;
		}

		void TileRenderer::drawSquare(int screenX, int screenY, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, int size) {
			glBlitSquare(screenX, screenY, r, g, b, alpha, size);
		}

		void TileRenderer::drawSprite(int screenX, int screenY, uint32_t spriteId, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
			GameSprite* spr = g_items[spriteId].sprite;
			if (!spr) {
				return;
			}
			drawGameSprite(screenX, screenY, spr, r, g, b, alpha);
		}

		void TileRenderer::drawGameSprite(int screenX, int screenY, GameSprite* spr, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
			if (!spr) {
				return;
			}

			int sx = screenX - spr->getDrawOffset().first;
			int sy = screenY - spr->getDrawOffset().second;

			for (int cx = 0; cx < spr->width; ++cx) {
				for (int cy = 0; cy < spr->height; ++cy) {
					for (int cf = 0; cf < spr->layers; ++cf) {
						GLuint texnum = spr->getHardwareID(cx, cy, cf, -1, 0, 0, 0, 0);
						glBlitTexture(sx - cx * kTileSize, sy - cy * kTileSize, texnum, r, g, b, alpha);
					}
				}
			}
		}

		void TileRenderer::drawCreature(int screenX, int screenY, const Creature* creature, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
			if (!creature) {
				return;
			}

			if (creature->isSelected()) {
				r /= 2;
				g /= 2;
				b /= 2;
			}

			const Outfit& outfit = creature->getLookType();
			drawCreatureOutfit(screenX, screenY, outfit, creature->getDirection(), r, g, b, alpha);
		}

		void TileRenderer::drawCreatureOutfit(int screenX, int screenY, const Outfit& outfit, Direction dir, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
			GameSprite* spr = nullptr;

			// Mount first (if any)
			if (outfit.lookMount > 0) {
				spr = g_gui.gfx.getCreatureSprite(outfit.lookMount);
				if (spr) {
					int dx = screenX - spr->getDrawOffset().first;
					int dy = screenY - spr->getDrawOffset().second;

					for (int cx = 0; cx < spr->width; ++cx) {
						for (int cy = 0; cy < spr->height; ++cy) {
							GLuint texnum = spr->getHardwareID(cx, cy, static_cast<int>(dir), 0, 0, outfit, 0);
							glBlitTexture(dx - cx * kTileSize, dy - cy * kTileSize, texnum, r, g, b, alpha);
						}
					}
				}
			}

			// Main creature sprite
			if (outfit.lookType > 0) {
				spr = g_gui.gfx.getCreatureSprite(outfit.lookType);
				if (spr) {
					int dx = screenX - spr->getDrawOffset().first;
					int dy = screenY - spr->getDrawOffset().second;

					for (int addon = 0; addon < 3; ++addon) {
						if (addon > 0 && !(outfit.lookAddon & (1 << (addon - 1)))) {
							continue; // Skip unequipped addons
						}

						for (int cx = 0; cx < spr->width; ++cx) {
							for (int cy = 0; cy < spr->height; ++cy) {
								GLuint texnum = spr->getHardwareID(cx, cy, static_cast<int>(dir), addon, outfit.lookMount > 0 ? 1 : 0, outfit, 0);
								glBlitTexture(dx - cx * kTileSize, dy - cy * kTileSize, texnum, r, g, b, alpha);
							}
						}
					}
				}
			} else if (outfit.lookItem > 0) {
				// Item as creature visual
				drawSprite(screenX, screenY, outfit.lookItem, r, g, b, alpha);
			}
		}

		void TileRenderer::drawRawBrush(int screenX, int screenY, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
			if (!itemType || !itemType->sprite) {
				return;
			}

			GameSprite* spr = itemType->sprite;
			int sx = screenX - spr->getDrawOffset().first;
			int sy = screenY - spr->getDrawOffset().second;

			for (int cx = 0; cx < spr->width; ++cx) {
				for (int cy = 0; cy < spr->height; ++cy) {
					for (int cf = 0; cf < spr->layers; ++cf) {
						GLuint texnum = spr->getHardwareID(cx, cy, cf, -1, 0, 0, 0, 0);
						glBlitTexture(sx - cx * kTileSize, sy - cy * kTileSize, texnum, r, g, b, alpha);
					}
				}
			}
		}

		void TileRenderer::drawItem(int& drawX, int& drawY, const Tile* tile, Item* item, bool ephemeral, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, const RenderContext& ctx, const RenderOptions& opts) {
			if (!tile || !item) {
				return;
			}
			drawItemAt(drawX, drawY, tile->getPosition(), item, ephemeral, r, g, b, alpha, tile, ctx, opts);
		}

		void TileRenderer::drawItemAt(int& drawX, int& drawY, const Position& pos, Item* item, bool ephemeral, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, const Tile* tile, const RenderContext& ctx, const RenderOptions& opts) {
			if (!item) {
				return;
			}

			ItemType& it = g_items[item->getID()];

			// Locked door indicator
			if (!opts.ingame && opts.highlightLockedDoors && it.isDoor() && it.isLocked) {
				b /= 2;
				g /= 2;
			}

			// Selected item indicator
			if (!opts.ingame && !ephemeral && item->isSelected()) {
				r /= 2;
				b /= 2;
				g /= 2;
			}

			GameSprite* spr = it.sprite;

			// Handle invisible/technical items
			if (!opts.ingame && opts.showTechItems) {
				// Red for invalid ID
				if (it.id == 0) {
					drawSquare(drawX, drawY, r, 0, 0, alpha);
					return;
				}

				// Special client IDs visualization
				switch (it.clientID) {
					case 469: // Invisible stairs
						drawSquare(drawX, drawY, r, g, 0, alpha / 3 * 2);
						return;
					case 470:
					case 17970:
					case 20028:
					case 34168: // Invisible walkable
						drawSquare(drawX, drawY, r, 0, 0, alpha / 3 * 2);
						return;
					case 2187: // Invisible wall
						drawSquare(drawX, drawY, 0, g, b, 80);
						return;
				}

				// Primal light sources
				if ((it.clientID >= 39092 && it.clientID <= 39100) || it.clientID == 39236 || it.clientID == 39367 || it.clientID == 39368) {
					spr = g_items[SPRITE_LIGHTSOURCE].sprite;
					r = 0;
					alpha = 180;
				}
			}

			// Skip meta items or missing sprites
			if (it.isMetaItem() || !spr || (!ephemeral && it.pickupable && !opts.showItems)) {
				return;
			}

			int screenx = drawX - spr->getDrawOffset().first;
			int screeny = drawY - spr->getDrawOffset().second;

			// Update stacking offset
			drawX -= spr->getDrawHeight();
			drawY -= spr->getDrawHeight();

			// Calculate pattern based on position and item type
			int patternX = pos.x % spr->pattern_x;
			int patternY = pos.y % spr->pattern_y;
			int patternZ = pos.z % spr->pattern_z;
			int subtype = -1;

			if (it.isSplash() || it.isFluidContainer()) {
				subtype = item->getSubtype();
			} else if (it.isHangable && tile) {
				if (tile->hasProperty(HOOK_SOUTH)) {
					patternX = 1;
				} else if (tile->hasProperty(HOOK_EAST)) {
					patternX = 2;
				} else {
					patternX = 0;
				}
			} else if (it.stackable) {
				subtype = getStackIndex(item->getSubtype());
			}

			// Transparency for items
			if (!ephemeral && opts.transparentItems && (!it.isGroundTile() || spr->width > 1 || spr->height > 1) && !it.isSplash() && (!it.isBorder || spr->width > 1 || spr->height > 1)) {
				alpha /= 2;
			}

			// Podium handling
			Podium* podium = dynamic_cast<Podium*>(item);
			if (it.isPodium() && podium && !podium->hasShowPlatform() && !opts.ingame) {
				alpha = opts.showTechItems ? alpha / 2 : 0;
			}

			// Draw sprite layers
			int frame = item->getFrame();
			for (int cx = 0; cx < spr->width; ++cx) {
				for (int cy = 0; cy < spr->height; ++cy) {
					for (int cf = 0; cf < spr->layers; ++cf) {
						GLuint texnum = spr->getHardwareID(cx, cy, cf, subtype, patternX, patternY, patternZ, frame);
						glBlitTexture(screenx - cx * kTileSize, screeny - cy * kTileSize, texnum, r, g, b, alpha);
					}
				}
			}

			// Draw creature on podium
			if (ctx.zoom <= 3.0f && it.isPodium() && podium) {
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
				drawCreatureOutfit(drawX, drawY, outfit, static_cast<Direction>(podium->getDirection()), r, g, b, 255);
			}
		}

		void TileRenderer::drawTile(TileLocation* location, const RenderContext& ctx, const RenderOptions& opts, uint8_t r_override, uint8_t g_override, uint8_t b_override, uint8_t alpha_override) {
			if (!location) {
				return;
			}

			Tile* tile = location->get();
			if (!tile) {
				return;
			}

			if (opts.showOnlyModified && !tile->isModified()) {
				return;
			}

			const int mapX = location->getX();
			const int mapY = location->getY();
			const int mapZ = location->getZ();

			// Calculate screen position with floor offset
			const int offset = (mapZ <= kGroundLayer)
				? (kGroundLayer - mapZ) * kTileSize
				: kTileSize * (ctx.currentFloor - mapZ);

			const int screenX = (mapX * kTileSize) - ctx.viewScrollX - offset;
			const int screenY = (mapY * kTileSize) - ctx.viewScrollY - offset;

			// Early viewport culling
			constexpr int margin = kTileSize * 3;
			if (screenX < -margin || screenX > ctx.screensizeX * ctx.zoom + margin || screenY < -margin || screenY > ctx.screensizeY * ctx.zoom + margin) {
				return;
			}

			int drawX = screenX;
			int drawY = screenY;

			Color tileColor = calculateTileColor(tile, opts, ctx.currentHouseId);
			uint8_t r = (r_override == 255 ? tileColor.r : static_cast<uint8_t>(tileColor.r * (r_override / 255.0f)));
			uint8_t g = (g_override == 255 ? tileColor.g : static_cast<uint8_t>(tileColor.g * (g_override / 255.0f)));
			uint8_t b = (b_override == 255 ? tileColor.b : static_cast<uint8_t>(tileColor.b * (b_override / 255.0f)));
			uint8_t alpha = alpha_override;

			const bool onlyColors = opts.showAsMinimap || opts.showOnlyColors;

			if (onlyColors) {
				if (opts.showAsMinimap) {
					uint8_t mmColor = tile->getMiniMapColor();
					r = static_cast<uint8_t>((mmColor / 36) % 6 * 51);
					g = static_cast<uint8_t>((mmColor / 6) % 6 * 51);
					b = static_cast<uint8_t>(mmColor % 6 * 51);
					drawSquare(drawX, drawY, r, g, b, 255);
				} else if (r != 255 || g != 255 || b != 255) {
					drawSquare(drawX, drawY, r, g, b, 128);
				}
				return;
			}

			// Draw ground
			if (tile->ground) {
				if (opts.showPreview && ctx.zoom <= 2.0f) {
					tile->ground->animate();
				}
				drawItem(drawX, drawY, tile, tile->ground, false, r, g, b, 255, ctx, opts);
			} else if (opts.alwaysShowZones && (r != 255 || g != 255 || b != 255)) {
				drawRawBrush(drawX, drawY, &g_items[SPRITE_ZONE], r, g, b, 60);
			}

			// Draw items (skip if zoomed out too far)
			if (ctx.zoom < 10.0f || !opts.hideItemsWhenZoomed) {
				for (Item* item : tile->items) {
					if (opts.showPreview && ctx.zoom <= 2.0f) {
						item->animate();
					}

					if (item->isBorder()) {
						drawItem(drawX, drawY, tile, item, false, r, g, b, 255, ctx, opts);
					} else {
						uint8_t itemR = 255, itemG = 255, itemB = 255;
						if (opts.extendedHouseShader && opts.showHouses && tile->isHouseTile()) {
							if (static_cast<int>(tile->getHouseID()) == ctx.currentHouseId) {
								itemR /= 2;
							} else {
								itemR /= 2;
								itemG /= 2;
							}
						}
						drawItem(drawX, drawY, tile, item, false, itemR, itemG, itemB, 255, ctx, opts);
					}
				}

				// Draw creature
				if (tile->creature && opts.showCreatures) {
					drawCreature(drawX, drawY, tile->creature);
				}
			}

			if (ctx.zoom >= 10.0f) {
				return;
			}

			// Draw waypoint indicator
			if (!opts.ingame && opts.showWaypoints && ctx.editor) {
				Waypoint* wp = ctx.editor->map.waypoints.getWaypoint(location);
				if (wp) {
					drawSprite(drawX, drawY, SPRITE_WAYPOINT, 64, 64, 255);
				}
			}

			// Draw house exit
			if (tile->isHouseExit() && opts.showHouses) {
				if (tile->hasHouseExit(ctx.currentHouseId)) {
					drawSprite(drawX, drawY, SPRITE_HOUSE_EXIT, 64, 255, 255);
				} else {
					drawSprite(drawX, drawY, SPRITE_HOUSE_EXIT, 64, 64, 255);
				}
			}

			// Draw town temple
			if (opts.showTowns && ctx.editor && tile->isTownExit(ctx.editor->map)) {
				drawSprite(drawX, drawY, SPRITE_TOWN_TEMPLE, 255, 255, 64, 170);
			}

			// Draw spawn indicator
			if (tile->spawn && opts.showSpawns) {
				if (tile->spawn->isSelected()) {
					drawSprite(drawX, drawY, SPRITE_SPAWN, 128, 128, 128);
				} else {
					drawSprite(drawX, drawY, SPRITE_SPAWN, 255, 255, 255);
				}
			}
		}

	} // namespace render
} // namespace rme
