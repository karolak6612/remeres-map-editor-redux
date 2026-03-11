#ifndef RME_RENDERING_CORE_TILE_PLANNING_POOL_H_
#define RME_RENDERING_CORE_TILE_PLANNING_POOL_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

struct ChunkSourceSnapshot;
struct PreparedRenderChunk;
class TileRenderer;
struct FramePlanContext;

// Persistent worker pool for snapshot-driven tile planning.
// The caller submits one batch at a time and waits for completion, while the
// long-lived workers avoid per-frame thread creation overhead.
class TilePlanningPool {
public:
	explicit TilePlanningPool(size_t worker_count);
	~TilePlanningPool();

	TilePlanningPool(const TilePlanningPool&) = delete;
	TilePlanningPool& operator=(const TilePlanningPool&) = delete;

    void BuildChunks(
        TileRenderer& tile_renderer, const FramePlanContext& ctx, std::span<const ChunkSourceSnapshot> chunks,
        std::span<std::shared_ptr<const PreparedRenderChunk>> outputs, uint64_t generation
    );

	[[nodiscard]] size_t workerCount() const
	{
		return workers_.size();
	}

private:
	struct JobState {
		const FramePlanContext* ctx = nullptr;
		TileRenderer* tile_renderer = nullptr;
		std::span<const ChunkSourceSnapshot> chunks;
		std::span<std::shared_ptr<const PreparedRenderChunk>> outputs;
		size_t chunk_size = 1;
		size_t pending_workers = 0;
		uint64_t generation = 0;
		bool active = false;
	};

	void WorkerMain(std::stop_token stop_token);
	void ExecuteChunks(
		TileRenderer& tile_renderer, const FramePlanContext& ctx, std::span<const ChunkSourceSnapshot> chunks,
		std::span<std::shared_ptr<const PreparedRenderChunk>> outputs, size_t chunk_size, uint64_t generation
	);

	std::vector<std::jthread> workers_;
	mutable std::mutex job_mutex_;
	std::condition_variable_any job_cv_;
	std::mutex dispatch_mutex_;
	std::atomic<size_t> next_index_ {0};
	JobState job_;
};

#endif
