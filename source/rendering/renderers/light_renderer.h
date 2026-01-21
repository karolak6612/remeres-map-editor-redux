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

#ifndef RME_LIGHT_RENDERER_H_
#define RME_LIGHT_RENDERER_H_

#include "renderer_base.h"

// Forward declarations
class TileLocation;
class LightDrawer;
#include <memory>

namespace rme {
	namespace render {

		/// Handles rendering of lighting effects on the map
		/// Wraps and coordinates with existing LightDrawer
		class LightRenderer : public IRenderer {
		public:
			LightRenderer();
			~LightRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render all lights for the visible area
			/// @param ctx Render context with view bounds
			/// @param opts Rendering options
			void renderLights(const RenderContext& ctx, const RenderOptions& opts);

			/// Add a light source at the given location
			/// @param location Tile location with light source
			void addLight(TileLocation* location);

			/// Clear all accumulated lights (call before new frame)
			void clearLights();

			/// Set whether to show light strength labels
			void setShowLightStrength(bool show) {
				showLightStrength_ = show;
			}

			/// Set the existing LightDrawer for delegation
			void setLightDrawer(LightDrawer* drawer) {
				lightDrawer_ = drawer;
			}

		private:
			bool initialized_ = false;
			bool showLightStrength_ = false;
			std::unique_ptr<LightDrawer> lightDrawer_;
		};

	} // namespace render
} // namespace rme

#endif // RME_LIGHT_RENDERER_H_
