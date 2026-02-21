#ifndef RME_UTIL_JOB_SYSTEM_H_
#define RME_UTIL_JOB_SYSTEM_H_

#include "rendering/chunk/render_chunk.h"
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <atomic>

// A simple job payload
struct ChunkBuildJob {
	int chunk_x;
	int chunk_y;
	Map* map;
	TileRenderer* renderer;
	RenderView view;
	DrawingOptions options;
	uint32_t current_house_id;
};

// Result payload
struct ChunkBuildResult {
	int chunk_x;
	int chunk_y;
	ChunkData data;
};

class JobSystem {
public:
	JobSystem();
	~JobSystem();

	void submit(ChunkBuildJob job);

	// Returns a list of completed results. Non-blocking.
	std::vector<ChunkBuildResult> poll();

	void stop();

private:
	void workerLoop();

	std::thread worker_thread;
	std::atomic<bool> running;

	std::mutex queue_mutex;
	std::condition_variable queue_cv;
	std::queue<ChunkBuildJob> job_queue;

	std::mutex result_mutex;
	std::vector<ChunkBuildResult> results;
};

#endif
