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

#ifndef RME_I_RENDERER_H_
#define RME_I_RENDERER_H_

namespace rme {
	namespace render {

		// Forward declarations
		class RenderState;

		/// Base interface for all renderers
		/// Each renderer is responsible for a specific rendering pass
		/// Following the Single Responsibility Principle
		class IRenderer {
		public:
			virtual ~IRenderer() = default;

			/// Check if this renderer should be executed
			/// @return true if the renderer is enabled and should run
			[[nodiscard]] virtual bool isEnabled() const noexcept = 0;

			/// Execute the rendering pass
			/// @param state The current render state containing viewport, options, etc.
			virtual void render(const RenderState& state) = 0;

			/// Get the name of this renderer (for debugging)
			[[nodiscard]] virtual const char* getName() const noexcept = 0;

			// Prevent copying
			IRenderer(const IRenderer&) = delete;
			IRenderer& operator=(const IRenderer&) = delete;

		protected:
			IRenderer() = default;
			IRenderer(IRenderer&&) = default;
			IRenderer& operator=(IRenderer&&) = default;
		};

	} // namespace render
} // namespace rme

#endif // RME_I_RENDERER_H_
