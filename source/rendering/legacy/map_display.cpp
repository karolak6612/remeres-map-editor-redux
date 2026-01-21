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

#include "main.h"

#include <sstream>
#include <time.h>
#include <thread>
#include <chrono>
#include <wx/wfstream.h>

#include "gui.h"
#include "editor.h"
#include "brush.h"
#include "sprites.h"
#include "map.h"
#include "tile.h"
#include "old_properties_window.h"
#include "properties_window.h"
#include "tileset_window.h"
#include "palette_window.h"
#include "map_display.h"
#include "map_drawer.h"
#include "application.h"
#include "live_server.h"
#include "browse_tile_window.h"

#include "doodad_brush.h"
#include "house_exit_brush.h"
#include "house_brush.h"
#include "wall_brush.h"
#include "spawn_brush.h"
#include "creature_brush.h"
#include "ground_brush.h"
#include "waypoint_brush.h"
#include "raw_brush.h"
#include "table_brush.h"
#include "../input/camera_handler.h"
#include "../input/brush_handler.h"
#include "../input/selection_handler.h"

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
EVT_KEY_DOWN(MapCanvas::OnKeyDown)
EVT_KEY_DOWN(MapCanvas::OnKeyUp)

// Mouse events
EVT_MOTION(MapCanvas::OnMouseMove)
EVT_LEFT_UP(MapCanvas::OnMouseLeftRelease)
EVT_LEFT_DOWN(MapCanvas::OnMouseLeftClick)
EVT_LEFT_DCLICK(MapCanvas::OnMouseLeftDoubleClick)
EVT_MIDDLE_DOWN(MapCanvas::OnMouseCenterClick)
EVT_MIDDLE_UP(MapCanvas::OnMouseCenterRelease)
EVT_RIGHT_DOWN(MapCanvas::OnMouseRightClick)
EVT_RIGHT_UP(MapCanvas::OnMouseRightRelease)
EVT_MOUSEWHEEL(MapCanvas::OnWheel)
EVT_ENTER_WINDOW(MapCanvas::OnGainMouse)
EVT_LEAVE_WINDOW(MapCanvas::OnLoseMouse)

// Drawing events
EVT_PAINT(MapCanvas::OnPaint)
EVT_ERASE_BACKGROUND(MapCanvas::OnEraseBackground)

// Menu events
EVT_MENU(MAP_POPUP_MENU_CUT, MapCanvas::OnCut)
EVT_MENU(MAP_POPUP_MENU_COPY, MapCanvas::OnCopy)
EVT_MENU(MAP_POPUP_MENU_COPY_POSITION, MapCanvas::OnCopyPosition)
EVT_MENU(MAP_POPUP_MENU_PASTE, MapCanvas::OnPaste)
EVT_MENU(MAP_POPUP_MENU_DELETE, MapCanvas::OnDelete)
//----
EVT_MENU(MAP_POPUP_MENU_COPY_SERVER_ID, MapCanvas::OnCopyServerId)
EVT_MENU(MAP_POPUP_MENU_COPY_CLIENT_ID, MapCanvas::OnCopyClientId)
EVT_MENU(MAP_POPUP_MENU_COPY_NAME, MapCanvas::OnCopyName)
// ----
EVT_MENU(MAP_POPUP_MENU_ROTATE, MapCanvas::OnRotateItem)
EVT_MENU(MAP_POPUP_MENU_GOTO, MapCanvas::OnGotoDestination)
EVT_MENU(MAP_POPUP_MENU_SWITCH_DOOR, MapCanvas::OnSwitchDoor)
// ----
EVT_MENU(MAP_POPUP_MENU_SELECT_RAW_BRUSH, MapCanvas::OnSelectRAWBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, MapCanvas::OnSelectGroundBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, MapCanvas::OnSelectDoodadBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, MapCanvas::OnSelectCollectionBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, MapCanvas::OnSelectDoorBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_WALL_BRUSH, MapCanvas::OnSelectWallBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, MapCanvas::OnSelectCarpetBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, MapCanvas::OnSelectTableBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, MapCanvas::OnSelectCreatureBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, MapCanvas::OnSelectSpawnBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, MapCanvas::OnSelectHouseBrush)
EVT_MENU(MAP_POPUP_MENU_MOVE_TO_TILESET, MapCanvas::OnSelectMoveTo)
// ----
EVT_MENU(MAP_POPUP_MENU_PROPERTIES, MapCanvas::OnProperties)
// ----
EVT_MENU(MAP_POPUP_MENU_BROWSE_TILE, MapCanvas::OnBrowseTile)
END_EVENT_TABLE()

bool MapCanvas::processed[] = { 0 };

MapCanvas::MapCanvas(MapWindow* parent, Editor& editor_, int* attriblist) :
	wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
	editor_(editor_),
	floor_(GROUND_LAYER),
	zoom_(1.0),
	cursor_x_(-1),
	cursor_y_(-1),
	dragging_(false),
	boundbox_selection_(false),
	screendragging_(false),
	drawing_(false),
	dragging_draw_(false),
	replace_dragging_(false),

	screenshot_buffer_(nullptr),

	drag_start_x_(-1),
	drag_start_y_(-1),
	drag_start_z_(-1),

	last_cursor_map_x_(-1),
	last_cursor_map_y_(-1),
	last_cursor_map_z_(-1),

	last_click_map_x_(-1),
	last_click_map_y_(-1),
	last_click_map_z_(-1),
	last_click_abs_x_(-1),
	last_click_abs_y_(-1),
	last_click_x_(-1),
	last_click_y_(-1),

	last_mmb_click_x_(-1),
	last_mmb_click_y_(-1) {

	// Legacy components - kept for transitional period
	popup_menu_ = newd MapPopupMenu(editor_);
	animation_timer_ = newd AnimationTimer(this);
	drawer_ = new MapDrawer(this);
	keyCode_ = WXK_NONE;

	// Initialize new rendering architecture
	initializeRenderingSystems();
}

MapCanvas::~MapCanvas() {
	// Shutdown new rendering architecture
	shutdownRenderingSystems();

	// Cleanup legacy components
	delete popup_menu_;
	delete animation_timer_;
	delete drawer_;
	free(screenshot_buffer_);
}

void MapCanvas::Refresh() {
	if (refresh_watch_.Time() > g_settings.getInteger(Config::HARD_REFRESH_RATE)) {
		refresh_watch_.Start();
		wxGLCanvas::Update();
	}
	wxGLCanvas::Refresh();
}

void MapCanvas::SetZoom(double value) {
	if (value < 0.125) {
		value = 0.125;
	}

	if (value > 25.00) {
		value = 25.0;
	}

	if (zoom_ != value) {
		int center_x, center_y;
		GetScreenCenter(&center_x, &center_y);

		zoom_ = value;
		static_cast<MapWindow*>(GetParent())->SetScreenCenterPosition(Position(center_x, center_y, floor_));

		UpdatePositionStatus();
		UpdateZoomStatus();
		Refresh();
	}
}

void MapCanvas::GetViewBox(int* view_scroll_x_, int* view_scroll_y_, int* screensize_x, int* screensize_y) const {
	static_cast<MapWindow*>(GetParent())->GetViewSize(screensize_x, screensize_y);
	static_cast<MapWindow*>(GetParent())->GetViewStart(view_scroll_x_, view_scroll_y_);
}

void MapCanvas::OnPaint(wxPaintEvent& event) {
	SetCurrent(*g_gui.GetGLContext(this));

	if (g_gui.IsRenderingEnabled()) {
		DrawingOptions& options = drawer_->getOptions();
		if (screenshot_buffer_) {
			options.SetIngame();
		} else {
			options.transparent_floors = g_settings.getBoolean(Config::TRANSPARENT_FLOORS);
			options.transparent_items = g_settings.getBoolean(Config::TRANSPARENT_ITEMS);
			options.show_ingame_box = g_settings.getBoolean(Config::SHOW_INGAME_BOX);
			options.show_lights = g_settings.getBoolean(Config::SHOW_LIGHTS);
			options.show_light_str = g_settings.getBoolean(Config::SHOW_LIGHT_STR);
			options.show_tech_items = g_settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS);
			options.show_waypoints = g_settings.getBoolean(Config::SHOW_WAYPOINTS);
			options.show_grid = g_settings.getInteger(Config::SHOW_GRID);
			options.ingame = !g_settings.getBoolean(Config::SHOW_EXTRA);
			options.show_all_floors = g_settings.getBoolean(Config::SHOW_ALL_FLOORS);
			options.show_creatures = g_settings.getBoolean(Config::SHOW_CREATURES);
			options.show_spawns = g_settings.getBoolean(Config::SHOW_SPAWNS);
			options.show_houses = g_settings.getBoolean(Config::SHOW_HOUSES);
			options.show_shade = g_settings.getBoolean(Config::SHOW_SHADE);
			options.show_special_tiles = g_settings.getBoolean(Config::SHOW_SPECIAL_TILES);
			options.show_items = g_settings.getBoolean(Config::SHOW_ITEMS);
			options.highlight_items = g_settings.getBoolean(Config::HIGHLIGHT_ITEMS);
			options.highlight_locked_doors = g_settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS);
			options.show_blocking = g_settings.getBoolean(Config::SHOW_BLOCKING);
			options.show_tooltips = g_settings.getBoolean(Config::SHOW_TOOLTIPS);
			options.show_as_minimap = g_settings.getBoolean(Config::SHOW_AS_MINIMAP);
			options.show_only_colors = g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
			options.show_only_modified = g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES);
			options.show_preview = g_settings.getBoolean(Config::SHOW_PREVIEW);
			options.show_hooks = g_settings.getBoolean(Config::SHOW_WALL_HOOKS);
			options.hide_items_when_zoomed = g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED);
			options.show_towns = g_settings.getBoolean(Config::SHOW_TOWNS);
			options.always_show_zones = g_settings.getBoolean(Config::ALWAYS_SHOW_ZONES);
			options.extended_house_shader = g_settings.getBoolean(Config::EXT_HOUSE_SHADER);

			options.experimental_fog = g_settings.getBoolean(Config::EXPERIMENTAL_FOG);
		}

		options.dragging = boundbox_selection_;

		if (options.show_preview) {
			animation_timer_->Start();
		} else {
			animation_timer_->Stop();
		}

		drawer_->SetupVars();
		drawer_->SetupGL();
		drawer_->Draw();

		if (screenshot_buffer_) {
			drawer_->TakeScreenshot(screenshot_buffer_);
		}

		drawer_->Release();
	}

	// Clean unused textures
	g_gui.gfx.garbageCollection();

	// Swap buffer
	SwapBuffers();

	// FPS tracking and limiting
	static auto last_frame_time = std::chrono::steady_clock::now();
	static int frame_count = 0;
	static int current_fps = 0;
	static auto last_fps_update = std::chrono::steady_clock::now();

	auto now = std::chrono::steady_clock::now();
	frame_count++;

	// Update FPS counter every second
	auto fps_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_update);
	if (fps_elapsed.count() >= 1000) {
		current_fps = frame_count;
		frame_count = 0;
		last_fps_update = now;
	}

	// FPS limiting
	int fps_limit = g_settings.getInteger(Config::FRAME_RATE_LIMIT);
	if (fps_limit > 0) {
		auto target_frame_duration = std::chrono::microseconds(1000000 / fps_limit);
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_frame_time);
		if (elapsed < target_frame_duration) {
			std::this_thread::sleep_for(target_frame_duration - elapsed);
		}
	}
	last_frame_time = std::chrono::steady_clock::now();

	// Display FPS on status bar if enabled
	if (g_settings.getBoolean(Config::SHOW_FPS_COUNTER)) {
		static int last_displayed_fps = -1;
		if (current_fps != last_displayed_fps) {
			wxString fps_text;
			fps_text.Printf("FPS: %d", current_fps);
			g_gui.root->SetStatusText(fps_text, 0);
			last_displayed_fps = current_fps;
		}
	}

	// Send newd node requests
	editor_.SendNodeRequests();
}

