#include "lua_overlay_drawer.h"
#include "rendering/map_drawer.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/text_renderer.h"
#include "rendering/core/graphics.h"
#include "rendering/core/coordinate_mapper.h"
#include "lua/lua_script_manager.h"

LuaOverlayDrawer::LuaOverlayDrawer(MapDrawer* mapDrawer) : mapDrawer(mapDrawer) {
}

LuaOverlayDrawer::~LuaOverlayDrawer() {
}

void LuaOverlayDrawer::Draw(const RenderView& view, const DrawingOptions& options) {
	if (!g_luaScripts.isInitialized()) return;

	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (shows.empty()) return;

	auto* primitives = mapDrawer->getPrimitiveRenderer();
	auto* textRenderer = Graphics::getInstance().getTextRenderer();
	auto* spriteBatch = mapDrawer->getSpriteBatch();

	for (const auto& show : shows) {
		if (!g_luaScripts.isMapOverlayEnabled(show.overlayId)) continue;

		MapViewInfo viewInfo;
		viewInfo.start_x = view.start_x;
		viewInfo.start_y = view.start_y;
		viewInfo.end_x = view.end_x;
		viewInfo.end_y = view.end_y;
		viewInfo.floor = view.floor;
		viewInfo.zoom = view.zoom;
		viewInfo.view_scroll_x = view.view_scroll_x;
		viewInfo.view_scroll_y = view.view_scroll_y;
		viewInfo.tile_size = view.tile_size;
		viewInfo.screen_width = view.screensize_x;
		viewInfo.screen_height = view.screensize_y;

		std::vector<MapOverlayCommand> commands;
		g_luaScripts.collectMapOverlayCommands(viewInfo, commands);

		for (const auto& cmd : commands) {
			DrawColor color(cmd.color.Red(), cmd.color.Green(), cmd.color.Blue(), cmd.color.Alpha());

			float screenX = 0, screenY = 0;

			if (cmd.screen_space) {
				screenX = cmd.x;
				screenY = cmd.y;
			} else {
				if (cmd.z != viewInfo.floor) continue; // Only draw on current floor if world space
				int mapped_x, mapped_y;
				CoordinateMapper::MapToScreen(cmd.x, cmd.y, view.start_x, view.start_y, view.zoom, cmd.z, 1.0, &mapped_x, &mapped_y);
				screenX = mapped_x - view.view_scroll_x;
				screenY = mapped_y - view.view_scroll_y;
			}

			switch (cmd.type) {
				case MapOverlayCommand::Type::Rect: {
					float w = cmd.screen_space ? cmd.w : cmd.w * viewInfo.tile_size;
					float h = cmd.screen_space ? cmd.h : cmd.h * viewInfo.tile_size;
					if (cmd.filled) {
						primitives->drawFilledRect(screenX, screenY, w, h, color);
					} else {
						primitives->drawRect(screenX, screenY, w, h, color, cmd.width);
					}
					break;
				}
				case MapOverlayCommand::Type::Line: {
					float screenX2 = 0, screenY2 = 0;
					if (cmd.screen_space) {
						screenX2 = cmd.x2;
						screenY2 = cmd.y2;
					} else {
						if (cmd.z2 != viewInfo.floor) continue;
						int mapped_x2, mapped_y2;
						CoordinateMapper::MapToScreen(cmd.x2, cmd.y2, view.start_x, view.start_y, view.zoom, cmd.z2, 1.0, &mapped_x2, &mapped_y2);
						screenX2 = mapped_x2 - view.view_scroll_x;
						screenY2 = mapped_y2 - view.view_scroll_y;
					}
					primitives->drawLine(screenX, screenY, screenX2, screenY2, color, cmd.width);
					break;
				}
				case MapOverlayCommand::Type::Text: {
					// Use standard text rendering; if specialized font needed, must update.
					// Assuming TextRenderer can draw simple text:
					if (textRenderer) {
						textRenderer->drawText(cmd.text, screenX, screenY, 14.0f, color);
					}
					break;
				}
				case MapOverlayCommand::Type::Sprite: {
					if (spriteBatch) {
						// Sprites draw size in world coordinates is usually 32x32 stretched by zoom
						float size = cmd.screen_space ? 32.0f : viewInfo.tile_size;
						spriteBatch->drawSprite(cmd.sprite_id, screenX, screenY, color);
					}
					break;
				}
			}
		}
	}
}
