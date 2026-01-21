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

#ifndef RME_RENDER_PASS_H_
#define RME_RENDER_PASS_H_

// Use the existing RenderPass enum from render_types.h
#include "../types/render_types.h"

namespace rme {
	namespace render {

		// RenderPass enum is already defined in render_types.h
		// This header provides utility functions for working with passes

		/// Get the name of a render pass for debugging
		inline const char* getRenderPassName(RenderPass pass) {
			switch (pass) {
				case RenderPass::Background:
					return "Background";
				case RenderPass::Tiles:
					return "Tiles";
				case RenderPass::Selection:
					return "Selection";
				case RenderPass::DraggingShadow:
					return "DraggingShadow";
				case RenderPass::HigherFloors:
					return "HigherFloors";
				case RenderPass::Brush:
					return "Brush";
				case RenderPass::Grid:
					return "Grid";
				case RenderPass::Light:
					return "Light";
				case RenderPass::UI:
					return "UI";
				case RenderPass::Tooltips:
					return "Tooltips";
				default:
					return "Unknown";
			}
		}

		/// Check if a pass should be skipped based on options
		inline bool shouldSkipPass(RenderPass pass, bool showGrid, bool showLights, bool showTooltips) {
			switch (pass) {
				case RenderPass::Grid:
					return !showGrid;
				case RenderPass::Light:
					return !showLights;
				case RenderPass::Tooltips:
					return !showTooltips;
				default:
					return false;
			}
		}

	} // namespace render
} // namespace rme

#endif // RME_RENDER_PASS_H_