void MapCanvas::TakeScreenshot(wxFileName path, wxString format) {
	int screensize_x, screensize_y;
	GetViewBox(&view_scroll_x_, &view_scroll_y_, &screensize_x, &screensize_y);

	delete[] screenshot_buffer_;
	screenshot_buffer_ = newd uint8_t[3 * screensize_x * screensize_y];

	// Draw the window
	Refresh();
	wxGLCanvas::Update(); // Forces immediate redraws the window.

	// screenshot_buffer_ should now contain the screenbuffer
	if (screenshot_buffer_ == nullptr) {
		g_gui.PopupDialog("Capture failed", "Image capture failed. Old Video Driver?", wxOK);
	} else {
		// We got the shit
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);
		wxImage screenshot(screensize_x, screensize_y, screenshot_buffer_);

		time_t t = time(nullptr);
		struct tm* current_time = localtime(&t);
		ASSERT(current_time);

		wxString date;
		date << "screenshot_" << (1900 + current_time->tm_year);
		if (current_time->tm_mon < 9) {
			date << "-"
				 << "0" << current_time->tm_mon + 1;
		} else {
			date << "-" << current_time->tm_mon + 1;
		}
		date << "-" << current_time->tm_mday;
		date << "-" << current_time->tm_hour;
		date << "-" << current_time->tm_min;
		date << "-" << current_time->tm_sec;

		int type = 0;
		path.SetName(date);
		if (format == "bmp") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_BMP;
		} else if (format == "png") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_PNG;
		} else if (format == "jpg" || format == "jpeg") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_JPEG;
		} else if (format == "tga") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_TGA;
		} else {
			g_gui.SetStatusText("Unknown screenshot format \'" + format + "\", switching to default (png)");
			path.SetExt("png");
			;
			type = wxBITMAP_TYPE_PNG;
		}

		path.Mkdir(0755, wxPATH_MKDIR_FULL);
		wxFileOutputStream of(path.GetFullPath());
		if (of.IsOk()) {
			if (screenshot.SaveFile(of, static_cast<wxBitmapType>(type))) {
				g_gui.SetStatusText("Took screenshot and saved as " + path.GetFullName());
			} else {
				g_gui.PopupDialog("File error", "Couldn't save image file correctly.", wxOK);
			}
		} else {
			g_gui.PopupDialog("File error", "Couldn't open file " + path.GetFullPath() + " for writing.", wxOK);
		}
	}

	Refresh();

	screenshot_buffer_ = nullptr;
}

void MapCanvas::ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y) {
	int start_x, start_y;
	static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

	screen_x *= GetContentScaleFactor();
	screen_y *= GetContentScaleFactor();

	if (screen_x < 0) {
		*map_x = (start_x + screen_x) / TileSize;
	} else {
		*map_x = int(start_x + (screen_x * zoom_)) / TileSize;
	}

	if (screen_y < 0) {
		*map_y = (start_y + screen_y) / TileSize;
	} else {
		*map_y = int(start_y + (screen_y * zoom_)) / TileSize;
	}

	if (floor_ <= GROUND_LAYER) {
		*map_x += GROUND_LAYER - floor_;
		*map_y += GROUND_LAYER - floor_;
	} /* else {
		 *map_x += MAP_MAX_LAYER - floor_;
		 *map_y += MAP_MAX_LAYER - floor_;
	 }*/
}

void MapCanvas::GetScreenCenter(int* map_x, int* map_y) {
	int width, height;
	static_cast<MapWindow*>(GetParent())->GetViewSize(&width, &height);
	return ScreenToMap(width / 2, height / 2, map_x, map_y);
}

Position MapCanvas::GetCursorPosition() const {
	return Position(last_cursor_map_x_, last_cursor_map_y_, floor_);
}

void MapCanvas::UpdatePositionStatus(int x, int y) {
	if (x == -1) {
		x = cursor_x_;
	}
	if (y == -1) {
		y = cursor_y_;
	}

	int map_x, map_y;
	ScreenToMap(x, y, &map_x, &map_y);

	wxString ss;
	ss << "x: " << map_x << " y:" << map_y << " z:" << floor_;
	g_gui.root->SetStatusText(ss, 2);

	ss = "";
	Tile* tile = editor_.map.getTile(map_x, map_y, floor_);
	if (tile) {
		if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
			ss << "Spawn radius: " << tile->spawn->getSize();
		} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			ss << (tile->creature->isNpc() ? "NPC" : "Monster");
			ss << " \"" << wxstr(tile->creature->getName()) << "\" spawntime: " << tile->creature->getSpawnTime();
		} else if (Item* item = tile->getTopItem()) {
			ss << "Item \"" << wxstr(item->getName()) << "\"";
			ss << " id:" << item->getID();
			ss << " cid:" << item->getClientID();
			if (item->getUniqueID()) {
				ss << " uid:" << item->getUniqueID();
			}
			if (item->getActionID()) {
				ss << " aid:" << item->getActionID();
			}
			if (item->hasWeight()) {
				wxString s;
				s.Printf("%.2f", item->getWeight());
				ss << " weight: " << s;
			}
		} else {
			ss << "Nothing";
		}
	} else {
		ss << "Nothing";
	}

	if (editor_.IsLive()) {
		editor_.GetLive().updateCursor(Position(map_x, map_y, floor_));
	}

	g_gui.root->SetStatusText(ss, 1);
}

void MapCanvas::UpdateZoomStatus() {
	int percentage = (int)((1.0 / zoom_) * 100);
	wxString ss;
	ss << "zoom_: " << percentage << "%";
	g_gui.root->SetStatusText(ss, 3);
}

void MapCanvas::OnMouseMove(wxMouseEvent& event) {
	if (screendragging_) {
		static_cast<MapWindow*>(GetParent())->ScrollRelative(int(g_settings.getFloat(Config::SCROLL_SPEED) * zoom_ * (event.GetX() - cursor_x_)), int(g_settings.getFloat(Config::SCROLL_SPEED) * zoom_ * (event.GetY() - cursor_y_)));
		Refresh();
	}

	cursor_x_ = event.GetX();
	cursor_y_ = event.GetY();

	// Delegate to InputDispatcher
	inputDispatcher_.handleMouseMove(cursor_x_, cursor_y_, event.ControlDown(), event.AltDown(), event.ShiftDown());

	int mouse_map_x, mouse_map_y;
	MouseToMap(&mouse_map_x, &mouse_map_y);

	bool map_update = false;
	if (last_cursor_map_x_ != mouse_map_x || last_cursor_map_y_ != mouse_map_y || last_cursor_map_z_ != floor_) {
		map_update = true;
	}
	last_cursor_map_x_ = mouse_map_x;
	last_cursor_map_y_ = mouse_map_y;
	last_cursor_map_z_ = floor_;

	if (map_update) {
		UpdatePositionStatus(cursor_x_, cursor_y_);
		UpdateZoomStatus();
	}

	if (g_gui.IsSelectionMode()) {
		if (map_update && isPasting()) {
			Refresh();
		} else if (map_update && dragging_) {
			wxString ss;

			int move_x = drag_start_x_ - mouse_map_x;
			int move_y = drag_start_y_ - mouse_map_y;
			int move_z = drag_start_z_ - floor_;
			ss << "Dragging " << -move_x << "," << -move_y << "," << -move_z;
			g_gui.SetStatusText(ss);

			Refresh();
		} else if (boundbox_selection_) {
			if (map_update) {
				wxString ss;

				int move_x = std::abs(last_click_map_x_ - mouse_map_x);
				int move_y = std::abs(last_click_map_y_ - mouse_map_y);
				ss << "Selection " << move_x + 1 << ":" << move_y + 1;
				g_gui.SetStatusText(ss);
			}

			Refresh();
		}
	} else { // Drawing mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (map_update && drawing_ && brush) {
			if (brush->isDoodad()) {
				if (event.ControlDown()) {
					PositionVector tilestodraw;
					getTilesToDraw(mouse_map_x, mouse_map_y, floor_, &tilestodraw, nullptr);
					editor_.undraw(tilestodraw, event.ShiftDown() || event.AltDown());
				} else {
					editor_.draw(Position(mouse_map_x, mouse_map_y, floor_), event.ShiftDown() || event.AltDown());
				}
			} else if (brush->isDoor()) {
				if (!brush->canDraw(&editor_.map, Position(mouse_map_x, mouse_map_y, floor_))) {
					// We don't have to waste an action in this case...
				} else {
					PositionVector tilestodraw;
					PositionVector tilestoborder;

					tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor_));

					tilestoborder.push_back(Position(mouse_map_x, mouse_map_y - 1, floor_));
					tilestoborder.push_back(Position(mouse_map_x - 1, mouse_map_y, floor_));
					tilestoborder.push_back(Position(mouse_map_x, mouse_map_y + 1, floor_));
					tilestoborder.push_back(Position(mouse_map_x + 1, mouse_map_y, floor_));

					if (event.ControlDown()) {
						editor_.undraw(tilestodraw, tilestoborder, event.AltDown());
					} else {
						editor_.draw(tilestodraw, tilestoborder, event.AltDown());
					}
				}
			} else if (brush->needBorders()) {
				PositionVector tilestodraw, tilestoborder;

				getTilesToDraw(mouse_map_x, mouse_map_y, floor_, &tilestodraw, &tilestoborder);

				if (event.ControlDown()) {
					editor_.undraw(tilestodraw, tilestoborder, event.AltDown());
				} else {
					editor_.draw(tilestodraw, tilestoborder, event.AltDown());
				}
			} else if (brush->oneSizeFitsAll()) {
				drawing_ = true;
				PositionVector tilestodraw;
				tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor_));

				if (event.ControlDown()) {
					editor_.undraw(tilestodraw, event.AltDown());
				} else {
					editor_.draw(tilestodraw, event.AltDown());
				}
			} else { // No borders
				PositionVector tilestodraw;

				for (int y = -g_gui.GetBrushSize(); y <= g_gui.GetBrushSize(); y++) {
					for (int x = -g_gui.GetBrushSize(); x <= g_gui.GetBrushSize(); x++) {
						if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
							tilestodraw.push_back(Position(mouse_map_x + x, mouse_map_y + y, floor_));
						} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
							double distance = sqrt(double(x * x) + double(y * y));
							if (distance < g_gui.GetBrushSize() + 0.005) {
								tilestodraw.push_back(Position(mouse_map_x + x, mouse_map_y + y, floor_));
							}
						}
					}
				}
				if (event.ControlDown()) {
					editor_.undraw(tilestodraw, event.AltDown());
				} else {
					editor_.draw(tilestodraw, event.AltDown());
				}
			}

			// Create newd doodad layout (does nothing if a non-doodad brush is selected)
			g_gui.FillDoodadPreviewBuffer();

			g_gui.RefreshView();
		} else if (dragging_draw_) {
			g_gui.RefreshView();
		} else if (map_update && brush) {
			Refresh();
		}
	}
}

