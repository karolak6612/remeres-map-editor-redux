#ifndef RME_TILESET_MOVE_QUEUE_H_
#define RME_TILESET_MOVE_QUEUE_H_

#include "brushes/brush_enums.h"
#include "item_definitions/core/item_definition_types.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Materials;

class TilesetMoveQueue {
public:
	struct Target {
		PaletteType palette = TILESET_UNKNOWN;
		std::string tileset_name;

		bool operator==(const Target& other) const {
			return palette == other.palette && tileset_name == other.tileset_name;
		}

		bool isValid() const {
			return palette != TILESET_UNKNOWN && !tileset_name.empty();
		}
	};

	struct Entry {
		ServerItemId item_id = 0;
		Target source;
		Target target;
	};

	struct ApplyResult {
		bool success = false;
		int applied = 0;
		int skipped = 0;
		std::vector<std::string> warnings;
		std::string error;
	};

	void QueueMove(ServerItemId item_id, const Target& source, const Target& target);
	bool Remove(ServerItemId item_id);
	void Clear();
	ApplyResult Apply(Materials& materials);

	bool Empty() const {
		return entries.empty();
	}

	size_t Size() const {
		return entries.size();
	}

	bool IsQueued(ServerItemId item_id) const;
	const Entry* Find(ServerItemId item_id) const;
	const Target* QueuedSource(ServerItemId item_id) const;
	const Target* QueuedTarget(ServerItemId item_id) const;

	std::vector<Entry> EntriesForTarget(PaletteType palette, std::string_view tileset_name) const;
	std::vector<Entry> Entries() const;

	const std::vector<Target>& RecentTargets() const {
		return recent_targets;
	}

private:
	void RememberRecentTarget(const Target& target);

	std::unordered_map<ServerItemId, Entry> entries;
	std::vector<Target> recent_targets;
};

#endif
