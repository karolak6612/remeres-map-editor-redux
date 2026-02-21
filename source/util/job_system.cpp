#include "util/job_system.h"
#include <chrono>

JobSystem::JobSystem() : running(true) {
	worker_thread = std::thread(&JobSystem::workerLoop, this);
}

JobSystem::~JobSystem() {
	stop();
}

void JobSystem::stop() {
	if (running) {
		running = false;
		queue_cv.notify_all();
		if (worker_thread.joinable()) {
			worker_thread.join();
		}
	}
}

void JobSystem::submit(ChunkBuildJob job) {
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		job_queue.push(job);
	}
	queue_cv.notify_one();
}

std::vector<ChunkBuildResult> JobSystem::poll() {
	std::vector<ChunkBuildResult> out_results;
	{
		std::lock_guard<std::mutex> lock(result_mutex);
		if (!results.empty()) {
			out_results = std::move(results);
			results.clear(); // results vector is now moved-from (empty)
		}
	}
	return out_results;
}

void JobSystem::workerLoop() {
	while (running) {
		ChunkBuildJob job;
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			queue_cv.wait(lock, [this] { return !job_queue.empty() || !running; });

			if (!running) {
				break;
			}

			job = job_queue.front();
			job_queue.pop();
		}

		// Execute job
		ChunkData data = RenderChunk::rebuild(job.chunk_x, job.chunk_y, *job.map, *job.renderer, job.view, job.options, job.current_house_id);

		// Store result
		{
			std::lock_guard<std::mutex> lock(result_mutex);
			results.push_back({job.chunk_x, job.chunk_y, std::move(data)});
		}
	}
}
