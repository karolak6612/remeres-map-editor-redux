#ifndef RME_GAME_ITEM_METADATA_H
#define RME_GAME_ITEM_METADATA_H

#include <map>
#include <string>
#include <cstdint>
#include "util/definitions.h" // For FileName, if applicable, or use standard includes

// Forward declarations
class wxString;

enum class TechItemType {
	NONE = 0,
	INVISIBLE_STAIRS,
	INVISIBLE_WALKABLE,
	INVISIBLE_WALL,
	PRIMAL_LIGHT,
	CLIENT_ID_ZERO
};

struct ItemMetadata {
	TechItemType techType = TechItemType::NONE;
	uint16_t disguiseID = 0; // 0 means no disguise

	bool isEmpty() const {
		return techType == TechItemType::NONE && disguiseID == 0;
	}
};

class ItemMetadataManager {
public:
	ItemMetadataManager();
	~ItemMetadataManager();

	bool load(const std::string& filename);
	bool save(const std::string& filename);

	const ItemMetadata& getMetadata(uint16_t id) const;
	void setMetadata(uint16_t id, const ItemMetadata& metadata);
	void removeMetadata(uint16_t id);

	const std::map<uint16_t, ItemMetadata>& getAllMetadata() const {
		return metadataMap;
	}

	// Helper to populate legacy hardcoded values if file doesn't exist
	void applyLegacyDefaults();

private:
	std::map<uint16_t, ItemMetadata> metadataMap;
	std::string currentFilename;
};

#endif // RME_GAME_ITEM_METADATA_H
