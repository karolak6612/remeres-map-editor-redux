#include "ui/managers/vsync_policy.h"

VSyncMode sanitizeVSyncMode(int raw_value) noexcept {
	switch (raw_value) {
	case static_cast<int>(VSyncMode::Off):
		return VSyncMode::Off;
	case static_cast<int>(VSyncMode::Adaptive):
		return VSyncMode::Adaptive;
	case static_cast<int>(VSyncMode::On):
	default:
		return VSyncMode::On;
	}
}

int getSwapIntervalForMode(VSyncMode mode) noexcept {
	switch (mode) {
	case VSyncMode::Off:
		return 0;
	case VSyncMode::Adaptive:
		return -1;
	case VSyncMode::On:
	default:
		return 1;
	}
}

VSyncApplicationOutcome classifySwapIntervalResult(VSyncMode requested_mode, SwapIntervalApplicationResult result) noexcept {
	switch (result) {
	case SwapIntervalApplicationResult::Set:
		return VSyncApplicationOutcome::Applied;
	case SwapIntervalApplicationResult::NonAdaptive:
		return requested_mode == VSyncMode::Adaptive ? VSyncApplicationOutcome::AdaptiveFallback : VSyncApplicationOutcome::Applied;
	case SwapIntervalApplicationResult::NotSet:
	default:
		return VSyncApplicationOutcome::Failed;
	}
}
