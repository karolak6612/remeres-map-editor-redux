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

#ifndef RME_I_TEXTURE_CACHE_H_
#define RME_I_TEXTURE_CACHE_H_

#include <cstdint>

// OpenGL type for texture IDs
using GLuint = unsigned int;

namespace rme {
	namespace render {

		/// Interface for texture caching and binding
		/// Abstracts OpenGL texture management for testability and potential optimization
		class ITextureCache {
		public:
			virtual ~ITextureCache() = default;

			/// Get or create a texture for a sprite ID
			/// @param spriteId The sprite ID to get texture for
			/// @return OpenGL texture ID, or 0 if failed
			[[nodiscard]] virtual GLuint getTexture(uint32_t spriteId) = 0;

			/// Bind a texture for rendering
			/// Implementations should cache the last bound texture to avoid redundant binds
			/// @param textureId The OpenGL texture ID to bind
			virtual void bindTexture(GLuint textureId) = 0;

			/// Unbind any currently bound texture
			virtual void unbindTexture() = 0;

			/// Run garbage collection to free unused textures
			/// Should be called periodically (e.g., once per frame)
			virtual void garbageCollect() = 0;

			/// Clear all cached textures
			virtual void clear() = 0;

			/// Get number of textures currently cached
			[[nodiscard]] virtual size_t getCachedTextureCount() const noexcept = 0;

			/// Get approximate memory usage in bytes
			[[nodiscard]] virtual size_t getMemoryUsage() const noexcept = 0;

			// Prevent copying
			ITextureCache(const ITextureCache&) = delete;
			ITextureCache& operator=(const ITextureCache&) = delete;

		protected:
			ITextureCache() = default;
			ITextureCache(ITextureCache&&) = default;
			ITextureCache& operator=(ITextureCache&&) = default;
		};

	} // namespace render
} // namespace rme

#endif // RME_I_TEXTURE_CACHE_H_
