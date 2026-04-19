#include "ui/managers/vsync_policy.h"

#include <cassert>

int main() {
	assert(sanitizeVSyncMode(-10) == VSyncMode::On);
	assert(sanitizeVSyncMode(static_cast<int>(VSyncMode::Off)) == VSyncMode::Off);
	assert(sanitizeVSyncMode(static_cast<int>(VSyncMode::Adaptive)) == VSyncMode::Adaptive);

	assert(getSwapIntervalForMode(VSyncMode::Off) == 0);
	assert(getSwapIntervalForMode(VSyncMode::On) == 1);
	assert(getSwapIntervalForMode(VSyncMode::Adaptive) == -1);

	assert(classifySwapIntervalResult(VSyncMode::Adaptive, SwapIntervalApplicationResult::NonAdaptive) == VSyncApplicationOutcome::AdaptiveFallback);
	assert(classifySwapIntervalResult(VSyncMode::On, SwapIntervalApplicationResult::Set) == VSyncApplicationOutcome::Applied);
	assert(classifySwapIntervalResult(VSyncMode::Off, SwapIntervalApplicationResult::NotSet) == VSyncApplicationOutcome::Failed);

	return 0;
}
