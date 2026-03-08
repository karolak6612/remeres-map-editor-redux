#ifndef RME_STARTUP_LIST_BOX_H_
#define RME_STARTUP_LIST_BOX_H_

#include <unordered_map>
#include <vector>

#include <wx/vlbox.h>

#include "ui/welcome/startup_types.h"

class StartupListBox : public wxVListBox {
public:
	StartupListBox(wxWindow* parent, wxWindowID id);

	void SetItems(std::vector<StartupListItem> items);
	[[nodiscard]] const StartupListItem* GetItem(size_t index) const;

private:
	void OnDrawBackground(wxDC& dc, const wxRect& rect, size_t index) const override;
	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const override;
	wxCoord OnMeasureItem(size_t index) const override;

	[[nodiscard]] wxBitmap GetIconBitmap(const std::string& art_id) const;

	std::vector<StartupListItem> m_items;
	mutable std::unordered_map<std::string, wxBitmap> m_icon_cache;
};

#endif
