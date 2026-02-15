#ifndef RME_UI_CONTROLS_VIRTUAL_BRUSH_LIST_H_
#define RME_UI_CONTROLS_VIRTUAL_BRUSH_LIST_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <vector>
#include <unordered_map>

class Brush;

class VirtualBrushList : public NanoVGCanvas {
public:
	VirtualBrushList(wxWindow* parent, wxWindowID id = wxID_ANY);
	~VirtualBrushList() override;

	void Clear();
	void SetNoMatches();
	void AddBrush(Brush* brush);
	Brush* GetSelectedBrush() const;
	void SetSelection(int index);
	int GetSelection() const;
	int GetItemCount() const;

	// Optimization: Call this after adding multiple brushes to update scrollbar once
	void RecalculateLayout();

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void OnMouseDown(wxMouseEvent& event);
	void OnMouseDoubleClick(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void UpdateLayout();
	int HitTest(int x, int y) const;
	wxRect GetItemRect(int index) const;

	bool cleared;
	bool no_matches;
	std::vector<Brush*> brushlist;
	int selected_index;
	int hover_index;
	int item_height;

	static constexpr int PADDING = 2;

	// UTF8 name cache
	mutable std::unordered_map<const Brush*, std::string> m_utf8NameCache;
};

#endif
