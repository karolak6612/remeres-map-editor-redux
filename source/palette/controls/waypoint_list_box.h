//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_WAYPOINT_LIST_BOX_H_
#define RME_WAYPOINT_LIST_BOX_H_

#include "app/main.h"
#include "util/nanovg_listbox.h"
#include <vector>
#include <string>

class WaypointListBox : public NanoVGListBox {
public:
	WaypointListBox(wxWindow* parent, wxWindowID id);
	~WaypointListBox();

	void SetWaypoints(const std::vector<std::string>& names);
	wxString GetSelectedWaypoint() const;

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

protected:
	std::vector<std::string> m_items;
};

#endif
