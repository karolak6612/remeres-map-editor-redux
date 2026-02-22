#include "rendering/chunk/chunk_manager.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/spatial_hash_grid.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/shared_geometry.h"
#include "rendering/core/sprite_instance.h"
#include "rendering/chunk/job_system.h"
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "rendering/core/atlas_manager.h"
#include "util/file_system.h"
#include <fstream>
#include <sstream>

constexpr const char* chunk_vert_default = R"(
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aRect;
layout (location = 3) in vec4 aUV;
layout (location = 4) in vec4 aTint;
layout (location = 5) in float aLayer;

out vec3 TexCoord;
out vec4 Tint;

uniform mat4 uMVP;

void main() {
    vec2 pos = aRect.xy + aPos * aRect.zw;
    gl_Position = uMVP * vec4(pos, 0.0, 1.0);
    TexCoord = vec3(mix(aUV.xy, aUV.zw, aTexCoord), aLayer);
    Tint = aTint;
}
)";

constexpr const char* chunk_frag_default = R"(
#version 460 core
in vec3 TexCoord;
in vec4 Tint;
out vec4 FragColor;

uniform sampler2DArray uAtlas;
uniform vec4 uGlobalTint;

void main() {
    FragColor = texture(uAtlas, TexCoord) * Tint * uGlobalTint;
}
)";

static std::string LoadShaderSource(const std::string& filename, const char* fallback) {
	std::string path = FileSystem::GetDataDirectory().ToStdString() + "/shaders/" + filename;
	std::ifstream file(path);
	if (file.is_open()) {
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}
	// Fallback to source dir for dev environment (optional, but good if data dir not populated)
	return fallback;
}

ChunkManager::ChunkManager() {
	job_system = std::make_unique<JobSystem>();
}

ChunkManager::~ChunkManager() {
	job_system->stop();
}

uint64_t ChunkManager::makeKey(int chunk_x, int chunk_y) {
	return (static_cast<uint64_t>(static_cast<uint32_t>(chunk_x)) << 32) | static_cast<uint32_t>(chunk_y);
}

