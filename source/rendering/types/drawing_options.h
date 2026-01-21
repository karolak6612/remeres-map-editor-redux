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

#ifndef RME_DRAWING_OPTIONS_H_
#define RME_DRAWING_OPTIONS_H_

#include <cstdint>

namespace rme {
	namespace render {

		/// Drawing options controlling what elements are rendered and how
		/// Extracted from the original DrawingOptions in map_drawer.h
		struct DrawingOptions {
			// Floor rendering
			bool transparentFloors = false;
			bool transparentItems = false;
			bool showAllFloors = true;
			bool showShade = true;

			// Entity visibility
			bool showCreatures = true;
			bool showSpawns = true;
			bool showItems = true;
			bool showTechItems = true;
			bool showWaypoints = true;

			// Overlays
			bool showGrid = false;
			bool showLights = false;
			bool showLightStrength = true;
			bool showHouses = true;
			bool showSpecialTiles = true;
			bool showTowns = false;
			bool showBlocking = false;
			bool showTooltips = false;
			bool showPreview = false;
			bool showHooks = false;
			bool showIngameBox = false;

			// Highlighting
			bool highlightItems = false;
			bool highlightLockedDoors = true;

			// Display modes
			bool ingame = false;
			bool asMinimap = false;
			bool onlyColors = false;
			bool onlyModified = false;

			// Performance
			bool hideItemsWhenZoomed = true;

			// Experimental
			bool experimentalFog = false;
			bool extendedHouseShader = false;

			// Runtime state (should these be separate?)
			bool dragging = false;

			/// Reset to default values
			void setDefault() {
				*this = DrawingOptions {};
			}

			/// Configure for in-game style rendering
			void setIngame() {
				ingame = true;
				showGrid = false;
				showHouses = false;
				showSpecialTiles = false;
				showBlocking = false;
				showTooltips = false;
				showPreview = false;
			}

			/// Check if lights should be drawn
			[[nodiscard]] bool shouldDrawLight() const noexcept {
				return showLights && !asMinimap;
			}

			/// Check if entities (creatures/spawns) should be drawn
			[[nodiscard]] bool shouldDrawEntities() const noexcept {
				return (showCreatures || showSpawns) && !asMinimap;
			}

			/// Check if any overlays are enabled
			[[nodiscard]] bool hasOverlays() const noexcept {
				return showGrid || showHouses || showSpecialTiles || showBlocking || showTooltips || showWaypoints;
			}
		};

	} // namespace render
} // namespace rme

#endif // RME_DRAWING_OPTIONS_H_
