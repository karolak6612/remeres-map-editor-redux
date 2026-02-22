//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "ui/palettes/managers/palette_manager.h"
#include "app/managers/version_manager.h"
#include "ui/gui.h"
#include "ui/palettes/palette_window.h"
#include "game/materials.h"
#include "brushes/managers/brush_manager.h"

PaletteManager g_palettes;

PaletteManager::PaletteManager() :
	palette_creation_counter(0) {
}

PaletteManager::~PaletteManager() {
	spdlog::info("PaletteManager destructor started");
	spdlog::default_logger()->flush();

	spdlog::info("PaletteManager destructor finished");
	spdlog::default_logger()->flush();
}

PaletteWindow* PaletteManager::GetPalette() {
	if (palettes.empty()) {
		return nullptr;
	}
	return palettes.front();
}

PaletteWindow* PaletteManager::NewPalette() {
	return CreatePalette();
}

void PaletteManager::ActivatePalette(PaletteWindow* p) {
	auto it = std::find(palettes.begin(), palettes.end(), p);
	if (it != palettes.end()) {
		palettes.erase(it);
		palettes.push_front(p);
	}
}

void PaletteManager::RebuildPalettes() {
	std::list<PaletteWindow*> tmp = palettes;
	for (auto& piter : tmp) {
		piter->ReloadSettings(g_gui.IsEditorOpen() ? &g_gui.GetCurrentMap() : nullptr);
	}
	g_gui.aui_manager->Update();
}

void PaletteManager::RefreshPalettes(Map* m, bool usedefault) {
	for (auto& palette : palettes) {
		palette->OnUpdate(m ? m : (usedefault ? (g_gui.IsEditorOpen() ? &g_gui.GetCurrentMap() : nullptr) : nullptr));
	}
	g_brush_manager.SelectBrush();
}

void PaletteManager::RefreshOtherPalettes(PaletteWindow* p) {
	for (auto& palette : palettes) {
		if (palette != p) {
			palette->OnUpdate(g_gui.IsEditorOpen() ? &g_gui.GetCurrentMap() : nullptr);
		}
	}
	g_brush_manager.SelectBrush();
}

void PaletteManager::ShowPalette() {
	if (palettes.empty()) {
		return;
	}

	for (auto& palette : palettes) {
		if (g_gui.aui_manager->GetPane(palette).IsShown()) {
			return;
		}
	}

	g_gui.aui_manager->GetPane(palettes.front()).Show(true);
	g_gui.aui_manager->Update();
}

void PaletteManager::SelectPalettePage(PaletteType pt) {
	if (palettes.empty()) {
		CreatePalette();
	}
	PaletteWindow* p = GetPalette();
	if (!p) {
		return;
	}

	ShowPalette();
	p->SelectPage(pt);
	g_gui.aui_manager->Update();
	g_brush_manager.SelectBrushInternal(p->GetSelectedBrush());
}

void PaletteManager::DestroyPalettes() {
	spdlog::info("PaletteManager::DestroyPalettes called - destroying {} palettes", palettes.size());
	spdlog::default_logger()->flush();
	for (auto palette : palettes) {
		spdlog::info("PaletteManager::DestroyPalettes - detaching and destroying a palette");
		spdlog::default_logger()->flush();
		g_gui.aui_manager->DetachPane(palette);
		palette->Destroy();
	}
	spdlog::info("PaletteManager::DestroyPalettes - clearing palettes list");
	spdlog::default_logger()->flush();
	palettes.clear();

	spdlog::info("PaletteManager::DestroyPalettes - updating aui_manager");
	spdlog::default_logger()->flush();
	g_gui.aui_manager->Update();

	spdlog::info("PaletteManager::DestroyPalettes finished");
	spdlog::default_logger()->flush();
}

PaletteWindow* PaletteManager::CreatePalette() {
	if (!g_version.IsVersionLoaded()) {
		return nullptr;
	}

	auto* palette = newd PaletteWindow(g_gui.root, g_materials.tilesets);
	wxString name = wxString::Format("Palette_%llu", ++palette_creation_counter);
	g_gui.aui_manager->AddPane(palette, wxAuiPaneInfo().Name(name).Caption("Palette").TopDockable(true).BottomDockable(true));
	g_gui.aui_manager->Update();

	palettes.push_front(palette);
	g_brush_manager.SelectBrushInternal(palette->GetSelectedBrush());
	palette->OnUpdate(g_gui.IsEditorOpen() ? &g_gui.GetCurrentMap() : nullptr);
	return palette;
}
