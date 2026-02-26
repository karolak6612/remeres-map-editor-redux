//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RECENT_FILE_LIST_BOX_H_
#define RME_RECENT_FILE_LIST_BOX_H_

#include "app/main.h"
#include "util/nanovg_listbox.h"
#include <vector>
#include <string>

struct RecentFileItem {
	wxString path;
	wxString date;
};

class RecentFileListBox : public NanoVGListBox {
public:
	RecentFileListBox(wxWindow* parent, wxWindowID id);
	~RecentFileListBox();

	void SetRecentFiles(const std::vector<wxString>& files);
	wxString GetSelectedFile() const;

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

protected:
	std::vector<RecentFileItem> m_items;
	int m_iconImage;
};

#endif
