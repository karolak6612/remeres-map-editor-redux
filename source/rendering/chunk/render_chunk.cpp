#include "rendering/chunk/render_chunk.h"
#include "rendering/chunk/collectors.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/spatial_hash_grid.h"
#include <chrono>
#include <shared_mutex>
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/chunk/chunk_manager.h" // For CHUNK_SIZE

RenderChunk::RenderChunk(int chunk_x, int chunk_y) : chunk_x(chunk_x), chunk_y(chunk_y) {
}

RenderChunk::~RenderChunk() {
}

RenderChunk::RenderChunk(RenderChunk&&) noexcept = default;
RenderChunk& RenderChunk::operator=(RenderChunk&&) noexcept = default;

ChunkData RenderChunk::rebuild(int chunk_x, int chunk_y, Map& map, TileRenderer& renderer, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id) {
	SpriteCollector sprite_collector;
	LightCollector light_collector;

	int start_x = chunk_x * 32;
	int start_y = chunk_y * 32;
	int end_x = start_x + 32;
	int end_y = start_y + 32;

	RenderView proxy_view = view;
	proxy_view.view_scroll_x = 0;
	proxy_view.view_scroll_y = 0;

	for (int z = view.start_z; z >= view.superend_z; --z) {
		// Acquire read lock on the grid only for the duration of processing this Z-level
		std::shared_lock lock(map.getGrid().grid_mutex);

		// Shade Injection
		if (z == view.end_z && view.start_z != view.end_z && options.show_shade) {
			int offset = (z <= GROUND_LAYER)
				? (GROUND_LAYER - z) * TILE_SIZE
				: TILE_SIZE * (view.floor - z);

			float sx = start_x * TILE_SIZE - offset;
			float sy = start_y * TILE_SIZE - offset;
			float sw = 32.0f * TILE_SIZE;
			float sh = 32.0f * TILE_SIZE;

			glm::vec4 shade_color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);

			if (options.white_pixel_region) {
				sprite_collector.draw(sx, sy, sw, sh, *options.white_pixel_region, shade_color.r, shade_color.g, shade_color.b, shade_color.a);
			}
		}

		// Reuse visitLeaves logic
		map.getGrid().visitLeaves(start_x, start_y, end_x, end_y, [&](MapNode* nd, int nd_start_x, int nd_start_y) {
			if (!nd->isVisible(z > GROUND_LAYER)) return;

			Floor* floor = nd->getFloor(z);
			if (!floor) return;

			TileLocation* location = floor->locs;
			int base_x = nd_start_x;
			int base_y = nd_start_y;

			for (int lx = 0; lx < 4; ++lx) {
				for (int ly = 0; ly < 4; ++ly, ++location) {
					int tx = base_x + lx;
					int ty = base_y + ly;

					if (tx >= start_x && tx < end_x && ty >= start_y && ty < end_y) {
						renderer.DrawTile(sprite_collector, location, proxy_view, options, current_house_id);
						renderer.AddLight(location, proxy_view, options, light_collector);
					}
				}
			}
		});
	}

	ChunkData data;
	data.sprites = sprite_collector.takeSprites();
	data.lights = light_collector.takeLights();
	data.missing_sprites = sprite_collector.getMissingSprites();
	return data;
}

void RenderChunk::upload(const ChunkData& data) {
	sprites = data.sprites;
	lights = data.lights;
	last_build_time = std::chrono::steady_clock::now().time_since_epoch().count();
}
