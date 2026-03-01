#ifndef RME_RENDERING_IO_SPRITE_LOADER_H_
#define RME_RENDERING_IO_SPRITE_LOADER_H_

#include <vector>
#include <string>
#include <memory>
#include <wx/string.h>
#include <wx/filename.h>
#include <atomic>

#include "app/client_version.h"

class SpriteDatabase;

class SpriteLoader {
public:
	SpriteLoader() = default;
	~SpriteLoader() = default;

	SpriteLoader(const SpriteLoader&) = delete;
	SpriteLoader& operator=(const SpriteLoader&) = delete;

	void clear();

	bool loadEditorSprites(SpriteDatabase& db);
	bool loadSpriteMetadata(SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings);
	bool loadSpriteData(SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings);
	bool loadSpriteDump(std::unique_ptr<uint8_t[]>& target, uint16_t& size, int sprite_id);

	wxFileName getMetadataFileName() const {
		return client_version ? client_version->getMetadataPath() : wxFileName();
	}
	wxFileName getSpritesFileName() const {
		return client_version ? client_version->getSpritesPath() : wxFileName();
	}

	bool hasTransparency() const { return has_transparency; }
	bool isUnloaded() const { return unloaded.load(); }
	const std::string& getSpriteFile() const { return spritefile; }
	bool isExtended() const { return is_extended; }

	DatFormat getDatFormat() const { return dat_format; }
	ClientVersion* getClientVersion() const { return client_version; }

	void setClientVersion(ClientVersion* version) { client_version = version; }
	void setDatFormat(DatFormat format) { dat_format = format; }
	void setIsExtended(bool extended) { is_extended = extended; }
	void setHasTransparency(bool trans) { has_transparency = trans; }
	void setHasFrameDurations(bool framedurs) { has_frame_durations = framedurs; }
	void setHasFrameGroups(bool framegrps) { has_frame_groups = framegrps; }
	void setSpriteFile(const std::string& file) { spritefile = file; }
	void setUnloaded(bool st) { unloaded.store(st); }
	
	bool hasFrameDurations() const { return has_frame_durations; }
	bool hasFrameGroups() const { return has_frame_groups; }

private:
	ClientVersion* client_version = nullptr;
	std::string spritefile;
	std::atomic<bool> unloaded{true};

	DatFormat dat_format = DAT_FORMAT_UNKNOWN;
	bool is_extended = false;
	bool has_transparency = false;
	bool has_frame_durations = false;
	bool has_frame_groups = false;
};

#endif
