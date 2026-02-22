#ifndef RME_RENDERING_CHUNK_CHUNK_MANAGER_H_
#define RME_RENDERING_CHUNK_CHUNK_MANAGER_H_

#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/gl_resources.h"
#include "rendering/chunk/render_chunk.h"
#include "rendering/chunk/job_system.h"
#include "rendering/core/ring_buffer.h"
#include "rendering/core/multi_draw_indirect_renderer.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>

class Map;
class TileRenderer;

class ChunkManager {
public:
	ChunkManager();
	~ChunkManager();

	bool initialize();
	void draw(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id);
	void collectLights(std::vector<LightBuffer::Light>& out_lights, const RenderView& view);

	static uint64_t makeKey(int chunk_x, int chunk_y);

	static constexpr int CHUNK_SIZE = 32;
	static constexpr int CHUNK_SHIFT = 5;
	static constexpr int CHUNK_MASK = 31;

private:
	void update(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id);
	bool checkChunkDirty(int cx, int cy, Map& map, uint64_t last_build_time);
	void processResults();

	std::unique_ptr<ShaderProgram> shader;
	std::unique_ptr<GLVertexArray> vao;
	std::unique_ptr<JobSystem> job_system;

	std::unordered_map<uint64_t, std::unique_ptr<RenderChunk>> chunks;
	std::unordered_set<uint64_t> pending_jobs;

	// Optimization: MDI + RingBuffer
	RingBuffer ring_buffer;
	MultiDrawIndirectRenderer mdi_renderer;
	bool use_mdi = false;
};

#endif
