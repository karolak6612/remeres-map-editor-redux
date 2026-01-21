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

#ifndef RME_RENDER_COLOR_H_
#define RME_RENDER_COLOR_H_

#include <cstdint>
#include <algorithm>

// Forward declaration for wxColour conversion
class wxColour;

namespace rme {
	namespace render {

		/// RGBA color with 8-bit components
		struct Color {
			uint8_t r = 255;
			uint8_t g = 255;
			uint8_t b = 255;
			uint8_t a = 255;

			constexpr Color() = default;

			constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) { }

			/// Create from wxColour (defined in cpp to avoid wx header dependency)
			explicit Color(const wxColour& wx);

			/// Convert to wxColour (defined in cpp)
			[[nodiscard]] wxColour toWx() const;

			/// Create a copy with different alpha
			[[nodiscard]] constexpr Color withAlpha(uint8_t newAlpha) const noexcept {
				return Color(r, g, b, newAlpha);
			}

			/// Create a dimmed version (divide each component)
			[[nodiscard]] constexpr Color dimmed(uint8_t factor = 2) const noexcept {
				return Color(r / factor, g / factor, b / factor, a);
			}

			/// Create a brightened version (multiply, clamped)
			[[nodiscard]] constexpr Color brightened(float factor = 1.5f) const noexcept {
				return Color(
					static_cast<uint8_t>(std::min(255.0f, r * factor)),
					static_cast<uint8_t>(std::min(255.0f, g * factor)),
					static_cast<uint8_t>(std::min(255.0f, b * factor)),
					a
				);
			}

			/// Blend with another color (linear interpolation)
			[[nodiscard]] constexpr Color blend(const Color& other, float t) const noexcept {
				return Color(
					static_cast<uint8_t>(r + (other.r - r) * t),
					static_cast<uint8_t>(g + (other.g - g) * t),
					static_cast<uint8_t>(b + (other.b - b) * t),
					static_cast<uint8_t>(a + (other.a - a) * t)
				);
			}

			[[nodiscard]] constexpr bool operator==(const Color& other) const noexcept {
				return r == other.r && g == other.g && b == other.b && a == other.a;
			}
			[[nodiscard]] constexpr bool operator!=(const Color& other) const noexcept {
				return !(*this == other);
			}

			/// Check if fully transparent
			[[nodiscard]] constexpr bool isTransparent() const noexcept {
				return a == 0;
			}

			/// Check if fully opaque
			[[nodiscard]] constexpr bool isOpaque() const noexcept {
				return a == 255;
			}
		};

		/// Predefined colors
		namespace colors {
			inline constexpr Color White { 255, 255, 255, 255 };
			inline constexpr Color Black { 0, 0, 0, 255 };
			inline constexpr Color Red { 255, 0, 0, 255 };
			inline constexpr Color Green { 0, 255, 0, 255 };
			inline constexpr Color Blue { 0, 0, 255, 255 };
			inline constexpr Color Yellow { 255, 255, 0, 255 };
			inline constexpr Color Cyan { 0, 255, 255, 255 };
			inline constexpr Color Magenta { 255, 0, 255, 255 };
			inline constexpr Color Orange { 255, 165, 0, 255 };
			inline constexpr Color Transparent { 0, 0, 0, 0 };

			// Editor-specific colors
			inline constexpr Color SelectionBlue { 0, 0, 255, 128 };
			inline constexpr Color SelectionGreen { 0, 255, 0, 128 };
			inline constexpr Color BrushPreview { 255, 255, 255, 160 };
			inline constexpr Color GridLine { 255, 255, 255, 64 };
			inline constexpr Color ErrorRed { 255, 0, 0, 200 };

			// House colors
			inline constexpr Color HouseExit { 0, 255, 0, 160 };
			inline constexpr Color HouseTile { 0, 0, 255, 96 };
		}

	} // namespace render
} // namespace rme

#endif // RME_RENDER_COLOR_H_
