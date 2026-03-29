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

#include "map/tile_operations.h"
#include "app/main.h"
#include "rendering/ui/popup_action_handler.h"
#include "editor/editor.h"
#include "editor/action_queue.h"
#include "map/tile.h"
#include "game/item.h"
#include "ui/gui.h"
#include "app/preferences.h"
#include "app/visuals.h"
#include "ui/browse_tile_window.h"
#include "ui/tileset_window.h"
#include "ui/dialog_helper.h"
#include "brushes/brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include <ranges>
#include "brushes/door/door_brush.h"

void PopupActionHandler::RotateItem(Editor& editor) {
	Tile* tile = editor.selection.getSelectedTile();

	std::unique_ptr<Action> action = editor.actionQueue->createAction(ACTION_ROTATE_ITEM);

	std::unique_ptr<Tile> new_tile(TileOperations::deepCopy(tile, editor.map));

	ItemVector selected_items = TileOperations::getSelectedItems(new_tile.get());
	ASSERT(!selected_items.empty());

	selected_items.front()->doRotate();

	action->addChange(std::make_unique<Change>(std::move(new_tile)));

	editor.actionQueue->addAction(std::move(action));
	g_gui.RefreshView();
}

void PopupActionHandler::GotoDestination(Editor& editor) {
	Tile* tile = editor.selection.getSelectedTile();
	ItemVector selected_items = TileOperations::getSelectedItems(tile);
	ASSERT(!selected_items.empty());
	Teleport* teleport = dynamic_cast<Teleport*>(selected_items.front());
	if (teleport) {
		Position pos = teleport->getDestination();
		g_gui.SetScreenCenterPosition(pos);
	}
}

void PopupActionHandler::SwitchDoor(Editor& editor) {
	Tile* tile = editor.selection.getSelectedTile();

	std::unique_ptr<Action> action = editor.actionQueue->createAction(ACTION_SWITCHDOOR);

	std::unique_ptr<Tile> new_tile(TileOperations::deepCopy(tile, editor.map));

	ItemVector selected_items = TileOperations::getSelectedItems(new_tile.get());
	ASSERT(!selected_items.empty());

	DoorBrush::switchDoor(selected_items.front());

	action->addChange(std::make_unique<Change>(std::move(new_tile)));

	editor.actionQueue->addAction(std::move(action));
	g_gui.RefreshView();
}

void PopupActionHandler::BrowseTile(Editor& editor, int cursor_x, int cursor_y) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	std::unique_ptr<Tile> new_tile(TileOperations::deepCopy(tile, editor.map));

	wxDialog* w = new BrowseTileWindow(g_gui.root, new_tile.get(), wxPoint(cursor_x, cursor_y));

	int ret = w->ShowModal();
	if (ret != 0) {
		std::unique_ptr<Action> action = editor.actionQueue->createAction(ACTION_DELETE_TILES);
		action->addChange(std::make_unique<Change>(std::move(new_tile)));
		editor.addAction(std::move(action));
	}

	w->Destroy();
}

void PopupActionHandler::OpenProperties(Editor& editor) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (tile) {
		DialogHelper::OpenProperties(editor, tile);
	}
}

void PopupActionHandler::OpenVisualEditor(Editor& editor, Tile* tile) {
	if (!tile) {
		return;
	}

	std::optional<VisualEditContext> context;
	const ItemVector selected_items = TileOperations::getSelectedItems(tile);
	if (!selected_items.empty()) {
		context = VisualEditContext { .seed_rule = Visuals::MakeItemRule(selected_items.back()->getID()) };
	} else if (tile->spawn) {
		context = VisualEditContext { .seed_rule = Visuals::MakeMarkerRule(tile->spawn->isSelected() ? MarkerVisualKind::SpawnSelected : MarkerVisualKind::Spawn) };
	} else if (editor.map.waypoints.getWaypoint(tile->getLocation())) {
		context = VisualEditContext { .seed_rule = Visuals::MakeMarkerRule(MarkerVisualKind::Waypoint) };
	} else if (tile->isTownExit(editor.map)) {
		context = VisualEditContext { .seed_rule = Visuals::MakeMarkerRule(MarkerVisualKind::TownTemple) };
	} else if (tile->isHouseExit()) {
		uint32_t current_house_id = 0;
		if (Brush* brush = g_gui.GetCurrentBrush()) {
			if (brush->is<HouseBrush>()) {
				current_house_id = brush->as<HouseBrush>()->getHouseID();
			} else if (brush->is<HouseExitBrush>()) {
				current_house_id = brush->as<HouseExitBrush>()->getHouseID();
			}
		}
		const MarkerVisualKind kind = tile->hasHouseExit(current_house_id) ? MarkerVisualKind::HouseExitCurrent : MarkerVisualKind::HouseExitOther;
		context = VisualEditContext { .seed_rule = Visuals::MakeMarkerRule(kind) };
	} else if (tile->isHouseTile()) {
		context = VisualEditContext { .seed_rule = Visuals::MakeTileRule(TileVisualKind::HouseOverlay) };
	} else if (tile->isPZ()) {
		context = VisualEditContext { .seed_rule = Visuals::MakeTileRule(TileVisualKind::Pz) };
	} else if (tile->getMapFlags() & TILESTATE_PVPZONE) {
		context = VisualEditContext { .seed_rule = Visuals::MakeTileRule(TileVisualKind::Pvp) };
	} else if (tile->getMapFlags() & TILESTATE_NOLOGOUT) {
		context = VisualEditContext { .seed_rule = Visuals::MakeTileRule(TileVisualKind::NoLogout) };
	} else if (tile->getMapFlags() & TILESTATE_NOPVP) {
		context = VisualEditContext { .seed_rule = Visuals::MakeTileRule(TileVisualKind::NoPvp) };
	} else if (tile->isBlocking()) {
		context = VisualEditContext { .seed_rule = Visuals::MakeTileRule(TileVisualKind::Blocking) };
	}

	if (!context.has_value()) {
		return;
	}

	PreferencesWindow dialog(g_gui.root, PreferencesPageSelection::Visuals, context);
	dialog.ShowModal();
}

void PopupActionHandler::SelectMoveTo(Editor& editor) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	std::unique_ptr<Tile> new_tile(TileOperations::deepCopy(tile, editor.map));

	wxDialog* w = nullptr;

	ItemVector selected_items = TileOperations::getSelectedItems(new_tile.get());

	Item* item = nullptr;
	for (auto* item_ptr : std::ranges::reverse_view(selected_items)) {
		if (item_ptr->isSelected()) {
			item = item_ptr;
			break;
		}
	}

	if (item) {
		w = newd TilesetWindow(g_gui.root, &editor.map, new_tile.get(), item);
	} else {
		return;
	}

	int ret = w->ShowModal();
	if (ret != 0) {
		std::unique_ptr<Action> action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(std::make_unique<Change>(std::move(new_tile)));
		editor.addAction(std::move(action));

		g_gui.RebuildPalettes();
	}
	w->Destroy();
}
