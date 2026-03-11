#include "app/main.h"

#include "rendering/core/tile_planning_pool.h"

#include "rendering/core/chunk_source_snapshot.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/prepared_render_chunk.h"
#include "rendering/core/prepared_render_chunk_builder.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"
#include "rendering/drawers/tiles/tile_renderer.h"

#include <algorithm>
#include <spdlog/spdlog.h>

TilePlanningPool::TilePlanningPool(size_t worker_count)
{
	workers_.reserve(worker_count);
	for (size_t i = 0; i < worker_count; ++i) {
		workers_.emplace_back([this](std::stop_token stop_token) { WorkerMain(stop_token); });
	}
	spdlog::info("TilePlanningPool: started {} persistent planning worker threads", workers_.size());
}

TilePlanningPool::~TilePlanningPool()
{
	for (auto& worker : workers_) {
		worker.request_stop();
	}
	job_cv_.notify_all();
}

void TilePlanningPool::BuildChunks(
	TileRenderer& tile_renderer, const FramePlanContext& ctx, std::span<const ChunkSourceSnapshot> chunks,
	std::span<std::shared_ptr<const PreparedRenderChunk>> outputs, uint64_t generation
)
{
	if (chunks.empty()) {
		return;
	}

	if (workers_.empty()) {
		ExecuteChunks(tile_renderer, ctx, chunks, outputs, chunks.size(), generation);
		return;
	}

	std::lock_guard dispatch_lock(dispatch_mutex_);
	const size_t chunk_size = std::max<size_t>(1, (chunks.size() + workers_.size()) / (workers_.size() + 1));

	{
		std::lock_guard lock(job_mutex_);
		job_.ctx = &ctx;
		job_.tile_renderer = &tile_renderer;
		job_.chunks = chunks;
		job_.outputs = outputs;
		job_.chunk_size = chunk_size;
		job_.pending_workers = workers_.size();
		job_.generation = generation;
		job_.active = true;
		next_index_.store(0, std::memory_order_release);
	}

	job_cv_.notify_all();

	// The caller participates in planning to reduce idle time while the
	// persistent workers consume the remaining chunks.
	ExecuteChunks(tile_renderer, ctx, chunks, outputs, chunk_size, generation);

	std::unique_lock lock(job_mutex_);
	job_cv_.wait(lock, [&] { return !job_.active; });
}

void TilePlanningPool::WorkerMain(std::stop_token stop_token)
{
	uint64_t observed_generation = 0;

	while (!stop_token.stop_requested()) {
		JobState local_job;
		{
			std::unique_lock lock(job_mutex_);
			job_cv_.wait(lock, stop_token, [&] { return stop_token.stop_requested() || (job_.active && job_.generation != observed_generation); });
			if (stop_token.stop_requested()) {
				return;
			}

			local_job = job_;
			observed_generation = job_.generation;
		}

		ExecuteChunks(*local_job.tile_renderer, *local_job.ctx, local_job.chunks, local_job.outputs, local_job.chunk_size, observed_generation);

		std::lock_guard lock(job_mutex_);
		if (job_.active && job_.generation == observed_generation) {
			if (--job_.pending_workers == 0) {
				job_.active = false;
				job_cv_.notify_all();
			}
		}
	}
}

void TilePlanningPool::ExecuteChunks(
	TileRenderer& tile_renderer, const FramePlanContext& ctx, std::span<const ChunkSourceSnapshot> chunks,
	std::span<std::shared_ptr<const PreparedRenderChunk>> outputs, size_t chunk_size, uint64_t generation
)
{
	for (;;) {
		const size_t begin = next_index_.fetch_add(chunk_size, std::memory_order_relaxed);
		if (begin >= chunks.size()) {
			return;
		}

		const size_t end = std::min(chunks.size(), begin + chunk_size);
		for (size_t i = begin; i < end; ++i) {
			outputs[i] = PreparedRenderChunkBuilder::Build(tile_renderer, ctx, chunks[i], generation);
		}
	}
}
