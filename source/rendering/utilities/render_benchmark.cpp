#include "rendering/utilities/render_benchmark.h"
#include <sstream>
#include <iomanip>

RenderBenchmark& RenderBenchmark::Get() {
	static RenderBenchmark instance;
	return instance;
}

void RenderBenchmark::StartFrame() {
	for (auto& metric : currentFrameMetrics) {
		metric.store(0, std::memory_order_relaxed);
	}
	startTime = std::chrono::high_resolution_clock::now();
}

void RenderBenchmark::EndFrame() {
	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
	RecordMetric(Metric::TotalFrameTime, duration);

	std::lock_guard<std::mutex> lock(mutex);
    for (size_t i = 0; i < static_cast<size_t>(Metric::MetricCount); ++i) {
        accumulatedMetrics[i] += currentFrameMetrics[i].load(std::memory_order_relaxed);
    }
    frameCount++;
}

void RenderBenchmark::RecordMetric(Metric metric, int64_t value) {
	if (metric < Metric::MetricCount) {
		currentFrameMetrics[static_cast<size_t>(metric)].store(value, std::memory_order_relaxed);
	}
}

void RenderBenchmark::IncrementMetric(Metric metric, int64_t value) {
	if (metric < Metric::MetricCount) {
		currentFrameMetrics[static_cast<size_t>(metric)].fetch_add(value, std::memory_order_relaxed);
	}
}

int64_t RenderBenchmark::GetMetric(Metric metric) const {
	if (metric < Metric::MetricCount) {
		return currentFrameMetrics[static_cast<size_t>(metric)].load(std::memory_order_relaxed);
	}
	return 0;
}

std::string RenderBenchmark::GetReport() const {
	std::lock_guard<std::mutex> lock(mutex);
	std::stringstream ss;
    if (frameCount == 0) return "No frames recorded.";

	ss << "Render Benchmark Report (" << frameCount << " frames avg):\n";
	ss << "--------------------------------------------------\n";

    auto printMetric = [&](const char* name, Metric m) {
		size_t idx = static_cast<size_t>(m);
		if (idx < accumulatedMetrics.size()) {
            ss << std::left << std::setw(30) << name << ": "
               << (accumulatedMetrics[idx] / frameCount) << "\n";
		}
    };

	printMetric("Total Frame Time (us)", Metric::TotalFrameTime);
	printMetric("Map Traversal Time (us)", Metric::MapTraversalTime);
	printMetric("Visible Nodes Visited", Metric::VisibleNodesVisited);
	printMetric("Visible Nodes Culled", Metric::VisibleNodesCulled);
	printMetric("Tiles Processed", Metric::TilesProcessed);
	printMetric("Tiles Drawn", Metric::TilesDrawn);
	printMetric("Items Processed", Metric::ItemsProcessed);
	printMetric("Items Drawn", Metric::ItemsDrawn);
	printMetric("Light Calculations", Metric::LightCalculations);
	printMetric("Animation Updates", Metric::AnimationUpdates);
    printMetric("Draw Calls", Metric::DrawCalls);

	return ss.str();
}

void RenderBenchmark::Reset() {
	std::lock_guard<std::mutex> lock(mutex);
    accumulatedMetrics.fill(0);
    frameCount = 0;
}
