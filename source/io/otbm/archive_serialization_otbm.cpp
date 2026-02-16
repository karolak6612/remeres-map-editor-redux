#include "archive_serialization_otbm.h"

#include "io/archive_io.h"
#include "ui/gui.h"
#include "map/map.h"
#include <spdlog/spdlog.h>
#include <span>
#include <format>
#include <sstream>

bool ArchiveSerializationOTBM::loadMapFromOTGZ(IOMapOTBM& iomap, Map& map, const FileName& filename) {
#ifdef OTGZ_SUPPORT
	ArchiveReader reader;
	if (!reader.open(nstr(filename.GetFullPath()))) {
		return false;
	}

	g_gui.SetLoadDone(0, "Decompressing archive...");

	// Load OTBM
	if (auto otbmBuffer = reader.extractFile("world/map.otbm")) {
		spdlog::debug("Loading OTBM map from OTGZ archive");
		if (otbmBuffer->size() < 4) {
			return false;
		}

		g_gui.SetLoadDone(0, "Loading OTBM map...");
		// Create a memory handle, skip 4-byte magic
		MemoryNodeFileReadHandle f(otbmBuffer->data() + 4, otbmBuffer->size() - 4);
		if (!iomap.loadMap(map, f)) {
			spdlog::error("Could not load OTBM file inside archive");
			return false;
		}
	} else {
		spdlog::error("OTBM file not found inside archive.");
		return false;
	}

	// Load Houses
	if (auto houseBuffer = reader.extractFile("world/houses.xml")) {
		pugi::xml_document doc;
		if (doc.load_buffer(houseBuffer->data(), houseBuffer->size())) {
			if (!iomap.loadHouses(map, doc)) {
				spdlog::warn("Failed to load houses from archive.");
			}
		} else {
			spdlog::warn("Failed to load houses due to XML parse error.");
		}
	}

	// Load Spawns
	if (auto spawnBuffer = reader.extractFile("world/spawns.xml")) {
		pugi::xml_document doc;
		if (doc.load_buffer(spawnBuffer->data(), spawnBuffer->size())) {
			if (!iomap.loadSpawns(map, doc)) {
				spdlog::warn("Failed to load spawns from archive.");
			}
		} else {
			spdlog::warn("Failed to load spawns due to XML parse error.");
		}
	}

	return true;
#else
	return false;
#endif
}

bool ArchiveSerializationOTBM::saveMapToOTGZ(IOMapOTBM& iomap, Map& map, const FileName& identifier) {
#ifdef OTGZ_SUPPORT
	ArchiveWriter writer;
	if (!writer.open(nstr(identifier.GetFullPath()))) {
		return false;
	}

	g_gui.SetLoadDone(0, "Saving spawns...");

	pugi::xml_document spawnDoc;
	if (iomap.saveSpawns(map, spawnDoc)) {
		std::ostringstream streamData;
		spawnDoc.save(streamData, "", pugi::format_raw, pugi::encoding_utf8);
		std::string xmlData = streamData.str();
		writer.addFile("world/spawns.xml", std::span(reinterpret_cast<const uint8_t*>(xmlData.data()), xmlData.size()));
	}

	g_gui.SetLoadDone(0, "Saving houses...");

	pugi::xml_document houseDoc;
	if (iomap.saveHouses(map, houseDoc)) {
		std::ostringstream streamData;
		houseDoc.save(streamData, "", pugi::format_raw, pugi::encoding_utf8);
		std::string xmlData = streamData.str();
		writer.addFile("world/houses.xml", std::span(reinterpret_cast<const uint8_t*>(xmlData.data()), xmlData.size()));
	}

	g_gui.SetLoadDone(0, "Saving OTBM map...");

	MemoryNodeFileWriteHandle otbmWriter;
	iomap.saveMap(map, otbmWriter);

	g_gui.SetLoadDone(75, "Compressing...");

	std::vector<uint8_t> otbmData;
	otbmData.reserve(otbmWriter.getSize() + 4);
	const char* magic = "OTBM";
	otbmData.insert(otbmData.end(), magic, magic + 4);
	otbmData.insert(otbmData.end(), otbmWriter.getMemory(), otbmWriter.getMemory() + otbmWriter.getSize());

	writer.addFile("world/map.otbm", otbmData);

	g_gui.DestroyLoadBar();
	return true;
#else
	return false;
#endif
}