void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event) {
	inputDispatcher_.handleMouseUp(event.GetX(), event.GetY(), rme::input::MouseButton::Left, event.ControlDown(), event.AltDown(), event.ShiftDown());
	OnMouseActionRelease(event);
}

void MapCanvas::OnMouseLeftClick(wxMouseEvent& event) {
	inputDispatcher_.handleMouseDown(event.GetX(), event.GetY(), rme::input::MouseButton::Left, event.ControlDown(), event.AltDown(), event.ShiftDown());
	OnMouseActionClick(event);
}

void MapCanvas::OnMouseLeftDoubleClick(wxMouseEvent& event) {
	if (g_settings.getInteger(Config::DOUBLECLICK_PROPERTIES)) {
		int mouse_map_x, mouse_map_y;
		ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
		Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);

		if (tile && tile->size() > 0) {
			Tile* new_tile = tile->deepCopy(editor_.map);
			wxDialog* w = nullptr;
			if (new_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
				w = newd OldPropertiesWindow(g_gui.root, &editor_.map, new_tile, new_tile->spawn);
			} else if (new_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
				w = newd OldPropertiesWindow(g_gui.root, &editor_.map, new_tile, new_tile->creature);
			} else if (Item* item = new_tile->getTopItem()) {
				if (editor_.map.getVersion().otbm >= MAP_OTBM_4) {
					w = newd PropertiesWindow(g_gui.root, &editor_.map, new_tile, item);
				} else {
					w = newd OldPropertiesWindow(g_gui.root, &editor_.map, new_tile, item);
				}
			} else {
				return;
			}

			int ret = w->ShowModal();
			if (ret != 0) {
				Action* action = editor_.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
				action->addChange(newd Change(new_tile));
				editor_.addAction(action);
			} else {
				// Cancel!
				delete new_tile;
			}
			w->Destroy();
		}
	}
}

void MapCanvas::OnMouseCenterClick(wxMouseEvent& event) {
	inputDispatcher_.handleMouseDown(event.GetX(), event.GetY(), rme::input::MouseButton::Middle, event.ControlDown(), event.AltDown(), event.ShiftDown());
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMousePropertiesClick(event);
	} else {
		OnMouseCameraClick(event);
	}
}

void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event) {
	inputDispatcher_.handleMouseUp(event.GetX(), event.GetY(), rme::input::MouseButton::Middle, event.ControlDown(), event.AltDown(), event.ShiftDown());
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMousePropertiesRelease(event);
	} else {
		OnMouseCameraRelease(event);
	}
}

void MapCanvas::OnMouseRightClick(wxMouseEvent& event) {
	inputDispatcher_.handleMouseDown(event.GetX(), event.GetY(), rme::input::MouseButton::Right, event.ControlDown(), event.AltDown(), event.ShiftDown());
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMouseCameraClick(event);
	} else {
		OnMousePropertiesClick(event);
	}
}

void MapCanvas::OnMouseRightRelease(wxMouseEvent& event) {
	inputDispatcher_.handleMouseUp(event.GetX(), event.GetY(), rme::input::MouseButton::Right, event.ControlDown(), event.AltDown(), event.ShiftDown());
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMouseCameraRelease(event);
	} else {
		OnMousePropertiesRelease(event);
	}
}

void MapCanvas::OnMouseActionClick(wxMouseEvent& event) {
	SetFocus();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	if (event.ControlDown() && event.AltDown()) {
		Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
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
		if (isPasting()) {
			// Set paste to false (no rendering etc.)
			EndPasting();

			// Paste to the map
			editor_.copybuffer.paste(editor_, Position(mouse_map_x, mouse_map_y, floor_));

			// Start dragging_
			dragging_ = true;
			drag_start_x_ = mouse_map_x;
			drag_start_y_ = mouse_map_y;
			drag_start_z_ = floor_;
		} else {
			do {
				boundbox_selection_ = false;
				if (event.ShiftDown()) {
					boundbox_selection_ = true;

					if (!event.ControlDown()) {
						editor_.selection.start(); // Start selection session
						editor_.selection.clear(); // Clear out selection
						editor_.selection.finish(); // End selection session
						editor_.selection.updateSelectionCount();
					}
				} else if (event.ControlDown()) {
					Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
					if (tile) {
						if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
							editor_.selection.start(); // Start selection session
							if (tile->spawn->isSelected()) {
								editor_.selection.remove(tile, tile->spawn);
							} else {
								editor_.selection.add(tile, tile->spawn);
							}
							editor_.selection.finish(); // Finish selection session
							editor_.selection.updateSelectionCount();
						} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
							editor_.selection.start(); // Start selection session
							if (tile->creature->isSelected()) {
								editor_.selection.remove(tile, tile->creature);
							} else {
								editor_.selection.add(tile, tile->creature);
							}
							editor_.selection.finish(); // Finish selection session
							editor_.selection.updateSelectionCount();
						} else {
							Item* item = tile->getTopItem();
							if (item) {
								editor_.selection.start(); // Start selection session
								if (item->isSelected()) {
									editor_.selection.remove(tile, item);
								} else {
									editor_.selection.add(tile, item);
								}
								editor_.selection.finish(); // Finish selection session
								editor_.selection.updateSelectionCount();
							}
						}
					}
				} else {
					Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
					if (!tile) {
						editor_.selection.start(); // Start selection session
						editor_.selection.clear(); // Clear out selection
						editor_.selection.finish(); // End selection session
						editor_.selection.updateSelectionCount();
					} else if (tile->isSelected()) {
						dragging_ = true;
						drag_start_x_ = mouse_map_x;
						drag_start_y_ = mouse_map_y;
						drag_start_z_ = floor_;
					} else {
						editor_.selection.start(); // Start a selection session
						editor_.selection.clear();
						editor_.selection.commit();
						if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
							editor_.selection.add(tile, tile->spawn);
							dragging_ = true;
							drag_start_x_ = mouse_map_x;
							drag_start_y_ = mouse_map_y;
							drag_start_z_ = floor_;
						} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
							editor_.selection.add(tile, tile->creature);
							dragging_ = true;
							drag_start_x_ = mouse_map_x;
							drag_start_y_ = mouse_map_y;
							drag_start_z_ = floor_;
						} else {
							Item* item = tile->getTopItem();
							if (item) {
								editor_.selection.add(tile, item);
								dragging_ = true;
								drag_start_x_ = mouse_map_x;
								drag_start_y_ = mouse_map_y;
								drag_start_z_ = floor_;
							}
						}
						editor_.selection.finish(); // Finish the selection session
						editor_.selection.updateSelectionCount();
					}
				}
			} while (false);
		}
	} else if (g_gui.GetCurrentBrush()) { // Drawing mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (event.ShiftDown() && brush->canDrag()) {
			dragging_draw_ = true;
		} else {
			if (g_gui.GetBrushSize() == 0 && !brush->oneSizeFitsAll()) {
				drawing_ = true;
			} else {
				drawing_ = g_gui.GetCurrentBrush()->canSmear();
			}
			if (brush->isWall()) {
				if (event.AltDown() && g_gui.GetBrushSize() == 0) {
					// z0mg, just clicked a tile, shift variaton.
					if (event.ControlDown()) {
						editor_.undraw(Position(mouse_map_x, mouse_map_y, floor_), event.AltDown());
					} else {
						editor_.draw(Position(mouse_map_x, mouse_map_y, floor_), event.AltDown());
					}
				} else {
					PositionVector tilestodraw;
					PositionVector tilestoborder;

					int start_map_x = mouse_map_x - g_gui.GetBrushSize();
					int start_map_y = mouse_map_y - g_gui.GetBrushSize();
					int end_map_x = mouse_map_x + g_gui.GetBrushSize();
					int end_map_y = mouse_map_y + g_gui.GetBrushSize();

					for (int y = start_map_y - 1; y <= end_map_y + 1; ++y) {
						for (int x = start_map_x - 1; x <= end_map_x + 1; ++x) {
							if ((x <= start_map_x + 1 || x >= end_map_x - 1) || (y <= start_map_y + 1 || y >= end_map_y - 1)) {
								tilestoborder.push_back(Position(x, y, floor_));
							}
							if (((x == start_map_x || x == end_map_x) || (y == start_map_y || y == end_map_y)) && ((x >= start_map_x && x <= end_map_x) && (y >= start_map_y && y <= end_map_y))) {
								tilestodraw.push_back(Position(x, y, floor_));
							}
						}
					}
					if (event.ControlDown()) {
						editor_.undraw(tilestodraw, tilestoborder, event.AltDown());
					} else {
						editor_.draw(tilestodraw, tilestoborder, event.AltDown());
					}
				}
			} else if (brush->isDoor()) {
				PositionVector tilestodraw;
				PositionVector tilestoborder;

				tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor_));

				tilestoborder.push_back(Position(mouse_map_x, mouse_map_y - 1, floor_));
				tilestoborder.push_back(Position(mouse_map_x - 1, mouse_map_y, floor_));
				tilestoborder.push_back(Position(mouse_map_x, mouse_map_y + 1, floor_));
				tilestoborder.push_back(Position(mouse_map_x + 1, mouse_map_y, floor_));

				if (event.ControlDown()) {
					editor_.undraw(tilestodraw, tilestoborder, event.AltDown());
				} else {
					editor_.draw(tilestodraw, tilestoborder, event.AltDown());
				}
			} else if (brush->isDoodad() || brush->isSpawn() || brush->isCreature()) {
				if (event.ControlDown()) {
					if (brush->isDoodad()) {
						PositionVector tilestodraw;
						getTilesToDraw(mouse_map_x, mouse_map_y, floor_, &tilestodraw, nullptr);
						editor_.undraw(tilestodraw, event.AltDown());
					} else {
						editor_.undraw(Position(mouse_map_x, mouse_map_y, floor_), event.ShiftDown() || event.AltDown());
					}
				} else {
					bool will_show_spawn = false;
					if (brush->isSpawn() || brush->isCreature()) {
						if (!g_settings.getBoolean(Config::SHOW_SPAWNS)) {
							Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
							if (!tile || !tile->spawn) {
								will_show_spawn = true;
							}
						}
					}

					editor_.draw(Position(mouse_map_x, mouse_map_y, floor_), event.ShiftDown() || event.AltDown());

					if (will_show_spawn) {
						Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
						if (tile && tile->spawn) {
							g_settings.setInteger(Config::SHOW_SPAWNS, true);
							g_gui.UpdateMenubar();
						}
					}
				}
			} else {
				if (brush->isGround() && event.AltDown()) {
					replace_dragging_ = true;
					Tile* draw_tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
					if (draw_tile) {
						editor_.replace_brush = draw_tile->getGroundBrush();
					} else {
						editor_.replace_brush = nullptr;
					}
				}

				if (brush->needBorders()) {
					PositionVector tilestodraw;
					PositionVector tilestoborder;

					bool fill = keyCode_ == WXK_CONTROL_D && event.ControlDown() && brush->isGround();
					getTilesToDraw(mouse_map_x, mouse_map_y, floor_, &tilestodraw, &tilestoborder, fill);

					if (!fill && event.ControlDown()) {
						editor_.undraw(tilestodraw, tilestoborder, event.AltDown());
					} else {
						editor_.draw(tilestodraw, tilestoborder, event.AltDown());
					}
				} else if (brush->oneSizeFitsAll()) {
					if (brush->isHouseExit() || brush->isWaypoint()) {
						editor_.draw(Position(mouse_map_x, mouse_map_y, floor_), event.AltDown());
					} else {
						PositionVector tilestodraw;
						tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor_));
						if (event.ControlDown()) {
							editor_.undraw(tilestodraw, event.AltDown());
						} else {
							editor_.draw(tilestodraw, event.AltDown());
						}
					}
				} else {
					PositionVector tilestodraw;

					getTilesToDraw(mouse_map_x, mouse_map_y, floor_, &tilestodraw, nullptr);

					if (event.ControlDown()) {
						editor_.undraw(tilestodraw, event.AltDown());
					} else {
						editor_.draw(tilestodraw, event.AltDown());
					}
				}
			}
			// Change the doodad layout brush
			g_gui.FillDoodadPreviewBuffer();
		}
	}
	last_click_x_ = int(event.GetX() * zoom_);
	last_click_y_ = int(event.GetY() * zoom_);

	int start_x, start_y;
	static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);
	last_click_abs_x_ = last_click_x_ + start_x;
	last_click_abs_y_ = last_click_y_ + start_y;

	last_click_map_x_ = mouse_map_x;
	last_click_map_y_ = mouse_map_y;
	last_click_map_z_ = floor_;
	g_gui.RefreshView();
	g_gui.UpdateMinimap();
}

