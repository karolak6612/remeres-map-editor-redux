//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "../../main.h"
#include "render_state.h"
#include "../../editor.h"
#include <chrono>

namespace rme {
	namespace render {

		RenderState::RenderState() {
			context.clear();
		}

		void RenderState::beginFrame() {
			// Calculate delta time
			auto now = std::chrono::high_resolution_clock::now();
			auto duration = now.time_since_epoch();
			double currentTime = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;

			if (lastFrameTime_ > 0.0) {
				deltaTime = currentTime - lastFrameTime_;
			} else {
				deltaTime = 1.0 / 60.0; // Assume 60 FPS for first frame
			}
			lastFrameTime_ = currentTime;

			frameTime = currentTime;
			++frameNumber;
		}

		void RenderState::endFrame() {
			// Clear dirty flags after frame is complete
			contextDirty = false;
			optionsDirty = false;
		}

		void RenderState::setViewport(int width, int height, float zoom) {
			if (context.viewportWidth != width || context.viewportHeight != height || context.zoom != zoom) {
				context.viewportWidth = width;
				context.viewportHeight = height;
				context.zoom = zoom;
				context.tileSize = static_cast<int>(kTileSize * zoom);
				contextDirty = true;
			}
		}

		void RenderState::setVisibleRange(int startX, int startY, int endX, int endY) {
			if (context.startX != startX || context.startY != startY || context.endX != endX || context.endY != endY) {
				context.startX = startX;
				context.startY = startY;
				context.endX = endX;
				context.endY = endY;
				contextDirty = true;
			}
		}

		void RenderState::setScroll(int scrollX, int scrollY) {
			if (context.scrollX != scrollX || context.scrollY != scrollY) {
				context.scrollX = scrollX;
				context.scrollY = scrollY;
				contextDirty = true;
			}
		}

		void RenderState::setFloor(int floor) {
			if (context.currentFloor != floor) {
				context.currentFloor = floor;
				contextDirty = true;
			}
		}

		void RenderState::setMousePosition(int mapX, int mapY) {
			context.mouseMapX = mapX;
			context.mouseMapY = mapY;
			// Mouse position doesn't invalidate context (not used for rendering decisions)
		}

		Map* RenderState::map() const noexcept {
			return editor_ ? &editor_->map : nullptr;
		}

	} // namespace render
} // namespace rme
