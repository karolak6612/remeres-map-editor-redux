#include "rendering/core/sprite_archive.h"

#include "app/definitions.h"
#include "io/filehandle.h"

#include <format>
#include <utility>
#include <wx/filename.h>
#include <wx/string.h>

namespace {
	constexpr uint32_t kSpriteDataOffset = 3;

	bool readSpriteCount(FileReadHandle& file, bool is_extended, uint32_t& sprite_count) {
		if (is_extended) {
			return file.getU32(sprite_count);
		}

		uint16_t compact_count = 0;
		if (!file.getU16(compact_count)) {
			return false;
		}
		sprite_count = compact_count;
		return true;
	}

	bool readSpriteOffsets(FileReadHandle& file, uint32_t sprite_count, std::vector<uint32_t>& offsets) {
		offsets.assign(static_cast<size_t>(sprite_count) + 1, 0);
		for (uint32_t sprite_id = 1; sprite_id <= sprite_count; ++sprite_id) {
			if (!file.getU32(offsets[sprite_id])) {
				return false;
			}
		}
		return true;
	}
}

SpriteArchive::SpriteArchive(std::string filename, bool is_extended, uint32_t sprite_count, std::vector<uint32_t> sprite_offsets) :
	filename_(std::move(filename)),
	is_extended_(is_extended),
	sprite_count_(sprite_count),
	sprite_offsets_(std::move(sprite_offsets)) {
}

std::shared_ptr<SpriteArchive> SpriteArchive::load(const wxFileName& path, bool is_extended, wxString& error, std::vector<std::string>& warnings) {
	FileReadHandle file(path.GetFullPath().ToStdString());
	if (!file.isOk()) {
		error = wxString::FromUTF8(std::format("Failed to open {} for reading: {}", path.GetFullPath().utf8_string(), file.getErrorMessage()));
		return nullptr;
	}

	uint32_t signature = 0;
	uint32_t sprite_count = 0;
	if (!file.getU32(signature) || !readSpriteCount(file, is_extended, sprite_count)) {
		error = "Failed to read sprites header.";
		return nullptr;
	}
	(void)signature;
	if (sprite_count > MAX_SPRITES) {
		error = wxString::FromUTF8(std::format("Sprite count {} exceeds MAX_SPRITES={}.", sprite_count, MAX_SPRITES));
		return nullptr;
	}

	std::vector<uint32_t> offsets;
	if (!readSpriteOffsets(file, sprite_count, offsets)) {
		error = "Failed to read sprites index table.";
		return nullptr;
	}

	if (sprite_count == 0) {
		warnings.push_back("Sprite archive contains zero sprites.");
	}

	return std::shared_ptr<SpriteArchive>(new SpriteArchive(path.GetFullPath().ToStdString(), is_extended, sprite_count, std::move(offsets)));
}

bool SpriteArchive::readCompressed(uint32_t sprite_id, std::unique_ptr<uint8_t[]>& target, uint16_t& size) const {
	size = 0;
	target.reset();

	if (sprite_id == 0) {
		return true;
	}
	if (sprite_id >= sprite_offsets_.size()) {
		return false;
	}

	const uint32_t offset = sprite_offsets_[sprite_id];
	if (offset == 0) {
		return true;
	}

	FileReadHandle file(filename_);
	if (!file.isOk()) {
		return false;
	}
	if (!file.seek(offset + kSpriteDataOffset)) {
		return false;
	}

	uint16_t compressed_size = 0;
	if (!file.getU16(compressed_size)) {
		return false;
	}

	auto buffer = std::make_unique<uint8_t[]>(compressed_size);
	if (!file.getRAW(buffer.get(), compressed_size)) {
		return false;
	}

	size = compressed_size;
	target = std::move(buffer);
	return true;
}
