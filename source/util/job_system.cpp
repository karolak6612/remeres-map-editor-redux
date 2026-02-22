#include "util/job_system.h"
#include <chrono>
#include <algorithm>

JobSystem::JobSystem() : running(true) {
	unsigned int thread_count = std::max(1u, std::thread::hardware_concurrency() / 2);
	for (unsigned int i = 0; i < thread_count; ++i) {
		workers.emplace_back(&JobSystem::workerLoop, this);
	}
}

JobSystem::~JobSystem() {
	stop();
}

void JobSystem::stop() {
	bool expected = true;
	if (running.compare_exchange_strong(expected, false)) {
		queue_cv.notify_all();
		for (auto& worker : workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}
}

void JobSystem::submit(ChunkBuildJob job) {
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		job_queue.push(std::move(job));
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
				// Drop remaining jobs (intentional)
				break;
			}

			job = std::move(job_queue.front());
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
