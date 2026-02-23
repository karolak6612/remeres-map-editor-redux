#ifndef RME_RENDERING_UTILITIES_RENDER_BENCHMARK_H_
#define RME_RENDERING_UTILITIES_RENDER_BENCHMARK_H_

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

class RenderBenchmark {
public:
	enum class Metric {
		TotalFrameTime,
		MapTraversalTime,
		VisibleNodesVisited,
		VisibleNodesCulled,
		TilesProcessed,
		TilesDrawn,
		ItemsProcessed,
		ItemsDrawn,
		LightCalculations,
		AnimationUpdates,
		DrawCalls,
		Unknown
	};

	static RenderBenchmark& Get();

	void StartFrame();
	void EndFrame();

	void RecordMetric(Metric metric, int64_t value);
	void IncrementMetric(Metric metric, int64_t value = 1);

	int64_t GetMetric(Metric metric) const;
	std::string GetReport() const;
    void Reset();

private:
	RenderBenchmark() = default;
	~RenderBenchmark() = default;

	struct FrameStats {
		std::map<Metric, int64_t> metrics;
		std::chrono::high_resolution_clock::time_point startTime;
	};

	FrameStats currentFrame;
    std::map<Metric, int64_t> accumulatedMetrics;
    int frameCount = 0;
};

#endif
