#include "app/main.h"
#include "replacement_engine.h"

#include "editor/editor.h"
#include "ui/gui.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/complexitem.h"
#include <algorithm>
#include <map>
#include <string>

ReplacementEngine::ReplacementEngine() : rng(std::random_device {}()) { }

bool ReplacementEngine::ResolveReplacement(uint16_t& resultId, const ReplacementRule& rule) {
	if (rule.targets.empty()) {
		return false;
	}

	// Calculate total probability weight
	int totalProb = 0;
	for (const auto& target : rule.targets) {
		totalProb += target.probability;
	}

	if (totalProb <= 0) {
		return false;
	}

	// Use a single roll from 1 to max(100, totalProb)
	// This ensures that if the UI says 100% total, exactly one target is picked.
	// If total is < 100, the remainder is "no replacement".
	std::uniform_int_distribution<int> dist(1, std::max(100, totalProb));
	int roll = dist(rng);

	int currentSum = 0;
	for (const auto& target : rule.targets) {
		currentSum += target.probability;
		if (roll <= currentSum) {
			resultId = target.id;
			return true;
		}
	}

	return false;
}

void ReplacementEngine::ProcessItemRef(std::unique_ptr<Item>& itemRef, const std::map<uint16_t, const ReplacementRule*>& ruleMap) {
	if (!itemRef) {
		return;
	}

	Item* item = itemRef.get();
	auto it = ruleMap.find(item->getID());

	if (it != ruleMap.end()) {
		uint16_t newId;
		if (ResolveReplacement(newId, *it->second)) {
			if (newId == TRASH_ITEM_ID || newId == 0) {
				// Keep existing object but set ID to 0 (Trash)
				item->setID(0);
				// Do not recurse into trash items
				return;
			} else {
				// Create new item of correct class
				const uint16_t oldId = item->getID();
				item->setID(newId);
				std::unique_ptr<Item> newItem = item->deepCopy();
				item->setID(oldId); // Restore old ID before destruction (good practice)

				if (newItem) {
					itemRef = std::move(newItem);
				}
			}
		}
	}

	// If replaced, we have a new item in itemRef (or same if not replaced).
	// If new item is a container, recurse.
	// If old item was a container and we replaced it, we lost its contents (expected behavior).
	if (itemRef) {
		if (Container* c = itemRef->asContainer()) {
			ProcessContainer(c, ruleMap);
		}
	}
}

void ReplacementEngine::ProcessContainer(Container* container, const std::map<uint16_t, const ReplacementRule*>& ruleMap) {
	if (!container) return;
	auto& items = container->getVector();
	for (auto& itemRef : items) {
		ProcessItemRef(itemRef, ruleMap);
	}
}

void ReplacementEngine::ProcessTile(Tile* tile, const std::map<uint16_t, const ReplacementRule*>& ruleMap) {
	if (!tile) return;
	if (tile->ground) {
		ProcessItemRef(tile->ground, ruleMap);
	}

	// Iterate over items. Note: ProcessItemRef might replace the unique_ptr in place.
	// It does not change vector size or invalid iterators (as long as we don't insert/erase).
	// But std::vector iterators are invalidated by reallocation.
	// Replacement in place (assignment to unique_ptr) does NOT reallocate vector.
	for (auto& itemRef : tile->items) {
		ProcessItemRef(itemRef, ruleMap);
	}
}

void ReplacementEngine::ExecuteReplacement(Editor* editor, const std::vector<ReplacementRule>& rules, ReplaceScope scope, const std::vector<Position>* posVec) {
	if (rules.empty()) {
		return;
	}

	std::map<uint16_t, const ReplacementRule*> ruleMap;
	for (const auto& rule : rules) {
		if (rule.fromId != 0) {
			ruleMap[rule.fromId] = &rule;
		}
	}

	if (scope == ReplaceScope::Viewport && posVec) {
		// Use Viewport scope
		for (const auto& pos : *posVec) {
			Tile* tile = editor->map.getTile(pos);
			ProcessTile(tile, ruleMap);
		}
	} else if (scope == ReplaceScope::Selection && !editor->selection.empty()) {
		// Use Selection scope
		for (Tile* tile : editor->selection.getTiles()) {
			ProcessTile(tile, ruleMap);
		}
	} else if (scope == ReplaceScope::AllMap) {
		// Use All Map scope
		for (auto& tile_loc : editor->map.tiles()) {
			ProcessTile(tile_loc.get(), ruleMap);
		}
	}

	editor->map.doChange();
	g_gui.RefreshView();
}