void MapCanvas::OnMouseActionRelease(wxMouseEvent& event) {
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	int move_x = last_click_map_x_ - mouse_map_x;
	int move_y = last_click_map_y_ - mouse_map_y;
	int move_z = last_click_map_z_ - floor_;

	if (g_gui.IsSelectionMode()) {
		if (dragging_ && (move_x != 0 || move_y != 0 || move_z != 0)) {
			editor_.moveSelection(Position(move_x, move_y, move_z));
		} else {
			if (boundbox_selection_) {
				if (mouse_map_x == last_click_map_x_ && mouse_map_y == last_click_map_y_ && event.ControlDown()) {
					// Mouse hasn't moved, do control+shift thingy!
					Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
					if (tile) {
						editor_.selection.start(); // Start a selection session
						if (tile->isSelected()) {
							editor_.selection.remove(tile);
						} else {
							editor_.selection.add(tile);
						}
						editor_.selection.finish(); // Finish the selection session
						editor_.selection.updateSelectionCount();
					}
				} else {
					// The cursor has moved, do some boundboxing!
					if (last_click_map_x_ > mouse_map_x) {
						int tmp = mouse_map_x;
						mouse_map_x = last_click_map_x_;
						last_click_map_x_ = tmp;
					}
					if (last_click_map_y_ > mouse_map_y) {
						int tmp = mouse_map_y;
						mouse_map_y = last_click_map_y_;
						last_click_map_y_ = tmp;
					}

					int numtiles = 0;
					int threadcount = std::max(g_settings.getInteger(Config::WORKER_THREADS), 1);

					int start_x = 0, start_y = 0, start_z = 0;
					int end_x = 0, end_y = 0, end_z = 0;

					switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
						case SELECT_CURRENT_FLOOR: {
							start_z = end_z = floor_;
							start_x = last_click_map_x_;
							start_y = last_click_map_y_;
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							break;
						}
						case SELECT_ALL_FLOORS: {
							start_x = last_click_map_x_;
							start_y = last_click_map_y_;
							start_z = MAP_MAX_LAYER;
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							end_z = floor_;

							if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
								start_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
								start_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);

								end_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
								end_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
							}

							numtiles = (start_z - end_z) * (end_x - start_x) * (end_y - start_y);
							break;
						}
						case SELECT_VISIBLE_FLOORS: {
							start_x = last_click_map_x_;
							start_y = last_click_map_y_;
							if (floor_ <= GROUND_LAYER) {
								start_z = GROUND_LAYER;
							} else {
								start_z = std::min(MAP_MAX_LAYER, floor_ + 2);
							}
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							end_z = floor_;

							if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
								start_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
								start_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);

								end_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
								end_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
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
					int width = end_x - start_x;
					if (width < threadcount) {
						threadcount = min(1, width);
					}
					// Let's divide!
					int remainder = width;
					int cleared = 0;
					std::vector<SelectionThread*> threads;
					if (width == 0) {
						threads.push_back(newd SelectionThread(editor_, Position(start_x, start_y, start_z), Position(start_x, end_y, end_z)));
					} else {
						for (int i = 0; i < threadcount; ++i) {
							int chunksize = width / threadcount;
							// The last threads takes all the remainder
							if (i == threadcount - 1) {
								chunksize = remainder;
							}
							threads.push_back(newd SelectionThread(editor_, Position(start_x + cleared, start_y, start_z), Position(start_x + cleared + chunksize, end_y, end_z)));
							cleared += chunksize;
							remainder -= chunksize;
						}
					}
					ASSERT(cleared == width);
					ASSERT(remainder == 0);

					editor_.selection.start(); // Start a selection session
					for (std::vector<SelectionThread*>::iterator iter = threads.begin(); iter != threads.end(); ++iter) {
						(*iter)->Execute();
					}
					for (std::vector<SelectionThread*>::iterator iter = threads.begin(); iter != threads.end(); ++iter) {
						editor_.selection.join(*iter);
					}
					editor_.selection.finish(); // Finish the selection session
					editor_.selection.updateSelectionCount();
				}
			} else if (event.ControlDown()) {
				////
			} else {
				// User hasn't moved anything, meaning selection/deselection
				Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
				if (tile) {
					if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
						if (!tile->spawn->isSelected()) {
							editor_.selection.start(); // Start a selection session
							editor_.selection.add(tile, tile->spawn);
							editor_.selection.finish(); // Finish the selection session
							editor_.selection.updateSelectionCount();
						}
					} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						if (!tile->creature->isSelected()) {
							editor_.selection.start(); // Start a selection session
							editor_.selection.add(tile, tile->creature);
							editor_.selection.finish(); // Finish the selection session
							editor_.selection.updateSelectionCount();
						}
					} else {
						Item* item = tile->getTopItem();
						if (item && !item->isSelected()) {
							editor_.selection.start(); // Start a selection session
							editor_.selection.add(tile, item);
							editor_.selection.finish(); // Finish the selection session
							editor_.selection.updateSelectionCount();
						}
					}
				}
			}
		}
		editor_.actionQueue->resetTimer();
		dragging_ = false;
		boundbox_selection_ = false;
	} else if (g_gui.GetCurrentBrush()) { // Drawing mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (dragging_draw_) {
			if (brush->isSpawn()) {
				int start_map_x = std::min(last_click_map_x_, mouse_map_x);
				int start_map_y = std::min(last_click_map_y_, mouse_map_y);
				int end_map_x = std::max(last_click_map_x_, mouse_map_x);
				int end_map_y = std::max(last_click_map_y_, mouse_map_y);

				int map_x = start_map_x + (end_map_x - start_map_x) / 2;
				int map_y = start_map_y + (end_map_y - start_map_y) / 2;

				int width = min(g_settings.getInteger(Config::MAX_SPAWN_RADIUS), ((end_map_x - start_map_x) / 2 + (end_map_y - start_map_y) / 2) / 2);
				int old = g_gui.GetBrushSize();
				g_gui.SetBrushSize(width);
				editor_.draw(Position(map_x, map_y, floor_), event.AltDown());
				g_gui.SetBrushSize(old);
			} else {
				PositionVector tilestodraw;
				PositionVector tilestoborder;
				if (brush->isWall()) {
					int start_map_x = std::min(last_click_map_x_, mouse_map_x);
					int start_map_y = std::min(last_click_map_y_, mouse_map_y);
					int end_map_x = std::max(last_click_map_x_, mouse_map_x);
					int end_map_y = std::max(last_click_map_y_, mouse_map_y);

					for (int y = start_map_y - 1; y <= end_map_y + 1; y++) {
						for (int x = start_map_x - 1; x <= end_map_x + 1; x++) {
							if ((x <= start_map_x + 1 || x >= end_map_x - 1) || (y <= start_map_y + 1 || y >= end_map_y - 1)) {
								tilestoborder.push_back(Position(x, y, floor_));
							}
							if (((x == start_map_x || x == end_map_x) || (y == start_map_y || y == end_map_y)) && ((x >= start_map_x && x <= end_map_x) && (y >= start_map_y && y <= end_map_y))) {
								tilestodraw.push_back(Position(x, y, floor_));
							}
						}
					}
				} else {
					if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
						if (last_click_map_x_ > mouse_map_x) {
							int tmp = mouse_map_x;
							mouse_map_x = last_click_map_x_;
							last_click_map_x_ = tmp;
						}
						if (last_click_map_y_ > mouse_map_y) {
							int tmp = mouse_map_y;
							mouse_map_y = last_click_map_y_;
							last_click_map_y_ = tmp;
						}

						for (int x = last_click_map_x_ - 1; x <= mouse_map_x + 1; x++) {
							for (int y = last_click_map_y_ - 1; y <= mouse_map_y + 1; y++) {
								if ((x <= last_click_map_x_ || x >= mouse_map_x) || (y <= last_click_map_y_ || y >= mouse_map_y)) {
									tilestoborder.push_back(Position(x, y, floor_));
								}
								if ((x >= last_click_map_x_ && x <= mouse_map_x) && (y >= last_click_map_y_ && y <= mouse_map_y)) {
									tilestodraw.push_back(Position(x, y, floor_));
								}
							}
						}
					} else {
						int start_x, end_x;
						int start_y, end_y;
						int width = std::max(
							std::abs(
								std::max(mouse_map_y, last_click_map_y_) - std::min(mouse_map_y, last_click_map_y_)
							),
							std::abs(
								std::max(mouse_map_x, last_click_map_x_) - std::min(mouse_map_x, last_click_map_x_)
							)
						);
						if (mouse_map_x < last_click_map_x_) {
							start_x = last_click_map_x_ - width;
							end_x = last_click_map_x_;
						} else {
							start_x = last_click_map_x_;
							end_x = last_click_map_x_ + width;
						}
						if (mouse_map_y < last_click_map_y_) {
							start_y = last_click_map_y_ - width;
							end_y = last_click_map_y_;
						} else {
							start_y = last_click_map_y_;
							end_y = last_click_map_y_ + width;
						}

						int center_x = start_x + (end_x - start_x) / 2;
						int center_y = start_y + (end_y - start_y) / 2;
						float radii = width / 2.0f + 0.005f;

						for (int y = start_y - 1; y <= end_y + 1; y++) {
							float dy = center_y - y;
							for (int x = start_x - 1; x <= end_x + 1; x++) {
								float dx = center_x - x;
								// printf("%f;%f\n", dx, dy);
								float distance = sqrt(dx * dx + dy * dy);
								if (distance < radii) {
									tilestodraw.push_back(Position(x, y, floor_));
								}
								if (std::abs(distance - radii) < 1.5) {
									tilestoborder.push_back(Position(x, y, floor_));
								}
							}
						}
					}
				}
				if (event.ControlDown()) {
					editor_.undraw(tilestodraw, tilestoborder, event.AltDown());
				} else {
					editor_.draw(tilestodraw, tilestoborder, event.AltDown());
				}
			}
		}
		editor_.actionQueue->resetTimer();
		drawing_ = false;
		dragging_draw_ = false;
		replace_dragging_ = false;
		editor_.replace_brush = nullptr;
	}
	g_gui.RefreshView();
	g_gui.UpdateMinimap();
}

