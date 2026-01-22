//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "view_manager.h"
#include "../../logging/logger.h"

namespace rme {
	namespace render {

		ViewManager::ViewManager() {
			// Initialize with defaults if needed
		}

		void ViewManager::setViewportSize(int width, int height) {
			if (width != mapper_.viewWidth() || height != mapper_.viewHeight()) {
				// LOG_RENDER_DEBUG("[VIEW] Viewport size changed: {}x{}", width, height);
				mapper_.setViewport(width, height, mapper_.zoom());
				notifyListeners();
			}
		}

		void ViewManager::setZoom(float zoom) {
			// Clamp zoom level (legacy limits)
			const float clampedZoom = std::max(0.125f, std::min(25.0f, zoom));
			if (clampedZoom != mapper_.zoom()) {
				// LOG_RENDER_DEBUG("[VIEW] Zoom changed: {}", clampedZoom);
				mapper_.setViewport(mapper_.viewWidth(), mapper_.viewHeight(), clampedZoom);
				notifyListeners();
			}
		}

		void ViewManager::setScroll(int x, int y) {
			if (x != mapper_.scrollX() || y != mapper_.scrollY()) {
				// LOG_RENDER_TRACE("[VIEW] Scroll changed: ({}, {})", x, y);
				mapper_.setScroll(x, y);
				notifyListeners();
			}
		}

		void ViewManager::setFloor(int floor) {
			const int clampedFloor = std::max(0, std::min(15, floor));
			if (clampedFloor != mapper_.floor()) {
				// LOG_RENDER_DEBUG("[VIEW] Floor changed: {}", clampedFloor);
				mapper_.setFloor(clampedFloor);
				notifyListeners();
			}
		}

		void ViewManager::addViewChangeListener(ViewChangeCallback callback) {
			listeners_.push_back(std::move(callback));
		}

		void ViewManager::notifyListeners() {
			for (const auto& callback : listeners_) {
				if (callback) {
					callback();
				}
			}
		}

	} // namespace render
} // namespace rme
