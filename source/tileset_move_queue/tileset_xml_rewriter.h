#ifndef RME_TILESET_XML_REWRITER_H_
#define RME_TILESET_XML_REWRITER_H_

#include "tileset_move_queue/tileset_move_queue.h"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class TilesetXmlRewriter {
public:
	struct Result {
		bool success = false;
		int applied = 0;
		int skipped = 0;
		std::vector<std::string> warnings;
		std::string error;
	};

	Result Apply(const std::vector<TilesetMoveQueue::Entry>& entries) const;

private:
	static std::string ResolveTilesetsPath();

public:
	static std::string DefaultTilesetsPath() {
		return ResolveTilesetsPath();
	}

	static std::unordered_set<std::string> LoadXmlTilesetNames();
	static bool IsVirtualRuntimeTilesetName(std::string_view tileset_name);
};

#endif
