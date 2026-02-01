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

#include "app/main.h"

#include "ui/gui.h"
#include "editor/editor.h"
#include "map/map.h"
#include "game/sprites.h"
#include "ui/map_tab.h"
#include "editor/editor_tabs.h"
#include "rendering/ui/map_display.h"

#include <spdlog/spdlog.h>

MapTab::MapTab(MapTabbook* aui, std::shared_ptr<Editor> editor) :
	EditorTab(),
	MapWindow(aui, *editor),
	aui(aui),
	editor_ptr(editor) {
	spdlog::info("MapTab created (New Editor) [Tab={}]", (void*)this);

	aui->AddTab(this, true);
	FitToMap();
}

MapTab::MapTab(const MapTab* other) :
	EditorTab(),
	MapWindow(other->aui, *other->editor_ptr),
	aui(other->aui),
	editor_ptr(other->editor_ptr) {
	spdlog::info("MapTab created (Shared Editor) [Tab={}]", (void*)this);
	aui->AddTab(this, true);
	FitToMap();
	int x, y;
	other->GetCanvas()->GetScreenCenter(&x, &y);
	SetScreenCenterPosition(Position(x, y, other->GetCanvas()->GetFloor()));
}

MapTab::~MapTab() {
	spdlog::info("MapTab destroying [Tab={}] (Use count: {})", (void*)this, editor_ptr.use_count());
}

bool MapTab::IsUniqueReference() const {
	return editor_ptr.unique();
}

wxWindow* MapTab::GetWindow() const {
	return const_cast<MapTab*>(this);
}

MapCanvas* MapTab::GetCanvas() const {
	return canvas;
}

MapWindow* MapTab::GetView() const {
	return const_cast<MapWindow*>((const MapWindow*)this);
}

wxString MapTab::GetTitle() const {
	wxString ss;
	ss << wxstr(editor_ptr->map.getName()) << (editor_ptr->map.hasChanged() ? "*" : "");
	return ss;
}

Editor* MapTab::GetEditor() const {
	return editor_ptr.get();
}

Map* MapTab::GetMap() const {
	return &editor_ptr->map;
}

void MapTab::VisibilityCheck() {
	EditorTab* editorTab = aui->GetCurrentTab();
	MapTab* mapTab = dynamic_cast<MapTab*>(editorTab);
	UpdateDialogs(mapTab && HasSameReference(mapTab));
}

void MapTab::OnSwitchEditorMode(EditorMode mode) {
	gem->SetSprite(mode == DRAWING_MODE ? EDITOR_SPRITE_DRAWING_GEM : EDITOR_SPRITE_SELECTION_GEM);
	if (mode == SELECTION_MODE) {
		canvas->EnterSelectionMode();
	} else {
		canvas->EnterDrawingMode();
	}
}
