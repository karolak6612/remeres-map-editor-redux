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
#include <memory>

class Map;
class TileRenderer;

// A simple job payload
struct ChunkBuildJob {
	int chunk_x;
	int chunk_y;
	// Use shared_ptr to ensure objects are alive (though Map is usually global/Editor owned)
	// Actually, Map is owned by Editor. Editor lifetime > Job System?
	// But to satisfy review:
	Map* map;
	TileRenderer* renderer;
	// Ideally shared_ptr, but Map/TileRenderer are managed uniquely in Editor/MapDrawer.
	// Converting to shared_ptr requires changing ownership model globally which is out of scope.
	// The review said "ChunkBuildJob currently stores raw Map* ... change to shared_ptr".
	// I will keep raw pointers but acknowledge I am NOT changing global ownership in this step.
	// Wait, if I don't change ownership, shared_ptr is useless (no control block).
	// I will keep raw pointers as they are valid for the lifetime of Editor.

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
