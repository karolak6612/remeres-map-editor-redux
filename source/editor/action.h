//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

/**
 * @file action.h
 * @brief Defines the action and undo/redo system.
 *
 * Contains classes for representing atomic changes (Change), user actions (Action),
 * and grouped actions (BatchAction) that can be undone or redone.
 */

#ifndef RME_ACTION_H_
#define RME_ACTION_H_

#include "map/position.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

class Editor;
class Tile;
class House;
class Waypoint;
class Change;
class Action;
class BatchAction;
class ActionQueue;

/**
 * @brief Identifies the type of modification in a Change object.
 */
enum ChangeType {
	CHANGE_NONE,
	CHANGE_TILE, ///< Modification to a tile (items, ground, etc.).
	CHANGE_MOVE_HOUSE_EXIT, ///< Relocation of a house exit.
	CHANGE_MOVE_WAYPOINT, ///< Relocation of a waypoint.
};

/**
 * @brief Data payload for a house exit change.
 */
struct HouseExitChangeData {
	uint32_t houseId;
	Position pos;
};

/**
 * @brief Data payload for a waypoint change.
 */
struct WaypointChangeData {
	std::string name;
	Position pos;
};

/**
 * @brief Represents a single atomic modification to the map state.
 *
 * A Change object holds the data necessary to reverse a specific operation,
 * such as a snapshot of a tile before modification.
 */
class Change {
private:
	using Data = std::variant<std::monostate, std::unique_ptr<Tile>, HouseExitChangeData, WaypointChangeData>;
	ChangeType type;
	Data data;

	Change();

public:
	/**
	 * @brief Constructs a change based on a tile's state.
	 * @param tile The tile being modified.
	 */
	Change(Tile* tile);

	/**
	 * @brief Creates a change for a house exit move.
	 * @param house The house.
	 * @param where The old position of the exit.
	 * @return Pointer to the new Change.
	 */
	static Change* Create(House* house, const Position& where);

	/**
	 * @brief Creates a change for a waypoint move.
	 * @param wp The waypoint.
	 * @param where The old position.
	 * @return Pointer to the new Change.
	 */
	static Change* Create(Waypoint* wp, const Position& where);
	~Change();

	/**
	 * @brief Clears the change data.
	 */
	void clear();

	/**
	 * @brief Gets the type of change.
	 * @return ChangeType enum.
	 */
	ChangeType getType() const {
		return type;
	}

	/**
	 * @brief Gets the stored tile snapshot (if applicable).
	 * @return Const pointer to Tile.
	 */
	const Tile* getTile() const;

	/**
	 * @brief Gets the house exit data (if applicable).
	 * @return Const pointer to HouseExitChangeData.
	 */
	const HouseExitChangeData* getHouseExitData() const;

	/**
	 * @brief Gets the waypoint data (if applicable).
	 * @return Const pointer to WaypointChangeData.
	 */
	const WaypointChangeData* getWaypointData() const;

	// Get memory footprint
	/**
	 * @brief Calculates memory usage of this change.
	 * @return Size in bytes.
	 */
	uint32_t memsize() const;

	friend class Action;
};

using ChangeList = std::vector<std::unique_ptr<Change>>;

class DirtyList;

/**
 * @brief Identifies the high-level user action being performed.
 */
enum ActionIdentifier {
	ACTION_MOVE,
	ACTION_REMOTE,
	ACTION_SELECT,
	ACTION_DELETE_TILES,
	ACTION_CUT_TILES,
	ACTION_PASTE_TILES,
	ACTION_RANDOMIZE,
	ACTION_BORDERIZE,
	ACTION_DRAW,
	ACTION_SWITCHDOOR,
	ACTION_ROTATE_ITEM,
	ACTION_REPLACE_ITEMS,
	ACTION_CHANGE_PROPERTIES,
};

/**
 * @brief Represents a user command that can be undone/redone.
 *
 * An Action consists of a list of Change objects.
 */
class Action {
public:
	virtual ~Action();

	/**
	 * @brief Adds a change to this action.
	 * @param t The change to add.
	 */
	void addChange(std::unique_ptr<Change> t) {
		changes.push_back(std::move(t));
	}

	// Get memory footprint
	/**
	 * @brief Estimates approximate memory usage.
	 * @return Size in bytes.
	 */
	size_t approx_memsize() const;

	/**
	 * @brief Calculates exact memory usage.
	 * @return Size in bytes.
	 */
	size_t memsize() const;

	/**
	 * @brief Gets the number of changes in this action.
	 * @return Change count.
	 */
	size_t size() const {
		return changes.size();
	}

	/**
	 * @brief Gets the type of action.
	 * @return ActionIdentifier enum.
	 */
	ActionIdentifier getType() const {
		return type;
	}

	/**
	 * @brief Commits the action (redo).
	 * @param dirty_list List to track modified areas.
	 */
	void commit(DirtyList* dirty_list);

	/**
	 * @brief Checks if the action is currently committed.
	 * @return true if committed.
	 */
	bool isCommited() const {
		return commited;
	}

	/**
	 * @brief Reverses the action (undo).
	 * @param dirty_list List to track modified areas.
	 */
	void undo(DirtyList* dirty_list);

	/**
	 * @brief Re-applies the action (redo).
	 * @param dirty_list List to track modified areas.
	 */
	void redo(DirtyList* dirty_list) {
		commit(dirty_list);
	}

protected:
	Action(Editor& editor, ActionIdentifier ident);

	bool commited;
	ChangeList changes;
	Editor& editor;
	ActionIdentifier type;

	friend class ActionQueue;
};

using ActionVector = std::vector<std::unique_ptr<Action>>;

/**
 * @brief Group of actions treated as a single undo/redo step.
 *
 * Used for complex operations like "smearing" a brush where multiple
 * draw actions occur but should be undone together.
 */
class BatchAction {
public:
	virtual ~BatchAction();

	/**
	 * @brief Resets the merge timer.
	 */
	void resetTimer() {
		timestamp = 0;
	}

	// Get memory footprint
	/**
	 * @brief Calculates memory usage.
	 * @param resize Whether to compact internal storage.
	 * @return Size in bytes.
	 */
	size_t memsize(bool resize = false) const;

	/**
	 * @brief Gets number of sub-actions.
	 * @return Action count.
	 */
	size_t size() const {
		return batch.size();
	}

	/**
	 * @brief Gets the batch action type.
	 * @return ActionIdentifier.
	 */
	ActionIdentifier getType() const {
		return type;
	}

	/**
	 * @brief Adds an action to the batch.
	 * @param action The action to add.
	 */
	virtual void addAction(std::unique_ptr<Action> action);

	/**
	 * @brief Adds an action and immediately commits it.
	 * @param action The action to add and commit.
	 */
	virtual void addAndCommitAction(std::unique_ptr<Action> action);

protected:
	BatchAction(Editor& editor, ActionIdentifier ident);

	virtual void commit();
	virtual void undo();
	virtual void redo();

	void merge(BatchAction* other);

	Editor& editor;
	int timestamp;
	uint32_t memory_size;
	ActionIdentifier type;
	ActionVector batch;

	friend class ActionQueue;
};

#endif
