#include "ui/menubar/navigation_menu_handler.h"
#include "app/main.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"
#include "ui/dialogs/find_dialog.h"
#include "ui/map_tab.h"
#include "ui/map_window.h"
#include "ui/dialogs/goto_position_dialog.h"
#include "ui/find_item_window.h"
#include "ui/main_menubar.h"
#include "app/preferences.h"
#include "ui/result_window.h"
#include "editor/operations/search_operations.h"

namespace {
	void selectFoundBrush(const FindItemDialog& dialog) {
		if (const Brush* brush = dialog.getResult()) {
			const PaletteType palette = dialog.getResultKind() == AdvancedFinderCatalogKind::Creature ? TILESET_CREATURE : TILESET_RAW;
			g_gui.SelectBrush(brush, palette);
		}
	}

	void showItemSearchResults(uint16_t item_id, const wxString& load_bar_label) {
		EditorOperations::ItemSearcher finder(item_id, static_cast<uint32_t>(g_settings.getInteger(Config::REPLACE_SIZE)));
		g_gui.CreateLoadBar(load_bar_label);
		foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, false);
		g_gui.DestroyLoadBar();

		if (finder.limitReached()) {
			wxString msg;
			msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
			DialogUtil::PopupDialog("Notice", msg, wxOK);
		}

		SearchResultWindow* window = g_gui.ShowSearchWindow();
		window->Clear();
		for (const auto& [tile, item] : finder.result) {
			window->AddPosition(wxstr(item->getName()), tile->getPosition());
		}
	}

	void showCreatureSearchResults(const FindItemDialog& dialog, const wxString& load_bar_label) {
		EditorOperations::CreatureSearcher finder(dialog.getResult(), static_cast<uint32_t>(g_settings.getInteger(Config::REPLACE_SIZE)));
		g_gui.CreateLoadBar(load_bar_label);
		foreach_TileOnMap(g_gui.GetCurrentMap(), finder);
		g_gui.DestroyLoadBar();

		if (finder.limitReached()) {
			wxString msg;
			msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
			DialogUtil::PopupDialog("Notice", msg, wxOK);
		}

		SearchResultWindow* window = g_gui.ShowSearchWindow();
		window->Clear();
		for (const auto& [tile, creature] : finder.result) {
			window->AddPosition(wxstr(creature->getName()), tile->getPosition());
		}
	}
}

NavigationMenuHandler::NavigationMenuHandler(MainFrame* frame, MainMenuBar* menubar) :
	frame(frame), menubar(menubar) {
}

NavigationMenuHandler::~NavigationMenuHandler() {
}

void NavigationMenuHandler::OnZoomIn(wxCommandEvent& event) {
	double zoom = g_gui.GetCurrentZoom();
	g_gui.SetCurrentZoom(zoom - 0.1);
}

void NavigationMenuHandler::OnZoomOut(wxCommandEvent& event) {
	double zoom = g_gui.GetCurrentZoom();
	g_gui.SetCurrentZoom(zoom + 0.1);
}

void NavigationMenuHandler::OnZoomNormal(wxCommandEvent& event) {
	g_gui.SetCurrentZoom(1.0);
}

void NavigationMenuHandler::OnGotoPreviousPosition(wxCommandEvent& WXUNUSED(event)) {
	MapTab* mapTab = g_gui.GetCurrentMapTab();
	if (mapTab) {
		mapTab->GoToPreviousCenterPosition();
	}
}

void NavigationMenuHandler::OnGotoPosition(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	// Display dialog, it also controls the actual jump
	GotoPositionDialog dlg(frame, *g_gui.GetCurrentEditor());
	dlg.ShowModal();
}

void NavigationMenuHandler::OnJumpToBrush(wxCommandEvent& WXUNUSED(event)) {
	if (!g_version.IsVersionLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindDialog* dlg = newd FindBrushDialog(frame);

	// Display dialog to user
	dlg->ShowModal();

	// Retrieve result, if null user canceled
	const Brush* brush = dlg->getResult();
	if (brush) {
		g_gui.SelectBrush(brush, TILESET_UNKNOWN);
	}
	dlg->Destroy();
}

void NavigationMenuHandler::OnJumpToItemBrush(wxCommandEvent& WXUNUSED(event)) {
	if (!g_version.IsVersionLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindItemDialog dialog(frame, "Jump to Item", false, FindItemDialog::ActionSet::SearchAndSelect, AdvancedFinderDefaultAction::SelectItem, true);
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::JUMP_TO_ITEM_MODE));
	const int modal_result = dialog.ShowModal();
	if (modal_result != wxID_CANCEL) {
		if (dialog.getResultAction() == FindItemDialog::ResultAction::SearchMap) {
			if (dialog.getResultKind() == AdvancedFinderCatalogKind::Creature) {
				showCreatureSearchResults(dialog, "Searching map...");
			} else {
				showItemSearchResults(dialog.getResultID(), "Searching map...");
			}
		} else if (dialog.getResultAction() == FindItemDialog::ResultAction::SelectItem) {
			selectFoundBrush(dialog);
		}
		g_settings.setInteger(Config::JUMP_TO_ITEM_MODE, (int)dialog.getSearchMode());
	}
}

void NavigationMenuHandler::OnChangeFloor(wxCommandEvent& event) {
	// Workaround to stop events from looping
	if (menubar->checking_programmaticly) {
		return;
	}

	// this will have to be changed if you want to have more floors
	// see MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	if (MAP_MAX_LAYER < 16) {
		for (int i = 0; i < MAP_LAYERS; ++i) {
			if (menubar->IsItemChecked(MenuBar::ActionID(MenuBar::FLOOR_0 + i))) {
				g_gui.ChangeFloor(i);
			}
		}
	}
}
