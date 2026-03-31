#ifndef RME_PREFERENCES_VISUALS_PAGE_PREVIEW_H_
#define RME_PREFERENCES_VISUALS_PAGE_PREVIEW_H_

#include "app/visuals.h"

#include <optional>

#include <wx/bitmap.h>
#include <wx/panel.h>

class VisualsPreviewPanel final : public wxPanel {
public:
	explicit VisualsPreviewPanel(wxWindow* parent, int edge = 32);

	void SetRule(std::optional<VisualRule> rule);

private:
	void RebuildCache();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

	std::optional<VisualRule> rule_;
	wxBitmap cached_bitmap_;
	bool cache_dirty_ = true;
};

#endif
