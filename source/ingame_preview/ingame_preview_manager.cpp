#include <iostream>
#include "ingame_preview/ingame_preview_manager.h"
#include "ingame_preview/ingame_preview_window.h"
#include "util/image_manager.h"

#include "ui/gui.h"
#include "editor/editor.h"
#include <wx/aui/aui.h>

IngamePreview::IngamePreviewManager g_preview;

namespace IngamePreview {

	IngamePreviewManager::IngamePreviewManager() {
	}

	IngamePreviewManager::~IngamePreviewManager() {
		spdlog::debug("IngamePreviewManager destructor started");
		spdlog::default_logger()->flush();
		if (window) {
			if (g_gui.aui_manager) {
				spdlog::debug("IngamePreviewManager destructor - detaching window from aui_manager");
				spdlog::default_logger()->flush();
				g_gui.aui_manager->DetachPane(window);
			}
			// If window is still valid here (not destroyed by parent), destroy it.
			// However, since we bind to wxEVT_DESTROY, 'window' should be null if it was already destroyed.
			// If we are here, it means we own it or it's still alive.
			// But since it has a parent (g_gui.root), it should be destroyed by parent usually.
			// Explicitly destroying it is safe if it's top level or we want to force it.
			// Given this is a global destructor, we should be careful.
			// But for safety, we just nullify if we don't own it exclusively.
			// Actually, let's trust the parent or explicit Destroy() call.
		}
		spdlog::debug("IngamePreviewManager destructor finished");
		spdlog::default_logger()->flush();
	}

	void IngamePreviewManager::Create() {

		if (!g_gui.aui_manager) {
			return;
		}

		if (window) {
			g_gui.aui_manager->GetPane(window).Show(true);
		} else {
			window = new IngamePreviewWindow(g_gui.root);

			// Safety: Listen for window destruction (e.g. if parent is destroyed)
			window->Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
				if (e.GetEventObject() == window) {
					window = nullptr;
				}
				e.Skip();
			});

			g_gui.aui_manager->AddPane(window, wxAuiPaneInfo().Name("IngamePreview").Caption("In-game Preview").Right().Dockable(true).FloatingSize(400, 300).MinSize(200, 150).Hide());

			g_gui.aui_manager->GetPane(window).Show(true);
		}
		g_gui.aui_manager->Update();
	}

	void IngamePreviewManager::Hide() {
		if (!g_gui.aui_manager) {
			return;
		}

		if (window) {
			g_gui.aui_manager->GetPane(window).Show(false);
			g_gui.aui_manager->Update();
		}
	}

	void IngamePreviewManager::Destroy() {
		spdlog::debug("IngamePreviewManager::Destroy called");
		spdlog::default_logger()->flush();
		if (window) {
			if (g_gui.aui_manager) {
				spdlog::debug("IngamePreviewManager::Destroy - detaching window from aui_manager");
				spdlog::default_logger()->flush();
				g_gui.aui_manager->DetachPane(window);
			}

			spdlog::debug("IngamePreviewManager::Destroy - destroying window");
			spdlog::default_logger()->flush();

			// We must set window to nullptr before calling Destroy() if we want to avoid recursion issues
			// but we need the pointer to call Destroy.
			// The bind handler will set window = nullptr when Destroy() runs.
			// But we can also set it explicitly afterwards if it wasn't immediate (it usually is for Destroy()).
			wxWindow* win = window;
			window = nullptr; // Safety first
			win->Destroy();
		}
		spdlog::debug("IngamePreviewManager::Destroy finished");
		spdlog::default_logger()->flush();
	}

	void IngamePreviewManager::Update() {
		if (window && IsVisible()) {
			window->UpdateState();
		}
	}

	bool IngamePreviewManager::IsVisible() const {
		if (window && g_gui.aui_manager) {
			const wxAuiPaneInfo& pi = g_gui.aui_manager->GetPane(window);
			return pi.IsShown();
		}
		return false;
	}

} // namespace IngamePreview
