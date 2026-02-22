//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_TELEPORT_PROPERTY_PANEL_H_
#define RME_UI_TELEPORT_PROPERTY_PANEL_H_

#include "ui/tile_properties/item_property_panel.h"
#include <wx/spinctrl.h>

class TeleportPropertyPanel : public ItemPropertyPanel {
public:
	TeleportPropertyPanel(wxWindow* parent);
	virtual ~TeleportPropertyPanel();

	void SetItem(Item* item, Tile* tile, Map* map) override;

	void OnDestChange(wxSpinEvent& event);

protected:
	wxStaticBoxSizer* dest_sizer;
	wxSpinCtrl* x_spin;
	wxSpinCtrl* y_spin;
	wxSpinCtrl* z_spin;
};

#endif
