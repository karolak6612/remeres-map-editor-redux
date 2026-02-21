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

	// Use map grid mutex for safety
	std::shared_lock lock(map.getGrid().grid_mutex);

	// Iterate all tiles in the chunk

	// Note: We need to use the view passed to rebuild, not current global view.
	// But we must zero out the scroll so the VBO contains coordinates relative to (0,0) - offset.
	RenderView proxy_view = view;
	proxy_view.view_scroll_x = 0;
	proxy_view.view_scroll_y = 0;

	int map_z_start = view.start_z; // e.g. 7
	int map_z_end = view.end_z; // e.g. 7
	// Wait, map_drawer loops: for (int map_z = view.start_z; map_z >= view.superend_z; map_z--)

	for (int z = view.start_z; z >= view.superend_z; --z) {

		// Shade Injection
		if (z == view.end_z && view.start_z != view.end_z && options.show_shade) {
			// Draw shade rect for this chunk
			// Shade logic: full screen rect.
			// But we are in a chunk.
			// So we draw a rect covering this chunk.
			// Coords relative to 0,0 - offset?
			// `ShadeDrawer` uses `sprite_batch.drawRect(0, 0, w, h)`. (Screen space).
			// Here we are in World Space (mostly).
			// We need to draw a rect at the chunk's world position for this Z level.
			// `draw_x` calculation involves `offset`.
			// `offset = (map_z <= GROUND_LAYER) ? ...`
			// We need to calculate the offset for `view.end_z`.
			// Actually `ShadeDrawer` draws at `view.end_z`.
			// So we use the offset for `z`.
			// Wait, `ShadeDrawer` draws *between* layers.
			// It draws AFTER `start_z`...`end_z + 1`. BEFORE `end_z`.
			// So if we iterate `z` downwards:
			// 7, 6...
			// If `z == end_z` (e.g. 7), we draw shade BEFORE drawing layer 7?
			// `MapDrawer`:
			// if (map_z == view.end_z ...) shade_drawer->draw().
			// if (map_z >= view.end_z) DrawMapLayer().
			// So Shade is drawn BEFORE Layer `end_z` is drawn.
			// This means Shade is UNDER Layer `end_z`.
			// And ON TOP of Layer `end_z + 1`? No, loop goes down.
			// 7 (start), 6, 5...
			// If end_z = 5.
			// z=7. Draw Layer 7.
			// z=6. Draw Layer 6.
			// z=5. Draw Shade. Draw Layer 5.
			// So Shade is drawn ON TOP of 6? No, z=6 drawn before.
			// So Shade is ON TOP of 6. And UNDER 5.
			// So Shade covers everything below 5.
			// Correct.

			// We need to calculate the rect for this chunk.
			// Chunk covers `start_x` to `end_x`.
			// We need `draw_x` for `start_x` at `z`.
			// And width 32*TILE_SIZE.

			int offset = (z <= GROUND_LAYER)
				? (GROUND_LAYER - z) * TILE_SIZE
				: TILE_SIZE * (view.floor - z);

			// But `proxy_view` has logic for offset inside `DrawTile`?
			// `MapLayerDrawer` calculates `offset` and passes `draw_x` manually.
			// `TileRenderer::DrawTile` logic:
			// `if (in_draw_x != -1) ... else ... view.IsTileVisible...`
			// We call `DrawTile` without explicit coords?
			// `renderer.DrawTile(sprite_collector, location, proxy_view, options, current_house_id);`
			// We didn't pass `draw_x`.
			// `TileRenderer::DrawTile` calculates `draw_x` from `location` and `view`.
			// `TileRenderer.cpp`:
			// `int draw_x = ((map_x * TILE_SIZE) - view.view_scroll_x) - offset;`
			// We set `view_scroll_x` to 0.
			// So `draw_x = map_x * TILE_SIZE - offset`.

			// So for shade, we manually calculate:
			float sx = start_x * TILE_SIZE - offset;
			float sy = start_y * TILE_SIZE - offset;
			float sw = 32.0f * TILE_SIZE;
			float sh = 32.0f * TILE_SIZE;

			glm::vec4 shade_color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);

			// We need AtlasManager.
			// RenderChunk doesn't have access to Graphics?
			// We can get it from `g_gui.gfx`. (Thread safe? `getAtlasManager` returns pointer. Atlas itself is read-only during draw?)
			// Yes, typically.
			if (g_gui.gfx.hasAtlasManager()) {
				sprite_collector.drawRect(sx, sy, sw, sh, shade_color, *g_gui.gfx.getAtlasManager());
			}
		}

		int safe_start_x = start_x;
		int safe_end_x = end_x; // 32

		// Iterate nodes in 32x32 area
		// 32 tiles = 8 nodes (4 tiles per node).
		int start_nx = start_x / 4;
		int end_nx = (end_x + 3) / 4;
		int start_ny = start_y / 4;
		int end_ny = (end_y + 3) / 4;

		for (int nx = start_nx; nx < end_nx; ++nx) {
			for (int ny = start_ny; ny < end_ny; ++ny) {
				// Convert nx, ny to tile coords
				int node_pixel_x = nx * 4 * TILE_SIZE; // Not needed for logic

				MapNode* nd = map.getGrid().getLeaf(nx * 4, ny * 4); // getLeaf takes tile coords
				if (!nd) continue;

				if (!nd->isVisible(z > GROUND_LAYER)) {
					// Logic for placeholders?
					// If we are caching, we probably want to skip or draw placeholder.
					// For now skip.
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
		glNamedBufferData(vbo->GetID(), data.sprites.size() * sizeof(SpriteInstance), data.sprites.data(), GL_STATIC_DRAW);
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
