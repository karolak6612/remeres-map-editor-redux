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

#ifndef RME_I_RENDER_TARGET_H_
#define RME_I_RENDER_TARGET_H_

#include "../types/render_types.h"

namespace rme {
	namespace render {

		/// Interface for a render target (screen, texture, etc.)
		/// Allows rendering to different destinations
		class IRenderTarget {
		public:
			virtual ~IRenderTarget() = default;

			/// Bind this render target for rendering
			virtual void bind() = 0;

			/// Unbind this render target
			virtual void unbind() = 0;

			/// Get the width of the render target
			[[nodiscard]] virtual int getWidth() const noexcept = 0;

			/// Get the height of the render target
			[[nodiscard]] virtual int getHeight() const noexcept = 0;

			/// Get the size of the render target
			[[nodiscard]] virtual Size getSize() const noexcept {
				return Size(getWidth(), getHeight());
			}

			/// Clear the render target with a color
			virtual void clear(float r, float g, float b, float a = 1.0f) = 0;

			// Prevent copying
			IRenderTarget(const IRenderTarget&) = delete;
			IRenderTarget& operator=(const IRenderTarget&) = delete;

		protected:
			IRenderTarget() = default;
			IRenderTarget(IRenderTarget&&) = default;
			IRenderTarget& operator=(IRenderTarget&&) = default;
		};

	} // namespace render
} // namespace rme

#endif // RME_I_RENDER_TARGET_H_
