//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "../../logging/logger.h"
#include "../../main.h"
#include "selection_handler.h"
#include "../canvas/map_canvas.h"
#include "../../creature.h"
#include "../../editor.h"
#include "../../gui.h"

namespace rme {
	namespace input {

		SelectionInputHandler::SelectionInputHandler(rme::canvas::MapCanvas* canvas, Editor& editor) :
			canvas_(canvas), editor_(editor) {
		}

		void SelectionInputHandler::onMouseDown(const MouseEvent& event) {
			if (!g_gui.IsSelectionMode() || !event.isLeftButton()) {
				return;
			}

			int mouse_map_x = event.mapPos.x;
			int mouse_map_y = event.mapPos.y;
			int floor = event.mapPos.z;

			LOG_RENDER_INFO("[SELECTION] Mouse down at ({}, {}, {})", mouse_map_x, mouse_map_y, floor);

			/*
						if (canvas_->isPasting()) {
							// Committing a paste
							canvas_->EndPasting();
							editor_.copybuffer.paste(editor_, Position(mouse_map_x, mouse_map_y, floor));
							canvas_->dragging_ = true;
							return;
						}
			*/

			if (event.modifiers.shift) {
				canvas_->boundBoxSelection = true;
				if (!event.modifiers.ctrl) {
					editor_.selection.start();
					editor_.selection.clear();
					editor_.selection.finish();
				}
			} else {
				Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor);
				if (tile && tile->isSelected() && !event.modifiers.ctrl) {
					canvas_->dragging_ = true;
				} else if (event.modifiers.ctrl) {
					// Toggle selection (handled in onMouseClick mostly, but legacy does some things here)
				} else if (tile) {
					// Handle initial selection for dragging
					editor_.selection.start();
					editor_.selection.clear();
					if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
						editor_.selection.add(tile, tile->spawn);
						canvas_->dragging_ = true;
					} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						editor_.selection.add(tile, tile->creature);
						canvas_->dragging_ = true;
					} else {
						Item* item = tile->getTopItem();
						if (item) {
							editor_.selection.add(tile, item);
							canvas_->dragging_ = true;
						}
					}
					editor_.selection.finish();
					editor_.selection.updateSelectionCount();
				} else {
					// Clicked on empty air
					editor_.selection.start();
					editor_.selection.clear();
					editor_.selection.finish();
					editor_.selection.updateSelectionCount();
				}
			}
		}

		void SelectionInputHandler::onMouseUp(const MouseEvent& event) {
			if (event.isLeftButton()) {
				// dragging_ and boundbox_selection_ are reset in onDragEnd or here
				// Legacy resets them at the end of OnMouseActionRelease
				canvas_->dragging_ = false;
				canvas_->boundbox_selection_ = false;
			}
		}

		void SelectionInputHandler::onMouseClick(const MouseEvent& event) {
			if (!g_gui.IsSelectionMode() || !event.isLeftButton()) {
				return;
			}

			// Legacy handles most clicks in OnMouseActionClick (which is our onMouseDown/onMouseDrag)
			// But toggle logic fits well here
			if (event.modifiers.ctrl && !event.modifiers.shift) {
				Tile* tile = editor_.map.getTile(event.mapPos.x, event.mapPos.y, event.mapPos.z);
				if (tile) {
					editor_.selection.start();
					// Toggle logic from legacy
					if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
						if (tile->spawn->isSelected()) {
							editor_.selection.remove(tile, tile->spawn);
						} else {
							editor_.selection.add(tile, tile->spawn);
						}
					} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						if (tile->creature->isSelected()) {
							editor_.selection.remove(tile, tile->creature);
						} else {
							editor_.selection.add(tile, tile->creature);
						}
					} else {
						Item* item = tile->getTopItem();
						if (item) {
							if (item->isSelected()) {
								editor_.selection.remove(tile, item);
							} else {
								editor_.selection.add(tile, item);
							}
						}
					}
					editor_.selection.finish();
					editor_.selection.updateSelectionCount();
				}
			}
		}

		void SelectionInputHandler::onMouseDrag(const MouseEvent& event, const DragState& drag) {
			if (!g_gui.IsSelectionMode()) {
				return;
			}

			if (drag.button == MouseButton::Left) {
				if (canvas_->boundBoxSelection) {
					// Update selection box bounds in context for renderer
					canvas_->renderState_.context.boundBoxSelection = true;
					// Note: actual coords would be stored in state
				} else if (canvas_->dragging_) {
					// Movement is implicit by RenderCoordinator using Context::dragOffset
				}
			}
		}

		void SelectionInputHandler::onDragEnd(const DragState& drag) {
			if (!g_gui.IsSelectionMode()) {
				return;
			}

			if (drag.button == MouseButton::Left) {
				if (canvas_->boundBoxSelection) {
					// Commit selection box (select all items in area)
					int x1 = std::min(drag.startMap.x, drag.currentMap.x);
					int y1 = std::min(drag.startMap.y, drag.currentMap.y);
					int x2 = std::max(drag.startMap.x, drag.currentMap.x);
					int y2 = std::max(drag.startMap.y, drag.currentMap.y);

					LOG_RENDER_INFO("[SELECTION] Committing box selection: ({},{}) to ({},{})", x1, y1, x2, y2);

					editor_.selection.start();
					for (int y = y1; y <= y2; ++y) {
						for (int x = x1; x <= x2; ++x) {
							Tile* tile = editor_.map.getTile(x, y, drag.startMap.z);
							if (tile) {
								editor_.selection.add(tile, tile->getTopItem());
							}
						}
					}
					editor_.selection.finish();
					editor_.selection.updateSelectionCount();
				} else if (canvas_->dragging_) {
					// Commit move items
					int dx = drag.currentMap.x - drag.startMap.x;
					int dy = drag.currentMap.y - drag.startMap.y;
					int dz = drag.currentMap.z - drag.startMap.z;

					if (dx != 0 || dy != 0 || dz != 0) {
						editor_.moveSelection(Position(dx, dy, dz));
					}
				}
			}
			canvas_->dragging_ = false;
			canvas_->boundBoxSelection = false;
			canvas_->renderState_.context.boundBoxSelection = false;
		}

	} // namespace input
} // namespace rme
