#ifndef RME_RENDERING_CHUNK_CHUNK_MANAGER_H_
#define RME_RENDERING_CHUNK_CHUNK_MANAGER_H_

#include "rendering/chunk/render_chunk.h"
#include "util/job_system.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/shader_program.h"
#include <unordered_map>
#include <memory>

class Map;
class TileRenderer;
struct RenderView;
struct DrawingOptions;

class ChunkManager {
public:
	ChunkManager();
	~ChunkManager();

	bool initialize();

	void draw(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id);
	void collectLights(std::vector<LightBuffer::Light>& out_lights, const RenderView& view);

private:
	void update(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id);
	void processResults();
	uint64_t makeKey(int chunk_x, int chunk_y);

	std::unordered_map<uint64_t, std::unique_ptr<RenderChunk>> chunks;
	std::unique_ptr<JobSystem> job_system;

	std::unique_ptr<GLVertexArray> vao;
	std::unique_ptr<ShaderProgram> shader;

	// Track submitted jobs to avoid duplicates
	std::unordered_map<uint64_t, bool> pending_jobs;
};

#endif
