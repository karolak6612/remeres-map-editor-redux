//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/ui/selection_controller.h"
#include "editor/selection_thread.h"
#include "rendering/ui/map_display.h"
#include "editor/editor.h"
#include "editor/action_queue.h"
#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/spawn.h"
#include "game/creature.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "rendering/ui/brush_selector.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/raw/raw_brush.h"
#include "ui/dialog_helper.h"

namespace {
	enum class TileSelectionTargetKind {
		None,
		Spawn,
		NpcSpawn,
		Creature,
		Item,
	};

	struct TileSelectionTarget {
		TileSelectionTargetKind kind = TileSelectionTargetKind::None;
		Item* item = nullptr;
		Spawn* spawn = nullptr;
		Creature* creature = nullptr;

		[[nodiscard]] bool exists() const {
			return kind != TileSelectionTargetKind::None;
		}

		[[nodiscard]] bool isSelected() const {
			switch (kind) {
				case TileSelectionTargetKind::Spawn:
				case TileSelectionTargetKind::NpcSpawn:
					return spawn && spawn->isSelected();
				case TileSelectionTargetKind::Creature:
					return creature && creature->isSelected();
				case TileSelectionTargetKind::Item:
					return item && item->isSelected();
				case TileSelectionTargetKind::None:
					return false;
			}
			return false;
		}
	};

	[[nodiscard]] TileSelectionTarget getPrimaryTileSelectionTarget(Tile* tile) {
		if (!tile) {
			return {};
		}
		if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
			return TileSelectionTarget { .kind = TileSelectionTargetKind::Spawn, .spawn = tile->spawn.get() };
		}
		if (tile->npc_spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
			return TileSelectionTarget { .kind = TileSelectionTargetKind::NpcSpawn, .spawn = tile->npc_spawn.get() };
		}
		if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			return TileSelectionTarget { .kind = TileSelectionTargetKind::Creature, .creature = tile->creature.get() };
		}
		if (Item* item = tile->getTopItem()) {
			// Items remain selectable even when hidden so property editing and recovery workflows
			// can still target the tile's top item through selection mode.
			return TileSelectionTarget { .kind = TileSelectionTargetKind::Item, .item = item };
		}
		return {};
	}

	void addTileSelectionTarget(Selection& selection, Tile* tile, const TileSelectionTarget& target) {
		switch (target.kind) {
			case TileSelectionTargetKind::Spawn:
			case TileSelectionTargetKind::NpcSpawn:
				selection.add(tile, target.spawn);
				break;
			case TileSelectionTargetKind::Creature:
				selection.add(tile, target.creature);
				break;
			case TileSelectionTargetKind::Item:
				selection.add(tile, target.item);
				break;
			case TileSelectionTargetKind::None:
				break;
		}
	}

	void removeTileSelectionTarget(Selection& selection, Tile* tile, const TileSelectionTarget& target) {
		switch (target.kind) {
			case TileSelectionTargetKind::Spawn:
			case TileSelectionTargetKind::NpcSpawn:
				selection.remove(tile, target.spawn);
				break;
			case TileSelectionTargetKind::Creature:
				selection.remove(tile, target.creature);
				break;
			case TileSelectionTargetKind::Item:
				selection.remove(tile, target.item);
				break;
			case TileSelectionTargetKind::None:
				break;
		}
	}

	bool togglePrimaryTileSelection(Editor& editor, Tile* tile) {
		const TileSelectionTarget target = getPrimaryTileSelectionTarget(tile);
		if (!target.exists()) {
			return false;
		}

		editor.selection.start();
		if (target.isSelected()) {
			removeTileSelectionTarget(editor.selection, tile, target);
		} else {
			addTileSelectionTarget(editor.selection, tile, target);
		}
		editor.selection.finish();
		editor.selection.updateSelectionCount();
		return true;
	}

	bool selectPrimaryTileTarget(Editor& editor, Tile* tile, bool clear_existing, bool commit_selection, bool only_if_unselected) {
		const TileSelectionTarget target = getPrimaryTileSelectionTarget(tile);
		if (!target.exists()) {
			return false;
		}
		if (only_if_unselected && target.isSelected()) {
			return false;
		}

		editor.selection.start();
		if (clear_existing) {
			editor.selection.clear();
			if (commit_selection) {
				editor.selection.commit();
			}
		}
		addTileSelectionTarget(editor.selection, tile, target);
		editor.selection.finish();
		editor.selection.updateSelectionCount();
		return true;
	}
}

SelectionController::SelectionController(MapCanvas* canvas, Editor& editor) :
	canvas(canvas),
	editor(editor),
	dragging(false),
	boundbox_selection(false),
	drag_start_pos(Position()) {
}

SelectionController::~SelectionController() {
}

