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

#ifndef RME_MAP_DRAWER_H_
#define RME_MAP_DRAWER_H_
#include <memory>
class GameSprite;

struct NVGcontext;
// Storage during drawing, for option caching
#include "rendering/core/drawing_options.h"
#include "rendering/core/light_buffer.h"
#include "app/definitions.h"
#include "game/outfit.h"
#include "game/creature.h"

#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/shader_program.h"

#include "rendering/utilities/light_drawer.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/cursors/live_cursor_drawer.h"
#include "rendering/drawers/overlays/selection_drawer.h"
#include "rendering/drawers/cursors/brush_cursor_drawer.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"
#include "rendering/drawers/cursors/drag_shadow_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/drawers/overlays/preview_drawer.h"
#include "rendering/drawers/tiles/shade_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/map_layer_drawer.h"

class MapCanvas;

class MapDrawer {
	MapCanvas* canvas;
	Editor& editor;
	DrawingOptions options;
	RenderView view;

	LightBuffer light_buffer;

	LightDrawer light_drawer;
	TooltipDrawer tooltip_drawer;
	GridDrawer grid_drawer;
	LiveCursorDrawer live_cursor_drawer;
	SelectionDrawer selection_drawer;
	BrushCursorDrawer brush_cursor_drawer;
	BrushOverlayDrawer brush_overlay_drawer;
	DragShadowDrawer drag_shadow_drawer;
	FloorDrawer floor_drawer;
	SpriteDrawer sprite_drawer;
	ItemDrawer item_drawer;
	MarkerDrawer marker_drawer;
	PreviewDrawer preview_drawer;
	ShadeDrawer shade_drawer;
	CreatureDrawer creature_drawer;
	CreatureNameDrawer creature_name_drawer;
	HookIndicatorDrawer hook_indicator_drawer;
	DoorIndicatorDrawer door_indicator_drawer;
	TileRenderer tile_renderer;
	MapLayerDrawer map_layer_drawer;

	SpriteBatch sprite_batch;
	PrimitiveRenderer primitive_renderer;

	// Post-processing
	std::unique_ptr<GLFramebuffer> scale_fbo;
	std::unique_ptr<GLTextureResource> scale_texture;
	int fbo_width = 0;
	int fbo_height = 0;
	bool m_lastAaMode = false;

	std::unique_ptr<GLVertexArray> pp_vao;
	std::unique_ptr<GLBuffer> pp_vbo;
	std::unique_ptr<GLBuffer> pp_ebo;

	void InitPostProcess();
	void DrawPostProcess(const RenderView& view, const DrawingOptions& options);
	void UpdateFBO(const RenderView& view, const DrawingOptions& options);

protected:
	friend class BrushOverlayDrawer;
	friend class DragShadowDrawer;
	friend class FloorDrawer;

public:
	MapDrawer(MapCanvas* canvas);
	~MapDrawer();

	void SetupVars();
	void SetupGL();
	void Release();

	void Draw();
	void DrawBackground();
	void DrawMap();
	void DrawLiveCursors();
	void DrawIngameBox(const ViewBounds& bounds);

	void DrawGrid(const ViewBounds& bounds);
	void DrawTooltips(NVGcontext* vg);
	void DrawHookIndicators(NVGcontext* vg);
	void DrawDoorIndicators(NVGcontext* vg);
	void ClearFrameOverlays();
	void DrawCreatureNames(NVGcontext* vg);

	void DrawLight();

	void TakeScreenshot(uint8_t* screenshot_buffer);

	DrawingOptions& getOptions() {
		return options;
	}

	SpriteBatch* getSpriteBatch() {
		return &sprite_batch;
	}
	PrimitiveRenderer* getPrimitiveRenderer() {
		return &primitive_renderer;
	}
	TileRenderer* getTileRenderer() {
		return &tile_renderer;
	}
	DoorIndicatorDrawer* getDoorIndicatorDrawer() {
		return &door_indicator_drawer;
	}

private:
	void DrawMapLayer(int map_z, bool live_client);
	bool renderers_initialized = false;
};

#endif
