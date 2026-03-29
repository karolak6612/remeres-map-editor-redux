#ifndef RME_PREFERENCES_VISUALS_PAGE_PREVIEW_H_
#define RME_PREFERENCES_VISUALS_PAGE_PREVIEW_H_

#include "app/visuals.h"

#include <optional>

#include <wx/panel.h>

class VisualsPreviewPanel final : public wxPanel {
public:
	explicit VisualsPreviewPanel(wxWindow* parent);

	void SetRule(std::optional<VisualRule> rule);

private:
	void OnPaint(wxPaintEvent& event);

	std::optional<VisualRule> rule_;
};

#endif
