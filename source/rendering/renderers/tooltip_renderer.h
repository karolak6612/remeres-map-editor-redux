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

#ifndef RME_TOOLTIP_RENDERER_H_
#define RME_TOOLTIP_RENDERER_H_

#include "renderer_base.h"
#include "../interfaces/i_font_renderer.h"
#include <string>
#include <vector>

namespace rme {
	namespace render {

		/// Structure to hold tooltip data
		struct Tooltip {
			int x = 0;
			int y = 0;
			std::string text;
			Color color = colors::White;
			bool ellipsis = false;

			Tooltip() = default;
			Tooltip(int x, int y, const std::string& text, const Color& color = colors::White) : x(x), y(y), text(text), color(color) { }
		};

		/// Handles rendering of tooltips on the map
		/// Extracts tooltip drawing from MapDrawer::DrawTooltips
		class TooltipRenderer : public IRenderer {
		public:
			TooltipRenderer();
			~TooltipRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Add a tooltip to be rendered
			/// @param x Screen X position
			/// @param y Screen Y position
			/// @param text Tooltip text
			/// @param color Text color
			void addTooltip(int x, int y, const std::string& text, const Color& color = colors::White);

			/// Render all queued tooltips
			void renderTooltips();

			/// Clear all queued tooltips (call after rendering)
			void clearTooltips();

			/// Set font renderer to use
			void setFontRenderer(IFontRenderer* renderer) {
				fontRenderer_ = renderer;
			}

			/// Get the number of pending tooltips
			[[nodiscard]] size_t getTooltipCount() const noexcept {
				return tooltips_.size();
			}

		private:
			bool initialized_ = false;
			std::vector<Tooltip> tooltips_;
			IFontRenderer* fontRenderer_ = nullptr;

			void renderTooltip(const Tooltip& tooltip);
			int getTextWidth(const std::string& text);
		};

	} // namespace render
} // namespace rme

#endif // RME_TOOLTIP_RENDERER_H_
