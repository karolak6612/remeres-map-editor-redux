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
class HookIndicatorDrawer;
class DoorIndicatorDrawer;

// Storage during drawing, for option caching
#include "rendering/core/render_settings.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/light_buffer.h"
#include "app/definitions.h"
#include "game/outfit.h"
#include "game/creature.h"

#include "rendering/core/render_view.h"
#include "rendering/core/view_snapshot.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/ui/tooltip_renderer.h"
#include "rendering/ui/nvg_image_cache.h"

class PostProcessPipeline;
class GridDrawer;

class LightDrawer;
class GraphicsSpriteResolver;
class LiveCursorDrawer;
class BrushCursorDrawer;
class BrushOverlayDrawer;
class DragShadowDrawer;
class FloorDrawer;
class SpriteDrawer;
class ItemDrawer;
class MapLayerDrawer;
class CreatureDrawer;
class MarkerDrawer;
class PreviewDrawer;
class ShadeDrawer;
class TileRenderer;
class CreatureNameDrawer;
class HookIndicatorDrawer;
class DoorIndicatorDrawer;

class MapDrawer {
	Editor& editor;
	RenderSettings render_settings;
	FrameOptions frame_options;
	ViewState view;
	ViewSnapshot snapshot_;
	std::unique_ptr<LightDrawer> light_drawer;
	LightBuffer light_buffer;
	FrameAccumulators accumulators_;
	TooltipRenderer tooltip_renderer;
	NVGImageCache nvg_image_cache;
	std::unique_ptr<GridDrawer> grid_drawer;
	std::unique_ptr<LiveCursorDrawer> live_cursor_drawer;
	std::unique_ptr<BrushCursorDrawer> brush_cursor_drawer;
	std::unique_ptr<BrushOverlayDrawer> brush_overlay_drawer;
	std::unique_ptr<DragShadowDrawer> drag_shadow_drawer;
	std::unique_ptr<FloorDrawer> floor_drawer;
	std::unique_ptr<SpriteDrawer> sprite_drawer;
	std::unique_ptr<MapLayerDrawer> map_layer_drawer;
	std::unique_ptr<CreatureDrawer> creature_drawer;
	std::unique_ptr<ItemDrawer> item_drawer;
	std::unique_ptr<MarkerDrawer> marker_drawer;
	std::unique_ptr<PreviewDrawer> preview_drawer;
	std::unique_ptr<ShadeDrawer> shade_drawer;
	std::unique_ptr<TileRenderer> tile_renderer;
	std::unique_ptr<CreatureNameDrawer> creature_name_drawer;
	std::unique_ptr<HookIndicatorDrawer> hook_indicator_drawer;
	std::unique_ptr<DoorIndicatorDrawer> door_indicator_drawer;
	std::unique_ptr<GraphicsSpriteResolver> sprite_resolver;
	std::unique_ptr<SpriteBatch> sprite_batch;
	std::unique_ptr<PrimitiveRenderer> primitive_renderer;

	// Post-processing
	std::unique_ptr<PostProcessPipeline> post_process_;

public:
	MapDrawer(Editor& editor);
	~MapDrawer();

	void SetupVars(const ViewSnapshot& snapshot);
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

	RenderSettings& getRenderSettings() {
		return render_settings;
	}
	FrameOptions& getFrameOptions() {
		return frame_options;
	}
	const ViewSnapshot& getSnapshot() const {
		return snapshot_;
	}

	SpriteBatch* getSpriteBatch() {
		return sprite_batch.get();
	}
	PrimitiveRenderer* getPrimitiveRenderer() {
		return primitive_renderer.get();
	}
	TileRenderer* getTileRenderer() {
		return tile_renderer.get();
	}

private:
	void DrawMapLayer(int map_z, bool live_client, const FloorViewParams& floor_params);
	bool renderers_initialized = false;
};

#endif
