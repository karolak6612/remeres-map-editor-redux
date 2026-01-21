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

#ifndef RME_SPRITE_LOADER_H_
#define RME_SPRITE_LOADER_H_

#include "client_version.h"
#include <wx/filename.h>
#include <string>
#include <vector>
#include <cstdint>

// Forward declarations
class FileReadHandle;
class GameSprite;
class GraphicManager;

namespace rme {
	namespace render {

		/// Result of a sprite loading operation
		struct SpriteLoadResult {
			bool success = false;
			std::string errorMessage;
			std::vector<std::string> warnings;
			uint16_t itemCount = 0;
			uint16_t creatureCount = 0;
		};

		/// Sprite metadata format information
		struct SpriteMetadataInfo {
			uint16_t itemCount = 0;
			uint16_t creatureCount = 0;
			bool hasTransparency = false;
			bool hasFrameDurations = false;
			bool hasFrameGroups = false;
			bool isExtended = false;

			wxFileName metadataFile;
			wxFileName spritesFile;
		};

		/// Interface for loading sprite metadata and data
		/// Extracted from GraphicManager to follow SRP
		/// Note: This is a facade that delegates to GraphicManager during transition
		class SpriteLoader {
		public:
			explicit SpriteLoader(GraphicManager& graphicManager);
			~SpriteLoader();

			/// Load OTFI (extended sprite) information
			/// @param filename Path to OTFI file
			/// @param error Output error message
			/// @param warnings Output warning messages
			/// @return true if successful
			bool loadOTFI(const std::string& filename, std::string& error, std::vector<std::string>& warnings);

			/// Load sprite metadata from .dat file
			/// @param filename Path to .dat file
			/// @param error Output error message
			/// @param warnings Output warning messages
			/// @return true if successful
			bool loadMetadata(const std::string& filename, std::string& error, std::vector<std::string>& warnings);

			/// Load sprite data from .spr file
			/// @param filename Path to .spr file
			/// @param error Output error message
			/// @param warnings Output warning messages
			/// @return true if successful
			bool loadSpriteData(const std::string& filename, std::string& error, std::vector<std::string>& warnings);

			/// Load all sprites from a directory (convenience method)
			/// @param directory Directory containing .dat and .spr files
			/// @return Load result with success status and counts
			SpriteLoadResult loadFromDirectory(const std::string& directory);

			/// Get metadata information after loading
			[[nodiscard]] const SpriteMetadataInfo& getMetadataInfo() const noexcept {
				return metadataInfo_;
			}

			/// Check if sprites are loaded
			[[nodiscard]] bool isLoaded() const noexcept {
				return isLoaded_;
			}

			// Prevent copying
			SpriteLoader(const SpriteLoader&) = delete;
			SpriteLoader& operator=(const SpriteLoader&) = delete;

		private:
			bool loadSpriteMetadataFlags(FileReadHandle& file, GameSprite* sType, std::string& error, std::vector<std::string>& warnings);
			bool loadSpriteDump(uint8_t*& target, uint16_t& size, int sprite_id);

			friend class ::GraphicManager;

			GraphicManager& graphicManager_;
			SpriteMetadataInfo metadataInfo_;
			DatFormat datFormat_ = DAT_FORMAT_UNKNOWN;
			bool otfiFound_ = false;
			bool isLoaded_ = false;

			std::string spriteFile_; // Used if memcaching is OFF
		};

	} // namespace render
} // namespace rme

#endif // RME_SPRITE_LOADER_H_
