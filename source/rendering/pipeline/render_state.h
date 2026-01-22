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

#ifndef RME_RENDER_STATE_H_
#define RME_RENDER_STATE_H_

#include <cstdint>
#include "../renderers/renderer_base.h"

class Editor;
class Map;

namespace rme {
	namespace render {

		/// Render state that persists across frames
		/// Contains all the state needed for a complete render pass
		class RenderState {
		public:
			RenderState();
			~RenderState() = default;

			// View state
			// View state
			RenderContext context;
			struct RenderOptions options;

			// Frame timing
			uint32_t frameNumber = 0;
			double deltaTime = 0.0;
			double frameTime = 0.0;

			// Dirty flags for optimization
			bool contextDirty = true;
			bool optionsDirty = true;

			/// Reset state for a new frame
			void beginFrame();

			/// Finalize state after rendering
			void endFrame();

			/// Mark context as dirty (needs recalculation)
			void invalidateContext() {
				contextDirty = true;
			}

			/// Mark options as dirty (need to be reapplied)
			void invalidateOptions() {
				optionsDirty = true;
			}

			/// Update viewport dimensions
			void setViewport(int width, int height, float zoom = 1.0f);

			/// Update visible tile range
			void setVisibleRange(int startX, int startY, int endX, int endY);

			/// Update scroll position
			void setScroll(int scrollX, int scrollY);

			/// Update current floor
			void setFloor(int floor);

			/// Update mouse position in map coordinates
			void setMousePosition(int mapX, int mapY);

			/// Set current animation time
			void setTime(double time) {
				frameTime = time;
			}

			/// Set/Get Editor
			void setEditor(Editor* editor) {
				editor_ = editor;
			}
			[[nodiscard]] Editor* editor() const noexcept {
				return editor_;
			}

			/// Set/Get Map (convenience)
			[[nodiscard]] Map* map() const noexcept;

		private:
			// Timing tracking
			double lastFrameTime_ = 0.0;

			// Context
			Editor* editor_ = nullptr;
		};

	} // namespace render
} // namespace rme

#endif // RME_RENDER_STATE_H_