void MapCanvas::OnMouseCameraClick(wxMouseEvent& event) {
	SetFocus();

	last_mmb_click_x_ = event.GetX();
	last_mmb_click_y_ = event.GetY();
	if (event.ControlDown()) {
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

		static_cast<MapWindow*>(GetParent())->ScrollRelative(int(-screensize_x * (1.0 - zoom_) * (std::max(cursor_x_, 1) / double(screensize_x))), int(-screensize_y * (1.0 - zoom_) * (std::max(cursor_y_, 1) / double(screensize_y))));
		zoom_ = 1.0;
		Refresh();
	} else {
		screendragging_ = true;
	}
}

void MapCanvas::OnMouseCameraRelease(wxMouseEvent& event) {
	SetFocus();
	screendragging_ = false;
	if (event.ControlDown()) {
		// ...
		// Haven't moved much, it's a click!
	} else if (last_mmb_click_x_ > event.GetX() - 3 && last_mmb_click_x_ < event.GetX() + 3 && last_mmb_click_y_ > event.GetY() - 3 && last_mmb_click_y_ < event.GetY() + 3) {
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);
		static_cast<MapWindow*>(GetParent())->ScrollRelative(int(zoom_ * (2 * cursor_x_ - screensize_x)), int(zoom_ * (2 * cursor_y_ - screensize_y)));
		Refresh();
	}
}

void MapCanvas::OnMousePropertiesClick(wxMouseEvent& event) {
	SetFocus();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
	Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);

	if (g_gui.IsDrawingMode()) {
		g_gui.SetSelectionMode();
	}

	EndPasting();

	boundbox_selection_ = false;
	if (event.ShiftDown()) {
		boundbox_selection_ = true;

		if (!event.ControlDown()) {
			editor_.selection.start(); // Start selection session
			editor_.selection.clear(); // Clear out selection
			editor_.selection.finish(); // End selection session
			editor_.selection.updateSelectionCount();
		}
	} else if (!tile) {
		editor_.selection.start(); // Start selection session
		editor_.selection.clear(); // Clear out selection
		editor_.selection.finish(); // End selection session
		editor_.selection.updateSelectionCount();
	} else if (tile->isSelected()) {
		// Do nothing!
	} else {
		editor_.selection.start(); // Start a selection session
		editor_.selection.clear();
		editor_.selection.commit();
		if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
			editor_.selection.add(tile, tile->spawn);
		} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			editor_.selection.add(tile, tile->creature);
		} else {
			Item* item = tile->getTopItem();
			if (item) {
				editor_.selection.add(tile, item);
			}
		}
		editor_.selection.finish(); // Finish the selection session
		editor_.selection.updateSelectionCount();
	}

	last_click_x_ = int(event.GetX() * zoom_);
	last_click_y_ = int(event.GetY() * zoom_);

	int start_x, start_y;
	static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);
	last_click_abs_x_ = last_click_x_ + start_x;
	last_click_abs_y_ = last_click_y_ + start_y;

	last_click_map_x_ = mouse_map_x;
	last_click_map_y_ = mouse_map_y;
	g_gui.RefreshView();
}

void MapCanvas::OnMousePropertiesRelease(wxMouseEvent& event) {
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	if (g_gui.IsDrawingMode()) {
		g_gui.SetSelectionMode();
	}

	if (boundbox_selection_) {
		if (mouse_map_x == last_click_map_x_ && mouse_map_y == last_click_map_y_ && event.ControlDown()) {
			// Mouse hasn't move, do control+shift thingy!
			Tile* tile = editor_.map.getTile(mouse_map_x, mouse_map_y, floor_);
			if (tile) {
				editor_.selection.start(); // Start a selection session
				if (tile->isSelected()) {
					editor_.selection.remove(tile);
				} else {
					editor_.selection.add(tile);
				}
				editor_.selection.finish(); // Finish the selection session
				editor_.selection.updateSelectionCount();
			}
		} else {
			// The cursor has moved, do some boundboxing!
			if (last_click_map_x_ > mouse_map_x) {
				int tmp = mouse_map_x;
				mouse_map_x = last_click_map_x_;
				last_click_map_x_ = tmp;
			}
			if (last_click_map_y_ > mouse_map_y) {
				int tmp = mouse_map_y;
				mouse_map_y = last_click_map_y_;
				last_click_map_y_ = tmp;
			}

			editor_.selection.start(); // Start a selection session
			switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
				case SELECT_CURRENT_FLOOR: {
					for (int x = last_click_map_x_; x <= mouse_map_x; x++) {
						for (int y = last_click_map_y_; y <= mouse_map_y; y++) {
							Tile* tile = editor_.map.getTile(x, y, floor_);
							if (!tile) {
								continue;
							}
							editor_.selection.add(tile);
						}
					}
					break;
				}
				case SELECT_ALL_FLOORS: {
					int start_x, start_y, start_z;
					int end_x, end_y, end_z;

					start_x = last_click_map_x_;
					start_y = last_click_map_y_;
					start_z = MAP_MAX_LAYER;
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					end_z = floor_;

					if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
						start_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
						start_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);

						end_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
						end_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
					}

					for (int z = start_z; z >= end_z; z--) {
						for (int x = start_x; x <= end_x; x++) {
							for (int y = start_y; y <= end_y; y++) {
								Tile* tile = editor_.map.getTile(x, y, z);
								if (!tile) {
									continue;
								}
								editor_.selection.add(tile);
							}
						}
						if (z <= GROUND_LAYER && g_settings.getInteger(Config::COMPENSATED_SELECT)) {
							start_x++;
							start_y++;
							end_x++;
							end_y++;
						}
					}
					break;
				}
				case SELECT_VISIBLE_FLOORS: {
					int start_x, start_y, start_z;
					int end_x, end_y, end_z;

					start_x = last_click_map_x_;
					start_y = last_click_map_y_;
					if (floor_ <= GROUND_LAYER) {
						start_z = GROUND_LAYER;
					} else {
						start_z = std::min(MAP_MAX_LAYER, floor_ + 2);
					}
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					end_z = floor_;

					if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
						start_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
						start_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);

						end_x -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
						end_y -= (floor_ < GROUND_LAYER ? GROUND_LAYER - floor_ : 0);
					}

					for (int z = start_z; z >= end_z; z--) {
						for (int x = start_x; x <= end_x; x++) {
							for (int y = start_y; y <= end_y; y++) {
								Tile* tile = editor_.map.getTile(x, y, z);
								if (!tile) {
									continue;
								}
								editor_.selection.add(tile);
							}
						}
						if (z <= GROUND_LAYER && g_settings.getInteger(Config::COMPENSATED_SELECT)) {
							start_x++;
							start_y++;
							end_x++;
							end_y++;
						}
					}
					break;
				}
			}
			editor_.selection.finish(); // Finish the selection session
			editor_.selection.updateSelectionCount();
		}
	} else if (event.ControlDown()) {
		// Nothing
	}

	popup_menu_->Update();
	PopupMenu(popup_menu_);

	editor_.actionQueue->resetTimer();
	dragging_ = false;
	boundbox_selection_ = false;

	last_cursor_map_x_ = mouse_map_x;
	last_cursor_map_y_ = mouse_map_y;
	last_cursor_map_z_ = floor_;

	g_gui.RefreshView();
}

void MapCanvas::OnWheel(wxMouseEvent& event) {
	inputDispatcher_.handleMouseWheel(event.GetX(), event.GetY(), event.GetWheelRotation(), event.ControlDown(), event.AltDown(), event.ShiftDown());

	if (event.ControlDown()) {
		static double diff = 0.0;
		diff += event.GetWheelRotation();
		if (diff <= 1.0 || diff >= 1.0) {
			if (diff < 0.0) {
				g_gui.ChangeFloor(floor_ - 1);
			} else {
				g_gui.ChangeFloor(floor_ + 1);
			}
			diff = 0.0;
		}
		UpdatePositionStatus();
	} else if (event.AltDown()) {
		static double diff = 0.0;
		diff += event.GetWheelRotation();
		if (diff <= 1.0 || diff >= 1.0) {
			if (diff < 0.0) {
				g_gui.IncreaseBrushSize();
			} else {
				g_gui.DecreaseBrushSize();
			}
			diff = 0.0;
		}
	} else {
		double diff = -event.GetWheelRotation() * g_settings.getFloat(Config::ZOOM_SPEED) / 640.0;
		double oldzoom = zoom_;
		zoom_ += diff;

		if (zoom_ < 0.125) {
			diff = 0.125 - oldzoom;
			zoom_ = 0.125;
		}
		if (zoom_ > 25.00) {
			diff = 25.00 - oldzoom;
			zoom_ = 25.0;
		}

		UpdateZoomStatus();

		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

		// This took a day to figure out!
		int scroll_x = int(screensize_x * diff * (std::max(cursor_x_, 1) / double(screensize_x))) * GetContentScaleFactor();
		int scroll_y = int(screensize_y * diff * (std::max(cursor_y_, 1) / double(screensize_y))) * GetContentScaleFactor();

		static_cast<MapWindow*>(GetParent())->ScrollRelative(-scroll_x, -scroll_y);
	}

	Refresh();
}

