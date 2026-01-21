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

#ifndef RME_GRID_RENDERER_H_
#define RME_GRID_RENDERER_H_

#include "renderer_base.h"

namespace rme {
	namespace render {

		/// Handles rendering of map grid lines
		/// Extracts grid drawing logic from MapDrawer::DrawGrid
		class GridRenderer : public IRenderer {
		public:
			GridRenderer();
			~GridRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render grid lines over the visible area
			/// @param ctx Render context with view bounds
			/// @param gridType Type of grid (0=none, 1=small, 2=large)
			/// @param color Grid line color
			void renderGrid(const RenderContext& ctx, int gridType, const Color& color = Color(128, 128, 128, 128));

			/// Render a single grid cell outline
			/// @param screenX Cell screen X position
			/// @param screenY Cell screen Y position
			/// @param color Grid line color
			void renderGridCell(int screenX, int screenY, const Color& color);

			/// Set grid visibility
			void setGridType(int type) {
				gridType_ = type;
			}
			[[nodiscard]] int getGridType() const noexcept {
				return gridType_;
			}

		private:
			bool initialized_ = false;
			int gridType_ = 0;

			void renderSmallGrid(const RenderContext& ctx, const Color& color);
			void renderLargeGrid(const RenderContext& ctx, const Color& color);
		};

	} // namespace render
} // namespace rme

#endif // RME_GRID_RENDERER_H_
