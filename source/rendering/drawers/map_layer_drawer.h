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

#include <vector>

struct ChunkSourceSnapshot;
class IMapAccess;
struct RenderChunkKey;
class TileRenderer;
class GridDrawer;
class PendingNodeRequests;
class TileLocation;
class TilePlanningPool;
struct FramePlanContext;
struct FloorViewParams;
struct VisibleChunkList;

class MapLayerDrawer {
public:
    MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, IMapAccess* map_access, PendingNodeRequests* pending_requests = nullptr);
    ~MapLayerDrawer();

    [[nodiscard]] VisibleChunkList BuildVisibleChunkList(int map_z, const FloorViewParams& floor_params) const;
    [[nodiscard]] ChunkSourceSnapshot BuildChunkSourceSnapshot(
        const FramePlanContext& ctx, const RenderChunkKey& key, bool live_client, uint32_t current_house_id
    );
    void setPlanningPool(TilePlanningPool* planning_pool)
    {
        planning_pool_ = planning_pool;
    }

    TileRenderer* tile_renderer;
    GridDrawer* grid_drawer;
    IMapAccess* map_access;
    PendingNodeRequests* pending_requests_;
    TilePlanningPool* planning_pool_ = nullptr;
};

#endif
