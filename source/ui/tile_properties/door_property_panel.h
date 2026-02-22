//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_DOOR_PROPERTY_PANEL_H_
#define RME_UI_DOOR_PROPERTY_PANEL_H_

#include "ui/tile_properties/item_property_panel.h"
#include <wx/spinctrl.h>

class DoorPropertyPanel : public ItemPropertyPanel {
public:
	DoorPropertyPanel(wxWindow* parent);
	virtual ~DoorPropertyPanel();

	void SetItem(Item* item, Tile* tile, Map* map) override;

	void OnDoorIdChange(wxSpinEvent& event);

protected:
	wxStaticBoxSizer* door_sizer;
	wxSpinCtrl* door_id_spin;
};

#endif
