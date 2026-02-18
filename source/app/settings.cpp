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

#include "app/settings.h"
#include "ui/gui_ids.h"
#include "app/client_version.h"
#include "app/main.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <toml++/toml.h>

#include <iostream>
#include <string>
#include <fstream>

static toml::table g_settings_table;

toml::table& Settings::getTable() {
	return g_settings_table;
}

Settings g_settings;

Settings::Settings() :
	store(Config::LAST) {
	setDefaults();
}

Settings::~Settings() {
	////
}

bool Settings::getBoolean(uint32_t key) const {
	if (key > Config::LAST) {
		return false;
	}

	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_INT) {
		return dv.intval != 0;
	}
	return false;
}

int Settings::getInteger(uint32_t key) const {
	if (key > Config::LAST) {
		return 0;
	}
	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_INT) {
		return dv.intval;
	}
	return 0;
}

float Settings::getFloat(uint32_t key) const {
	if (key > Config::LAST) {
		return 0.0;
	}
	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_FLOAT) {
		return dv.floatval;
	}
	return 0.0;
}

std::string Settings::getString(uint32_t key) const {
	if (key > Config::LAST) {
		return "";
	}
	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_STR && dv.strval != nullptr) {
		return *dv.strval;
	}
	return "";
}

void Settings::setInteger(uint32_t key, int newval) {
	if (key > Config::LAST) {
		return;
	}
	DynamicValue& dv = store[key];
	if (dv.type == TYPE_INT) {
		dv.intval = newval;
	} else if (dv.type == TYPE_NONE) {
		dv.type = TYPE_INT;
		dv.intval = newval;
	}
}

void Settings::setFloat(uint32_t key, float newval) {
	if (key > Config::LAST) {
		return;
	}
	DynamicValue& dv = store[key];
	if (dv.type == TYPE_FLOAT) {
		dv.floatval = newval;
	} else if (dv.type == TYPE_NONE) {
		dv.type = TYPE_FLOAT;
		dv.floatval = newval;
	}
}

void Settings::setString(uint32_t key, std::string newval) {
	if (key > Config::LAST) {
		return;
	}
	DynamicValue& dv = store[key];
	if (dv.type == TYPE_STR) {
		delete dv.strval;
		dv.strval = newd std::string(newval);
	} else if (dv.type == TYPE_NONE) {
		dv.type = TYPE_STR;
		dv.strval = newd std::string(newval);
	}
}

std::string Settings::DynamicValue::str() {
	switch (type) {
		case TYPE_FLOAT:
			return f2s(floatval);
		case TYPE_STR:
			return std::string(*strval);
		case TYPE_INT:
			return i2s(intval);
		default:
		case TYPE_NONE:
			return "";
	}
}

static std::string toLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

