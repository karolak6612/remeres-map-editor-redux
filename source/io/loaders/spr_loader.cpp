#include "io/loaders/spr_loader.h"

#include "rendering/core/sprite_database.h"
#include "rendering/io/sprite_loader.h"
#include "rendering/core/normal_image.h"
#include "io/filehandle.h"
#include "app/settings.h"
#include <vector>
#include <format>
#include <memory>

// Anonymous namespace for constants
namespace {
	constexpr uint32_t SPRITE_DATA_OFFSET = 3;
	constexpr uint32_t SPRITE_ADDRESS_SIZE_EXTENDED = 4;
	constexpr uint32_t SPRITE_ADDRESS_SIZE_NORMAL = 2;
}

bool SprLoader::LoadData(SpriteLoader& loader, SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	FileReadHandle fh(nstr(datafile.GetFullPath()));

	if (!fh.isOk()) {
		error = wxstr(std::format("Failed to open file {} for reading", datafile.GetFullPath().utf8_string()));
		return false;
	}

	// Local helper lambda for safe get
	auto safe_get_u32 = [&](uint32_t& out) -> bool {
		if (!fh.getU32(out)) {
			error = wxstr(fh.getErrorMessage());
			return false;
		}
		return true;
	};
	auto safe_get_u16 = [&](uint16_t& out) -> bool {
		if (!fh.getU16(out)) {
			error = wxstr(fh.getErrorMessage());
			return false;
		}
		return true;
	};

	uint32_t sprSignature;
	if (!safe_get_u32(sprSignature)) {
		return false;
	}

	uint32_t total_pics = 0;
	if (loader.isExtended()) {
		if (!safe_get_u32(total_pics)) {
			return false;
		}
	} else {
		uint16_t u16 = 0;
		if (!safe_get_u16(u16)) {
			return false;
		}
		total_pics = u16;
	}

	if (total_pics > MAX_SPRITES) {
		error = wxstr(std::format("Sprite count {} exceeds limit (MAX_SPRITES={})", total_pics, MAX_SPRITES));
		return false;
	}

	loader.setSpriteFile(nstr(datafile.GetFullPath()));
	loader.setUnloaded(false);

	if (!g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
		return true;
	}

	// Pre-allocate image_space if total_pics is known
	// Resize image_space to match exact sprite count, removing potential stale entries
	db.getImageSpace().resize(total_pics + 1);

	std::vector<uint32_t> sprite_indexes = ReadSpriteIndexes(fh, total_pics, error);
	if (sprite_indexes.empty() && total_pics > 0) {
		return false;
	}

	return ReadSprites(db, fh, sprite_indexes, warnings, error);
}

std::vector<uint32_t> SprLoader::ReadSpriteIndexes(FileReadHandle& fh, uint32_t total_pics, wxString& error) {
	std::vector<uint32_t> sprite_indexes;
	sprite_indexes.reserve(total_pics);

	for (uint32_t i = 0; i < total_pics; ++i) {
		uint32_t index;
		if (!fh.getU32(index)) {
			error = wxstr(fh.getErrorMessage());
			return {};
		}
		sprite_indexes.push_back(index);
	}
	return sprite_indexes;
}

bool SprLoader::ReadSprites(SpriteDatabase& db, FileReadHandle& fh, const std::vector<uint32_t>& sprite_indexes, std::vector<std::string>& warnings, wxString& error) {
	int id = 1;
	for (uint32_t index : sprite_indexes) {
		if (index == 0) {
			id++;
			continue;
		}

		uint32_t seek_pos = index + SPRITE_DATA_OFFSET;
		if (!fh.seek(seek_pos)) {
			// Seek failed, likely bad index or EOF. Log it.
			warnings.push_back(std::format("SprLoader: Failed to seek to sprite data at offset {} for id {}", seek_pos, id));
			continue;
		}

		uint16_t size;
		if (!fh.getU16(size)) {
			error = wxstr(fh.getErrorMessage());
			return false;
		}

		if (id < db.getImageSpace().size() && db.getImageSpace()[id]) {
			NormalImage* spr = dynamic_cast<NormalImage*>(db.getImageSpace()[id].get());
			if (spr) {
				if (size > 0) {
					if (spr->size > 0) {
						// Duplicate GameSprite id
						warnings.push_back(std::format("items.spr: Duplicate GameSprite id {}", id));
						if (!fh.seekRelative(size)) {
							error = wxstr(fh.getErrorMessage());
							return false;
						}
					} else {
						spr->id = id;
						spr->size = size;
						spr->dump = std::make_unique<uint8_t[]>(size);
						if (!fh.getRAW(spr->dump.get(), size)) {
							error = wxstr(fh.getErrorMessage());
							return false;
						}
					}
				}
			} else {
				warnings.push_back(std::format("SprLoader: Failed to cast sprite id {} to NormalImage", id));
				if (size > 0) {
					if (!fh.seekRelative(size)) {
						error = wxstr(fh.getErrorMessage());
						return false;
					}
				}
			}
		} else {
			if (!fh.seekRelative(size)) {
				error = wxstr(fh.getErrorMessage());
				return false;
			}
		}
		id++;
	}
	return true;
}

bool SprLoader::LoadDump(SpriteLoader& loader, std::unique_ptr<uint8_t[]>& target, uint16_t& size, int sprite_id) {
	return LoadDump(loader.getSpriteFile(), loader.isExtended(), target, size, sprite_id);
}

bool SprLoader::LoadDump(const std::string& filename, bool extended, std::unique_ptr<uint8_t[]>& target, uint16_t& size, int sprite_id) {
	if (sprite_id == 0) {
		// Empty GameSprite
		size = 0;
		target.reset();
		return true;
	}

	if (filename.empty()) {
		return false;
	}

	FileReadHandle fh(filename);
	if (!fh.isOk()) {
		return false;
	}

	uint32_t address_size = extended ? SPRITE_ADDRESS_SIZE_EXTENDED : SPRITE_ADDRESS_SIZE_NORMAL;
	if (!fh.seek(address_size + sprite_id * sizeof(uint32_t))) {
		return false;
	}

	uint32_t to_seek = 0;
	if (fh.getU32(to_seek)) {
		if (to_seek == 0) {
			// Offset 0 means empty/transparent sprite
			size = 0;
			target.reset(); // ensure null
			return true;
		}
		if (!fh.seek(to_seek + SPRITE_DATA_OFFSET)) {
			return false;
		}
		uint16_t sprite_size;
		if (fh.getU16(sprite_size)) {
			target = std::make_unique<uint8_t[]>(size = sprite_size);
			if (fh.getRAW(target.get(), sprite_size)) {
				return true;
			}
			target.reset();
		}
	}
	return false;
}
