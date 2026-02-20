//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_CONTAINER_PROPERTY_PANEL_H_
#define RME_UI_CONTAINER_PROPERTY_PANEL_H_

#include "ui/tile_properties/item_property_panel.h"
#include <vector>

class ContainerItemButton;

class ContainerPropertyPanel : public ItemPropertyPanel {
public:
	ContainerPropertyPanel(wxWindow* parent);
	virtual ~ContainerPropertyPanel();

	void SetItem(Item* item, Tile* tile, Map* map) override;

	void OnContainerItemClick(wxCommandEvent& event);
	void OnContainerItemRightClick(wxMouseEvent& event);

	void OnAddItem(wxCommandEvent& event);
	void OnEditItem(wxCommandEvent& event);
	void OnRemoveItem(wxCommandEvent& event);

protected:
	void RebuildGrid();

	wxStaticBoxSizer* contents_sizer;
	std::vector<ContainerItemButton*> container_items;
	ContainerItemButton* last_clicked_button;
};

#endif
