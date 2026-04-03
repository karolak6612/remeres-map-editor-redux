#include "app/main.h"

#include <spdlog/spdlog.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "editor/editor.h"
#include "rendering/drawers/minimap_drawer.h"
#include "rendering/ui/map_display.h"
#include "rendering/ui/minimap_viewport.h"
#include "rendering/ui/minimap_window.h"
#include "ui/gui.h"
#include "ui/map_tab.h"
#include "ui/map_window.h"
#include "util/image_manager.h"

namespace {

wxGLAttributes& GetCoreProfileAttributes() {
	static wxGLAttributes attributes = []() {
		wxGLAttributes attr;
		attr.PlatformDefaults().Defaults().RGBA().DoubleBuffer().Depth(24).Stencil(8).EndList();
		return attr;
	}();
	return attributes;
}

wxButton* createHeaderButton(wxWindow* parent, const wxBitmap& bitmap, const wxString& label = wxString()) {
	auto* button = new wxButton(parent, wxID_ANY, label, wxDefaultPosition, wxSize(24, 22), wxBU_EXACTFIT);
	if (bitmap.IsOk()) {
		button->SetBitmap(bitmap);
	}
	button->SetMinSize(wxSize(24, 22));
	return button;
}

wxPanel* createSeparator(wxWindow* parent) {
	auto* separator = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(1, 18));
	separator->SetMinSize(wxSize(1, 18));
	separator->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
	return separator;
}

} // namespace

