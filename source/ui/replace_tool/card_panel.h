#ifndef RME_UI_REPLACE_TOOL_CARD_PANEL_H_
#define RME_UI_REPLACE_TOOL_CARD_PANEL_H_

#include "util/nanovg_canvas.h"
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/event.h>

class CardPanel : public NanoVGCanvas {
public:
	CardPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

	// Optional: Set specific card style properties if needed
	void SetShowFooter(bool show);
	void SetTitle(const wxString& title);
	wxSizer* GetContentSizer() const {
		return m_contentSizer;
	}
	wxSizer* GetFooterSizer() const {
		return m_footerSizer;
	}

	static const int HEADER_HEIGHT = 40;
	static const int FOOTER_HEIGHT = 40;

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	void OnSize(wxSizeEvent& event);

private:
	bool m_showFooter = false;
	wxString m_title;
	std::string m_cachedTitleStr;

	wxBoxSizer* m_mainSizer = nullptr;
	wxBoxSizer* m_contentSizer = nullptr;
	wxBoxSizer* m_footerSizer = nullptr;
};

#endif
