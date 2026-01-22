//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_ANIMATION_MANAGER_H_
#define RME_ANIMATION_MANAGER_H_

#include <functional>
#include <vector>
#include <chrono>

namespace rme {
	namespace render {

		/// Manages animation timing and updates
		class AnimationManager {
		public:
			using RequestRedrawCallback = std::function<void()>;

			AnimationManager();
			~AnimationManager() = default;

			/// Update animation state
			/// @return true if redraw is needed
			bool update();

			/// Get current time in milliseconds
			[[nodiscard]] double getTime() const {
				return currentTime_;
			}

			/// Set callback to request redraw
			void setRedrawCallback(RequestRedrawCallback callback);

			/// Start/Stop animation
			void start();
			void stop();

		private:
			RequestRedrawCallback redrawCallback_;

			// Timing
			std::chrono::high_resolution_clock::time_point startTime_;
			double currentTime_ = 0.0;
			bool running_ = false;

			// Animation frequency (e.g. 50ms)
			// Actually RME legacy uses 50ms? or just elapsed time.
			// We'll track elapsed time.
		};

	} // namespace render
} // namespace rme

#endif // RME_ANIMATION_MANAGER_H_
