#include "ui/menubar/menubar_action_manager.h"

#include "app/main.h"
#include "ui/main_menubar.h"
#include "ui/gui.h"
#include "editor/editor.h"
#include "editor/action_queue.h"
#include "app/preferences.h"
#include "ui/managers/recent_files_manager.h"
#include "util/image_manager.h"

void MenuBarActionManager::RegisterActions(MainMenuBar* mb, std::unordered_map<std::string, std::unique_ptr<MenuBar::Action>>& actions) {
	using namespace MenuBar;

#define MAKE_ACTION(id, kind, handler) actions[#id] = std::make_unique<MenuBar::Action>(#id, id, kind, wxCommandEventFunction(&MainMenuBar::handler))
#define MAKE_ACTION_ICON(id, kind, icon, handler) actions[#id] = std::make_unique<MenuBar::Action>(#id, icon, id, kind, wxCommandEventFunction(&MainMenuBar::handler))

#define MAKE_SET_ACTION(id, kind, setting_, handler)                                                                \
	actions[#id] = std::make_unique<MenuBar::Action>(#id, id, kind, wxCommandEventFunction(&MainMenuBar::handler)); \
	actions[#id]->setting = setting_

	MAKE_ACTION_ICON(NEW, wxITEM_NORMAL, ICON_NEW, OnNew);
	MAKE_ACTION_ICON(OPEN, wxITEM_NORMAL, ICON_OPEN, OnOpen);
	MAKE_ACTION_ICON(SAVE, wxITEM_NORMAL, ICON_SAVE, OnSave);
	MAKE_ACTION_ICON(SAVE_AS, wxITEM_NORMAL, ICON_SAVE, OnSaveAs);
	MAKE_ACTION_ICON(GENERATE_MAP, wxITEM_NORMAL, ICON_WAND_MAGIC, OnGenerateMap);
	MAKE_ACTION_ICON(CLOSE, wxITEM_NORMAL, ICON_XMARK, OnClose);

	MAKE_ACTION_ICON(IMPORT_MAP, wxITEM_NORMAL, ICON_FILE_IMPORT, OnImportMap);
	MAKE_ACTION_ICON(IMPORT_MONSTERS, wxITEM_NORMAL, ICON_DRAGON, OnImportMonsterData);
	MAKE_ACTION_ICON(IMPORT_MINIMAP, wxITEM_NORMAL, ICON_IMAGE, OnImportMinimap);

	MAKE_ACTION_ICON(EXPORT_TILESETS, wxITEM_NORMAL, ICON_FILE_EXPORT, OnExportTilesets);

	MAKE_ACTION_ICON(RELOAD_DATA, wxITEM_NORMAL, ICON_SYNC, OnReloadDataFiles);
	// MAKE_ACTION(RECENT_FILES, wxITEM_NORMAL, OnRecent);
	MAKE_ACTION_ICON(PREFERENCES, wxITEM_NORMAL, ICON_GEAR, OnPreferences);
	MAKE_ACTION_ICON(EXIT, wxITEM_NORMAL, ICON_POWER_OFF, OnQuit);

	MAKE_ACTION_ICON(UNDO, wxITEM_NORMAL, ICON_UNDO, OnUndo);
	MAKE_ACTION_ICON(REDO, wxITEM_NORMAL, ICON_REDO, OnRedo);

	MAKE_ACTION_ICON(FIND_ITEM, wxITEM_NORMAL, ICON_SEARCH, OnSearchForItem);
	MAKE_ACTION_ICON(REPLACE_ITEMS, wxITEM_NORMAL, ICON_SYNC, OnReplaceItems);
	MAKE_ACTION_ICON(SEARCH_ON_MAP_EVERYTHING, wxITEM_NORMAL, ICON_SEARCH, OnSearchForStuffOnMap);
	MAKE_ACTION_ICON(SEARCH_ON_MAP_UNIQUE, wxITEM_NORMAL, ICON_SEARCH, OnSearchForUniqueOnMap);
	MAKE_ACTION_ICON(SEARCH_ON_MAP_ACTION, wxITEM_NORMAL, ICON_SEARCH, OnSearchForActionOnMap);
	MAKE_ACTION_ICON(SEARCH_ON_MAP_CONTAINER, wxITEM_NORMAL, ICON_SEARCH, OnSearchForContainerOnMap);
	MAKE_ACTION_ICON(SEARCH_ON_MAP_WRITEABLE, wxITEM_NORMAL, ICON_SEARCH, OnSearchForWriteableOnMap);
	MAKE_ACTION_ICON(SEARCH_ON_SELECTION_EVERYTHING, wxITEM_NORMAL, ICON_SEARCH, OnSearchForStuffOnSelection);
	MAKE_ACTION_ICON(SEARCH_ON_SELECTION_UNIQUE, wxITEM_NORMAL, ICON_SEARCH, OnSearchForUniqueOnSelection);
	MAKE_ACTION_ICON(SEARCH_ON_SELECTION_ACTION, wxITEM_NORMAL, ICON_SEARCH, OnSearchForActionOnSelection);
	MAKE_ACTION_ICON(SEARCH_ON_SELECTION_CONTAINER, wxITEM_NORMAL, ICON_SEARCH, OnSearchForContainerOnSelection);
	MAKE_ACTION_ICON(SEARCH_ON_SELECTION_WRITEABLE, wxITEM_NORMAL, ICON_SEARCH, OnSearchForWriteableOnSelection);
	MAKE_ACTION_ICON(SEARCH_ON_SELECTION_ITEM, wxITEM_NORMAL, ICON_SEARCH, OnSearchForItemOnSelection);
	MAKE_ACTION_ICON(REPLACE_ON_SELECTION_ITEMS, wxITEM_NORMAL, ICON_SYNC, OnReplaceItemsOnSelection);
	MAKE_ACTION_ICON(REMOVE_ON_SELECTION_ITEM, wxITEM_NORMAL, ICON_TRASH_CAN, OnRemoveItemOnSelection);
	MAKE_ACTION(SELECT_MODE_COMPENSATE, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_LOWER, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_CURRENT, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_VISIBLE, wxITEM_RADIO, OnSelectionTypeChange);

	MAKE_ACTION_ICON(AUTOMAGIC, wxITEM_CHECK, ICON_WAND_SPARKLES, OnToggleAutomagic);
	MAKE_ACTION_ICON(BORDERIZE_SELECTION, wxITEM_NORMAL, ICON_BORDER_ALL, OnBorderizeSelection);
	MAKE_ACTION_ICON(BORDERIZE_MAP, wxITEM_NORMAL, ICON_BORDER_ALL, OnBorderizeMap);
	MAKE_ACTION_ICON(RANDOMIZE_SELECTION, wxITEM_NORMAL, ICON_SHUFFLE, OnRandomizeSelection);
	MAKE_ACTION_ICON(RANDOMIZE_MAP, wxITEM_NORMAL, ICON_SHUFFLE, OnRandomizeMap);
	MAKE_ACTION_ICON(GOTO_PREVIOUS_POSITION, wxITEM_NORMAL, ICON_ARROW_LEFT, OnGotoPreviousPosition);
	MAKE_ACTION_ICON(GOTO_POSITION, wxITEM_NORMAL, ICON_LOCATION, OnGotoPosition);
	MAKE_ACTION_ICON(JUMP_TO_BRUSH, wxITEM_NORMAL, ICON_PAINTBRUSH, OnJumpToBrush);
	MAKE_ACTION_ICON(JUMP_TO_ITEM_BRUSH, wxITEM_NORMAL, ICON_PAINTBRUSH, OnJumpToItemBrush);

	MAKE_ACTION_ICON(CUT, wxITEM_NORMAL, ICON_CUT, OnCut);
	MAKE_ACTION_ICON(COPY, wxITEM_NORMAL, ICON_COPY, OnCopy);
	MAKE_ACTION_ICON(PASTE, wxITEM_NORMAL, ICON_PASTE, OnPaste);

	MAKE_ACTION_ICON(EDIT_TOWNS, wxITEM_NORMAL, ICON_CITY, OnMapEditTowns);
	MAKE_ACTION_ICON(EDIT_ITEMS, wxITEM_NORMAL, ICON_CUBES, OnMapEditItems);
	MAKE_ACTION_ICON(EDIT_MONSTERS, wxITEM_NORMAL, ICON_DRAGON, OnMapEditMonsters);

	MAKE_ACTION_ICON(CLEAR_INVALID_HOUSES, wxITEM_NORMAL, ICON_HOUSE_CRACK, OnClearHouseTiles);
	MAKE_ACTION_ICON(CLEAR_MODIFIED_STATE, wxITEM_NORMAL, ICON_ERASER, OnClearModifiedState);
	MAKE_ACTION_ICON(MAP_REMOVE_ITEMS, wxITEM_NORMAL, ICON_TRASH_CAN, OnMapRemoveItems);
	MAKE_ACTION_ICON(MAP_REMOVE_CORPSES, wxITEM_NORMAL, ICON_SKULL, OnMapRemoveCorpses);
	MAKE_ACTION_ICON(MAP_REMOVE_UNREACHABLE_TILES, wxITEM_NORMAL, ICON_BAN, OnMapRemoveUnreachable);
	MAKE_ACTION_ICON(MAP_CLEANUP, wxITEM_NORMAL, ICON_BROOM, OnMapCleanup);
	MAKE_ACTION_ICON(MAP_CLEAN_HOUSE_ITEMS, wxITEM_NORMAL, ICON_HOUSE_MEDICAL, OnMapCleanHouseItems);
	MAKE_ACTION_ICON(MAP_PROPERTIES, wxITEM_NORMAL, ICON_GEAR, OnMapProperties);
	MAKE_ACTION_ICON(MAP_STATISTICS, wxITEM_NORMAL, ICON_CHART_BAR, OnMapStatistics);

	MAKE_ACTION(VIEW_TOOLBARS_BRUSHES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_POSITION, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_SIZES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_STANDARD, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION_ICON(NEW_VIEW, wxITEM_NORMAL, ICON_WINDOW_MAXIMIZE, OnNewView);
	MAKE_ACTION_ICON(TOGGLE_FULLSCREEN, wxITEM_NORMAL, ICON_EXPAND, OnToggleFullscreen);

	MAKE_ACTION_ICON(ZOOM_IN, wxITEM_NORMAL, ICON_PLUS, OnZoomIn);
	MAKE_ACTION_ICON(ZOOM_OUT, wxITEM_NORMAL, ICON_MINUS, OnZoomOut);
	MAKE_ACTION_ICON(ZOOM_NORMAL, wxITEM_NORMAL, ICON_MAGNIFYING_GLASS, OnZoomNormal);

	MAKE_ACTION(SHOW_SHADE, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ALL_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_HIGHER_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_LOCKED_DOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_EXTRA, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_INGAME_BOX, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHT_STR, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TECHNICAL_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WAYPOINTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_GRID, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_CREATURES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPAWNS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPECIAL, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_AS_MINIMAP, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_COLORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_MODIFIED, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_HOUSES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PATHING, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOOLTIPS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PREVIEW, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WALL_HOOKS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOWNS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(ALWAYS_SHOW_ZONES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(EXT_HOUSE_SHADER, wxITEM_CHECK, OnChangeViewSettings);

	MAKE_ACTION(EXPERIMENTAL_FOG, wxITEM_CHECK, OnChangeViewSettings); // experimental

	MAKE_ACTION(WIN_MINIMAP, wxITEM_NORMAL, OnMinimapWindow);
	MAKE_ACTION(WIN_TOOL_OPTIONS, wxITEM_NORMAL, OnToolOptionsWindow);
	MAKE_ACTION(WIN_INGAME_PREVIEW, wxITEM_NORMAL, OnIngamePreviewWindow);
	MAKE_ACTION_ICON(NEW_PALETTE, wxITEM_NORMAL, ICON_PLUS, OnNewPalette);
	MAKE_ACTION_ICON(TAKE_SCREENSHOT, wxITEM_NORMAL, ICON_CAMERA, OnTakeScreenshot);

	MAKE_ACTION_ICON(LIVE_START, wxITEM_NORMAL, ICON_SIGNAL, OnStartLive);
	MAKE_ACTION_ICON(LIVE_JOIN, wxITEM_NORMAL, ICON_NETWORK_WIRED, OnJoinLive);
	MAKE_ACTION_ICON(LIVE_CLOSE, wxITEM_NORMAL, ICON_POWER_OFF, OnCloseLive);

	MAKE_ACTION_ICON(SELECT_TERRAIN, wxITEM_NORMAL, ICON_MOUNTAIN, OnSelectTerrainPalette);
	MAKE_ACTION_ICON(SELECT_DOODAD, wxITEM_NORMAL, ICON_TREE, OnSelectDoodadPalette);
	MAKE_ACTION_ICON(SELECT_ITEM, wxITEM_NORMAL, ICON_CUBE, OnSelectItemPalette);
	MAKE_ACTION_ICON(SELECT_COLLECTION, wxITEM_NORMAL, ICON_LAYER_GROUP, OnSelectCollectionPalette);
	MAKE_ACTION_ICON(SELECT_CREATURE, wxITEM_NORMAL, ICON_DRAGON, OnSelectCreaturePalette);
	MAKE_ACTION_ICON(SELECT_HOUSE, wxITEM_NORMAL, ICON_HOUSE, OnSelectHousePalette);
	MAKE_ACTION_ICON(SELECT_WAYPOINT, wxITEM_NORMAL, ICON_FLAG, OnSelectWaypointPalette);
	MAKE_ACTION_ICON(SELECT_RAW, wxITEM_NORMAL, ICON_CUBE, OnSelectRawPalette);

	MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_1, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_2, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_3, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_4, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_5, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_6, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_7, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_8, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_9, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_10, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_11, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_12, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_13, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_14, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_15, wxITEM_RADIO, OnChangeFloor);

	MAKE_ACTION(DEBUG_VIEW_DAT, wxITEM_NORMAL, OnDebugViewDat);
	MAKE_ACTION(EXTENSIONS, wxITEM_NORMAL, OnListExtensions);
	MAKE_ACTION(GOTO_WEBSITE, wxITEM_NORMAL, OnGotoWebsite);
	MAKE_ACTION_ICON(ABOUT, wxITEM_NORMAL, ICON_INFO, OnAbout);

#undef MAKE_ACTION
#undef MAKE_ACTION_ICON
#undef MAKE_SET_ACTION
}

void MenuBarActionManager::UpdateState(MainMenuBar* mb) {
	using namespace MenuBar;
	// This updates all buttons and sets them to proper enabled/disabled state

	bool enable = !g_gui.IsWelcomeDialogShown();
	mb->menubar->Enable(enable);
	if (!enable) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (editor) {
		mb->EnableItem(UNDO, editor->actionQueue->canUndo());
		mb->EnableItem(REDO, editor->actionQueue->canRedo());
		mb->EnableItem(PASTE, editor->copybuffer.canPaste());
	} else {
		mb->EnableItem(UNDO, false);
		mb->EnableItem(REDO, false);
		mb->EnableItem(PASTE, false);
	}

	bool loaded = g_version.IsVersionLoaded();
	bool has_map = editor != nullptr;
	bool has_selection = editor && editor->hasSelection();
	bool is_live = editor && editor->live_manager.IsLive();
	bool is_host = has_map && !editor->live_manager.IsClient();
	bool is_local = has_map && !is_live;

	mb->EnableItem(CLOSE, is_local);
	mb->EnableItem(SAVE, is_host);
	mb->EnableItem(SAVE_AS, is_host);
	mb->EnableItem(GENERATE_MAP, false);

	mb->EnableItem(IMPORT_MAP, is_local);
	mb->EnableItem(IMPORT_MONSTERS, is_local);
	mb->EnableItem(IMPORT_MINIMAP, false);

	mb->EnableItem(EXPORT_TILESETS, loaded);

	mb->EnableItem(FIND_ITEM, is_host);
	mb->EnableItem(REPLACE_ITEMS, is_local);
	mb->EnableItem(SEARCH_ON_MAP_EVERYTHING, is_host);
	mb->EnableItem(SEARCH_ON_MAP_UNIQUE, is_host);
	mb->EnableItem(SEARCH_ON_MAP_ACTION, is_host);
	mb->EnableItem(SEARCH_ON_MAP_CONTAINER, is_host);
	mb->EnableItem(SEARCH_ON_MAP_WRITEABLE, is_host);
	mb->EnableItem(SEARCH_ON_SELECTION_EVERYTHING, has_selection && is_host);
	mb->EnableItem(SEARCH_ON_SELECTION_UNIQUE, has_selection && is_host);
	mb->EnableItem(SEARCH_ON_SELECTION_ACTION, has_selection && is_host);
	mb->EnableItem(SEARCH_ON_SELECTION_CONTAINER, has_selection && is_host);
	mb->EnableItem(SEARCH_ON_SELECTION_WRITEABLE, has_selection && is_host);
	mb->EnableItem(SEARCH_ON_SELECTION_ITEM, has_selection && is_host);
	mb->EnableItem(REPLACE_ON_SELECTION_ITEMS, has_selection && is_host);
	mb->EnableItem(REMOVE_ON_SELECTION_ITEM, has_selection && is_host);

	mb->EnableItem(CUT, has_map);
	mb->EnableItem(COPY, has_map);

	mb->EnableItem(BORDERIZE_SELECTION, has_map && has_selection);
	mb->EnableItem(BORDERIZE_MAP, is_local);
	mb->EnableItem(RANDOMIZE_SELECTION, has_map && has_selection);
	mb->EnableItem(RANDOMIZE_MAP, is_local);

	mb->EnableItem(GOTO_PREVIOUS_POSITION, has_map);
	mb->EnableItem(GOTO_POSITION, has_map);
	mb->EnableItem(JUMP_TO_BRUSH, loaded);
	mb->EnableItem(JUMP_TO_ITEM_BRUSH, loaded);

	mb->EnableItem(MAP_REMOVE_ITEMS, is_host);
	mb->EnableItem(MAP_REMOVE_CORPSES, is_local);
	mb->EnableItem(MAP_REMOVE_UNREACHABLE_TILES, is_local);
	mb->EnableItem(CLEAR_INVALID_HOUSES, is_local);
	mb->EnableItem(CLEAR_MODIFIED_STATE, is_local);

	mb->EnableItem(EDIT_TOWNS, is_local);
	mb->EnableItem(EDIT_ITEMS, false);
	mb->EnableItem(EDIT_MONSTERS, false);

	mb->EnableItem(MAP_CLEANUP, is_local);
	mb->EnableItem(MAP_PROPERTIES, is_local);
	mb->EnableItem(MAP_STATISTICS, is_local);

	mb->EnableItem(NEW_VIEW, has_map);
	mb->EnableItem(ZOOM_IN, has_map);
	mb->EnableItem(ZOOM_OUT, has_map);
	mb->EnableItem(ZOOM_NORMAL, has_map);

	if (has_map) {
		mb->CheckItem(SHOW_SPAWNS, g_settings.getBoolean(Config::SHOW_SPAWNS));
		mb->CheckItem(SHOW_LIGHTS, g_settings.getBoolean(Config::SHOW_LIGHTS));
	}

	mb->EnableItem(WIN_MINIMAP, loaded);
	mb->EnableItem(WIN_TOOL_OPTIONS, loaded);
	mb->EnableItem(WIN_INGAME_PREVIEW, loaded);
	mb->EnableItem(NEW_PALETTE, loaded);
	mb->EnableItem(SELECT_TERRAIN, loaded);
	mb->EnableItem(SELECT_DOODAD, loaded);
	mb->EnableItem(SELECT_ITEM, loaded);
	mb->EnableItem(SELECT_COLLECTION, loaded);
	mb->EnableItem(SELECT_HOUSE, loaded);
	mb->EnableItem(SELECT_CREATURE, loaded);
	mb->EnableItem(SELECT_WAYPOINT, loaded);
	mb->EnableItem(SELECT_RAW, loaded);

	mb->EnableItem(LIVE_START, is_local);
	mb->EnableItem(LIVE_JOIN, loaded);
	mb->EnableItem(LIVE_CLOSE, is_live);

	mb->EnableItem(DEBUG_VIEW_DAT, loaded);

	mb->UpdateFloorMenu();
}
