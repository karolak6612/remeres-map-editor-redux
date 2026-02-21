#ifndef RME_RENDERING_CHUNK_RENDER_CHUNK_H_
#define RME_RENDERING_CHUNK_RENDER_CHUNK_H_

#include "rendering/core/gl_resources.h"
#include "rendering/core/sprite_instance.h"
#include "rendering/core/light_buffer.h"
#include <vector>
#include <memory>
#include <atomic>

class Map;
class TileRenderer;
class Editor;
struct RenderView;
struct DrawingOptions;

struct ChunkData {
	std::vector<SpriteInstance> sprites;
	std::vector<LightBuffer::Light> lights;
	std::vector<uint32_t> missing_sprites;
};

class RenderChunk {
public:
	RenderChunk(int chunk_x, int chunk_y);
	~RenderChunk();

	// No copy
	RenderChunk(const RenderChunk&) = delete;
	RenderChunk& operator=(const RenderChunk&) = delete;

	// Move ok
	RenderChunk(RenderChunk&&) noexcept;
	RenderChunk& operator=(RenderChunk&&) noexcept;

	// Runs on background thread
	static ChunkData rebuild(int chunk_x, int chunk_y, Map& map, TileRenderer& renderer, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id);

	// Runs on main thread
	void upload(const ChunkData& data);

	// Runs on main thread
	// Binds this chunk's VBO to the provided VAO and draws
	void draw(uint32_t vao) const;

	uint64_t getLastBuildTime() const {
		return last_build_time;
	}

	const std::vector<LightBuffer::Light>& getLights() const {
		return lights;
	}

	bool isEmpty() const {
		return instance_count == 0;
	}

private:
	int chunk_x;
	int chunk_y;
	uint64_t last_build_time = 0;

	std::unique_ptr<GLBuffer> vbo;
	int instance_count = 0;

	std::vector<LightBuffer::Light> lights; // Cached lights for this chunk
};

#endif
