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

#ifndef RME_MAP_LAYER_DRAWER_H
#define RME_MAP_LAYER_DRAWER_H


class Editor;
class GridDrawer;
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"
class SpriteBatch;
class PrimitiveRenderer;

class MapLayerDrawer {
public:
	MapLayerDrawer(TileRenderer* tile_renderer);
	~MapLayerDrawer();

	void Draw(const DrawContext& ctx, const FloorViewParams& floor_params, const TileRenderContext& render_ctx, bool live_client);

private:
	TileRenderer* tile_renderer;
};

#endif
