#ifndef RME_RENDERING_CHUNK_CHUNK_MANAGER_H_
#define RME_RENDERING_CHUNK_CHUNK_MANAGER_H_

#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/gl_resources.h"
#include "rendering/chunk/render_chunk.h"
#include "rendering/chunk/job_system.h" // Includes ChunkBuildJob
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

private:
	void update(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id);
	void processResults();

	std::unique_ptr<ShaderProgram> shader;
	std::unique_ptr<GLVertexArray> vao;
	std::unique_ptr<JobSystem> job_system;

	std::unordered_map<uint64_t, std::unique_ptr<RenderChunk>> chunks;
	std::unordered_set<uint64_t> pending_jobs;
};

#endif
