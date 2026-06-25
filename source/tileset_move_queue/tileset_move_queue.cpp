#include "app/main.h"
#include "tileset_move_queue/tileset_move_queue.h"

#include "game/materials.h"
#include "item_definitions/core/item_definition_store.h"
#include "tileset_move_queue/tileset_xml_rewriter.h"
#include "util/file_system.h"

#include <algorithm>
#include <spdlog/spdlog.h>

void TilesetMoveQueue::QueueMove(ServerItemId item_id, const Target& source, const Target& target) {
	if (item_id == 0 || !source.isValid() || !target.isValid()) {
		return;
	}

	entries[item_id] = Entry {
		.item_id = item_id,
		.source = source,
		.target = target,
	};

	RememberRecentTarget(target);
}

bool TilesetMoveQueue::Remove(ServerItemId item_id) {
	return entries.erase(item_id) > 0;
}

void TilesetMoveQueue::Clear() {
	entries.clear();
	recent_targets.clear();
}

TilesetMoveQueue::ApplyResult TilesetMoveQueue::Apply(Materials& materials) {
	ApplyResult apply_result;
	const std::vector<Entry> queued_entries = Entries();
	if (queued_entries.empty()) {
		apply_result.success = true;
		return apply_result;
	}

	TilesetXmlRewriter rewriter;
	const auto rewrite_result = rewriter.Apply(queued_entries);
	apply_result.success = rewrite_result.success;
	apply_result.applied = rewrite_result.applied;
	apply_result.skipped = rewrite_result.skipped;
	apply_result.warnings = rewrite_result.warnings;
	apply_result.error = rewrite_result.error;
	if (!apply_result.success) {
		return apply_result;
	}

	wxString error;
	std::vector<std::string> reload_warnings;
	const FileName tilesets_path(wxstr(TilesetXmlRewriter::DefaultTilesetsPath()));
	materials.clear();
	if (!materials.loadMaterials(tilesets_path, error, reload_warnings)) {
		apply_result.success = false;
		apply_result.error = error.empty() ? "Failed to reload tilesets after apply." : nstr(error);
		return apply_result;
	}
	materials.loadExtensions(FileName(FileSystem::GetExtensionsDirectory()), error, reload_warnings);
	materials.createOtherTileset();
	if (!reload_warnings.empty()) {
		spdlog::warn("Tileset move queue reload produced {} background warnings; omitting them from the user-facing apply summary.", reload_warnings.size());
	}

	Clear();
	return apply_result;
}

bool TilesetMoveQueue::IsQueued(ServerItemId item_id) const {
	return entries.contains(item_id);
}

const TilesetMoveQueue::Entry* TilesetMoveQueue::Find(ServerItemId item_id) const {
	const auto it = entries.find(item_id);
	if (it == entries.end()) {
		return nullptr;
	}
	return &it->second;
}

const TilesetMoveQueue::Target* TilesetMoveQueue::QueuedSource(ServerItemId item_id) const {
	if (const Entry* entry = Find(item_id)) {
		return &entry->source;
	}
	return nullptr;
}

const TilesetMoveQueue::Target* TilesetMoveQueue::QueuedTarget(ServerItemId item_id) const {
	if (const Entry* entry = Find(item_id)) {
		return &entry->target;
	}
	return nullptr;
}

std::vector<TilesetMoveQueue::Entry> TilesetMoveQueue::EntriesForTarget(PaletteType palette, std::string_view tileset_name) const {
	std::vector<Entry> matches;
	matches.reserve(entries.size());

	for (const auto& [item_id, entry] : entries) {
		if (entry.target.palette != palette || entry.target.tileset_name != tileset_name) {
			continue;
		}
		matches.push_back(entry);
	}

	std::ranges::sort(matches, {}, &Entry::item_id);
	return matches;
}

std::vector<TilesetMoveQueue::Entry> TilesetMoveQueue::Entries() const {
	std::vector<Entry> values;
	values.reserve(entries.size());

	for (const auto& [item_id, entry] : entries) {
		values.push_back(entry);
	}

	std::ranges::sort(values, {}, &Entry::item_id);
	return values;
}

void TilesetMoveQueue::RememberRecentTarget(const Target& target) {
	if (!target.isValid()) {
		return;
	}

	std::erase(recent_targets, target);
	recent_targets.insert(recent_targets.begin(), target);
	if (recent_targets.size() > 3) {
		recent_targets.resize(3);
	}
}
