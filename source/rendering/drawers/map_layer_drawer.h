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

#include <iosfwd>
#include <span>
#include <vector>

class DrawCommandQueue;
class IMapAccess;
class TileRenderer;
class GridDrawer;
class PendingNodeRequests;
class TileLocation;
class TilePlanningPool;
struct TileRenderSnapshot;
struct VisibleFloorSnapshot;
struct DrawContext;
struct FramePlanContext;
struct FloorViewParams;
struct PreparedFrameBuffer;
struct PreparedFloorRange;
class SpriteBatch;
struct TileDrawPlan;

class MapLayerDrawer {
public:
    MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, IMapAccess* map_access, PendingNodeRequests* pending_requests = nullptr);
    ~MapLayerDrawer();

    [[nodiscard]] VisibleFloorSnapshot BuildVisibleFloorSnapshot(
        const FramePlanContext& ctx, int map_z, bool live_client, const FloorViewParams& floor_params, uint32_t current_house_id
    );
    [[nodiscard]] PreparedFloorRange PrepareFloor(
        const FramePlanContext& ctx, const VisibleFloorSnapshot& floor_snapshot, PreparedFrameBuffer& prepared
    );
    void setPlanningPool(TilePlanningPool* planning_pool)
    {
        planning_pool_ = planning_pool;
    }

private:
    void MergePlans(std::span<const TileDrawPlan> plans, PreparedFrameBuffer& prepared);
    void PlanTilesParallel(const FramePlanContext& ctx, bool draw_lights, std::span<const TileRenderSnapshot> tiles, std::span<TileDrawPlan> plans);

    TileRenderer* tile_renderer;
    GridDrawer* grid_drawer;
    IMapAccess* map_access;
    PendingNodeRequests* pending_requests_;
    TilePlanningPool* planning_pool_ = nullptr;
    std::vector<TileDrawPlan> reusable_plans_;
};

#endif
