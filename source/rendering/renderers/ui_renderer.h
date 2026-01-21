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

#ifndef RME_UI_RENDERER_H_
#define RME_UI_RENDERER_H_

#include "renderer_base.h"
#include "../interfaces/i_font_renderer.h"
#include <string>

namespace rme {
	namespace render {

		/// Handles rendering of UI overlays on the map canvas
		/// Includes ingame box, status indicators, cursor overlays
		class UIRenderer : public IRenderer {
		public:
			UIRenderer();
			~UIRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render all UI elements
			void renderUI(const RenderContext& ctx, const RenderOptions& opts);

			/// Render the ingame view box overlay
			/// @param centerX Center X in screen coordinates
			/// @param centerY Center Y in screen coordinates
			/// @param width Visible area width
			/// @param height Visible area height
			void renderIngameBox(int centerX, int centerY, int width, int height);

			/// Render live cursor positions (for multiplayer)
			/// @param cursorX Cursor X position
			/// @param cursorY Cursor Y position
			/// @param userName User name to display
			/// @param color Cursor color
			void renderLiveCursor(int cursorX, int cursorY, const std::string& userName, const Color& color);

			/// Render zoom level indicator
			/// @param zoom Current zoom level
			/// @param x Position X
			/// @param y Position Y
			void renderZoomIndicator(float zoom, int x, int y);

			/// Render floor indicator
			/// @param floor Current floor
			/// @param x Position X
			/// @param y Position Y
			void renderFloorIndicator(int floor, int x, int y);

			/// Render position indicator (coordinates)
			/// @param mapX Map X coordinate
			/// @param mapY Map Y coordinate
			/// @param mapZ Map Z coordinate
			/// @param x Screen position X
			/// @param y Screen position Y
			void renderPositionIndicator(int mapX, int mapY, int mapZ, int x, int y);

			/// Set font renderer to use
			void setFontRenderer(IFontRenderer* renderer) {
				fontRenderer_ = renderer;
			}

		private:
			bool initialized_ = false;
			IFontRenderer* fontRenderer_ = nullptr;

			void renderText(int x, int y, const std::string& text, const Color& color);
		};

	} // namespace render
} // namespace rme

#endif // RME_UI_RENDERER_H_
