//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/managers/minimap_manager.h"
#include "app/managers/version_manager.h"
#include "ui/gui.h"
#include "rendering/ui/minimap_window.h"
#include <wx/aui/aui.h>

MinimapManager g_minimap;

MinimapManager::MinimapManager() :
	minimap(nullptr) {
}

MinimapManager::~MinimapManager() {
	spdlog::info("MinimapManager destructor started");
	spdlog::default_logger()->flush();

	// Root window will destroy minimap window if it's a child.
	// But we should detach it from aui_manager if it exists.
	if (minimap && g_gui.aui_manager) {
		spdlog::info("MinimapManager destructor - detaching minimap from aui_manager");
		spdlog::default_logger()->flush();
		g_gui.aui_manager->DetachPane(minimap);
	}
	spdlog::info("MinimapManager destructor finished");
	spdlog::default_logger()->flush();
}

void MinimapManager::Create() {
	if (!g_version.IsVersionLoaded()) {
		return;
	}

	if (minimap) {
		g_gui.aui_manager->GetPane(minimap).Show(true);
	} else {
		minimap = newd MinimapWindow(g_gui.root);
		minimap->Show(true);
		g_gui.aui_manager->AddPane(minimap, wxAuiPaneInfo().Caption("Minimap"));
	}
	g_gui.aui_manager->Update();
}

void MinimapManager::Hide() {
	if (minimap) {
		g_gui.aui_manager->GetPane(minimap).Show(false);
		g_gui.aui_manager->Update();
	}
}

void MinimapManager::Destroy() {
	spdlog::info("MinimapManager::Destroy called");
	spdlog::default_logger()->flush();
	if (minimap) {
		if (g_gui.aui_manager) {
			spdlog::info("MinimapManager::Destroy - detaching minimap from aui_manager");
			spdlog::default_logger()->flush();
			g_gui.aui_manager->DetachPane(minimap);

			spdlog::info("MinimapManager::Destroy - updating aui_manager");
			spdlog::default_logger()->flush();
			g_gui.aui_manager->Update();
		}

		spdlog::info("MinimapManager::Destroy - calling minimap->Destroy()");
		spdlog::default_logger()->flush();
		minimap->Destroy();

		spdlog::info("MinimapManager::Destroy - resetting minimap pointer");
		spdlog::default_logger()->flush();
		minimap = nullptr;
	}
	spdlog::info("MinimapManager::Destroy finished");
	spdlog::default_logger()->flush();
}

void MinimapManager::Update(bool immediate) {
	if (IsVisible()) {
		if (immediate) {
			minimap->Refresh();
		} else {
			minimap->DelayedUpdate();
		}
	}
}

bool MinimapManager::IsVisible() const {
	if (minimap && g_gui.aui_manager) {
		const wxAuiPaneInfo& pi = g_gui.aui_manager->GetPane(minimap);
		if (pi.IsShown()) {
			return true;
		}
	}
	return false;
}
