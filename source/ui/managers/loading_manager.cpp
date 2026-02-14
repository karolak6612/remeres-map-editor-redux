//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/managers/loading_manager.h"
#include "ui/dialogs/nanovg_loading_dialog.h" // Include new dialog

#include "ui/gui.h"
#include "editor/editor.h"
#include "ui/map_tab.h"
#include "live/live_server.h"

LoadingManager g_loading;

LoadingManager::LoadingManager() :
	progressBar(nullptr),
	progressFrom(0),
	progressTo(100),
	currentProgress(-1) {
}

LoadingManager::~LoadingManager() {
	DestroyLoadBar();
}

void LoadingManager::CreateLoadBar(wxString message, bool canCancel) {
	progressText = message;

	progressFrom = 0;
	progressTo = 100;
	currentProgress = -1;

	// Use NanoVGLoadingDialog instead of wxGenericProgressDialog
	progressBar = newd NanoVGLoadingDialog(g_gui.root, "Loading", progressText);
	progressBar->Show();

	if (g_gui.tabbook) {
		for (int idx = 0; idx < g_gui.tabbook->GetTabCount(); ++idx) {
			auto* mt = dynamic_cast<MapTab*>(g_gui.tabbook->GetTab(idx));
			if (mt && mt->GetEditor()->live_manager.IsServer()) {
				mt->GetEditor()->live_manager.GetServer()->startOperation(progressText);
			}
		}
	}
	progressBar->Update(0, progressText);
}

void LoadingManager::SetLoadScale(int32_t from, int32_t to) {
	progressFrom = from;
	progressTo = to;
}

bool LoadingManager::SetLoadDone(int32_t done, const wxString& newMessage) {
	if (done == 100) {
		DestroyLoadBar();
		return true;
	} else if (done == currentProgress) {
		return true;
	}

	if (!newMessage.empty()) {
		progressText = newMessage;
	}

	int32_t newProgress = progressFrom + static_cast<int32_t>((done / 100.f) * (progressTo - progressFrom));
	newProgress = std::max<int32_t>(0, std::min<int32_t>(100, newProgress));

	bool continueLoading = true;
	if (progressBar) {
		// Update NanoVG dialog
		// Pass progressText only if it changed?
		// Update(value, msg) uses msg if not empty.
		continueLoading = progressBar->Update(newProgress, progressText);
		currentProgress = newProgress;
	}

	if (g_gui.tabbook) {
		for (int32_t index = 0; index < g_gui.tabbook->GetTabCount(); ++index) {
			auto* mapTab = dynamic_cast<MapTab*>(g_gui.tabbook->GetTab(index));
			if (mapTab && mapTab->GetEditor()) {
				LiveServer* server = mapTab->GetEditor()->live_manager.GetServer();
				if (server) {
					server->updateOperation(newProgress);
				}
			}
		}
	}

	return continueLoading;
}

void LoadingManager::DestroyLoadBar() {
	if (progressBar) {
		progressBar->Show(false);
		currentProgress = -1;

		progressBar->Destroy();
		progressBar = nullptr;

		if (g_gui.root && g_gui.root->IsActive()) {
			g_gui.root->Raise();
		} else if (g_gui.root) {
			g_gui.root->RequestUserAttention();
		}
	}
}