MinimapWindow::MinimapWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(260, 200)) {
	auto* root_sizer = new wxBoxSizer(wxVERTICAL);
	auto* header_sizer = new wxBoxSizer(wxHORIZONTAL);

	zoom_out_button_ = createHeaderButton(this, IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(14, 14)));
	zoom_in_button_ = createHeaderButton(this, IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(14, 14)));
	floor_up_button_ = createHeaderButton(this, IMAGE_MANAGER.GetBitmap(ICON_ARROW_UP, wxSize(14, 14)));
	floor_down_button_ = createHeaderButton(this, IMAGE_MANAGER.GetBitmap(ICON_ARROW_DOWN, wxSize(14, 14)));
	help_button_ = createHeaderButton(this, IMAGE_MANAGER.GetBitmap(ICON_QUESTION_CIRCLE, wxSize(14, 14)));
	zoom_label_ = new wxStaticText(this, wxID_ANY, "1:1");
	floor_label_ = new wxStaticText(this, wxID_ANY, "F: 7");

	header_sizer->Add(zoom_out_button_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
	header_sizer->Add(zoom_label_, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 4);
	header_sizer->Add(zoom_in_button_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
	header_sizer->Add(createSeparator(this), 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	header_sizer->Add(floor_label_, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 4);
	header_sizer->Add(floor_up_button_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
	header_sizer->Add(floor_down_button_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
	header_sizer->Add(createSeparator(this), 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	header_sizer->Add(help_button_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
	header_sizer->AddStretchSpacer(1);

	help_panel_ = new wxPanel(this, wxID_ANY);
	auto* help_sizer = new wxBoxSizer(wxVERTICAL);
	help_text_ = new wxStaticText(help_panel_, wxID_ANY,
		"Left drag: pan minimap\n"
		"Ctrl+Left Click: move editor camera\n"
		"Wheel: zoom minimap\n"
		"Ctrl+Wheel: change minimap floor");
	help_sizer->Add(help_text_, 0, wxALL | wxEXPAND, 6);
	help_panel_->SetSizer(help_sizer);
	help_panel_->Hide();

	canvas_ = new MinimapCanvas(this);

	root_sizer->Add(header_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 2);
	root_sizer->Add(help_panel_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 2);
	root_sizer->Add(canvas_, 1, wxEXPAND | wxALL, 2);
	SetSizer(root_sizer);

	Bind(wxEVT_CLOSE_WINDOW, &MinimapWindow::OnClose, this);
	Bind(wxEVT_SIZE, &MinimapWindow::OnSize, this);
	zoom_out_button_->Bind(wxEVT_BUTTON, &MinimapWindow::OnZoomOut, this);
	zoom_in_button_->Bind(wxEVT_BUTTON, &MinimapWindow::OnZoomIn, this);
	floor_up_button_->Bind(wxEVT_BUTTON, &MinimapWindow::OnFloorUp, this);
	floor_down_button_->Bind(wxEVT_BUTTON, &MinimapWindow::OnFloorDown, this);
	help_button_->Bind(wxEVT_BUTTON, &MinimapWindow::OnHelpClick, this);
	help_button_->Bind(wxEVT_ENTER_WINDOW, &MinimapWindow::OnHelpEnter, this);
	help_button_->Bind(wxEVT_LEAVE_WINDOW, &MinimapWindow::OnHelpLeave, this);

	SyncHeaderState();
}

MinimapWindow::~MinimapWindow() {
}

void MinimapWindow::DelayedUpdate() {
	RefreshMinimap(false);
}

void MinimapWindow::RefreshMinimap(bool immediate) {
	EnsureActiveViewportState();
	SyncHeaderState();
	if (!canvas_) {
		return;
	}

	if (immediate) {
		canvas_->Refresh();
	} else {
		canvas_->DelayedUpdate();
	}
}

void MinimapWindow::SyncHeaderState() {
	if (help_text_) {
		help_text_->Wrap(std::max(120, GetClientSize().GetWidth() - 20));
	}

	if (g_gui.IsLoading()) {
		zoom_label_->SetLabel("1:1");
		floor_label_->SetLabel("F: -");
		zoom_out_button_->Enable(false);
		zoom_in_button_->Enable(false);
		floor_up_button_->Enable(false);
		floor_down_button_->Enable(false);
	} else if (auto* state = GetActiveViewportState()) {
		zoom_label_->SetLabel(wxString::FromUTF8(MinimapViewport::GetZoomLabel(state->zoom_step).data()));
		floor_label_->SetLabel(wxString::Format("F: %d", state->floor));
		zoom_out_button_->Enable(state->zoom_step > 0);
		zoom_in_button_->Enable(state->zoom_step < static_cast<int>(MinimapViewport::ZoomFactors.size()) - 1);
		floor_up_button_->Enable(state->floor > 0);
		floor_down_button_->Enable(state->floor < MAP_MAX_LAYER);
	} else {
		zoom_label_->SetLabel("1:1");
		floor_label_->SetLabel("F: -");
		zoom_out_button_->Enable(false);
		zoom_in_button_->Enable(false);
		floor_up_button_->Enable(false);
		floor_down_button_->Enable(false);
	}
}

void MinimapWindow::StepZoom(int delta) {
	if (auto* state = GetActiveViewportState()) {
		const int next_step = MinimapViewport::ClampZoomStep(state->zoom_step + delta);
		if (next_step != state->zoom_step) {
			state->zoom_step = next_step;
			if (canvas_) {
				canvas_->NormalizeViewportState();
				canvas_->Refresh();
			}
		}
	}
	SyncHeaderState();
}

void MinimapWindow::StepFloor(int delta) {
	if (auto* state = GetActiveViewportState()) {
		const int next_floor = MinimapViewport::ClampFloor(state->floor + delta);
		if (next_floor != state->floor) {
			state->floor = next_floor;
			if (canvas_) {
				canvas_->Refresh();
			}
		}
	}
	SyncHeaderState();
}

void MinimapWindow::JumpCameraTo(int map_x, int map_y) {
	auto* state = GetActiveViewportState();
	if (!state) {
		return;
	}

	g_gui.SetScreenCenterPosition(Position(map_x, map_y, state->floor));
	g_gui.RefreshView();
	RefreshMinimap(true);
}

Editor* MinimapWindow::GetActiveEditor() const {
	MapTab* map_tab = g_gui.GetCurrentMapTab();
	return map_tab ? map_tab->GetEditor() : nullptr;
}

MapCanvas* MinimapWindow::GetActiveCanvas() const {
	MapTab* map_tab = g_gui.GetCurrentMapTab();
	return map_tab ? map_tab->GetCanvas() : nullptr;
}

MapWindow* MinimapWindow::GetActiveMapWindow() const {
	MapTab* map_tab = g_gui.GetCurrentMapTab();
	return map_tab ? map_tab->GetView() : nullptr;
}

MinimapViewportState* MinimapWindow::GetActiveViewportState() const {
	if (g_gui.IsLoading()) {
		return nullptr;
	}

	MapWindow* map_window = GetActiveMapWindow();
	if (!map_window) {
		return nullptr;
	}

	if (!map_window->GetMinimapViewportState().initialized) {
		return nullptr;
	}
	return &map_window->GetMinimapViewportState();
}

MinimapViewportState* MinimapWindow::EnsureActiveViewportState() {
	if (g_gui.IsLoading()) {
		return nullptr;
	}

	MapWindow* map_window = GetActiveMapWindow();
	if (!map_window) {
		return nullptr;
	}

	map_window->EnsureMinimapViewportInitialized();
	return &map_window->GetMinimapViewportState();
}

void MinimapWindow::OnClose(wxCloseEvent& event) {
	wxUnusedVar(event);
	g_gui.DestroyMinimap();
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	SyncHeaderState();
	event.Skip();
}

void MinimapWindow::OnZoomOut(wxCommandEvent& event) {
	wxUnusedVar(event);
	StepZoom(-1);
}

void MinimapWindow::OnZoomIn(wxCommandEvent& event) {
	wxUnusedVar(event);
	StepZoom(1);
}

void MinimapWindow::OnFloorUp(wxCommandEvent& event) {
	wxUnusedVar(event);
	StepFloor(-1);
}

void MinimapWindow::OnFloorDown(wxCommandEvent& event) {
	wxUnusedVar(event);
	StepFloor(1);
}

void MinimapWindow::OnHelpEnter(wxMouseEvent& event) {
	help_hovered_ = true;
	UpdateHelpVisibility();
	event.Skip();
}

void MinimapWindow::OnHelpLeave(wxMouseEvent& event) {
	help_hovered_ = false;
	UpdateHelpVisibility();
	event.Skip();
}

void MinimapWindow::OnHelpClick(wxCommandEvent& event) {
	wxUnusedVar(event);
	help_pinned_ = !help_pinned_;
	UpdateHelpVisibility();
}

void MinimapWindow::UpdateHelpVisibility() {
	const bool show_help = help_hovered_ || help_pinned_;
	help_panel_->Show(show_help);
	Layout();
}

MinimapCanvas::MinimapCanvas(MinimapWindow* parent) :
	wxGLCanvas(parent, GetCoreProfileAttributes(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
	owner_(parent),
	drawer(std::make_unique<MinimapDrawer>()),
	update_timer(this) {
	m_glContext = std::make_unique<wxGLContext>(this, g_gui.GetGLContext(this));
	if (!m_glContext || !m_glContext->IsOK()) {
		spdlog::error("MinimapCanvas: Failed to create wxGLContext");
	}

	Bind(wxEVT_LEFT_DOWN, &MinimapCanvas::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &MinimapCanvas::OnLeftUp, this);
	Bind(wxEVT_MOTION, &MinimapCanvas::OnMouseMove, this);
	Bind(wxEVT_MOUSEWHEEL, &MinimapCanvas::OnMouseWheel, this);
	Bind(wxEVT_ENTER_WINDOW, &MinimapCanvas::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &MinimapCanvas::OnMouseLeave, this);
	Bind(wxEVT_SIZE, &MinimapCanvas::OnSize, this);
	Bind(wxEVT_PAINT, &MinimapCanvas::OnPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &MinimapCanvas::OnEraseBackground, this);
	Bind(wxEVT_TIMER, &MinimapCanvas::OnDelayedUpdate, this, wxID_ANY);
	Bind(wxEVT_KEY_DOWN, &MinimapCanvas::OnKey, this);
}

MinimapCanvas::~MinimapCanvas() {
	StopInteraction();

	bool context_ok = false;
	if (m_glContext) {
		context_ok = g_gl_context.EnsureContextCurrent(*m_glContext, this);
	}

	if (context_ok) {
		if (drawer) {
			drawer->ReleaseGL();
		}
	} else {
		spdlog::warn("MinimapCanvas: Destroying without a current OpenGL context. Cleanup might be incomplete.");
	}

	g_gl_context.UnregisterCanvas(this);
}

void MinimapCanvas::DelayedUpdate() {
	update_timer.Start(g_settings.getInteger(Config::MINIMAP_UPDATE_DELAY), true);
}

void MinimapCanvas::NormalizeViewportState() {
	if (auto* state = owner_->GetActiveViewportState()) {
		ClampViewportState(*state);
	}
}

void MinimapCanvas::OnPaint(wxPaintEvent& event) {
	wxUnusedVar(event);
	wxPaintDC dc(this);

	if (!m_glContext) {
		spdlog::error("MinimapCanvas::OnPaint - No context");
		return;
	}

	SetCurrent(*m_glContext);

	static bool glad_initialized = false;
	if (!glad_initialized) {
		if (!gladLoadGL()) {
			spdlog::error("MinimapCanvas::OnPaint - Failed to load GLAD");
			return;
		}
		glad_initialized = true;
	}

	if (g_gui.IsLoading() || !g_gui.IsEditorOpen()) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers();
		return;
	}

	Editor* editor = owner_->GetActiveEditor();
	MapCanvas* active_canvas = owner_->GetActiveCanvas();
	MinimapViewportState* state = owner_->GetActiveViewportState();
	if (!editor || !active_canvas || !state) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers();
		return;
	}

	ClampViewportState(*state);
	drawer->Draw(dc, GetClientSize(), *editor, active_canvas, *state);
	SwapBuffers();
}

void MinimapCanvas::OnDelayedUpdate(wxTimerEvent& event) {
	wxUnusedVar(event);
	Refresh();
}

void MinimapCanvas::OnSize(wxSizeEvent& event) {
	NormalizeViewportState();
	Refresh();
	event.Skip();
}

void MinimapCanvas::OnLeftDown(wxMouseEvent& event) {
	if (g_gui.IsLoading() || !g_gui.IsEditorOpen()) {
		return;
	}

	SetFocus();
	CaptureMouse();

	if (auto* state = owner_->GetActiveViewportState()) {
		if (event.ControlDown()) {
			ctrl_click_pending_ = true;
			is_dragging_ = false;
			drag_origin_ = event.GetPosition();
		} else {
			is_dragging_ = true;
			ctrl_click_pending_ = false;
			drag_origin_ = event.GetPosition();
			drag_origin_center_x_ = state->center_x;
			drag_origin_center_y_ = state->center_y;
			state->tracking_main_camera = false;
		}
	}
}

void MinimapCanvas::OnLeftUp(wxMouseEvent& event) {
	if (g_gui.IsLoading() || !g_gui.IsEditorOpen()) {
		StopInteraction();
		return;
	}

	if (ctrl_click_pending_ && event.ControlDown()) {
		int map_x = 0;
		int map_y = 0;
		drawer->ScreenToMap(event.GetX(), event.GetY(), map_x, map_y);
		owner_->JumpCameraTo(map_x, map_y);
	}

	StopInteraction();
}

void MinimapCanvas::OnMouseMove(wxMouseEvent& event) {
	if (ctrl_click_pending_ && event.LeftIsDown()) {
		const wxPoint delta = event.GetPosition() - drag_origin_;
		if (std::abs(delta.x) > 2 || std::abs(delta.y) > 2) {
			ctrl_click_pending_ = false;
		}
	}

	if (!is_dragging_ || !event.LeftIsDown()) {
		return;
	}

	MinimapViewportState* state = owner_->GetActiveViewportState();
	if (!state) {
		return;
	}

	const wxPoint delta = event.GetPosition() - drag_origin_;
	const double zoom_factor = MinimapViewport::GetZoomFactor(state->zoom_step);
	state->center_x = drag_origin_center_x_ - delta.x * zoom_factor;
	state->center_y = drag_origin_center_y_ - delta.y * zoom_factor;
	ClampViewportState(*state);
	owner_->SyncHeaderState();
	Refresh();
}

void MinimapCanvas::OnMouseWheel(wxMouseEvent& event) {
	if (g_gui.IsLoading() || !g_gui.IsEditorOpen()) {
		return;
	}

	if (event.ControlDown()) {
		owner_->StepFloor(event.GetWheelRotation() > 0 ? -1 : 1);
	} else {
		owner_->StepZoom(event.GetWheelRotation() > 0 ? 1 : -1);
	}
}

void MinimapCanvas::OnMouseEnter(wxMouseEvent& event) {
	event.Skip();
}

void MinimapCanvas::OnMouseLeave(wxMouseEvent& event) {
	if (!HasCapture()) {
		StopInteraction();
	}
	event.Skip();
}

void MinimapCanvas::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

void MinimapCanvas::ClampViewportState(MinimapViewportState& state) const {
	Editor* editor = owner_->GetActiveEditor();
	if (!editor) {
		return;
	}

	const wxSize size = GetClientSize();
	const int map_width = editor->map.getWidth();
	const int map_height = editor->map.getHeight();
	const double visible_width = std::min(static_cast<double>(map_width), std::max(1.0, size.GetWidth() * MinimapViewport::GetZoomFactor(state.zoom_step)));
	const double visible_height = std::min(static_cast<double>(map_height), std::max(1.0, size.GetHeight() * MinimapViewport::GetZoomFactor(state.zoom_step)));

	if (map_width > 0) {
		if (visible_width >= map_width) {
			state.center_x = map_width / 2.0;
		} else {
			const double half_width = visible_width / 2.0;
			state.center_x = std::clamp(state.center_x, half_width, map_width - half_width);
		}
	}

	if (map_height > 0) {
		if (visible_height >= map_height) {
			state.center_y = map_height / 2.0;
		} else {
			const double half_height = visible_height / 2.0;
			state.center_y = std::clamp(state.center_y, half_height, map_height - half_height);
		}
	}

	state.floor = MinimapViewport::ClampFloor(state.floor);
}

void MinimapCanvas::StopInteraction() {
	is_dragging_ = false;
	ctrl_click_pending_ = false;
	if (HasCapture()) {
		ReleaseMouse();
	}
}
