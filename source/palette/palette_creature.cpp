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

#include "palette/palette_creature.h"
#include "palette/panels/brush_panel.h"
#include "brushes/creature/creature_brush.h"
#include "game/creatures.h"

#include "app/settings.h"
#include "brushes/brush.h"
#include "editor/editor.h"
#include "ui/gui.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/spawn/npc_spawn_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/zone/zone_brush.h"
#include "game/materials.h"
#include "util/image_manager.h"

// ============================================================================
// Creature palette

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	handling_event(false) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	choicebook = newd wxChoicebook(this, wxID_ANY);
	topsizer->Add(choicebook, 1, wxEXPAND);

	// Footer for brushes and settings
	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Brushes");

	wxFlexGridSizer* grid = newd wxFlexGridSizer(3, 10, 10);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), wxID_ANY, "Spawntime"));
	creature_spawntime_spin = newd wxSpinCtrl(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_CREATURE_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_SPAWNTIME)), wxDefaultPosition, FROM_DIP(this, wxSize(64, 20)), wxSP_ARROW_KEYS, 0, 86400, g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	creature_spawntime_spin->SetToolTip("Spawn time (seconds)");
	grid->Add(creature_spawntime_spin, 0, wxEXPAND);
	creature_brush_button = newd wxToggleButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_CREATURE_BRUSH_BUTTON, "Place Creature");
	creature_brush_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_DRAGON, wxSize(16, 16)));
	creature_brush_button->SetToolTip("Place Creature");
	grid->Add(creature_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), wxID_ANY, "Spawn size"));
	spawn_size_spin = newd wxSpinCtrl(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_CREATURE_SPAWN_SIZE, i2ws(5), wxDefaultPosition, FROM_DIP(this, wxSize(64, 20)), wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
	spawn_size_spin->SetToolTip("Spawn radius");
	grid->Add(spawn_size_spin, 0, wxEXPAND);
	spawn_brush_button = newd wxToggleButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_SPAWN_BRUSH_BUTTON, "Place Spawn");
	spawn_brush_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_FIRE, wxSize(16, 16)));
	spawn_brush_button->SetToolTip("Place Spawn");
	grid->Add(spawn_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), wxID_ANY, "NPC time"));
	npc_spawntime_spin = newd wxSpinCtrl(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_NPC_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_NPC_SPAWNTIME)), wxDefaultPosition, FROM_DIP(this, wxSize(64, 20)), wxSP_ARROW_KEYS, 0, 86400, g_settings.getInteger(Config::DEFAULT_NPC_SPAWNTIME));
	npc_spawntime_spin->SetToolTip("NPC spawn time (seconds)");
	grid->Add(npc_spawntime_spin, 0, wxEXPAND);
	npc_spawn_brush_button = newd wxToggleButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_NPC_SPAWN_BRUSH_BUTTON, "Place NPC Spawn");
	npc_spawn_brush_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_USER, wxSize(16, 16)));
	npc_spawn_brush_button->SetToolTip("Place NPC Spawn");
	grid->Add(npc_spawn_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), wxID_ANY, "NPC size"));
	npc_spawn_size_spin = newd wxSpinCtrl(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_NPC_SPAWN_SIZE, i2ws(g_settings.getInteger(Config::CURRENT_NPC_SPAWN_RADIUS)), wxDefaultPosition, FROM_DIP(this, wxSize(64, 20)), wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), g_settings.getInteger(Config::CURRENT_NPC_SPAWN_RADIUS));
	npc_spawn_size_spin->SetToolTip("NPC spawn radius");
	grid->Add(npc_spawn_size_spin, 0, wxEXPAND);
	zone_brush_button = newd wxToggleButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_ZONE_BRUSH_BUTTON, "Place Zone");
	zone_brush_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MARKER, wxSize(16, 16)));
	zone_brush_button->SetToolTip("Place Zone");
	grid->Add(zone_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), wxID_ANY, "Zone"));
	zone_name_combo = newd wxComboBox(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_ZONE_NAME);
	zone_name_combo->SetToolTip("Select or type the zone name to paint");
	grid->Add(zone_name_combo, 0, wxEXPAND);
	grid->AddSpacer(0);

	sidesizer->Add(grid, 0, wxEXPAND);
	topsizer->Add(sidesizer, 0, wxEXPAND);

	SetSizerAndFit(topsizer);

	Bind(wxEVT_CHOICEBOOK_PAGE_CHANGING, &CreaturePalettePanel::OnSwitchingPage, this);
	Bind(wxEVT_CHOICEBOOK_PAGE_CHANGED, &CreaturePalettePanel::OnPageChanged, this);

	Bind(wxEVT_TOGGLEBUTTON, &CreaturePalettePanel::OnClickCreatureBrushButton, this, PALETTE_CREATURE_BRUSH_BUTTON);
	Bind(wxEVT_TOGGLEBUTTON, &CreaturePalettePanel::OnClickSpawnBrushButton, this, PALETTE_SPAWN_BRUSH_BUTTON);
	Bind(wxEVT_TOGGLEBUTTON, &CreaturePalettePanel::OnClickNpcSpawnBrushButton, this, PALETTE_NPC_SPAWN_BRUSH_BUTTON);
	Bind(wxEVT_TOGGLEBUTTON, &CreaturePalettePanel::OnClickZoneBrushButton, this, PALETTE_ZONE_BRUSH_BUTTON);

	Bind(wxEVT_SPINCTRL, &CreaturePalettePanel::OnChangeSpawnTime, this, PALETTE_CREATURE_SPAWN_TIME);
	Bind(wxEVT_SPINCTRL, &CreaturePalettePanel::OnChangeSpawnSize, this, PALETTE_CREATURE_SPAWN_SIZE);
	Bind(wxEVT_SPINCTRL, &CreaturePalettePanel::OnChangeNpcSpawnTime, this, PALETTE_NPC_SPAWN_TIME);
	Bind(wxEVT_SPINCTRL, &CreaturePalettePanel::OnChangeNpcSpawnSize, this, PALETTE_NPC_SPAWN_SIZE);
	Bind(wxEVT_TEXT, &CreaturePalettePanel::OnZoneNameChanged, this, PALETTE_ZONE_NAME);

	OnUpdate();
}

