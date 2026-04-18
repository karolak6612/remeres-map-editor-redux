#include "palette/palette_creature.h"

#include "palette/panels/brush_panel.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/spawn/npc_spawn_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "game/creatures.h"

#include "app/settings.h"
#include "brushes/brush.h"
#include "brushes/managers/brush_manager.h"
#include "game/materials.h"
#include "ui/gui.h"

#include <array>
#include <wx/srchctrl.h>

// ============================================================================
// Creature palette

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	search_ctrl = newd wxSearchCtrl(this, wxID_ANY);
	search_ctrl->SetDescriptiveText("Search monsters or NPCs");
	search_ctrl->ShowCancelButton(true);
	topsizer->Add(search_ctrl, 0, wxEXPAND | wxBOTTOM, FromDIP(4));

	choicebook = newd wxChoicebook(this, wxID_ANY);
	topsizer->Add(choicebook, 1, wxEXPAND);

	SetSizerAndFit(topsizer);

	search_ctrl->Bind(wxEVT_TEXT, &CreaturePalettePanel::OnSearchChanged, this);
	search_ctrl->Bind(wxEVT_SEARCHCTRL_CANCEL_BTN, &CreaturePalettePanel::OnSearchChanged, this);
	Bind(wxEVT_CHOICEBOOK_PAGE_CHANGING, &CreaturePalettePanel::OnSwitchingPage, this);
	Bind(wxEVT_CHOICEBOOK_PAGE_CHANGED, &CreaturePalettePanel::OnPageChanged, this);

	OnUpdate();
}

PaletteType CreaturePalettePanel::GetType() const {
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush() {
	if (choicebook->GetPageCount() == 0) {
		return;
	}

	if (auto* panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage())) {
		panel->SelectFirstBrush();
	}
}

Brush* CreaturePalettePanel::GetSelectedBrush() const {
	return GetSelectedCreatureBrush();
}

Brush* CreaturePalettePanel::GetSelectedCreatureBrush() const {
	if (choicebook->GetPageCount() == 0) {
		return nullptr;
	}

	auto* panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (!panel) {
		return nullptr;
	}

	Brush* brush = panel->GetSelectedBrush();
	return brush && brush->is<CreatureBrush>() ? brush : nullptr;
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
		return false;
	}

	if (whatbrush->is<CreatureBrush>()) {
		for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
			auto* panel = reinterpret_cast<BrushPanel*>(choicebook->GetPage(i));
			if (panel->SelectBrush(whatbrush)) {
				if (choicebook->GetSelection() != i) {
					choicebook->SetSelection(i);
				}
				return true;
			}
		}
	} else if (whatbrush->is<SpawnBrush>() || whatbrush->is<NpcSpawnBrush>()) {
		return true;
	}

	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const {
	return g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS);
}

void CreaturePalettePanel::OnUpdate() {
	choicebook->DeleteAllPages();
	g_materials.createOtherTileset();

	const BrushListType ltype = static_cast<BrushListType>(g_settings.getInteger(Config::PALETTE_CREATURE_STYLE));
	constexpr auto preferred_tilesets = std::to_array<std::string_view>({
		"Monsters",
		"NPCs",
	});

	for (const std::string_view tileset_name : preferred_tilesets) {
		auto it = g_materials.tilesets.find(std::string { tileset_name });
		if (it == g_materials.tilesets.end()) {
			continue;
		}

		const TilesetCategory* category = it->second->getCategory(TILESET_CREATURE);
		if (!category) {
			continue;
		}

		auto* panel = newd BrushPanel(choicebook);
		panel->SetListType(ltype);
		panel->AssignTileset(category);
		panel->LoadContents();
		choicebook->AddPage(panel, wxstr(it->second->name));
	}

	if (choicebook->GetPageCount() > 0) {
		choicebook->SetSelection(0);
	}
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size) {
	(void)shape;
	(void)size;
}

void CreaturePalettePanel::OnSwitchIn() {
	g_gui.ActivatePalette(GetParentPalette());
	SyncSpawnControlsToSelection();
}

void CreaturePalettePanel::SelectTileset(size_t index) {
	if (choicebook->GetPageCount() > index) {
		choicebook->SetSelection(index);
	}
}

void CreaturePalettePanel::OnRefreshTilesets() {
	OnUpdate();
}

void CreaturePalettePanel::SetListType(BrushListType ltype) {
	for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
		reinterpret_cast<BrushPanel*>(choicebook->GetPage(i))->SetListType(ltype);
	}
}

void CreaturePalettePanel::SetListType(wxString ltype) {
	if (ltype == "Icons") {
		SetListType(BRUSHLIST_LARGE_ICONS);
	} else if (ltype == "List") {
		SetListType(BRUSHLIST_LISTBOX);
	}
}

void CreaturePalettePanel::SelectCreature(size_t index) {
	(void)index;
	if (choicebook->GetPageCount() > 0) {
		auto* panel = reinterpret_cast<BrushPanel*>(choicebook->GetPage(choicebook->GetSelection()));
		panel->SelectFirstBrush();
	}
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	if (CreatureType* creature_type = g_creatures[name]) {
		if (creature_type->brush) {
			SelectBrush(creature_type->brush);
		}
	}
}

void CreaturePalettePanel::SelectCreatureFromSearch(const wxString& query) {
	const std::string filter = as_lower_str(nstr(query));
	if (filter.empty()) {
		return;
	}

	for (const auto& [_, creature_type] : g_creatures) {
		if (!creature_type || !creature_type->brush) {
			continue;
		}

		if (!wxstr(as_lower_str(creature_type->name)).Contains(wxstr(filter))) {
			continue;
		}

		SelectBrush(creature_type->brush);
		return;
	}
}

bool CreaturePalettePanel::IsNpcPageSelected() const {
	const int selection = choicebook ? choicebook->GetSelection() : wxNOT_FOUND;
	if (!choicebook || selection == wxNOT_FOUND) {
		return false;
	}

	return choicebook->GetPageText(static_cast<size_t>(selection)).CmpNoCase("NPCs") == 0;
}

void CreaturePalettePanel::SyncSpawnControlsToSelection() const {
	if (IsNpcPageSelected()) {
		g_brush_manager.SetNpcSpawnTime(g_settings.getInteger(Config::DEFAULT_NPC_SPAWNTIME));
		g_gui.SetBrushSize(g_settings.getInteger(Config::CURRENT_NPC_SPAWN_RADIUS));
		return;
	}

	g_brush_manager.SetSpawnTime(g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	g_gui.SetBrushSize(g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
}

void CreaturePalettePanel::OnSwitchingPage(wxChoicebookEvent& event) {
	(void)event;
}

void CreaturePalettePanel::OnPageChanged(wxChoicebookEvent& event) {
	(void)event;
	SyncSpawnControlsToSelection();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnSearchChanged(wxCommandEvent& event) {
	SelectCreatureFromSearch(event.GetString());
}
