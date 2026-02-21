#include "rendering/core/render_chunk.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/render_view.h"
#include "app/definitions.h"
#include <glm/glm.hpp>
#include "rendering/core/light_buffer.h"

RenderChunk::RenderChunk() {
	instances.reserve(256);
}

void RenderChunk::Rebuild(MapNode* node, int map_z, TileRenderer* renderer, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id) {
	if (!node) {
		return;
	}

	is_dynamic = false;

	// Use temporary SpriteBatch to capture instances
	// We don't need to initialize GL resources for this batch as we only use it to accumulate instances
	SpriteBatch temp_batch;
	temp_batch.begin(glm::mat4(1.0f));

	LightBuffer temp_light_buffer;

	Floor* floor = node->getFloor(map_z);
	if (floor) {
		TileLocation* location = floor->locs;
		// Iterate 4x4 tiles in the node
		for (int i = 0; i < 16; ++i) {
			TileLocation* loc = &floor->locs[i];

			Position pos = loc->getPosition();

			// Use World Coordinates for caching
			int world_x = pos.x * TILE_SIZE;
			int world_y = pos.y * TILE_SIZE;

			bool dynamic_tile = false;
			// Pass explicit coordinates to bypass visibility culling in DrawTile
			renderer->DrawTile(temp_batch, loc, view, options, current_house_id, world_x, world_y, &dynamic_tile);

			if (dynamic_tile) {
				is_dynamic = true;
			}

			renderer->AddLight(loc, view, options, temp_light_buffer);
		}
	}

	instances = temp_batch.stealInstances();
	lights = std::move(temp_light_buffer.lights);

	// Update timestamp
	// Use relaxed ordering as exact synchronization isn't critical for visual cache
	last_rendered = node->last_modified.load(std::memory_order_relaxed);
}