void MapCanvas::OnLoseMouse(wxMouseEvent& event) {
	Refresh();
}

void MapCanvas::OnGainMouse(wxMouseEvent& event) {
	if (!event.LeftIsDown()) {
		dragging_ = false;
		boundbox_selection_ = false;
		drawing_ = false;
	}
	if (!event.MiddleIsDown()) {
		screendragging_ = false;
	}

	Refresh();
}

void MapCanvas::OnKeyDown(wxKeyEvent& event) {
	// char keycode = event.GetKeyCode();
	//  std::cout << "Keycode " << keycode << std::endl;
	switch (event.GetKeyCode()) {
		case WXK_NUMPAD_ADD:
		case WXK_PAGEUP: {
			g_gui.ChangeFloor(floor_ - 1);
			break;
		}
		case WXK_NUMPAD_SUBTRACT:
		case WXK_PAGEDOWN: {
			g_gui.ChangeFloor(floor_ + 1);
			break;
		}
		case WXK_NUMPAD_MULTIPLY: {
			double diff = -0.3;

			double oldzoom = zoom_;
			zoom_ += diff;

			if (zoom_ < 0.125) {
				diff = 0.125 - oldzoom;
				zoom_ = 0.125;
			}

			int screensize_x, screensize_y;
			static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

			// This took a day to figure out!
			int scroll_x = int(screensize_x * diff * (std::max(cursor_x_, 1) / double(screensize_x)));
			int scroll_y = int(screensize_y * diff * (std::max(cursor_y_, 1) / double(screensize_y)));

			static_cast<MapWindow*>(GetParent())->ScrollRelative(-scroll_x, -scroll_y);

			UpdatePositionStatus();
			UpdateZoomStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_DIVIDE: {
			double diff = 0.3;
			double oldzoom = zoom_;
			zoom_ += diff;

			if (zoom_ > 25.00) {
				diff = 25.00 - oldzoom;
				zoom_ = 25.0;
			}

			int screensize_x, screensize_y;
			static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

			// This took a day to figure out!
			int scroll_x = int(screensize_x * diff * (std::max(cursor_x_, 1) / double(screensize_x)));
			int scroll_y = int(screensize_y * diff * (std::max(cursor_y_, 1) / double(screensize_y)));

			static_cast<MapWindow*>(GetParent())->ScrollRelative(-scroll_x, -scroll_y);

			UpdatePositionStatus();
			UpdateZoomStatus();
			Refresh();
			break;
		}
		// This will work like crap with non-us layouts, well, sucks for them until there is another solution.
		case '[':
		case '+': {
			g_gui.IncreaseBrushSize();
			Refresh();
			break;
		}
		case ']':
		case '-': {
			g_gui.DecreaseBrushSize();
			Refresh();
			break;
		}
		case WXK_NUMPAD_UP:
		case WXK_UP: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom_ == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(start_x, int(start_y - TileSize * tiles * zoom_));
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_DOWN:
		case WXK_DOWN: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom_ == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(start_x, int(start_y + TileSize * tiles * zoom_));
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_LEFT:
		case WXK_LEFT: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom_ == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(int(start_x - TileSize * tiles * zoom_), start_y);
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_RIGHT:
		case WXK_RIGHT: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom_ == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(int(start_x + TileSize * tiles * zoom_), start_y);
			UpdatePositionStatus();
			Refresh();
			break;
		}
		case WXK_SPACE: { // Utility keys
			if (event.ControlDown()) {
				g_gui.FillDoodadPreviewBuffer();
				g_gui.RefreshView();
			} else {
				g_gui.SwitchMode();
			}
			break;
		}
		case WXK_TAB: { // Tab switch
			if (event.ShiftDown()) {
				g_gui.CycleTab(false);
			} else {
				g_gui.CycleTab(true);
			}
			break;
		}
		case WXK_DELETE: { // Delete
			editor_.destroySelection();
			g_gui.RefreshView();
			break;
		}
		case 'z':
		case 'Z': { // Rotate counterclockwise (actually shift variaton, but whatever... :P)
			int nv = g_gui.GetBrushVariation();
			--nv;
			if (nv < 0) {
				nv = max(0, (g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getMaxVariation() - 1 : 0));
			}
			g_gui.SetBrushVariation(nv);
			g_gui.RefreshView();
			break;
		}
		case 'x':
		case 'X': { // Rotate clockwise (actually shift variaton, but whatever... :P)
			int nv = g_gui.GetBrushVariation();
			++nv;
			if (nv >= (g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getMaxVariation() : 0)) {
				nv = 0;
			}
			g_gui.SetBrushVariation(nv);
			g_gui.RefreshView();
			break;
		}
		case 'q':
		case 'Q': { // Select previous brush
			g_gui.SelectPreviousBrush();
			break;
		}
		// Hotkeys
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			int index = event.GetKeyCode() - '0';
			if (event.ControlDown()) {
				Hotkey hk;
				if (g_gui.IsSelectionMode()) {
					int view_start_x, view_start_y;
					static_cast<MapWindow*>(GetParent())->GetViewStart(&view_start_x, &view_start_y);
					int view_start_map_x = view_start_x / TileSize, view_start_map_y = view_start_y / TileSize;

					int view_screensize_x, view_screensize_y;
					static_cast<MapWindow*>(GetParent())->GetViewSize(&view_screensize_x, &view_screensize_y);

					int map_x = int(view_start_map_x + (view_screensize_x * zoom_) / TileSize / 2);
					int map_y = int(view_start_map_y + (view_screensize_y * zoom_) / TileSize / 2);

					hk = Hotkey(Position(map_x, map_y, floor_));
				} else if (g_gui.GetCurrentBrush()) {
					// Drawing mode
					hk = Hotkey(g_gui.GetCurrentBrush());
				} else {
					break;
				}
				g_gui.SetHotkey(index, hk);
			} else {
				// Click hotkey
				Hotkey hk = g_gui.GetHotkey(index);
				if (hk.IsPosition()) {
					g_gui.SetSelectionMode();

					int map_x = hk.GetPosition().x;
					int map_y = hk.GetPosition().y;
					int map_z = hk.GetPosition().z;

					static_cast<MapWindow*>(GetParent())->Scroll(TileSize * map_x, TileSize * map_y, true);
					floor_ = map_z;

					g_gui.SetStatusText("Used hotkey " + i2ws(index));
					g_gui.RefreshView();
				} else if (hk.IsBrush()) {
					g_gui.SetDrawingMode();

					std::string name = hk.GetBrushname();
					Brush* brush = g_brushes.getBrush(name);
					if (brush == nullptr) {
						g_gui.SetStatusText("Brush \"" + wxstr(name) + "\" not found");
						return;
					}

					if (!g_gui.SelectBrush(brush)) {
						g_gui.SetStatusText("Brush \"" + wxstr(name) + "\" is not in any palette");
						return;
					}

					g_gui.SetStatusText("Used hotkey " + i2ws(index));
					g_gui.RefreshView();
				} else {
					g_gui.SetStatusText("Unassigned hotkey " + i2ws(index));
				}
			}
			break;
		}
		case 'd':
		case 'D': {
			keyCode_ = WXK_CONTROL_D;
			break;
		}
		default: {
			event.Skip();
			break;
		}
	}
}

void MapCanvas::OnKeyUp(wxKeyEvent& event) {
	keyCode_ = WXK_NONE;
}

void MapCanvas::OnCopy(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.IsSelectionMode()) {
		editor_.copybuffer.copy(editor_, GetFloor());
	}
}

void MapCanvas::OnCut(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.IsSelectionMode()) {
		editor_.copybuffer.cut(editor_, GetFloor());
	}
	g_gui.RefreshView();
}

void MapCanvas::OnPaste(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoPaste();
	g_gui.RefreshView();
}

void MapCanvas::OnDelete(wxCommandEvent& WXUNUSED(event)) {
	editor_.destroySelection();
	g_gui.RefreshView();
}

void MapCanvas::OnCopyPosition(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() == 0) {
		return;
	}

	Position minPos = editor_.selection.minPosition();
	Position maxPos = editor_.selection.maxPosition();

	std::ostringstream clip;
	if (minPos != maxPos) {
		clip << "{";
		clip << "fromx = " << minPos.x << ", ";
		clip << "tox = " << maxPos.x << ", ";
		clip << "fromy = " << minPos.y << ", ";
		clip << "toy = " << maxPos.y << ", ";
		if (minPos.z != maxPos.z) {
			clip << "fromz = " << minPos.z << ", ";
			clip << "toz = " << maxPos.z;
		} else {
			clip << "z = " << minPos.z;
		}
		clip << "}";
	} else {
		switch (g_settings.getInteger(Config::COPY_POSITION_FORMAT)) {
			case 0:
				clip << "{x = " << minPos.x << ", y = " << minPos.y << ", z = " << minPos.z << "}";
				break;
			case 1:
				clip << "{\"x\":" << minPos.x << ",\"y\":" << minPos.y << ",\"z\":" << minPos.z << "}";
				break;
			case 2:
				clip << minPos.x << ", " << minPos.y << ", " << minPos.z;
				break;
			case 3:
				clip << "(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")";
				break;
			case 4:
				clip << "Position(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")";
				break;
		}
	}

	if (wxTheClipboard->Open()) {
		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(wxstr(clip.str()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyServerId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor_.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor_.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getID()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyClientId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor_.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor_.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getClientID()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyName(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor_.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor_.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(wxstr(item->getName()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnBrowseTile(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}

	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	Tile* new_tile = tile->deepCopy(editor_.map);

	wxDialog* w = new BrowseTileWindow(g_gui.root, new_tile, wxPoint(cursor_x_, cursor_y_));

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor_.actionQueue->createAction(ACTION_DELETE_TILES);
		action->addChange(newd Change(new_tile));
		editor_.addAction(action);
	} else {
		// Cancel
		delete new_tile;
	}

	w->Destroy();
}

void MapCanvas::OnRotateItem(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor_.selection.getSelectedTile();

	Action* action = editor_.actionQueue->createAction(ACTION_ROTATE_ITEM);

	Tile* new_tile = tile->deepCopy(editor_.map);

	ItemVector selected_items = new_tile->getSelectedItems();
	ASSERT(selected_items.size() > 0);

	selected_items.front()->doRotate();

	action->addChange(newd Change(new_tile));

	editor_.actionQueue->addAction(action);
	g_gui.RefreshView();
}

void MapCanvas::OnGotoDestination(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor_.selection.getSelectedTile();
	ItemVector selected_items = tile->getSelectedItems();
	ASSERT(selected_items.size() > 0);
	Teleport* teleport = dynamic_cast<Teleport*>(selected_items.front());
	if (teleport) {
		Position pos = teleport->getDestination();
		g_gui.SetScreenCenterPosition(pos);
	}
}

void MapCanvas::OnSwitchDoor(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor_.selection.getSelectedTile();

	Action* action = editor_.actionQueue->createAction(ACTION_SWITCHDOOR);

	Tile* new_tile = tile->deepCopy(editor_.map);

	ItemVector selected_items = new_tile->getSelectedItems();
	ASSERT(selected_items.size() > 0);

	DoorBrush::switchDoor(selected_items.front());

	action->addChange(newd Change(new_tile));

	editor_.actionQueue->addAction(action);
	g_gui.RefreshView();
}

void MapCanvas::OnSelectRAWBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* item = tile->getTopSelectedItem();

	if (item && item->getRAWBrush()) {
		g_gui.SelectBrush(item->getRAWBrush(), TILESET_RAW);
	}
}

void MapCanvas::OnSelectGroundBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	GroundBrush* bb = tile->getGroundBrush();

	if (bb) {
		g_gui.SelectBrush(bb, TILESET_TERRAIN);
	}
}

void MapCanvas::OnSelectDoodadBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* item = tile->getTopSelectedItem();

	if (item) {
		g_gui.SelectBrush(item->getDoodadBrush(), TILESET_DOODAD);
	}
}

void MapCanvas::OnSelectDoorBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* item = tile->getTopSelectedItem();

	if (item) {
		g_gui.SelectBrush(item->getDoorBrush(), TILESET_TERRAIN);
	}
}