void SelectionController::HandleClick(const Position& mouse_map_pos, bool shift_down, bool ctrl_down, bool alt_down) {
	if (ctrl_down && alt_down) {
		Tile* tile = editor.map.getTile(mouse_map_pos);
		if (tile && tile->size() > 0) {
			// Select visible creature
			if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
				CreatureBrush* brush = tile->creature->getBrush();
				if (brush) {
					g_gui.SelectBrush(brush, TILESET_CREATURE);
					return;
				}
			}
			// Fall back to item selection
			Item* item = tile->getTopItem();
			if (item && item->getRAWBrush()) {
				g_gui.SelectBrush(item->getRAWBrush(), TILESET_RAW);
			}
		}
	} else if (g_gui.IsSelectionMode()) {
		if (canvas->isPasting()) {
			// Set paste to false (no rendering etc.)
			canvas->EndPasting();

			// Paste to the map
			editor.copybuffer.paste(editor, mouse_map_pos);

			// Start dragging
			dragging = true;
			drag_start_pos = mouse_map_pos;
		} else {
			boundbox_selection = false;
			if (shift_down) {
				boundbox_selection = true;

				if (!ctrl_down) {
					editor.selection.start(); // Start selection session
					editor.selection.clear(); // Clear out selection
					editor.selection.finish(); // End selection session
					editor.selection.updateSelectionCount();
				}
			} else if (ctrl_down) {
				Tile* tile = editor.map.getTile(mouse_map_pos);
				if (tile) {
					togglePrimaryTileSelection(editor, tile);
				}
			} else {
				Tile* tile = editor.map.getTile(mouse_map_pos);
				if (!tile) {
					editor.selection.start(); // Start selection session
					editor.selection.clear(); // Clear out selection
					editor.selection.finish(); // End selection session
					editor.selection.updateSelectionCount();
				} else if (tile->isSelected()) {
					dragging = true;
					drag_start_pos = mouse_map_pos;
				} else {
					if (selectPrimaryTileTarget(editor, tile, true, true, false)) {
						dragging = true;
						drag_start_pos = mouse_map_pos;
					}
				}
			}
		}
	}
}

void SelectionController::HandleDrag(const Position& mouse_map_pos, bool shift_down, bool ctrl_down, bool alt_down) {
	if (g_gui.IsSelectionMode()) {
		if (canvas->isPasting()) {
			canvas->Refresh();
		} else if (dragging) {
			wxString ss;

			int move_x = drag_start_pos.x - mouse_map_pos.x;
			int move_y = drag_start_pos.y - mouse_map_pos.y;
			int move_z = drag_start_pos.z - mouse_map_pos.z;
			ss << "Dragging " << -move_x << "," << -move_y << "," << -move_z;
			g_gui.SetStatusText(ss);

			canvas->Refresh();
		} else if (boundbox_selection) {
			// Calculate selection size
			int move_x = std::abs(canvas->last_click_map_x - mouse_map_pos.x);
			int move_y = std::abs(canvas->last_click_map_y - mouse_map_pos.y);
			wxString ss;
			ss << "Selection " << move_x + 1 << ":" << move_y + 1;
			g_gui.SetStatusText(ss);

			canvas->Refresh();
		}
	}
}

void SelectionController::HandleRelease(const Position& mouse_map_pos, bool shift_down, bool ctrl_down, bool alt_down) {
	int move_x = canvas->last_click_map_x - mouse_map_pos.x;
	int move_y = canvas->last_click_map_y - mouse_map_pos.y;
	int move_z = canvas->last_click_map_z - mouse_map_pos.z;

	if (g_gui.IsSelectionMode()) {
		if (dragging && (move_x != 0 || move_y != 0 || move_z != 0)) {
			editor.moveSelection(Position(move_x, move_y, move_z));
		} else {
			if (boundbox_selection) {
				if (mouse_map_pos.x == canvas->last_click_map_x && mouse_map_pos.y == canvas->last_click_map_y && ctrl_down) {
					// Mouse hasn't moved, do control+shift thingy!
					Tile* tile = editor.map.getTile(mouse_map_pos);
					if (tile) {
						editor.selection.start(); // Start a selection session
						if (tile->isSelected()) {
							editor.selection.remove(tile);
						} else {
							editor.selection.add(tile);
						}
						editor.selection.finish(); // Finish the selection session
						editor.selection.updateSelectionCount();
					}
				} else {
					ExecuteBoundboxSelection(Position(canvas->last_click_map_x, canvas->last_click_map_y, canvas->last_click_map_z), mouse_map_pos, mouse_map_pos.z);
				}
			} else if (ctrl_down) {
				////
			} else {
				// User hasn't moved anything, meaning selection/deselection
				Tile* tile = editor.map.getTile(mouse_map_pos);
				if (tile) {
					selectPrimaryTileTarget(editor, tile, false, false, true);
				}
			}
		}
		editor.actionQueue->resetTimer();
		dragging = false;
		boundbox_selection = false;
	}
}

void SelectionController::HandlePropertiesClick(const Position& mouse_map_pos, bool shift_down, bool ctrl_down, bool alt_down) {
	Tile* tile = editor.map.getTile(mouse_map_pos);

	if (g_gui.IsDrawingMode()) {
		g_gui.SetSelectionMode();
	}

	canvas->EndPasting();

	boundbox_selection = false;
	if (shift_down) {
		boundbox_selection = true;

		if (!ctrl_down) {
			editor.selection.start(); // Start selection session
			editor.selection.clear(); // Clear out selection
			editor.selection.finish(); // End selection session
			editor.selection.updateSelectionCount();
		}
	} else if (!tile) {
		editor.selection.start(); // Start selection session
		editor.selection.clear(); // Clear out selection
		editor.selection.finish(); // End selection session
		editor.selection.updateSelectionCount();
	} else if (tile->isSelected()) {
		// Do nothing!
	} else {
		selectPrimaryTileTarget(editor, tile, true, true, false);
	}
}

