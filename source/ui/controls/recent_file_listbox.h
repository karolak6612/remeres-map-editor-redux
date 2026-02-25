#ifndef RME_UI_CONTROLS_RECENT_FILE_LISTBOX_H_
#define RME_UI_CONTROLS_RECENT_FILE_LISTBOX_H_

#include "util/nanovg_listbox.h"
#include <wx/filename.h>

struct RecentFileItem {
	wxString mapName;
	wxString path;
	wxString date;
	wxString fullPath;
};

class RecentFileListBox : public NanoVGListBox {
public:
	RecentFileListBox(wxWindow* parent, wxWindowID id);
	virtual ~RecentFileListBox();

	void SetRecentFiles(const std::vector<wxString>& files);
	wxString GetSelectedFile() const;

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

	// Override to skip default background drawing
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;

protected:
	void OnMouseDoubleClick(wxMouseEvent& event);

	std::vector<RecentFileItem> m_items;

	// Helper to load icon
	int m_iconImage = 0;
};

#endif