void MapCanvas::OnSelectWallBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* wall = tile->getWall();
	WallBrush* wb = wall->getWallBrush();

	if (wb) {
		g_gui.SelectBrush(wb, TILESET_TERRAIN);
	}
}

void MapCanvas::OnSelectCarpetBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* wall = tile->getCarpet();
	CarpetBrush* cb = wall->getCarpetBrush();

	if (cb) {
		g_gui.SelectBrush(cb);
	}
}

void MapCanvas::OnSelectTableBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* wall = tile->getTable();
	TableBrush* tb = wall->getTableBrush();

	if (tb) {
		g_gui.SelectBrush(tb);
	}
}

void MapCanvas::OnSelectHouseBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	if (tile->isHouseTile()) {
		House* house = editor_.map.houses.getHouse(tile->getHouseID());
		if (house) {
			g_gui.house_brush->setHouse(house);
			g_gui.SelectBrush(g_gui.house_brush, TILESET_HOUSE);
		}
	}
}

void MapCanvas::OnSelectCollectionBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	for (auto* item : tile->items) {
		if (item->isWall()) {
			WallBrush* wb = item->getWallBrush();
			if (wb && wb->visibleInPalette() && wb->hasCollection()) {
				g_gui.SelectBrush(wb, TILESET_COLLECTION);
				return;
			}
		}
		if (item->isTable()) {
			TableBrush* tb = item->getTableBrush();
			if (tb && tb->visibleInPalette() && tb->hasCollection()) {
				g_gui.SelectBrush(tb, TILESET_COLLECTION);
				return;
			}
		}
		if (item->isCarpet()) {
			CarpetBrush* cb = item->getCarpetBrush();
			if (cb && cb->visibleInPalette() && cb->hasCollection()) {
				g_gui.SelectBrush(cb, TILESET_COLLECTION);
				return;
			}
		}
		if (Brush* db = item->getDoodadBrush()) {
			if (db && db->visibleInPalette() && db->hasCollection()) {
				g_gui.SelectBrush(db, TILESET_COLLECTION);
				return;
			}
		}
		if (item->isSelected()) {
			RAWBrush* rb = item->getRAWBrush();
			if (rb && rb->hasCollection()) {
				g_gui.SelectBrush(rb, TILESET_COLLECTION);
				return;
			}
		}
	}
	GroundBrush* gb = tile->getGroundBrush();
	if (gb && gb->visibleInPalette() && gb->hasCollection()) {
		g_gui.SelectBrush(gb, TILESET_COLLECTION);
		return;
	}
}

void MapCanvas::OnSelectCreatureBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	if (tile->creature) {
		g_gui.SelectBrush(tile->creature->getBrush(), TILESET_CREATURE);
	}
}

void MapCanvas::OnSelectSpawnBrush(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectBrush(g_gui.spawn_brush, TILESET_CREATURE);
}

void MapCanvas::OnSelectMoveTo(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}

	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	Tile* new_tile = tile->deepCopy(editor_.map);

	wxDialog* w = nullptr;

	ItemVector selected_items = new_tile->getSelectedItems();

	Item* item = nullptr;
	int count = 0;
	for (ItemVector::iterator it = selected_items.begin(); it != selected_items.end(); ++it) {
		++count;
		if ((*it)->isSelected()) {
			item = *it;
		}
	}

	if (item) {
		w = newd TilesetWindow(g_gui.root, &editor_.map, new_tile, item);
	} else {
		return;
	}

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor_.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor_.addAction(action);

		g_gui.RebuildPalettes();
	} else {
		// Cancel!
		delete new_tile;
	}
	w->Destroy();
}

void MapCanvas::OnProperties(wxCommandEvent& WXUNUSED(event)) {
	if (editor_.selection.size() != 1) {
		return;
	}

	Tile* tile = editor_.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	Tile* new_tile = tile->deepCopy(editor_.map);

	wxDialog* w = nullptr;

	if (new_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
		w = newd OldPropertiesWindow(g_gui.root, &editor_.map, new_tile, new_tile->spawn);
	} else if (new_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
		w = newd OldPropertiesWindow(g_gui.root, &editor_.map, new_tile, new_tile->creature);
	} else {
		ItemVector selected_items = new_tile->getSelectedItems();

		Item* item = nullptr;
		int count = 0;
		for (ItemVector::iterator it = selected_items.begin(); it != selected_items.end(); ++it) {
			++count;
			if ((*it)->isSelected()) {
				item = *it;
			}
		}

		if (item) {
			if (editor_.map.getVersion().otbm >= MAP_OTBM_4) {
				w = newd PropertiesWindow(g_gui.root, &editor_.map, new_tile, item);
			} else {
				w = newd OldPropertiesWindow(g_gui.root, &editor_.map, new_tile, item);
			}
		} else {
			return;
		}
	}

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor_.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor_.addAction(action);
	} else {
		// Cancel!
		delete new_tile;
	}
	w->Destroy();
}

void MapCanvas::ChangeFloor(int new_floor) {
	ASSERT(new_floor >= 0 || new_floor < MAP_LAYERS);
	int old_floor = floor_;
	floor_ = new_floor;
	if (old_floor != new_floor) {
		UpdatePositionStatus();
		g_gui.root->UpdateFloorMenu();
		g_gui.UpdateMinimap(true);
	}
	Refresh();
}

void MapCanvas::EnterDrawingMode() {
	dragging_ = false;
	boundbox_selection_ = false;
	EndPasting();
	Refresh();
}

void MapCanvas::EnterSelectionMode() {
	drawing_ = false;
	dragging_draw_ = false;
	replace_dragging_ = false;
	editor_.replace_brush = nullptr;
	Refresh();
}

bool MapCanvas::isPasting() const {
	return g_gui.IsPasting();
}

void MapCanvas::StartPasting() {
	g_gui.StartPasting();
}

void MapCanvas::EndPasting() {
	g_gui.EndPasting();
}

void MapCanvas::Reset() {
	cursor_x_ = 0;
	cursor_y_ = 0;

	zoom_ = 1.0;
	floor_ = GROUND_LAYER;

	dragging_ = false;
	boundbox_selection_ = false;
	screendragging_ = false;
	drawing_ = false;
	dragging_draw_ = false;

	replace_dragging_ = false;
	editor_.replace_brush = nullptr;

	drag_start_x_ = -1;
	drag_start_y_ = -1;
	drag_start_z_ = -1;

	last_click_map_x_ = -1;
	last_click_map_y_ = -1;
	last_click_map_z_ = -1;

	last_mmb_click_x_ = -1;
	last_mmb_click_y_ = -1;

	editor_.selection.clear();
	editor_.actionQueue->clear();
}

MapPopupMenu::MapPopupMenu(Editor& editorRef) :
	wxMenu(""), editor_(editorRef) {
	////
}

MapPopupMenu::~MapPopupMenu() {
	////
}