void SelectionController::HandlePropertiesRelease(const Position& mouse_map_pos, bool shift_down, bool ctrl_down, bool alt_down) {
	if (g_gui.IsDrawingMode()) {
		g_gui.SetSelectionMode();
	}

	if (boundbox_selection) {
		if (mouse_map_pos.x == canvas->last_click_map_x && mouse_map_pos.y == canvas->last_click_map_y && ctrl_down) {
			// Mouse hasn't move, do control+shift thingy!
			Tile* tile = editor.map.getTile(mouse_map_pos);
			if (tile) {
				editor.selection.start(); // Start a selection session
				if (tile->isSelected()) {
					editor.selection.remove(tile);
				} else {
					editor.selection.add(tile);
				}
				editor.selection.finish(); // Finish the selection session
				editor.selection.updateSelectionCount();
			}
		} else {
			ExecuteBoundboxSelection(Position(canvas->last_click_map_x, canvas->last_click_map_y, canvas->last_click_map_z), mouse_map_pos, mouse_map_pos.z);
		}
	} else if (ctrl_down) {
		// Nothing
	}

	editor.actionQueue->resetTimer();
	dragging = false;
	boundbox_selection = false;
}

void SelectionController::HandleDoubleClick(const Position& mouse_map_pos) {
	if (g_settings.getInteger(Config::DOUBLECLICK_PROPERTIES)) {
		Tile* tile = editor.map.getTile(mouse_map_pos);
		if (tile) {
			DialogHelper::OpenProperties(editor, tile);
		}
	}
}

void SelectionController::ExecuteBoundboxSelection(const Position& start_pos, const Position& end_pos, int floor) {
	int start_x = start_pos.x;
	int start_y = start_pos.y;
	int end_x = end_pos.x;
	int end_y = end_pos.y;

	if (start_x > end_x) {
		std::swap(start_x, end_x);
	}
	if (start_y > end_y) {
		std::swap(start_y, end_y);
	}

	int numtiles = 0;
	int threadcount = std::max(g_settings.getInteger(Config::WORKER_THREADS), 1);

	int s_x = 0, s_y = 0, s_z = 0;
	int e_x = 0, e_y = 0, e_z = 0;

	switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
		case SELECT_CURRENT_FLOOR: {
			s_z = e_z = floor;
			s_x = start_x;
			s_y = start_y;
			e_x = end_x;
			e_y = end_y;
			break;
		}
		case SELECT_ALL_FLOORS: {
			s_x = start_x;
			s_y = start_y;
			s_z = MAP_MAX_LAYER;
			e_x = end_x;
			e_y = end_y;
			e_z = floor;

			if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
				s_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
				s_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);

				e_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
				e_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
			}

			numtiles = (s_z - e_z) * (e_x - s_x) * (e_y - s_y);
			break;
		}
		case SELECT_VISIBLE_FLOORS: {
			s_x = start_x;
			s_y = start_y;
			if (floor <= GROUND_LAYER) {
				s_z = GROUND_LAYER;
			} else {
				s_z = std::min(MAP_MAX_LAYER, floor + 2);
			}
			e_x = end_x;
			e_y = end_y;
			e_z = floor;

			if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
				s_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
				s_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);

				e_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
				e_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
			}
			break;
		}
	}

	if (numtiles < 500) {
		// No point in threading for such a small set.
		threadcount = 1;
	}
	// Subdivide the selection area
	// We know it's a square, just split it into several areas
	int width = e_x - s_x;
	if (width < threadcount) {
		threadcount = std::min(1, width);
	}
	// Let's divide!
	int remainder = width;
	int cleared = 0;
	std::vector<std::unique_ptr<SelectionThread>> threads;
	if (width == 0) {
		threads.push_back(std::make_unique<SelectionThread>(editor, Position(s_x, s_y, s_z), Position(s_x, e_y, e_z)));
	} else {
		for (int i = 0; i < threadcount; ++i) {
			int chunksize = width / threadcount;
			// The last threads takes all the remainder
			if (i == threadcount - 1) {
				chunksize = remainder;
			}
			threads.push_back(std::make_unique<SelectionThread>(editor, Position(s_x + cleared, s_y, s_z), Position(s_x + cleared + chunksize, e_y, e_z)));
			cleared += chunksize;
			remainder -= chunksize;
		}
	}
	ASSERT(cleared == width);
	ASSERT(remainder == 0);

	editor.selection.start(); // Start a selection session
	for (auto& thread : threads) {
		thread->Start();
	}
	for (auto& thread : threads) {
		editor.selection.join(std::move(thread));
	}
	editor.selection.finish(); // Finish the selection session
	editor.selection.updateSelectionCount();
}