void Settings::IO(IOMode mode) {
	if (mode == LOAD) {
		if (std::ifstream("config.toml")) {
			try {
				g_settings_table = toml::parse_file("config.toml");
			} catch (const toml::parse_error& err) {
				spdlog::error("Failed to parse config.toml: {}", err.description());
			}
		}
	}

	toml::table* root = &g_settings_table;
	toml::table* cur_sec = root;

	using namespace Config;

#define section(s)                                                \
	do {                                                          \
		if (s[0] == '\0') {                                       \
			cur_sec = root;                                       \
		} else if (mode != DEFAULT) {                             \
			std::string sec_name = toLower(s);                    \
			cur_sec = root->get_as<toml::table>(sec_name);        \
			if (!cur_sec) {                                       \
				root->insert_or_assign(sec_name, toml::table {}); \
				cur_sec = root->get_as<toml::table>(sec_name);    \
			}                                                     \
		}                                                         \
	} while (false)

#define Bool(key, dflt)                                                  \
	do {                                                                 \
		std::string k = toLower(#key);                                   \
		if (mode == DEFAULT) {                                           \
			setInteger(key, dflt ? 1 : 0);                               \
		} else if (mode == SAVE) {                                       \
			cur_sec->insert_or_assign(k, getBoolean(key));               \
		} else if (mode == LOAD) {                                       \
			setInteger(key, (*cur_sec)[k].value_or(bool(dflt)) ? 1 : 0); \
		}                                                                \
	} while (false)

#define Int(key, dflt)                                               \
	do {                                                             \
		std::string k = toLower(#key);                               \
		if (mode == DEFAULT) {                                       \
			setInteger(key, dflt);                                   \
		} else if (mode == SAVE) {                                   \
			cur_sec->insert_or_assign(k, getInteger(key));           \
		} else if (mode == LOAD) {                                   \
			setInteger(key, (int)(*cur_sec)[k].value_or(int(dflt))); \
		}                                                            \
	} while (false)

#define IntToSave(key, dflt)                                         \
	do {                                                             \
		std::string k = toLower(#key);                               \
		if (mode == DEFAULT) {                                       \
			setInteger(key, dflt);                                   \
		} else if (mode == SAVE) {                                   \
			cur_sec->insert_or_assign(k, getInteger(key##_TO_SAVE)); \
		} else if (mode == LOAD) {                                   \
			setInteger(key, (int)(*cur_sec)[k].value_or((int)dflt)); \
			setInteger(key##_TO_SAVE, getInteger(key));              \
		}                                                            \
	} while (false)

#define Float(key, dflt)                                               \
	do {                                                               \
		std::string k = toLower(#key);                                 \
		if (mode == DEFAULT) {                                         \
			setFloat(key, dflt);                                       \
		} else if (mode == SAVE) {                                     \
			cur_sec->insert_or_assign(k, getFloat(key));               \
		} else if (mode == LOAD) {                                     \
			setFloat(key, (float)(*cur_sec)[k].value_or(float(dflt))); \
		}                                                              \
	} while (false)

#define String(key, dflt)                                              \
	do {                                                               \
		std::string k = toLower(#key);                                 \
		if (mode == DEFAULT) {                                         \
			setString(key, dflt);                                      \
		} else if (mode == SAVE) {                                     \
			cur_sec->insert_or_assign(k, getString(key));              \
		} else if (mode == LOAD) {                                     \
			setString(key, (*cur_sec)[k].value_or(std::string(dflt))); \
		}                                                              \
	} while (false)

	section("View");
	Bool(TRANSPARENT_FLOORS, false);
	Bool(TRANSPARENT_ITEMS, false);
	Bool(SHOW_ALL_FLOORS, true);
	Bool(SHOW_INGAME_BOX, false);
	Bool(SHOW_LIGHTS, false);
	Bool(SHOW_LIGHT_STR, true);
	Bool(SHOW_TECHNICAL_ITEMS, true);
	Bool(SHOW_WAYPOINTS, true);
	Bool(SHOW_GRID, false);
	Bool(SHOW_EXTRA, true);
	Bool(SHOW_SHADE, true);
	Bool(SHOW_SPECIAL_TILES, true);
	Bool(SHOW_SPAWNS, true);
	Bool(SHOW_ITEMS, true);
	Bool(HIGHLIGHT_ITEMS, false);
	Bool(HIGHLIGHT_LOCKED_DOORS, true);
	Bool(SHOW_CREATURES, true);
	Bool(SHOW_HOUSES, true);
	Bool(SHOW_BLOCKING, false);
	Bool(SHOW_TOOLTIPS, true);
	Bool(SHOW_ONLY_TILEFLAGS, false);
	Bool(SHOW_ONLY_MODIFIED_TILES, false);
	Bool(SHOW_PREVIEW, true);
	Bool(SHOW_AUTOBORDER_PREVIEW, true);
	Bool(SHOW_WALL_HOOKS, false);
	Bool(SHOW_TOWNS, false);
	Bool(ALWAYS_SHOW_ZONES, true);
	Bool(EXT_HOUSE_SHADER, true);
	Bool(DRAW_LOCKED_DOOR, false);

	section("General");
	Bool(GOTO_WEBSITE_ON_BOOT, false);
	Bool(USE_UPDATER, true);

	section("Version");
	Int(VERSION_ID, 0);
	Int(CHECK_SIGNATURES, 1);
	Int(USE_CUSTOM_DATA_DIRECTORY, 0);
	String(DATA_DIRECTORY, "");
	String(EXTENSIONS_DIRECTORY, "");
	String(ASSETS_DATA_DIRS, "");

	section("Editor");
	Int(WORKER_THREADS, 1);
	Bool(MERGE_MOVE, false);
	Bool(MERGE_PASTE, false);
	Int(UNDO_SIZE, 400);
	Int(UNDO_MEM_SIZE, 40);
	Bool(GROUP_ACTIONS, true);
	Int(SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	Bool(COMPENSATED_SELECT, true);
	Float(SCROLL_SPEED, 3.5f);
	Float(ZOOM_SPEED, 1.4f);
	Bool(SWITCH_MOUSEBUTTONS, false);
	Bool(DOUBLECLICK_PROPERTIES, true);
	Bool(LISTBOX_EATS_ALL_EVENTS, true);
	Bool(BORDER_IS_GROUND, true);
	Bool(BORDERIZE_PASTE, true);
	Bool(BORDERIZE_DRAG, true);
	Int(BORDERIZE_DRAG_THRESHOLD, 6000);
	Int(BORDERIZE_PASTE_THRESHOLD, 10000);
	Bool(ALWAYS_MAKE_BACKUP, false);
	Bool(USE_AUTOMAGIC, true);
	Bool(HOUSE_BRUSH_REMOVE_ITEMS, false);
	Bool(AUTO_ASSIGN_DOORID, true);
	Bool(ERASER_LEAVE_UNIQUE, true);
	Bool(DOODAD_BRUSH_ERASE_LIKE, false);
	Bool(WARN_FOR_DUPLICATE_ID, true);
	Bool(AUTO_CREATE_SPAWN, true);
	Int(DEFAULT_SPAWNTIME, 60);
	Int(MAX_SPAWN_RADIUS, 30);
	Int(CURRENT_SPAWN_RADIUS, 5);
	Int(DEFAULT_CLIENT_VERSION, CLIENT_VERSION_NONE);
	Bool(RAW_LIKE_SIMONE, true);
	Bool(ONLY_ONE_INSTANCE, true);
	Bool(SHOW_TILESET_EDITOR, false);
	Bool(USE_OTBM_4_FOR_ALL_MAPS, false);
	Bool(SAVE_WITH_OTB_MAGIC_NUMBER, false);
	Int(REPLACE_SIZE, 500);
	Int(COPY_POSITION_FORMAT, 0);
	String(RECENT_EDITED_MAP_PATH, "");
	String(RECENT_EDITED_MAP_POSITION, "");
	Int(FIND_ITEM_MODE, 0);
	Int(JUMP_TO_ITEM_MODE, 0);

	section("Graphics");
	Bool(TEXTURE_MANAGEMENT, true);
	Int(TEXTURE_CLEAN_PULSE, 15);
	Int(TEXTURE_LONGEVITY, 20);
	Int(TEXTURE_CLEAN_THRESHOLD, 2500);
	Int(SOFTWARE_CLEAN_THRESHOLD, 1800);
	Int(SOFTWARE_CLEAN_SIZE, 500);
	Bool(ICON_BACKGROUND, false);
	Int(HARD_REFRESH_RATE, 200);
	Bool(HIDE_ITEMS_WHEN_ZOOMED, true);
	String(SCREENSHOT_DIRECTORY, "");
	String(SCREENSHOT_FORMAT, "png");
	IntToSave(USE_MEMCACHED_SPRITES, 0); // This is special, keeping as IntToSave for now
	Int(MINIMAP_UPDATE_DELAY, 333);
	Bool(MINIMAP_VIEW_BOX, true);
	String(MINIMAP_EXPORT_DIR, "");
	String(TILESET_EXPORT_DIR, "");
	Int(FRAME_RATE_LIMIT, 144);
	Bool(SHOW_FPS_COUNTER, false);
	Int(ANTI_ALIASING, 0);
	String(SCREEN_SHADER, "None");

	Int(CURSOR_RED, 0);
	Int(CURSOR_GREEN, 166);
	Int(CURSOR_BLUE, 0);
	Int(CURSOR_ALPHA, 128);
	Int(CURSOR_ALT_RED, 0);
	Int(CURSOR_ALT_GREEN, 166);
	Int(CURSOR_ALT_BLUE, 0);
	Int(CURSOR_ALT_ALPHA, 128);

	section("UI");
	Bool(USE_LARGE_CONTAINER_ICONS, true);
	Bool(USE_LARGE_CHOOSE_ITEM_ICONS, true);
	Bool(USE_LARGE_TERRAIN_TOOLBAR, true);
	Bool(USE_LARGE_COLLECTION_TOOLBAR, true);
	Bool(USE_LARGE_DOODAD_SIZEBAR, true);
	Bool(USE_LARGE_ITEM_SIZEBAR, true);
	Bool(USE_LARGE_HOUSE_SIZEBAR, true);
	Bool(USE_LARGE_RAW_SIZEBAR, true);
	Bool(USE_GUI_SELECTION_SHADOW, false);
	Int(PALETTE_COL_COUNT, 8);
	String(PALETTE_TERRAIN_STYLE, "large icons");
	String(PALETTE_COLLECTION_STYLE, "large icons");
	String(PALETTE_DOODAD_STYLE, "large icons");
	String(PALETTE_ITEM_STYLE, "listbox");
	String(PALETTE_RAW_STYLE, "listbox");

	section("Window");
	String(PALETTE_LAYOUT, "");
	String(TOOL_OPTIONS_LAYOUT, "");
	Bool(INGAME_PREVIEW_VISIBLE, false);
	String(INGAME_PREVIEW_LAYOUT, "");
	String(HOUSE_PALETTE_LAYOUT, "");
	Bool(MINIMAP_VISIBLE, true);
	String(MINIMAP_LAYOUT, "");
	Int(WINDOW_HEIGHT, 500);
	Int(WINDOW_WIDTH, 700);
	Int(REPLACE_TOOL_WINDOW_WIDTH, 1400);
	Int(REPLACE_TOOL_WINDOW_HEIGHT, 850);
	Bool(WINDOW_MAXIMIZED, false);
	Bool(WELCOME_DIALOG, true);

	Bool(WELCOME_DIALOG, true);

	section("Hotkeys");
	String(NUMERICAL_HOTKEYS, "");

	section("Toolbars");
	Bool(SHOW_TOOLBAR_STANDARD, true);
	Bool(SHOW_TOOLBAR_BRUSHES, false);
	Bool(SHOW_TOOLBAR_POSITION, false);
	Bool(SHOW_TOOLBAR_SIZES, false);
	String(TOOLBAR_STANDARD_LAYOUT, "");
	String(TOOLBAR_BRUSHES_LAYOUT, "");
	String(TOOLBAR_POSITION_LAYOUT, "");
	String(TOOLBAR_SIZES_LAYOUT, "");

	// experimental
	section("experimental");
	Int(EXPERIMENTAL_FOG, 0);

	if (mode == SAVE) {
		std::ofstream file("config.toml");
		if (file.is_open()) {
			file << g_settings_table;
			if (file.fail()) {
				spdlog::error("Failed to write to config.toml");
			}
			file.close();
		} else {
			spdlog::error("Failed to open config.toml for writing");
		}
	}

#undef section
#undef Bool
#undef Int
#undef IntToSave
#undef Float
#undef String
}

void Settings::load() {
	IO(LOAD);
}

void Settings::save(bool endoftheworld) {
	IO(SAVE);
}
