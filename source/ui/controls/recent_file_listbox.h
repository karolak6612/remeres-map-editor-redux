#ifndef RME_UI_CONTROLS_RECENT_FILE_LISTBOX_H_
#define RME_UI_CONTROLS_RECENT_FILE_LISTBOX_H_

#include "util/nanovg_listbox.h"
#include <wx/string.h>
#include <vector>

struct RecentFile {
	wxString path;
	wxString name;
	wxString date;
};

class RecentFileListBox : public NanoVGListBox {
public:
	RecentFileListBox(wxWindow* parent, wxWindowID id);
	~RecentFileListBox();

	void SetRecentFiles(const std::vector<wxString>& files);
	wxString GetFilePath(int index) const;

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

private:
	std::vector<RecentFile> m_files;
};

#endif