void MapPopupMenu::Update() {
	// Clear the menu of all items
	while (GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		// If you add a submenu, this won't delete it.
		Delete(m_item);
	}

	bool anything_selected = editor_.selection.size() != 0;

	wxMenuItem* cutItem = Append(MAP_POPUP_MENU_CUT, "&Cut\tCTRL+X", "Cut out all selected items");
	cutItem->Enable(anything_selected);

	wxMenuItem* copyItem = Append(MAP_POPUP_MENU_COPY, "&Copy\tCTRL+C", "Copy all selected items");
	copyItem->Enable(anything_selected);

	wxMenuItem* copyPositionItem = Append(MAP_POPUP_MENU_COPY_POSITION, "&Copy Position", "Copy the position as a lua table");
	copyPositionItem->Enable(anything_selected);

	wxMenuItem* pasteItem = Append(MAP_POPUP_MENU_PASTE, "&Paste\tCTRL+V", "Paste items in the copybuffer here");
	pasteItem->Enable(editor_.copybuffer.canPaste());

	wxMenuItem* deleteItem = Append(MAP_POPUP_MENU_DELETE, "&Delete\tDEL", "Removes all seleceted items");
	deleteItem->Enable(anything_selected);

	if (anything_selected) {
		if (editor_.selection.size() == 1) {
			Tile* tile = editor_.selection.getSelectedTile();
			ItemVector selected_items = tile->getSelectedItems();

			bool hasWall = false;
			bool hasCarpet = false;
			bool hasTable = false;
			bool hasCollection = false;
			Item* topItem = nullptr;
			Item* topSelectedItem = (selected_items.size() == 1 ? selected_items.back() : nullptr);
			Creature* topCreature = tile->creature;
			Spawn* topSpawn = tile->spawn;

			for (auto* item : tile->items) {
				if (item->isWall()) {
					Brush* wb = item->getWallBrush();
					if (wb && wb->visibleInPalette()) {
						hasWall = true;
						hasCollection = hasCollection || wb->hasCollection();
					}
				}
				if (item->isTable()) {
					Brush* tb = item->getTableBrush();
					if (tb && tb->visibleInPalette()) {
						hasTable = true;
						hasCollection = hasCollection || tb->hasCollection();
					}
				}
				if (item->isCarpet()) {
					Brush* cb = item->getCarpetBrush();
					if (cb && cb->visibleInPalette()) {
						hasCarpet = true;
						hasCollection = hasCollection || cb->hasCollection();
					}
				}
				if (Brush* db = item->getDoodadBrush()) {
					hasCollection = hasCollection || db->hasCollection();
				}
				if (item->isSelected()) {
					topItem = item;
				}
			}
			if (!topItem) {
				topItem = tile->ground;
			}

			AppendSeparator();

			if (topSelectedItem) {
				Append(MAP_POPUP_MENU_COPY_SERVER_ID, "Copy Item Server Id", "Copy the server id of this item");
				Append(MAP_POPUP_MENU_COPY_CLIENT_ID, "Copy Item Client Id", "Copy the client id of this item");
				Append(MAP_POPUP_MENU_COPY_NAME, "Copy Item Name", "Copy the name of this item");
				AppendSeparator();
			}

			if (topSelectedItem || topCreature || topItem) {
				Teleport* teleport = dynamic_cast<Teleport*>(topSelectedItem);
				if (topSelectedItem && (topSelectedItem->isBrushDoor() || topSelectedItem->isRoteable() || teleport)) {

					if (topSelectedItem->isRoteable()) {
						Append(MAP_POPUP_MENU_ROTATE, "&Rotate item", "Rotate this item");
					}

					if (teleport && teleport->hasDestination()) {
						Append(MAP_POPUP_MENU_GOTO, "&Go To Destination", "Go to the destination of this teleport");
					}

					if (topSelectedItem->isDoor()) {
						if (topSelectedItem->isOpen()) {
							Append(MAP_POPUP_MENU_SWITCH_DOOR, "&Close door", "Close this door");
						} else {
							Append(MAP_POPUP_MENU_SWITCH_DOOR, "&Open door", "Open this door");
						}
						AppendSeparator();
					}
				}

				if (topCreature) {
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
				}

				if (topSpawn) {
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
				}

				Append(MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");

				if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
					Append(MAP_POPUP_MENU_MOVE_TO_TILESET, "Move To Tileset", "Move this item to any tileset");
				}

				if (hasWall) {
					Append(MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}

				if (hasCarpet) {
					Append(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, "Select Carpetbrush", "Uses the current item as a carpetbrush");
				}

				if (hasTable) {
					Append(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, "Select Tablebrush", "Uses the current item as a tablebrush");
				}

				if (topSelectedItem && topSelectedItem->getDoodadBrush() && topSelectedItem->getDoodadBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, "Select Doodadbrush", "Use this doodad brush");
				}

				if (topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush()) {
					Append(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, "Select Doorbrush", "Use this door brush");
				}

				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current item as a groundbrush");
				}

				if (hasCollection || topSelectedItem && topSelectedItem->hasCollectionBrush() || tile->getGroundBrush() && tile->getGroundBrush()->hasCollection()) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				if (tile->isHouseTile()) {
					Append(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.");
				}

				AppendSeparator();
				Append(MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
			} else {

				if (topCreature) {
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
				}

				if (topSpawn) {
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
				}

				Append(MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");
				if (hasWall) {
					Append(MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}
				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current tile as a groundbrush");
				}

				if (hasCollection || tile->getGroundBrush() && tile->getGroundBrush()->hasCollection()) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				if (tile->isHouseTile()) {
					Append(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.");
				}

				if (tile->hasGround() || topCreature || topSpawn) {
					AppendSeparator();
					Append(MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
				}
			}

			AppendSeparator();

			wxMenuItem* browseTile = Append(MAP_POPUP_MENU_BROWSE_TILE, "Browse Field", "Navigate from tile items");
			browseTile->Enable(anything_selected);
		}
	}
}

void MapCanvas::getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor_, PositionVector* tilestodraw, PositionVector* tilestoborder, bool fill /*= false*/) {
	if (fill) {
		Brush* brush = g_gui.GetCurrentBrush();
		if (!brush || !brush->isGround()) {
			return;
		}

		GroundBrush* newBrush = brush->asGround();
		Position position(mouse_map_x, mouse_map_y, floor_);

		Tile* tile = editor_.map.getTile(position);
		GroundBrush* oldBrush = nullptr;
		if (tile) {
			oldBrush = tile->getGroundBrush();
		}

		if (oldBrush && oldBrush->getID() == newBrush->getID()) {
			return;
		}

		if ((tile && tile->ground && !oldBrush) || (!tile && oldBrush)) {
			return;
		}

		if (tile && oldBrush) {
			GroundBrush* groundBrush = tile->getGroundBrush();
			if (!groundBrush || groundBrush->getID() != oldBrush->getID()) {
				return;
			}
		}

		std::fill(std::begin(processed), std::end(processed), false);
		floodFill(&editor_.map, position, BLOCK_SIZE / 2, BLOCK_SIZE / 2, oldBrush, tilestodraw);

	} else {
		for (int y = -g_gui.GetBrushSize() - 1; y <= g_gui.GetBrushSize() + 1; y++) {
			for (int x = -g_gui.GetBrushSize() - 1; x <= g_gui.GetBrushSize() + 1; x++) {
				if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
					if (x >= -g_gui.GetBrushSize() && x <= g_gui.GetBrushSize() && y >= -g_gui.GetBrushSize() && y <= g_gui.GetBrushSize()) {
						if (tilestodraw) {
							tilestodraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor_));
						}
					}
					if (std::abs(x) - g_gui.GetBrushSize() < 2 && std::abs(y) - g_gui.GetBrushSize() < 2) {
						if (tilestoborder) {
							tilestoborder->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor_));
						}
					}
				} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
					double distance = sqrt(double(x * x) + double(y * y));
					if (distance < g_gui.GetBrushSize() + 0.005) {
						if (tilestodraw) {
							tilestodraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor_));
						}
					}
					if (std::abs(distance - g_gui.GetBrushSize()) < 1.5) {
						if (tilestoborder) {
							tilestoborder->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor_));
						}
					}
				}
			}
		}
	}
}

bool MapCanvas::floodFill(Map* map, const Position& center, int x, int y, GroundBrush* brush, PositionVector* positions) {
	countMaxFills_++;
	if (countMaxFills_ > (BLOCK_SIZE * 4 * 4)) {
		countMaxFills_ = 0;
		return true;
	}

	if (x <= 0 || y <= 0 || x >= BLOCK_SIZE || y >= BLOCK_SIZE) {
		return false;
	}

	processed[getFillIndex(x, y)] = true;

	int px = (center.x + x) - (BLOCK_SIZE / 2);
	int py = (center.y + y) - (BLOCK_SIZE / 2);
	if (px <= 0 || py <= 0 || px >= map->getWidth() || py >= map->getHeight()) {
		return false;
	}

	Tile* tile = map->getTile(px, py, center.z);
	if ((tile && tile->ground && !brush) || (!tile && brush)) {
		return false;
	}

	if (tile && brush) {
		GroundBrush* groundBrush = tile->getGroundBrush();
		if (!groundBrush || groundBrush->getID() != brush->getID()) {
			return false;
		}
	}

	positions->push_back(Position(px, py, center.z));

	bool deny = false;
	if (!processed[getFillIndex(x - 1, y)]) {
		deny = floodFill(map, center, x - 1, y, brush, positions);
	}

	if (!deny && !processed[getFillIndex(x, y - 1)]) {
		deny = floodFill(map, center, x, y - 1, brush, positions);
	}

	if (!deny && !processed[getFillIndex(x + 1, y)]) {
		deny = floodFill(map, center, x + 1, y, brush, positions);
	}

	if (!deny && !processed[getFillIndex(x, y + 1)]) {
		deny = floodFill(map, center, x, y + 1, brush, positions);
	}

	return deny;
}

// ============================================================================
// AnimationTimer

AnimationTimer::AnimationTimer(MapCanvas* canvas) :
	wxTimer(),
	map_canvas(canvas),
	started(false) {
		////
	};

AnimationTimer::~AnimationTimer() {
	////
};

void AnimationTimer::Notify() {
	if (map_canvas->GetZoom() <= 2.0) {
		map_canvas->Refresh();
	}
};

void AnimationTimer::Start() {
	if (!started) {
		started = true;
		wxTimer::Start(100);
	}
};

void AnimationTimer::Stop() {
	if (started) {
		started = false;
		wxTimer::Stop();
	}
};

// ============================================================================
// NEW ARCHITECTURE - Rendering System Initialization
// ============================================================================

void MapCanvas::initializeRenderingSystems() {
	// Create and initialize render coordinator
	renderCoordinator_ = std::make_unique<rme::render::RenderCoordinator>();
	renderCoordinator_->initialize();

	// Initialize input dispatcher
	inputDispatcher_.initialize();

	// Create and register specialized handlers
	cameraHandler_ = std::make_unique<rme::input::CameraInputHandler>(this);
	brushHandler_ = std::make_unique<rme::input::BrushInputHandler>(this, editor_);
	selectionHandler_ = std::make_unique<rme::input::SelectionInputHandler>(this, editor_);

	inputDispatcher_.addReceiver(cameraHandler_.get());
	inputDispatcher_.addReceiver(brushHandler_.get());
	inputDispatcher_.addReceiver(selectionHandler_.get());
	inputDispatcher_.addReceiver(this);
}

void MapCanvas::shutdownRenderingSystems() {
	// Shutdown in reverse order
	inputDispatcher_.shutdown();

	if (renderCoordinator_) {
		renderCoordinator_->shutdown();
		renderCoordinator_.reset();
	}
}

void MapCanvas::syncRenderState() {
	// Sync viewport
	int screensize_x, screensize_y;
	GetViewBox(&view_scroll_x_, &view_scroll_y_, &screensize_x, &screensize_y);

	renderState_.setViewport(screensize_x, screensize_y, static_cast<float>(zoom_));
	renderState_.setFloor(floor_);
	renderState_.setScroll(view_scroll_x_, view_scroll_y_);

	// Calculate visible tile range
	auto& mapper = inputDispatcher_.coordinateMapper();
	renderState_.setVisibleRange(
		mapper.startTileX(), mapper.startTileY(),
		mapper.endTileX(), mapper.endTileY()
	);

	// Sync mouse position
	int mouse_map_x, mouse_map_y;
	MouseToMap(&mouse_map_x, &mouse_map_y);
	renderState_.setMousePosition(mouse_map_x, mouse_map_y);
}

// ============================================================================
// InputReceiver Interface Implementation
// ============================================================================

void MapCanvas::onMouseMove(const rme::input::MouseEvent& event) {
	// Update render state with mouse position
	renderState_.setMousePosition(event.mapPos.x, event.mapPos.y);
	Refresh();
}

void MapCanvas::onMouseClick(const rme::input::MouseEvent& event) {
	// Application click handling is in OnMouseActionClick
	Refresh();
}

void MapCanvas::onMouseDoubleClick(const rme::input::MouseEvent& event) {
	// Application double-click handling is in OnMouseLeftDoubleClick
	Refresh();
}

void MapCanvas::onMouseDrag(const rme::input::MouseEvent& event, const rme::input::DragState& drag) {
	// Application drag handling is in existing mouse handlers
	Refresh();
}

void MapCanvas::onMouseWheel(const rme::input::MouseEvent& event) {
	// Wheel handling is in OnWheel
	Refresh();
}
