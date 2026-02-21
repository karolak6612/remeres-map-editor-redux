#include "rendering/chunk/chunk_manager.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/spatial_hash_grid.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/shared_geometry.h"
#include "rendering/core/sprite_instance.h"
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "rendering/core/atlas_manager.h"

const char* chunk_vert = R"(
#version 450 core
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

const char* chunk_frag = R"(
#version 450 core
in vec3 TexCoord;
in vec4 Tint;
out vec4 FragColor;

uniform sampler2DArray uAtlas;
uniform vec4 uGlobalTint;

void main() {
    FragColor = texture(uAtlas, TexCoord) * Tint * uGlobalTint;
}
)";

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
	if (!shader->Load(chunk_vert, chunk_frag)) {
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

	// Setup Instance Buffer (Binding 1)
	glVertexArrayBindingDivisor(vao->GetID(), 1, 1);

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
	processResults();
	update(view, map, renderer, options, current_house_id);

	// Setup Shader
	shader->Use();

	// MVP = Projection * View.
	// Projection is view.projectionMatrix (Ortho).
	// View is Translation(-scroll_x, -scroll_y).
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-view.view_scroll_x, -view.view_scroll_y, 0.0f));
	glm::mat4 mvp = view.projectionMatrix * model;

	shader->SetMat4("uMVP", mvp);
	shader->SetInt("uAtlas", 0);
	shader->SetVec4("uGlobalTint", glm::vec4(1.0f));

	if (g_gui.gfx.ensureAtlasManager()) {
		g_gui.gfx.getAtlasManager()->bind(0);
	}

	// Draw visible chunks
	int start_cx = (view.start_x >> 5);
	int start_cy = (view.start_y >> 5);
	int end_cx = ((view.end_x + 31) >> 5);
	int end_cy = ((view.end_y + 31) >> 5);

	glBindVertexArray(vao->GetID());

	for (int cy = start_cy; cy <= end_cy; ++cy) {
		for (int cx = start_cx; cx <= end_cx; ++cx) {
			auto it = chunks.find(makeKey(cx, cy));
			if (it != chunks.end()) {
				it->second->draw(vao->GetID());
			}
		}
	}

	glBindVertexArray(0);
	shader->Unuse();
}

void ChunkManager::collectLights(std::vector<LightBuffer::Light>& out_lights, const RenderView& view) {
	int start_cx = (view.start_x >> 5);
	int start_cy = (view.start_y >> 5);
	int end_cx = ((view.end_x + 31) >> 5);
	int end_cy = ((view.end_y + 31) >> 5);

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

void ChunkManager::update(const RenderView& view, Map& map, TileRenderer& renderer, const DrawingOptions& options, uint32_t current_house_id) {
	int start_cx = (view.start_x >> 5);
	int start_cy = (view.start_y >> 5);
	int end_cx = ((view.end_x + 31) >> 5);
	int end_cy = ((view.end_y + 31) >> 5);

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

			// Check pending
			if (pending_jobs.count(key)) {
				continue;
			}

			// Check dirty
			bool dirty = false;
			if (chunk->getLastBuildTime() == 0) {
				dirty = true;
			} else {
				// Check 8x8 nodes
				int start_nx = cx * 8;
				int start_ny = cy * 8;

				// Optimization: Could cache MapNode pointers in RenderChunk
				for (int ny = 0; ny < 8; ++ny) {
					for (int nx = 0; nx < 8; ++nx) {
						// getLeaf logic is fast (bit shift + array lookup)
						// But we need to use map.getGrid().getLeaf
						// Note: We access map in Main Thread here, safe.
						MapNode* node = map.getGrid().getLeaf((start_nx + nx) * 4, (start_ny + ny) * 4);
						if (node) {
							if (node->getLastModified() > chunk->getLastBuildTime()) {
								dirty = true;
								goto dirty_found;
							}
						}
					}
				}
			}

			dirty_found:
			if (dirty) {
				ChunkBuildJob job;
				job.chunk_x = cx;
				job.chunk_y = cy;
				job.map = &map;
				job.renderer = &renderer;
				job.view = view;
				job.options = options;
				job.current_house_id = current_house_id;

				job_system->submit(job);
				pending_jobs[key] = true;
			}
		}
	}
}

void ChunkManager::processResults() {
	auto results = job_system->poll();
	bool missing_found = false;

	for (const auto& res : results) {
		// Handle missing sprites
		if (!res.data.missing_sprites.empty()) {
			if (g_gui.gfx.hasAtlasManager()) {
				for (uint32_t id : res.data.missing_sprites) {
					// Trigger load
					if (g_gui.gfx.getAtlasManager()->loadSprite(id)) {
						missing_found = true;
					}
				}
			}
		}

		uint64_t key = makeKey(res.chunk_x, res.chunk_y);
		auto it = chunks.find(key);
		if (it != chunks.end()) {
			it->second->upload(res.data);

			// If missing sprites were found, invalidate this chunk to retry immediately?
			// Or wait for next update?
			// If we loaded them, we should rebuild.
			if (!res.data.missing_sprites.empty() && missing_found) {
				it->second->forceDirty();
			}
		}
		pending_jobs.erase(key);
	}
}
