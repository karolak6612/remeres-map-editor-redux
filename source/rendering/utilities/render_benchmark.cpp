#include "rendering/utilities/render_benchmark.h"
#include <sstream>
#include <iomanip>

RenderBenchmark& RenderBenchmark::Get() {
	static RenderBenchmark instance;
	return instance;
}

void RenderBenchmark::StartFrame() {
	currentFrame.metrics.clear();
	currentFrame.startTime = std::chrono::high_resolution_clock::now();
}

void RenderBenchmark::EndFrame() {
	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - currentFrame.startTime).count();
	RecordMetric(Metric::TotalFrameTime, duration);

    for (const auto& [metric, value] : currentFrame.metrics) {
        accumulatedMetrics[metric] += value;
    }
    frameCount++;
}

void RenderBenchmark::RecordMetric(Metric metric, int64_t value) {
	currentFrame.metrics[metric] = value;
}

void RenderBenchmark::IncrementMetric(Metric metric, int64_t value) {
	currentFrame.metrics[metric] += value;
}

int64_t RenderBenchmark::GetMetric(Metric metric) const {
	auto it = currentFrame.metrics.find(metric);
	if (it != currentFrame.metrics.end()) {
		return it->second;
	}
	return 0;
}

std::string RenderBenchmark::GetReport() const {
	std::stringstream ss;
    if (frameCount == 0) return "No frames recorded.";

	ss << "Render Benchmark Report (" << frameCount << " frames avg):\n";
	ss << "--------------------------------------------------\n";

    auto printMetric = [&](const char* name, Metric m) {
        if (accumulatedMetrics.count(m)) {
            ss << std::left << std::setw(30) << name << ": "
               << (accumulatedMetrics.at(m) / frameCount) << "\n";
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
    accumulatedMetrics.clear();
    frameCount = 0;
}
