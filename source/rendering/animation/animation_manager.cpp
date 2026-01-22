//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "animation_manager.h"

namespace rme {
	namespace render {

		AnimationManager::AnimationManager() {
			startTime_ = std::chrono::high_resolution_clock::now();
		}

		void AnimationManager::setRedrawCallback(RequestRedrawCallback callback) {
			redrawCallback_ = std::move(callback);
		}

		void AnimationManager::start() {
			running_ = true;
			startTime_ = std::chrono::high_resolution_clock::now();
		}

		void AnimationManager::stop() {
			running_ = false;
		}

		bool AnimationManager::update() {
			if (!running_) {
				return false;
			}

			auto now = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
			double newTime = static_cast<double>(duration.count());

			// Check if enough time passed to warrant an update (e.g. every 50ms)
			// Or just update continuously?
			// Legacy RME likely updates on tick.
			// We'll just update time and let the loop decide redraws.
			// Ideally we return true if we crossed a frame boundary for animated tiles.
			// Standard Tibia animation duration is often multiples of 100ms or 500ms.

			bool needsUpdate = (static_cast<int>(newTime) / 50) != (static_cast<int>(currentTime_) / 50);

			currentTime_ = newTime;

			if (needsUpdate && redrawCallback_) {
				redrawCallback_();
				return true;
			}
			return false;
		}

	} // namespace render
} // namespace rme
