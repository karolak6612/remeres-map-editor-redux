#ifndef RME_RENDERING_CORE_TILE_PLANNING_POOL_H_
#define RME_RENDERING_CORE_TILE_PLANNING_POOL_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

class TileRenderer;
struct FramePlanContext;
struct TileDrawPlan;
struct TileRenderSnapshot;

// Persistent worker pool for snapshot-driven tile planning.
// The caller submits one batch at a time and waits for completion, while the
// long-lived workers avoid per-frame thread creation overhead.
class TilePlanningPool {
public:
	explicit TilePlanningPool(size_t worker_count);
	~TilePlanningPool();

	TilePlanningPool(const TilePlanningPool&) = delete;
	TilePlanningPool& operator=(const TilePlanningPool&) = delete;

	void Plan(
		TileRenderer& tile_renderer, const FramePlanContext& ctx, bool draw_lights, std::span<const TileRenderSnapshot> tiles,
		std::span<TileDrawPlan> plans
	);

	[[nodiscard]] size_t workerCount() const
	{
		return workers_.size();
	}

private:
	struct JobState {
		const FramePlanContext* ctx = nullptr;
		TileRenderer* tile_renderer = nullptr;
		std::span<const TileRenderSnapshot> tiles;
		std::span<TileDrawPlan> plans;
		bool draw_lights = false;
		size_t chunk_size = 1;
		size_t pending_workers = 0;
		uint64_t generation = 0;
		bool active = false;
	};

	void WorkerMain(std::stop_token stop_token);
	void ExecuteChunks(
		TileRenderer& tile_renderer, const FramePlanContext& ctx, bool draw_lights, std::span<const TileRenderSnapshot> tiles,
		std::span<TileDrawPlan> plans, size_t chunk_size
	);

	std::vector<std::jthread> workers_;
	mutable std::mutex job_mutex_;
	std::condition_variable_any job_cv_;
	std::mutex dispatch_mutex_;
	std::atomic<size_t> next_index_ {0};
	JobState job_;
};

#endif
