#include "app/main.h"

#include "rendering/core/tile_planning_pool.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/tile_render_snapshot.h"
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

void TilePlanningPool::Plan(
	TileRenderer& tile_renderer, const FramePlanContext& ctx, bool draw_lights, std::span<const TileRenderSnapshot> tiles,
	std::span<TileDrawPlan> plans
)
{
	if (tiles.empty()) {
		return;
	}

	if (workers_.empty()) {
		ExecuteChunks(tile_renderer, ctx, draw_lights, tiles, plans, tiles.size());
		return;
	}

	std::lock_guard dispatch_lock(dispatch_mutex_);
	const size_t chunk_size = std::max<size_t>(1, (tiles.size() + workers_.size()) / (workers_.size() + 1));

	{
		std::lock_guard lock(job_mutex_);
		job_.ctx = &ctx;
		job_.tile_renderer = &tile_renderer;
		job_.tiles = tiles;
		job_.plans = plans;
		job_.draw_lights = draw_lights;
		job_.chunk_size = chunk_size;
		job_.pending_workers = workers_.size();
		job_.generation += 1;
		job_.active = true;
		next_index_.store(0, std::memory_order_release);
	}

	job_cv_.notify_all();

	// The caller participates in planning to reduce idle time while the
	// persistent workers consume the remaining chunks.
	ExecuteChunks(tile_renderer, ctx, draw_lights, tiles, plans, chunk_size);

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

		ExecuteChunks(
			*local_job.tile_renderer, *local_job.ctx, local_job.draw_lights, local_job.tiles, local_job.plans, local_job.chunk_size
		);

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
	TileRenderer& tile_renderer, const FramePlanContext& ctx, bool draw_lights, std::span<const TileRenderSnapshot> tiles,
	std::span<TileDrawPlan> plans, size_t chunk_size
)
{
	for (;;) {
		const size_t begin = next_index_.fetch_add(chunk_size, std::memory_order_relaxed);
		if (begin >= tiles.size()) {
			return;
		}

		const size_t end = std::min(tiles.size(), begin + chunk_size);
		for (size_t i = begin; i < end; ++i) {
			plans[i].clear();
			tile_renderer.PlanTile(ctx, tiles[i], draw_lights, plans[i]);
		}
	}
}
