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

#ifndef RME_SELECTION_H
#define RME_SELECTION_H

#include "map/position.h"
#include <functional>
#include <vector>
#include <algorithm>
#include <atomic>

class Action;
class Editor;
class BatchAction;

class SelectionThread;

class Selection {
public:
	std::function<void(size_t)> onSelectionChange;

	Selection(Editor& editor);
	~Selection();

	// Selects the items on the tile/tiles
	// Won't work outside a selection session
	void add(Tile* tile, Item* item);
	void add(Tile* tile, Spawn* spawn);
	void add(Tile* tile, Creature* creature);
	void add(Tile* tile);
	void remove(Tile* tile, Item* item);
	void remove(Tile* tile, Spawn* spawn);
	void remove(Tile* tile, Creature* creature);
	void remove(Tile* tile);

	// The tile will be added to the list of selected tiles, however, the items on the tile won't be selected
	void addInternal(Tile* tile);
	void removeInternal(Tile* tile);

	// Clears the selection completely
	void clear();

	// Returns true when inside a session
	bool isBusy() {
		return busy;
	}

	//
	// Returns the bounds of the selection.
	// NOTE: Bounds queries should only be called from the main thread if
	// the selection is being modified by another thread (e.g. SelectionThread).
	Position minPosition() const;
	Position maxPosition() const;

	// This manages a "selection session"
	// Internal session doesn't store the result (eg. no undo)
	// Subthread means the session doesn't create a complete
	// action, just part of one to be merged with the main thread
	// later.
	enum SessionFlags {
		NONE,
		INTERNAL = 1,
		SUBTHREAD = 2,
	};

	void start(SessionFlags flags = NONE);
	void commit();
	void finish(SessionFlags flags = NONE);

	// Joins the selection instance in this thread with this instance
	// Ownership of the thread is transferred to join
	void join(std::unique_ptr<SelectionThread> thread);

	size_t size() const {
		flush();
		return tiles.size();
	}
	bool empty() const {
		flush();
		return tiles.empty();
	}
	void updateSelectionCount();
	auto begin() const {
		flush();
		return tiles.begin();
	}
	auto end() const {
		flush();
		return tiles.end();
	}
	const std::vector<Tile*>& getTiles() const {
		flush();
		return tiles;
	}
	Tile* getSelectedTile() const {
		flush();
		ASSERT(tiles.size() == 1);
		return *tiles.begin();
	}

private:
	void flush() const;
	void recalculateBounds() const;

	bool busy;
	bool deferred;
	Editor& editor;
	std::unique_ptr<BatchAction> session;
	std::unique_ptr<Action> subsession;

	// We use std::vector here instead of std::set for performance reasons.
	// Selections are typically small, and std::vector provides better cache locality
	// and fewer allocations. We maintain sorted order to allow O(log n) lookups.
	// These members are mutable to support a lazy-evaluation/deferred-flush strategy.
	// pending_adds and pending_removes store changes during a 'deferred' session,
	// which are merged into 'tiles' by the next call to flush().
	// This improves performance during batch selections (e.g. drag-select).
	mutable std::vector<Tile*> tiles;
	mutable std::vector<Tile*> pending_adds;
	mutable std::vector<Tile*> pending_removes;

	// Bounds are cached lazily for performance as selection sets can be large.
	mutable std::atomic<bool> bounds_dirty;
	mutable Position cached_min;
	mutable Position cached_max;

	friend class SelectionThread;
};

#endif
