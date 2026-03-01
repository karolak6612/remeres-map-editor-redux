#ifndef RME_RENDERING_IO_SPRITE_LOADER_H_
#define RME_RENDERING_IO_SPRITE_LOADER_H_

#include <vector>
#include <string>
#include <cstdint>
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

	bool hasFrameDurations() const { std::lock_guard<std::mutex> lock(mutex_); return has_frame_durations; }
	bool hasFrameGroups() const { std::lock_guard<std::mutex> lock(mutex_); return has_frame_groups; }

	bool hasTransparency() const { std::lock_guard<std::mutex> lock(mutex_); return has_transparency; }
	bool isUnloaded() const { std::lock_guard<std::mutex> lock(mutex_); return unloaded; }
	std::string getSpriteFile() const { std::lock_guard<std::mutex> lock(mutex_); return spritefile; }
	bool isExtended() const { std::lock_guard<std::mutex> lock(mutex_); return is_extended; }

	DatFormat getDatFormat() const { std::lock_guard<std::mutex> lock(mutex_); return dat_format; }
	const ClientVersion* getClientVersion() const { std::lock_guard<std::mutex> lock(mutex_); return client_version; }

	void setClientVersion(const ClientVersion* version) { std::lock_guard<std::mutex> lock(mutex_); client_version = version; }
	void setDatFormat(DatFormat format) { std::lock_guard<std::mutex> lock(mutex_); dat_format = format; }
	void setIsExtended(bool extended) { std::lock_guard<std::mutex> lock(mutex_); is_extended = extended; }
	void setHasTransparency(bool trans) { std::lock_guard<std::mutex> lock(mutex_); has_transparency = trans; }
	void setHasFrameDurations(bool framedurs) { std::lock_guard<std::mutex> lock(mutex_); has_frame_durations = framedurs; }
	void setHasFrameGroups(bool framegrps) { std::lock_guard<std::mutex> lock(mutex_); has_frame_groups = framegrps; }
	void setSpriteFile(const std::string& file) { std::lock_guard<std::mutex> lock(mutex_); spritefile = file; }
	void setUnloaded(bool st) { std::lock_guard<std::mutex> lock(mutex_); unloaded = st; }

private:
	mutable std::mutex mutex_;
	const ClientVersion* client_version = nullptr;
	std::string spritefile;
	bool unloaded = true;

	DatFormat dat_format = DAT_FORMAT_UNKNOWN;
	bool is_extended = false;
	bool has_transparency = false;
	bool has_frame_durations = false;
	bool has_frame_groups = false;
};

#endif