bool ChunkManager::initialize() {
	shader = std::make_unique<ShaderProgram>();
	std::string vs = LoadShaderSource("chunk.vert", chunk_vert_default);
	std::string fs = LoadShaderSource("chunk.frag", chunk_frag_default);

	if (!shader->Load(vs, fs)) {
		spdlog::error("ChunkManager: Failed to load shader");
		return false;
	}

	if (!SharedGeometry::Instance().initialize()) {
		return false;
	}

	vao = std::make_unique<GLVertexArray>();

	// Setup Quad VBO (Binding 0)
	glVertexArrayVertexBuffer(vao->GetID(), 0, SharedGeometry::Instance().getQuadVBO(), 0, 4 * sizeof(float));
	glVertexArrayElementBuffer(vao->GetID(), SharedGeometry::Instance().getQuadEBO());

	// Loc 0: position (vec2)
	glEnableVertexArrayAttrib(vao->GetID(), 0);
	glVertexArrayAttribFormat(vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao->GetID(), 0, 0);

	// Loc 1: texcoord (vec2)
	glEnableVertexArrayAttrib(vao->GetID(), 1);
	glVertexArrayAttribFormat(vao->GetID(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(vao->GetID(), 1, 0);

	// Initialize RingBuffer (1MB sprites per frame buffer?)
	// 64 bytes per sprite. 100k sprites = 6.4MB.
	if (!ring_buffer.initialize(sizeof(SpriteInstance), 100000)) {
		spdlog::error("ChunkManager: Failed to initialize RingBuffer");
		return false;
	}

	if (mdi_renderer.initialize()) {
		use_mdi = true;
	}

	// Setup Instance Buffer (Binding 1) - Linked to RingBuffer
	glVertexArrayBindingDivisor(vao->GetID(), 1, 1);
	glVertexArrayVertexBuffer(vao->GetID(), 1, ring_buffer.getBufferId(), 0, sizeof(SpriteInstance));

	// Loc 2: rect (vec4)
	glEnableVertexArrayAttrib(vao->GetID(), 2);
	glVertexArrayAttribFormat(vao->GetID(), 2, 4, GL_FLOAT, GL_FALSE, offsetof(SpriteInstance, x));
	glVertexArrayAttribBinding(vao->GetID(), 2, 1);

	// Loc 3: uv (vec4)
	glEnableVertexArrayAttrib(vao->GetID(), 3);
	glVertexArrayAttribFormat(vao->GetID(), 3, 4, GL_FLOAT, GL_FALSE, offsetof(SpriteInstance, u_min));
	glVertexArrayAttribBinding(vao->GetID(), 3, 1);

	// Loc 4: tint (vec4)
	glEnableVertexArrayAttrib(vao->GetID(), 4);
	glVertexArrayAttribFormat(vao->GetID(), 4, 4, GL_FLOAT, GL_FALSE, offsetof(SpriteInstance, r));
	glVertexArrayAttribBinding(vao->GetID(), 4, 1);

	// Loc 5: layer (float)
	glEnableVertexArrayAttrib(vao->GetID(), 5);
	glVertexArrayAttribFormat(vao->GetID(), 5, 1, GL_FLOAT, GL_FALSE, offsetof(SpriteInstance, atlas_layer));
	glVertexArrayAttribBinding(vao->GetID(), 5, 1);

	return true;
}

void ChunkManager::draw(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id) {
	if (!shader || !vao) {
		return;
	}

	processResults();
	update(view, map, renderer, options, current_house_id);

	// Setup Shader
	shader->Use();

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-view.view_scroll_x, -view.view_scroll_y, 0.0f));
	glm::mat4 mvp = view.projectionMatrix * model;

	shader->SetMat4("uMVP", mvp);
	shader->SetInt("uAtlas", 0);
	shader->SetVec4("uGlobalTint", glm::vec4(1.0f));

	if (g_gui.gfx.ensureAtlasManager()) {
		g_gui.gfx.getAtlasManager()->bind(0);

		// Draw visible chunks using RingBuffer and MDI
		int start_cx = (view.start_x >> CHUNK_SHIFT);
		int start_cy = (view.start_y >> CHUNK_SHIFT);
		int end_cx = ((view.end_x + CHUNK_MASK) >> CHUNK_SHIFT);
		int end_cy = ((view.end_y + CHUNK_MASK) >> CHUNK_SHIFT);

		glBindVertexArray(vao->GetID());

		// Bind RingBuffer (base)
		glBindBuffer(GL_ARRAY_BUFFER, ring_buffer.getBufferId());

		// Collect chunks to draw
		std::vector<RenderChunk*> visible_chunks;
		size_t total_sprites = 0;

		for (int cy = start_cy; cy <= end_cy; ++cy) {
			for (int cx = start_cx; cx <= end_cx; ++cx) {
				auto it = chunks.find(makeKey(cx, cy));
				if (it != chunks.end() && !it->second->isEmpty()) {
					visible_chunks.push_back(it->second.get());
					total_sprites += it->second->getSprites().size();
				}
			}
		}

		if (total_sprites > 0) {
			mdi_renderer.clear();

			// Map RingBuffer
			// If total sprites exceed buffer size, we should split batch. For simplicity, assume buffer is large enough or cap it.
			size_t max_capacity = ring_buffer.getMaxElements();
			if (total_sprites > max_capacity) {
				// TODO: Implement batch splitting if needed. For now, just cap.
				total_sprites = max_capacity;
			}

			void* ptr = ring_buffer.waitAndMap(total_sprites);
			if (ptr) {
				size_t processed = 0;
				for (RenderChunk* chunk : visible_chunks) {
					const auto& sprites = chunk->getSprites();
					size_t count = sprites.size();
					if (processed + count > total_sprites) break;

					memcpy(static_cast<uint8_t*>(ptr) + processed * sizeof(SpriteInstance), sprites.data(), count * sizeof(SpriteInstance));
					processed += count;
				}
				ring_buffer.finishWrite();

				// Issue a single MDI command for the entire batch (since we use a texture array and contiguous instance data)
				size_t offset = ring_buffer.getCurrentSectionOffset();
				GLuint baseInstance = static_cast<GLuint>(offset / sizeof(SpriteInstance));

				mdi_renderer.addDrawCommand(6, static_cast<GLuint>(total_sprites), 0, 0, baseInstance);

				mdi_renderer.upload();
				mdi_renderer.execute();

				ring_buffer.signalFinished();
			}
		}

		glBindVertexArray(0);
	}

	shader->Unuse();
}

