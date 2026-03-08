#ifndef RME_RENDERING_CORE_SPRITE_ARCHIVE_H_
#define RME_RENDERING_CORE_SPRITE_ARCHIVE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class wxFileName;
class wxString;

class SpriteArchive {
public:
	[[nodiscard]] static std::shared_ptr<SpriteArchive> load(const wxFileName& path, bool is_extended, wxString& error, std::vector<std::string>& warnings);

	[[nodiscard]] uint32_t spriteCount() const {
		return sprite_count_;
	}

	[[nodiscard]] bool isExtended() const {
		return is_extended_;
	}

	[[nodiscard]] const std::string& fileName() const {
		return filename_;
	}

	[[nodiscard]] bool readCompressed(uint32_t sprite_id, std::unique_ptr<uint8_t[]>& target, uint16_t& size) const;

private:
	SpriteArchive(std::string filename, bool is_extended, uint32_t sprite_count, std::vector<uint32_t> sprite_offsets);

	std::string filename_;
	bool is_extended_ = false;
	uint32_t sprite_count_ = 0;
	std::vector<uint32_t> sprite_offsets_;
};

#endif
