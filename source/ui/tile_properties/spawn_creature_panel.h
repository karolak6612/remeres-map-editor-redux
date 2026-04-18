//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_SPAWN_CREATURE_PANEL_H_
#define RME_UI_SPAWN_CREATURE_PANEL_H_

#include <wx/panel.h>
#include <functional>

class Tile;
class Map;
class wxStaticBitmap;
class wxStaticText;

class SpawnCreaturePanel : public wxPanel {
public:
	SpawnCreaturePanel(wxWindow* parent);
	virtual ~SpawnCreaturePanel();

	void SetTile(Tile* tile);
	void SetTile(Tile* tile, Map* map);

	void SetOnSpawnSelectedCallback(std::function<void()> cb) {
		on_spawn_selected_cb = std::move(cb);
	}
	void SetOnNpcSpawnSelectedCallback(std::function<void()> cb) {
		on_npc_spawn_selected_cb = std::move(cb);
	}
	void SetOnCreatureSelectedCallback(std::function<void()> cb) {
		on_creature_selected_cb = std::move(cb);
	}
	void SetOnZoneSelectedCallback(std::function<void()> cb) {
		on_zone_selected_cb = std::move(cb);
	}

protected:
	void OnSpawnClick(wxMouseEvent& event);
	void OnNpcSpawnClick(wxMouseEvent& event);
	void OnCreatureClick(wxMouseEvent& event);
	void OnZoneClick(wxMouseEvent& event);

	std::function<void()> on_spawn_selected_cb;
	std::function<void()> on_npc_spawn_selected_cb;
	std::function<void()> on_creature_selected_cb;
	std::function<void()> on_zone_selected_cb;

	wxStaticBitmap* spawn_bitmap;
	wxStaticBitmap* npc_spawn_bitmap;
	wxStaticBitmap* creature_bitmap;
	wxStaticBitmap* zone_bitmap;
	wxStaticText* spawn_text;
	wxStaticText* npc_spawn_text;
	wxStaticText* creature_text;
	wxStaticText* zone_text;
};

#endif
