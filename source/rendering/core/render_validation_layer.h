#ifndef RME_RENDERING_CORE_RENDER_VALIDATION_LAYER_H_
#define RME_RENDERING_CORE_RENDER_VALIDATION_LAYER_H_

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>

#include "rendering/core/prepared_frame_buffer.h"
#include "rendering/core/render_prep_snapshot.h"

class MapDrawer;

struct RenderValidationResult {
	bool ok = true;
	uint64_t generation = 0;
	std::string message;

	[[nodiscard]] explicit operator bool() const
	{
		return ok;
	}
};

// Lightweight shadow validator for the threaded rendering pipeline.
// Sampled prep snapshots are rebuilt on the main thread and compared against
// the prep-thread result before the prepared frame is applied.
class RenderValidationLayer {
public:
	explicit RenderValidationLayer(size_t sample_interval = 120);

	[[nodiscard]] bool shouldSample(uint64_t generation) const;
	void trackSnapshot(const RenderPrepSnapshot& snapshot);
	[[nodiscard]] std::optional<RenderValidationResult> validatePreparedFrame(MapDrawer& drawer, const PreparedFrameBuffer& prepared);
	void discardOlderThan(uint64_t generation);
	void clear();

private:
	size_t sample_interval_;
	std::deque<RenderPrepSnapshot> sampled_snapshots_;
};

#endif