PaletteType CreaturePalettePanel::GetType() const {
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush() {
	SelectCreatureBrush();
}

Brush* CreaturePalettePanel::GetSelectedBrush() const {
	if (creature_brush_button->GetValue()) {
		if (choicebook->GetPageCount() == 0) {
			return nullptr;
		}
		BrushPanel* bp = reinterpret_cast<BrushPanel*>(choicebook->GetCurrentPage());
		Brush* brush = bp->GetSelectedBrush();
		if (brush && brush->is<CreatureBrush>()) {
			if (const auto* creature_brush = brush->as<CreatureBrush>(); creature_brush && creature_brush->getType() && creature_brush->getType()->isNpc) {
				g_brush_manager.SetNpcSpawnTime(npc_spawntime_spin->GetValue());
			} else {
				g_brush_manager.SetSpawnTime(creature_spawntime_spin->GetValue());
			}
			return brush;
		}
	} else if (spawn_brush_button->GetValue()) {
		g_settings.setInteger(Config::CURRENT_SPAWN_RADIUS, spawn_size_spin->GetValue());
		g_settings.setInteger(Config::DEFAULT_SPAWNTIME, creature_spawntime_spin->GetValue());
		return g_brush_manager.spawn_brush;
	} else if (npc_spawn_brush_button->GetValue()) {
		g_settings.setInteger(Config::CURRENT_NPC_SPAWN_RADIUS, npc_spawn_size_spin->GetValue());
		g_settings.setInteger(Config::DEFAULT_NPC_SPAWNTIME, npc_spawntime_spin->GetValue());
		g_brush_manager.SetNpcSpawnTime(npc_spawntime_spin->GetValue());
		return g_brush_manager.npc_spawn_brush;
	} else if (zone_brush_button->GetValue()) {
		g_brush_manager.SetSelectedZone(nstr(zone_name_combo->GetValue()));
		return g_brush_manager.zone_brush;
	}
	return nullptr;
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
		return false;
	}

	if (whatbrush->is<CreatureBrush>()) {
		for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
			BrushPanel* bp = reinterpret_cast<BrushPanel*>(choicebook->GetPage(i));
			if (bp->SelectBrush(whatbrush)) {
				if (choicebook->GetSelection() != i) {
					choicebook->SetSelection(i);
				}
				SelectCreatureBrush();
				return true;
			}
		}
	} else if (whatbrush->is<SpawnBrush>()) {
		SelectSpawnBrush();
		return true;
	} else if (whatbrush->is<NpcSpawnBrush>()) {
		SelectNpcSpawnBrush();
		return true;
	} else if (whatbrush->is<ZoneBrush>()) {
		SelectZoneBrush();
		return true;
	}
	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const {
	if (npc_spawn_brush_button->GetValue()) {
		return npc_spawn_size_spin->GetValue();
	}
	return spawn_size_spin->GetValue();
}

void CreaturePalettePanel::OnUpdate() {
	choicebook->DeleteAllPages();
	g_materials.createOtherTileset();

	const BrushListType ltype = (BrushListType)g_settings.getInteger(Config::PALETTE_CREATURE_STYLE);

	for (const auto& tileset : GetSortedTilesets(g_materials.tilesets)) {
		const TilesetCategory* tsc = tileset->getCategory(TILESET_CREATURE);
		if ((tsc && tsc->size() > 0) || tileset->name == "NPCs" || tileset->name == "Others") {
			BrushPanel* bp = newd BrushPanel(choicebook);
			bp->SetListType(ltype);
			bp->AssignTileset(tsc);
			bp->LoadContents();
			choicebook->AddPage(bp, wxstr(tileset->name));
		}
	}
	if (choicebook->GetPageCount() > 0) {
		choicebook->SetSelection(0);
	}
	RefreshZoneChoices();
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size) {
	return spawn_size_spin->SetValue(size);
}

