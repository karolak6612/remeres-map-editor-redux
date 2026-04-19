#ifndef RME_RENDERING_UI_REPAINT_POLICY_H_
#define RME_RENDERING_UI_REPAINT_POLICY_H_

#include "rendering/ui/view_invalidation_state.h"

struct RepaintDecision {
	RepaintReason allowed_reasons = RepaintReason::None;
	bool should_refresh = false;
	bool immediate = false;
	int animation_interval_ms = 0;
};

[[nodiscard]] RepaintDecision EvaluateRepaintRequest(
	const ViewInvalidationState& state,
	RepaintReason requested_reasons,
	bool immediate,
	std::chrono::steady_clock::time_point now
);

[[nodiscard]] int GetAnimationRefreshIntervalMs(double zoom);

#endif
