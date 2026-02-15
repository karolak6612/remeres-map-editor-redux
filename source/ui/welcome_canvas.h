#ifndef RME_UI_WELCOME_CANVAS_H_
#define RME_UI_WELCOME_CANVAS_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <vector>
#include <string>
#include <wx/bitmap.h>
#include <nanovg.h>

class WelcomeCanvas : public NanoVGCanvas {
public:
	WelcomeCanvas(wxWindow* parent, const wxBitmap& logo, const std::vector<wxString>& recentFiles);
	~WelcomeCanvas() override;

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void OnMouseDown(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnLeave(wxMouseEvent& event);

	void UpdateLayout();

	struct UIElement {
		wxRect rect;
		int id; // wxID_NEW, wxID_OPEN, wxID_PREFERENCES, or index for recent files
		std::string label;
		std::string sublabel; // for recent file path
		int iconId; // texture id
		bool is_recent_file;
		bool is_checkbox;
		bool checked; // for checkbox
	};

	std::vector<UIElement> elements;
	int hover_element_index; // index in elements vector

	wxBitmap m_logoBitmap;
	int m_logoImageId;

	// Icons
	int m_iconNew;
	int m_iconOpen;
	int m_iconPreferences;

	std::vector<wxString> m_recentFiles;

	int GetHoveredElement(int x, int y) const;
	int CreateIconTexture(NVGcontext* vg, const wxString& iconName);
	int CreateBitmapTexture(NVGcontext* vg, const wxBitmap& bmp);

	// Cached colors/paints
	NVGcolor col_sidebar_bg;
	NVGcolor col_content_bg;
	NVGcolor col_text_main;
	NVGcolor col_text_sub;
	NVGcolor col_hover_bg;
	NVGcolor col_button_text;
};

#endif
