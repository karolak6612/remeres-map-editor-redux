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

/*
 * @file editor.h
 * @brief Core editor application logic.
 *
 * Defines the central Editor class which coordinates user actions, map modifications,
 * selections, and the undo/redo stack. It acts as the primary controller for the application.
 */

#ifndef RME_EDITOR_H
#define RME_EDITOR_H

#include "game/item.h"
#include "map/tile.h"
#include "io/iomap.h"
#include "map/map.h"

#include "editor/action.h"
#include "editor/selection.h"
#include "editor/operations/selection_operations.h"
#include "map/operations/map_processor.h"
#include "editor/persistence/editor_persistence.h"

#include <memory>

class BaseMap;
class CopyBuffer;
class LiveClient;
class LiveServer;
class LiveSocket;

#include "live/live_manager.h"

#include <functional>

/*
 * @brief The main controller for map editing operations.
 *
 * The Editor class encapsulates the current map state, selection, copy buffer,
 * and action history (undo/redo). It processes user input commands (draw, move, delete)
 * and dispatches them as Actions to the ActionQueue.
 *
 * It also integrates with the LiveManager for collaborative editing.
 */
class Editor {
public:
	/*
	 * @brief Constructs an Editor with a live client connection.
	 * @param copybuffer Shared copy buffer.
	 * @param version Target client version.
	 * @param client Live client instance.
	 */
	Editor(CopyBuffer& copybuffer, const MapVersion& version, std::unique_ptr<LiveClient> client);

	/*
	 * @brief Constructs an Editor by loading a map file.
	 * @param copybuffer Shared copy buffer.
	 * @param version Target client version.
	 * @param fn Filename to load.
	 */
	Editor(CopyBuffer& copybuffer, const MapVersion& version, const FileName& fn);

	/*
	 * @brief Constructs a blank Editor.
	 * @param copybuffer Shared copy buffer.
	 * @param version Target client version.
	 */
	Editor(CopyBuffer& copybuffer, const MapVersion& version);
	~Editor();

	// Live Manager
	LiveManager live_manager;

public:
	// Public members
	std::unique_ptr<ActionQueue> actionQueue;
	Selection selection;
	CopyBuffer& copybuffer;
	GroundBrush* replace_brush;
	Map map; // The map that is being edited

	std::function<void()> onStateChange;

	/*
	 * @brief Notifies listeners that the editor state has changed (e.g. for UI refresh).
	 */
	void notifyStateChange();

public: // Functions
	// Map handling

	// Adds an action to the action queue (this allows the user to undo the action)
	// Invalidates the action pointer
	/*
	 * @brief Adds a batch action (multiple changes) to the undo stack.
	 * @param action The batch action to execute.
	 * @param stacking_delay Delay in ms for action merging.
	 */
	void addBatch(std::unique_ptr<BatchAction> action, int stacking_delay = 0);

	/*
	 * @brief Adds a single action to the undo stack.
	 * @param action The action to execute.
	 * @param stacking_delay Delay in ms for action merging.
	 */
	void addAction(std::unique_ptr<Action> action, int stacking_delay = 0);

	// Selection
	/*
	 * @brief Checks if there is an active selection.
	 * @return true if selection is not empty.
	 */
	bool hasSelection() const {
		return selection.size() != 0;
	}
	// Some simple actions that work on the map (these will work through the undo queue)
	// Moves the selected area by the offset
	/*
	 * @brief Moves the currently selected tiles/items by a given offset.
	 * @param offset The (x, y, z) displacement vector.
	 */
	void moveSelection(Position offset);

	// Deletes all selected items
	/*
	 * @brief Deletes all items within the current selection.
	 */
	void destroySelection();

	// Borderizes the selected region
	/*
	 * @brief Auto-borders the edges of the current selection.
	 */
	void borderizeSelection();

	// Randomizes the ground in the selected region
	/*
	 * @brief Randomizes ground tiles within the selection (if supported by brush).
	 */
	void randomizeSelection();

	// Same as above although it applies to the entire map
	// action queue is flushed when these functions are called
	// showdialog is whether a progress bar should be shown
	/*
	 * @brief Runs auto-bordering on the entire map.
	 * @param showdialog If true, shows a progress dialog.
	 */
	void borderizeMap(bool showdialog);

	/*
	 * @brief Runs randomization on the entire map.
	 * @param showdialog If true, shows a progress dialog.
	 */
	void randomizeMap(bool showdialog);

	/*
	 * @brief Removes invalid house tiles across the map.
	 * @param showdialog If true, shows a progress dialog.
	 */
	void clearInvalidHouseTiles(bool showdialog);

	/*
	 * @brief Resets the "modified" flag on all tiles.
	 * @param showdialog If true, shows a progress dialog.
	 */
	void clearModifiedTileState(bool showdialog);

	// Draw using the current brush to the target position
	// alt is whether the ALT key is pressed
	/*
	 * @brief Draws with the current brush at a specific offset.
	 * @param offset Target position.
	 * @param alt Alt key state (affects brush behavior).
	 */
	void draw(const Position& offset, bool alt);

	/*
	 * @brief Erases (undraws) at a specific offset.
	 * @param offset Target position.
	 * @param alt Alt key state.
	 */
	void undraw(const Position& offset, bool alt);

	/*
	 * @brief Draws at multiple positions.
	 * @param posvec List of positions.
	 * @param alt Alt key state.
	 */
	void draw(const PositionVector& posvec, bool alt);

	/*
	 * @brief Draws at multiple positions with separate border targets.
	 * @param todraw Positions to paint.
	 * @param toborder Positions to check for bordering.
	 * @param alt Alt key state.
	 */
	void draw(const PositionVector& todraw, PositionVector& toborder, bool alt);

	/*
	 * @brief Erases at multiple positions.
	 * @param posvec List of positions.
	 * @param alt Alt key state.
	 */
	void undraw(const PositionVector& posvec, bool alt);

	/*
	 * @brief Erases at multiple positions with separate border targets.
	 * @param todraw Positions to erase.
	 * @param toborder Positions to check for bordering.
	 * @param alt Alt key state.
	 */
	void undraw(const PositionVector& todraw, PositionVector& toborder, bool alt);

protected:
	void drawInternal(const Position offset, bool alt, bool dodraw);
	void drawInternal(const PositionVector& posvec, bool alt, bool dodraw);
	void drawInternal(const PositionVector& todraw, PositionVector& toborder, bool alt, bool dodraw);

	Editor(const Editor&);
	Editor& operator=(const Editor&);
};

inline void Editor::draw(const Position& offset, bool alt) {
	drawInternal(offset, alt, true);
}
inline void Editor::undraw(const Position& offset, bool alt) {
	drawInternal(offset, alt, false);
}
inline void Editor::draw(const PositionVector& posvec, bool alt) {
	drawInternal(posvec, alt, true);
}
inline void Editor::draw(const PositionVector& todraw, PositionVector& toborder, bool alt) {
	drawInternal(todraw, toborder, alt, true);
}
inline void Editor::undraw(const PositionVector& posvec, bool alt) {
	drawInternal(posvec, alt, false);
}
inline void Editor::undraw(const PositionVector& todraw, PositionVector& toborder, bool alt) {
	drawInternal(todraw, toborder, alt, false);
}

#endif
