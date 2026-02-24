//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/ui/map_status_updater.h"
#include "ui/gui.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "live/live_server.h"
#include "rendering/utilities/tile_describer.h"
#include "app/settings.h"
#include <format>

// Static state variables
static int s_lastFloor = 7;
static double s_lastZoom = 1.0;
static wxString s_lastFPS = "";

static void RefreshField3() {
    if (g_gui.root) {
        wxString ss = std::format("Floor: {} | Zoom: {:.0f}% | {}", s_lastFloor, s_lastZoom * 100, s_lastFPS.ToStdString());
        g_gui.root->SetStatusText(ss, STATUS_FIELD_FLOOR_ZOOM);
    }
}

void MapStatusUpdater::Update(Editor& editor, int map_x, int map_y, int map_z) {
    wxString ss = std::format("x: {} y: {} z: {}", map_x, map_y, map_z);

    size_t selectionCount = editor.selection.size();
    if (selectionCount > 0) {
        ss += std::format(" | {} selected", selectionCount);
    }

    if (g_gui.root) {
        g_gui.root->SetStatusText(ss, 2);
    }

    ss = "";
    Tile* tile = editor.map.getTile(map_x, map_y, map_z);
    if (tile) {
        ss = TileDescriber::GetDescription(tile, g_settings.getInteger(Config::SHOW_SPAWNS), g_settings.getInteger(Config::SHOW_CREATURES));

        if (editor.live_manager.IsLive()) {
            editor.live_manager.GetSocket().updateCursor(Position(map_x, map_y, map_z));
        }
        if (g_gui.root) {
            g_gui.root->SetStatusText(ss, 1);
        }
    } else {
        if (g_gui.root) {
            g_gui.root->SetStatusText("Nothing", 1);
        }
    }
}

void MapStatusUpdater::UpdateFPS(const wxString& fps_status) {
    s_lastFPS = fps_status;
    RefreshField3();
}

void MapStatusUpdater::UpdateFloor(int floor) {
    s_lastFloor = floor;
    RefreshField3();
}

void MapStatusUpdater::UpdateZoom(double zoom) {
    s_lastZoom = zoom;
    RefreshField3();
}
