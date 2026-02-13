//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_MAP_STATISTICS_DIALOG_H_
#define RME_UI_MAP_STATISTICS_DIALOG_H_

#include "app/main.h"
#include <wx/dialog.h>
#include "map/map_statistics.h"

class Map;

class MapStatisticsDialog : public wxDialog {
public:
	MapStatisticsDialog(wxWindow* parent, Map* map);

private:
	void createUI();
	void OnExport(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	Map* map;
	MapStatistics stats;
};

#endif