void CreaturePalettePanel::OnSwitchIn() {
	g_gui.ActivatePalette(GetParentPalette());
	if (npc_spawn_brush_button->GetValue()) {
		g_gui.SetBrushSize(npc_spawn_size_spin->GetValue());
	} else if (spawn_brush_button->GetValue()) {
		g_gui.SetBrushSize(spawn_size_spin->GetValue());
	}
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
	if (choicebook->GetPageCount() > 0) {
		BrushPanel* bp = reinterpret_cast<BrushPanel*>(choicebook->GetPage(choicebook->GetSelection()));
		// BrushPanel doesn't easily expose selection by index,
		// but since this is usually used for "SelectFirstBrush" (index 0),
		// we can just select the first brush if bp is valid.
		bp->SelectFirstBrush();
		SelectCreatureBrush();
	}
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	// Better approach: use g_creatures to find the brush
	// and then call the existing SelectBrush(Brush*)
	if (CreatureType* ct = g_creatures[name]) {
		if (ct->brush) {
			SelectBrush(ct->brush);
		}
	}
}

void CreaturePalettePanel::SelectCreatureBrush() {
	if (choicebook->GetPageCount() > 0) {
		creature_brush_button->Enable(true);
		creature_brush_button->SetValue(true);
		spawn_brush_button->SetValue(false);
		npc_spawn_brush_button->SetValue(false);
		zone_brush_button->SetValue(false);
	} else {
		creature_brush_button->Enable(false);
		SelectSpawnBrush();
	}
}

void CreaturePalettePanel::SelectSpawnBrush() {
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(true);
	npc_spawn_brush_button->SetValue(false);
	zone_brush_button->SetValue(false);
	g_gui.SetBrushSize(spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectNpcSpawnBrush() {
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(false);
	npc_spawn_brush_button->SetValue(true);
	zone_brush_button->SetValue(false);
	g_gui.SetBrushSize(npc_spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectZoneBrush() {
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(false);
	npc_spawn_brush_button->SetValue(false);
	zone_brush_button->SetValue(true);
	if (zone_name_combo->GetValue().IsEmpty()) {
		zone_name_combo->SetValue("Zone 1");
	}
	g_brush_manager.SetSelectedZone(nstr(zone_name_combo->GetValue()));
}

void CreaturePalettePanel::RefreshZoneChoices() {
	const wxString previous_value = zone_name_combo->GetValue();
	zone_name_combo->Freeze();
	zone_name_combo->Clear();

	if (Editor* editor = g_gui.GetCurrentEditor()) {
		for (const auto& [name, id] : editor->map.zones) {
			(void)id;
			zone_name_combo->Append(wxstr(name));
		}
	}

	if (!previous_value.IsEmpty()) {
		if (zone_name_combo->FindString(previous_value) == wxNOT_FOUND) {
			zone_name_combo->Append(previous_value);
		}
		zone_name_combo->SetValue(previous_value);
	} else if (zone_name_combo->GetCount() > 0) {
		zone_name_combo->SetSelection(0);
	} else {
		zone_name_combo->SetValue("Zone 1");
	}

	g_brush_manager.SetSelectedZone(nstr(zone_name_combo->GetValue()));
	zone_name_combo->Thaw();
}

void CreaturePalettePanel::OnSwitchingPage(wxChoicebookEvent& event) {
	// Do nothing
}

void CreaturePalettePanel::OnPageChanged(wxChoicebookEvent& event) {
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickCreatureBrushButton(wxCommandEvent& event) {
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickSpawnBrushButton(wxCommandEvent& event) {
	SelectSpawnBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickNpcSpawnBrushButton(wxCommandEvent& event) {
	SelectNpcSpawnBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickZoneBrushButton(wxCommandEvent& event) {
	SelectZoneBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnChangeSpawnTime(wxSpinEvent& event) {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeSpawnSize(wxSpinEvent& event) {
	if (!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		if (spawn_brush_button->GetValue()) {
			g_gui.SetBrushSize(event.GetPosition());
		}
		handling_event = false;
	}
}

void CreaturePalettePanel::OnChangeNpcSpawnTime(wxSpinEvent& event) {
	g_brush_manager.SetNpcSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeNpcSpawnSize(wxSpinEvent& event) {
	if (!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		if (npc_spawn_brush_button->GetValue()) {
			g_gui.SetBrushSize(event.GetPosition());
		}
		handling_event = false;
	}
}

void CreaturePalettePanel::OnZoneNameChanged(wxCommandEvent& event) {
	g_brush_manager.SetSelectedZone(nstr(zone_name_combo->GetValue()));
	if (zone_brush_button->GetValue()) {
		g_gui.ActivatePalette(GetParentPalette());
		g_gui.SelectBrush();
	}
}
