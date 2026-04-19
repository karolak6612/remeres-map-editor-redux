#include "app/main.h"

#include "rendering/ui/repaint_policy.h"
#include <spdlog/spdlog.h>

namespace {
constexpr double reduced_rate_zoom_threshold = 2.0;
constexpr int near_refresh_ms = 1000 / 60;
constexpr int far_refresh_ms = 1000 / 20;
}

int GetAnimationRefreshIntervalMs(double zoom) {
	return zoom > reduced_rate_zoom_threshold ? far_refresh_ms : near_refresh_ms;
}

RepaintDecision EvaluateRepaintRequest(
	const ViewInvalidationState& state,
	RepaintReason requested_reasons,
	bool immediate,
	std::chrono::steady_clock::time_point now
) {
	RepaintDecision decision;
	decision.allowed_reasons = requested_reasons;
	decision.immediate = immediate;
	decision.animation_interval_ms = GetAnimationRefreshIntervalMs(state.zoom);

	if (!HasFlag(decision.allowed_reasons, RepaintReason::AnimationTick)) {
		decision.should_refresh = decision.allowed_reasons != RepaintReason::None;
		spdlog::trace("RepaintPolicy: non-animation refresh reasons={} immediate={}", std::to_underlying(decision.allowed_reasons), decision.immediate);
		return decision;
	}

	if (!state.animation_enabled) {
		ClearFlag(decision.allowed_reasons, RepaintReason::AnimationTick);
		decision.should_refresh = decision.allowed_reasons != RepaintReason::None;
		spdlog::trace("RepaintPolicy: dropped animation tick because animation is disabled; remaining reasons={}", std::to_underlying(decision.allowed_reasons));
		return decision;
	}

	const auto refresh_interval = std::chrono::milliseconds(decision.animation_interval_ms);
	if (state.last_animation_refresh != std::chrono::steady_clock::time_point{} && now - state.last_animation_refresh < refresh_interval) {
		ClearFlag(decision.allowed_reasons, RepaintReason::AnimationTick);
		spdlog::trace(
			"RepaintPolicy: throttled animation tick at zoom={} interval_ms={} remaining reasons={}",
			state.zoom,
			decision.animation_interval_ms,
			std::to_underlying(decision.allowed_reasons)
		);
	}

	decision.should_refresh = decision.allowed_reasons != RepaintReason::None;
	spdlog::trace(
		"RepaintPolicy: final decision should_refresh={} reasons={} immediate={} interval_ms={}",
		decision.should_refresh,
		std::to_underlying(decision.allowed_reasons),
		decision.immediate,
		decision.animation_interval_ms
	);
	return decision;
}
