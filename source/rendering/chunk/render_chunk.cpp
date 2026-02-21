#include "rendering/chunk/render_chunk.h"
#include "rendering/chunk/collectors.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/spatial_hash_grid.h"
#include <chrono>
#include <shared_mutex>

RenderChunk::RenderChunk(int chunk_x, int chunk_y) : chunk_x(chunk_x), chunk_y(chunk_y) {
	vbo = std::make_unique<GLBuffer>();
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

	// Note: We need to use the view passed to rebuild, not current global view.
	// But we must zero out the scroll so the VBO contains coordinates relative to (0,0) - offset.
	RenderView proxy_view = view;
	proxy_view.view_scroll_x = 0;
	proxy_view.view_scroll_y = 0;

	for (int z = view.start_z; z >= view.superend_z; --z) {
		// Acquire read lock on the grid only for the duration of processing this Z-level (or even per-node if fine-grained)
		// To balance responsiveness for writers (UI thread), we can lock/unlock per Z level.
		std::shared_lock lock(map.getGrid().grid_mutex);

		// Shade Injection
		if (z == view.end_z && view.start_z != view.end_z && options.show_shade) {
			// Draw shade rectangle covering this chunk at Z level when show_shade is enabled
			int offset = (z <= GROUND_LAYER)
				? (GROUND_LAYER - z) * TILE_SIZE
				: TILE_SIZE * (view.floor - z);

			float sx = start_x * TILE_SIZE - offset;
			float sy = start_y * TILE_SIZE - offset;
			float sw = 32.0f * TILE_SIZE;
			float sh = 32.0f * TILE_SIZE;

			glm::vec4 shade_color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);

			// Use safe accessor for AtlasManager.
			// Note: Accessing AtlasManager from background thread is generally unsafe if it reallocates.
			// However, in the absence of a thread-safe getter, we check validity.
			// Ideally, white pixel region should be cached or passed safely.
			if (g_gui.gfx.hasAtlasManager()) {
				sprite_collector.drawRect(sx, sy, sw, sh, shade_color, *g_gui.gfx.getAtlasManager());
			}
		}

		// Iterate nodes in 32x32 area
		// 32 tiles = 8 nodes (4 tiles per node).
		int start_nx = start_x / 4;
		int end_nx = (end_x + 3) / 4;
		int start_ny = start_y / 4;
		int end_ny = (end_y + 3) / 4;

		for (int nx = start_nx; nx < end_nx; ++nx) {
			for (int ny = start_ny; ny < end_ny; ++ny) {
				// getLeaf takes tile coords
				MapNode* nd = map.getGrid().getLeaf(nx * 4, ny * 4);
				if (!nd) continue;

				if (!nd->isVisible(z > GROUND_LAYER)) {
					continue;
				}

				Floor* floor = nd->getFloor(z);
				if (!floor) continue;

				TileLocation* location = floor->locs;
				int base_x = nx * 4;
				int base_y = ny * 4;

				for (int lx = 0; lx < 4; ++lx) {
					for (int ly = 0; ly < 4; ++ly, ++location) {
						int tx = base_x + lx;
						int ty = base_y + ly;

						if (tx >= start_x && tx < end_x && ty >= start_y && ty < end_y) {
							renderer.DrawTile(sprite_collector, location, proxy_view, options, current_house_id);
							// Add lights
							renderer.AddLight(location, proxy_view, options, light_collector);
						}
					}
				}
			}
		}
	}

	ChunkData data;
	data.sprites = sprite_collector.takeSprites();
	data.lights = light_collector.takeLights();
	data.missing_sprites = sprite_collector.getMissingSprites();
	return data;
}

void RenderChunk::upload(const ChunkData& data) {
	instance_count = data.sprites.size();
	lights = data.lights;

	if (instance_count > 0) {
		glNamedBufferData(vbo->GetID(), data.sprites.size() * sizeof(SpriteInstance), data.sprites.data(), GL_DYNAMIC_DRAW);
	}

	last_build_time = std::chrono::steady_clock::now().time_since_epoch().count();
}

void RenderChunk::draw(uint32_t vao) const {
	if (instance_count > 0) {
		// Bind VBO to binding point 1 (instance data)
		glVertexArrayVertexBuffer(vao, 1, vbo->GetID(), 0, sizeof(SpriteInstance));
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instance_count);
	}
}
