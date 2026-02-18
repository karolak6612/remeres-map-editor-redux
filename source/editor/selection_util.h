#ifndef RME_SELECTION_UTIL_H
#define RME_SELECTION_UTIL_H

#include "editor/editor.h"
#include "map/map.h"
#include "editor/selection.h"

template <typename ForeachType>
inline void foreach_ItemInSelection(Editor& editor, ForeachType& foreach) {
	long long done = 0;
	const auto& tiles = editor.selection.getTiles();
	std::vector<Container*> containers;
	containers.reserve(32);
	for (Tile* tile : tiles) {
		++done;
		foreach_ItemOnTile(editor.map, tile, foreach, done, containers);
	}
}

template <typename RemoveIfType>
inline int64_t RemoveItemInSelection(Editor& editor, RemoveIfType& condition) {
	int64_t done = 0;
	int64_t removed = 0;
	const auto& tiles = editor.selection.getTiles();

	for (Tile* tile : tiles) {
		++done;
		RemoveItemOnTile(editor.map, tile, condition, removed, done);
	}
	return removed;
}

#endif
