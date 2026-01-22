//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_VIEW_MANAGER_H_
#define RME_VIEW_MANAGER_H_

#include <functional>
#include <vector>
#include "../input/coordinate_mapper.h"
// #include "../../position.h" // If needed for Position types

namespace rme {
	namespace render {

		/// Manages the view state (zoom, scroll, floor) and coordinate transformation
		/// Acts as the source of truth for view parameters and notifies listeners of changes
		class ViewManager {
		public:
			using ViewChangeCallback = std::function<void()>;

			ViewManager();
			~ViewManager() = default;

			/// setViewport size
			void setViewportSize(int width, int height);

			/// Set zoom level
			/// @param zoom New zoom level
			void setZoom(float zoom);

			/// Set scroll position
			/// @param x Scroll X
			/// @param y Scroll Y
			void setScroll(int x, int y);

			/// Set current floor
			/// @param floor Floor level (0-15)
			void setFloor(int floor);

			/// Register callback for view changes
			void addViewChangeListener(ViewChangeCallback callback);

			// Accessors
			[[nodiscard]] float getZoom() const {
				return mapper_.zoom();
			}
			[[nodiscard]] int getScrollX() const {
				return mapper_.scrollX();
			}
			[[nodiscard]] int getScrollY() const {
				return mapper_.scrollY();
			}
			[[nodiscard]] int getFloor() const {
				return mapper_.floor();
			}
			[[nodiscard]] int getViewportWidth() const {
				return mapper_.viewWidth();
			}
			[[nodiscard]] int getViewportHeight() const {
				return mapper_.viewHeight();
			}

			/// Access the underlying coordinate mapper for transformations
			[[nodiscard]] const input::CoordinateMapper& getCoordinateMapper() const {
				return mapper_;
			}

			/// Helper to get visible tile range directly
			void getVisibleRange(int& startX, int& startY, int& endX, int& endY) const {
				startX = mapper_.startTileX();
				startY = mapper_.startTileY();
				endX = mapper_.endTileX();
				endY = mapper_.endTileY();
			}

		private:
			input::CoordinateMapper mapper_;
			std::vector<ViewChangeCallback> listeners_;

			void notifyListeners();
		};

	} // namespace render
} // namespace rme

#endif // RME_VIEW_MANAGER_H_