void ChunkManager::collectLights(std::vector<LightBuffer::Light>& out_lights, const RenderView& view) {
	int start_cx = (view.start_x >> CHUNK_SHIFT);
	int start_cy = (view.start_y >> CHUNK_SHIFT);
	int end_cx = ((view.end_x + CHUNK_MASK) >> CHUNK_SHIFT);
	int end_cy = ((view.end_y + CHUNK_MASK) >> CHUNK_SHIFT);

	for (int cy = start_cy; cy <= end_cy; ++cy) {
		for (int cx = start_cx; cx <= end_cx; ++cx) {
			auto it = chunks.find(makeKey(cx, cy));
			if (it != chunks.end()) {
				const auto& chunk_lights = it->second->getLights();
				out_lights.insert(out_lights.end(), chunk_lights.begin(), chunk_lights.end());
			}
		}
	}
}

bool ChunkManager::checkChunkDirty(int cx, int cy, Map& map, uint64_t last_build_time) {
	if (last_build_time == 0) {
		return true;
	}

	int start_x = cx * CHUNK_SIZE;
	int start_y = cy * CHUNK_SIZE;

	bool dirty = false;
	map.getGrid().visitLeaves(start_x, start_y, start_x + CHUNK_SIZE, start_y + CHUNK_SIZE, [&](MapNode* node, int, int) {
		if (dirty) return; // fast exit
		if (node->getLastModified() > last_build_time) {
			dirty = true;
		}
	});
	return dirty;
}

void ChunkManager::update(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id) {
	int start_cx = (view.start_x >> CHUNK_SHIFT);
	int start_cy = (view.start_y >> CHUNK_SHIFT);
	int end_cx = ((view.end_x + CHUNK_MASK) >> CHUNK_SHIFT);
	int end_cy = ((view.end_y + CHUNK_MASK) >> CHUNK_SHIFT);

	// Retrieve white pixel region safely on main thread
	DrawingOptions safe_options = options;
	if (g_gui.gfx.hasAtlasManager()) {
		safe_options.white_pixel_region = g_gui.gfx.getAtlasManager()->getWhitePixel();
	}

	for (int cy = start_cy; cy <= end_cy; ++cy) {
		for (int cx = start_cx; cx <= end_cx; ++cx) {
			uint64_t key = makeKey(cx, cy);
			RenderChunk* chunk = nullptr;

			auto it = chunks.find(key);
			if (it == chunks.end()) {
				auto new_chunk = std::make_unique<RenderChunk>(cx, cy);
				chunk = new_chunk.get();
				chunks[key] = std::move(new_chunk);
			} else {
				chunk = it->second.get();
			}

			if (pending_jobs.count(key)) {
				continue;
			}

			if (checkChunkDirty(cx, cy, map, chunk->getLastBuildTime())) {
				ChunkBuildJob job;
				job.chunk_x = cx;
				job.chunk_y = cy;
				job.map = &map;
				job.renderer = &renderer;
				job.view = view;
				job.options = safe_options;
				job.current_house_id = current_house_id;

				job_system->submit(std::move(job));
				pending_jobs.insert(key);
			}
		}
	}
}

void ChunkManager::processResults() {
	auto results = job_system->poll();

	for (const auto& res : results) {
		bool missing_loaded = false;

		// Handle missing sprites
		if (!res.data.missing_sprites.empty()) {
			if (g_gui.gfx.hasAtlasManager()) {
				for (uint32_t id : res.data.missing_sprites) {
					// Trigger load
					if (g_gui.gfx.getAtlasManager()->loadSprite(id)) {
						missing_loaded = true;
					}
				}
			}
		}

		uint64_t key = makeKey(res.chunk_x, res.chunk_y);
		auto it = chunks.find(key);
		if (it != chunks.end()) {
			it->second->upload(res.data);

			if (missing_loaded) {
				it->second->forceDirty();
			}
		}
		pending_jobs.erase(key);
	}
}
