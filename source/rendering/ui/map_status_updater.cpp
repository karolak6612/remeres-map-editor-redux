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
#include "rendering/ui/map_status_updater.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "live/live_server.h"
#include "rendering/utilities/tile_describer.h"
#include "app/settings.h"
#include "ui/gui.h"

void MapStatusUpdater::Update(GUI& gui, const Settings& settings, Editor& editor, int map_x, int map_y, int map_z) {
	wxString ss;
	ss << "x: " << map_x << " y:" << map_y << " z:" << map_z;
	gui.root->SetStatusText(ss, 2);

	ss = "";
	Tile* tile = editor.map.getTile(map_x, map_y, map_z);
	if (tile) {
		ss = TileDescriber::GetDescription(tile, settings.getInteger(Config::SHOW_SPAWNS), settings.getInteger(Config::SHOW_CREATURES));

		if (editor.live_manager.IsLive()) {
			editor.live_manager.GetSocket().updateCursor(Position(map_x, map_y, map_z));
		}
		gui.root->SetStatusText(ss, 1);
	} else {
		gui.root->SetStatusText("Nothing", 1);
	}
}

void MapStatusUpdater::UpdateFPS(GUI& gui, const wxString& fps_status) {
	gui.root->SetStatusText(fps_status, 0);
}
