#ifndef RME_RENDERING_CORE_RENDER_CHUNK_H_
#define RME_RENDERING_CORE_RENDER_CHUNK_H_

#include "rendering/core/sprite_instance.h"
#include "map/map_region.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/light_buffer.h"
#include <vector>
#include <cstdint>

class SpriteBatch;
class TileRenderer;
class RenderView;

class RenderChunk {
public:
	RenderChunk();
	~RenderChunk() = default;

	// Rebuilds the chunk geometry if necessary
	void Rebuild(MapNode* node, int map_z, TileRenderer* renderer, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id);

	const std::vector<SpriteInstance>& getInstances() const {
		return instances;
	}
	const std::vector<LightBuffer::Light>& getLights() const {
		return lights;
	}
	uint64_t getLastRendered() const {
		return last_rendered;
	}
	bool isDynamic() const {
		return is_dynamic;
	}

private:
	std::vector<SpriteInstance> instances;
	std::vector<LightBuffer::Light> lights;
	uint64_t last_rendered = 0;
	bool is_dynamic = false;
};

#endif
