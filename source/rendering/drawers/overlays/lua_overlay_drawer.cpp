#include "lua_overlay_drawer.h"
#include "rendering/map_drawer.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/text_renderer.h"
#include "rendering/core/graphics.h"
#include "rendering/core/coordinate_mapper.h"
#include "lua/lua_script_manager.h"
#include "ui/gui.h"

LuaOverlayDrawer::LuaOverlayDrawer(MapDrawer* mapDrawer) : mapDrawer(mapDrawer) {
}

LuaOverlayDrawer::~LuaOverlayDrawer() {
}

void LuaOverlayDrawer::Draw(const RenderView& view, const DrawingOptions& options) {
	if (!g_luaScripts.isInitialized()) return;

	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (shows.empty()) return;

	auto* primitives = mapDrawer->getPrimitiveRenderer();
	auto* spriteBatch = mapDrawer->getSpriteBatch();
	auto* atlas = g_gui.gfx.getAtlasManager();

	if (!primitives || !spriteBatch || !atlas) return;

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
		glm::vec4 color(cmd.color.Red() / 255.0f, cmd.color.Green() / 255.0f, cmd.color.Blue() / 255.0f, cmd.color.Alpha() / 255.0f);

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
				float w = cmd.screen_space ? cmd.w : cmd.w * viewInfo.tile_size * view.zoom;
				float h = cmd.screen_space ? cmd.h : cmd.h * viewInfo.tile_size * view.zoom;
				if (cmd.filled) {
					primitives->drawRect(glm::vec4(screenX, screenY, w, h), color);
				} else {
					primitives->drawBox(glm::vec4(screenX, screenY, w, h), color, static_cast<float>(cmd.width));
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
				primitives->drawLine(glm::vec2(screenX, screenY), glm::vec2(screenX2, screenY2), color);
				break;
			}
			case MapOverlayCommand::Type::Sprite: {
				const AtlasRegion* region = atlas->getRegion(cmd.sprite_id);
				if (region) {
					float sizeX = cmd.screen_space ? cmd.w : viewInfo.tile_size * view.zoom;
					float sizeY = cmd.screen_space ? cmd.h : viewInfo.tile_size * view.zoom;
					if (sizeX == 0) sizeX = 32.0f;
					if (sizeY == 0) sizeY = 32.0f;
					spriteBatch->draw(screenX, screenY, sizeX, sizeY, *region, color.r, color.g, color.b, color.a);
				}
				break;
			}
			default:
				break;
		}
	}
}

void LuaOverlayDrawer::DrawUI(NVGcontext* vg, const RenderView& view, const DrawingOptions& options) {
	if (!g_luaScripts.isInitialized() || !vg) return;

	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (shows.empty()) return;

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
		if (cmd.type != MapOverlayCommand::Type::Text) continue;

		glm::vec4 color(cmd.color.Red() / 255.0f, cmd.color.Green() / 255.0f, cmd.color.Blue() / 255.0f, cmd.color.Alpha() / 255.0f);

		float screenX = 0, screenY = 0;

		if (cmd.screen_space) {
			screenX = cmd.x;
			screenY = cmd.y;
		} else {
			if (cmd.z != viewInfo.floor) continue;
			int mapped_x, mapped_y;
			CoordinateMapper::MapToScreen(cmd.x, cmd.y, view.start_x, view.start_y, view.zoom, cmd.z, 1.0, &mapped_x, &mapped_y);
			screenX = mapped_x - view.view_scroll_x;
			screenY = mapped_y - view.view_scroll_y;
		}

		TextRenderer::DrawText(vg, static_cast<int>(screenX), static_cast<int>(screenY), cmd.text, color, 14.0f);
	}
}
