#include "ui/tool_options_surface.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "ui/theme.h"
#include "rendering/core/graphics.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/brush.h"
#include "game/sprites.h"
#include <format>
#include <algorithm>
#include <nanovg.h>

// Bring in specific brushes for identification/selection
#include "brushes/border/optional_border_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"

ToolOptionsSurface::ToolOptionsSurface(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS) {
	// SetBackgroundStyle(wxBG_STYLE_PAINT); // NanoVGCanvas handles this
	m_animTimer.SetOwner(this);

	// NanoVGCanvas handles Paint and EraseBackground
	// Bind(wxEVT_PAINT, &ToolOptionsSurface::OnPaint, this);
	// Bind(wxEVT_ERASE_BACKGROUND, &ToolOptionsSurface::OnEraseBackground, this);

	Bind(wxEVT_LEFT_DOWN, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEFT_DCLICK, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEFT_UP, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_MOTION, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEAVE_WINDOW, &ToolOptionsSurface::OnLeave, this);
	Bind(wxEVT_SIZE, &ToolOptionsSurface::OnSize, this);
	Bind(wxEVT_TIMER, &ToolOptionsSurface::OnTimer, this);

	m_animTimer.Start(16);
}

ToolOptionsSurface::~ToolOptionsSurface() {
}

wxSize ToolOptionsSurface::DoGetBestClientSize() const {
	// Calculate height based on current layout
	// Base padding
	int h = FromDIP(8);

	// Tools Section
	if (!tool_rects.empty()) {
		int max_y = 0;
		for (const auto& tr : tool_rects) {
			max_y = std::max(max_y, tr.rect.GetBottom());
		}
		h = max_y + FromDIP(SECTION_GAP);
	}

	// Sliders Section
	if (interactables.size_slider_rect.height > 0) {
		h += interactables.size_slider_rect.height + FromDIP(GRID_GAP);
	}
	if (interactables.thickness_slider_rect.height > 0) {
		h += interactables.thickness_slider_rect.height + FromDIP(SECTION_GAP);
	}

	// Checkboxes
	if (interactables.preview_check_rect.height > 0) {
		h += interactables.preview_check_rect.height + FromDIP(4);
	}
	if (interactables.lock_check_rect.height > 0) {
		h += interactables.lock_check_rect.height + FromDIP(4);
	}

	// Min width 200 DIP?
	return wxSize(FromDIP(240), h + FromDIP(8));
}

void ToolOptionsSurface::RebuildLayout() {
	tool_rects.clear();
	interactables.size_slider_rect = wxRect();
	interactables.thickness_slider_rect = wxRect();
	interactables.preview_check_rect = wxRect();
	interactables.lock_check_rect = wxRect();

	if (current_type == TILESET_UNKNOWN) {
		UpdateScrollbar(0);
		return;
	}

	int x = FromDIP(4);
	int y = FromDIP(4);
	const int w = GetClientSize().GetWidth();
	const int icon_sz = FromDIP(ICON_SIZE_LG); // Defaulting to large for "Pro" look
	const int gap = FromDIP(GRID_GAP);

	// 1. Tools
	bool has_tools = (current_type == TILESET_TERRAIN || current_type == TILESET_COLLECTION);

	if (has_tools) {
		// Populate tool list based on BrushManager
		std::vector<Brush*> brushes;

		if (g_brush_manager.optional_brush) {
			brushes.push_back(g_brush_manager.optional_brush);
		}
		if (g_brush_manager.eraser) {
			brushes.push_back(g_brush_manager.eraser);
		}
		if (g_brush_manager.pz_brush) {
			brushes.push_back(g_brush_manager.pz_brush);
		}
		if (g_brush_manager.rook_brush) {
			brushes.push_back(g_brush_manager.rook_brush);
		}
		if (g_brush_manager.nolog_brush) {
			brushes.push_back(g_brush_manager.nolog_brush);
		}
		if (g_brush_manager.pvp_brush) {
			brushes.push_back(g_brush_manager.pvp_brush);
		}

		// Doors?
		if (g_brush_manager.normal_door_brush) {
			brushes.push_back(g_brush_manager.normal_door_brush);
		}
		if (g_brush_manager.locked_door_brush) {
			brushes.push_back(g_brush_manager.locked_door_brush);
		}
		if (g_brush_manager.magic_door_brush) {
			brushes.push_back(g_brush_manager.magic_door_brush);
		}
		if (g_brush_manager.quest_door_brush) {
			brushes.push_back(g_brush_manager.quest_door_brush);
		}
		if (g_brush_manager.hatch_door_brush) {
			brushes.push_back(g_brush_manager.hatch_door_brush);
		}
		if (g_brush_manager.window_door_brush) {
			brushes.push_back(g_brush_manager.window_door_brush);
		}
		if (g_brush_manager.archway_door_brush) {
			brushes.push_back(g_brush_manager.archway_door_brush);
		}

		// Layout grid
		int cur_x = x;
		// Ensure w is at least somewhat sane to avoid infinite loops or weirdness
		int effective_w = std::max(w, icon_sz + gap + x);

		for (Brush* b : brushes) {
			if (cur_x + icon_sz > effective_w) {
				cur_x = x;
				y += icon_sz + gap;
			}

			ToolRect tr;
			tr.rect = wxRect(cur_x, y, icon_sz, icon_sz);
			tr.brush = b;
			tr.tooltip = b->getName(); // Or specific label
			tool_rects.push_back(tr);

			cur_x += icon_sz + gap;
		}
		if (!brushes.empty()) {
			y += icon_sz + FromDIP(SECTION_GAP);
		}
	}

	int slider_h = FromDIP(24);
	int slider_w = std::max(10, w - FromDIP(8));

	// 2. Size Slider
	bool show_size = (current_type != TILESET_UNKNOWN); // Most show size
	if (show_size) {
		interactables.size_slider_rect = wxRect(x, y, slider_w, slider_h);
		y += slider_h + gap;
	}

	// 3. Thickness Slider
	bool show_thickness = (current_type == TILESET_COLLECTION || current_type == TILESET_DOODAD);
	if (show_thickness) {
		interactables.thickness_slider_rect = wxRect(x, y, slider_w, slider_h);
		y += slider_h + gap;
	}

	// 4. Options
	y += FromDIP(8); // Extra spacer
	if (has_tools) { // Assume terrain
		interactables.preview_check_rect = wxRect(x, y, slider_w, FromDIP(20));
		y += FromDIP(24);
		interactables.lock_check_rect = wxRect(x, y, slider_w, FromDIP(20));
		y += FromDIP(24);
	}

	InvalidateBestSize();
	UpdateScrollbar(y + FromDIP(8));
}

void ToolOptionsSurface::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// 1. Draw Tools
	for (const auto& tr : tool_rects) {
		DrawToolIcon(vg, tr);
	}

	// 2. Draw Sliders
	if (interactables.size_slider_rect.height > 0) {
		DrawSlider(vg, interactables.size_slider_rect, "Size", current_size, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE, interactables.dragging_size);
	}
	if (interactables.thickness_slider_rect.height > 0) {
		DrawSlider(vg, interactables.thickness_slider_rect, "Thickness", current_thickness, MIN_BRUSH_THICKNESS, MAX_BRUSH_THICKNESS, interactables.dragging_thickness);
	}

	// 3. Checkboxes
	if (interactables.preview_check_rect.height > 0) {
		DrawCheckbox(vg, interactables.preview_check_rect, "Preview Border", show_preview, interactables.hover_preview);
	}
	if (interactables.lock_check_rect.height > 0) {
		DrawCheckbox(vg, interactables.lock_check_rect, "Lock Doors (Shift)", lock_doors, interactables.hover_lock);
	}
}

void ToolOptionsSurface::DrawToolIcon(NVGcontext* vg, const ToolRect& tr) {
	bool is_selected = (active_brush == tr.brush);
	bool is_hover = (hover_brush == tr.brush);

	float x = static_cast<float>(tr.rect.x);
	float y = static_cast<float>(tr.rect.y);
	float w = static_cast<float>(tr.rect.width);
	float h = static_cast<float>(tr.rect.height);

	// Background
	if (is_selected) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgFillColor(vg, nvgRGBA(60, 80, 100, 255)); // Theme accent color approximation
		nvgFill(vg);

		nvgStrokeColor(vg, nvgRGBA(100, 150, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	} else if (is_hover) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 30));
		nvgFill(vg);
	}

	// Draw Brush Sprite
	if (tr.brush) {
		Sprite* s = tr.brush->getSprite();
		if (!s && tr.brush->getLookID() != 0) {
			s = g_gui.gfx.getSprite(tr.brush->getLookID());
		}

		if (s) {
			int tex = GetOrCreateSpriteTexture(vg, s);
			if (tex > 0) {
				nvgSave(vg);

				float cx = x + w * 0.5f;
				float cy = y + h * 0.5f;

				nvgTranslate(vg, cx, cy);

				// Animated rotation for active brush
				if (is_selected) {
					nvgRotate(vg, m_brushRotation);
				}

				// Scale pulse for hover
				if (is_hover && !is_selected) {
					float scale = 1.0f + 0.1f * sinf(m_brushRotation * 2.0f);
					nvgScale(vg, scale, scale);
				}

				// Draw centered (32x32 usually)
				float sw = 32.0f;
				float sh = 32.0f;

				NVGpaint imgPaint = nvgImagePattern(vg, -sw * 0.5f, -sh * 0.5f, sw, sh, 0.0f, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, -sw * 0.5f, -sh * 0.5f, sw, sh);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);

				nvgRestore(vg);
			}
		} else {
			// Fallback text
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			wxString label = tr.tooltip.Left(1);
			nvgText(vg, x + w * 0.5f, y + h * 0.5f, label.ToUTF8(), nullptr);
		}
	}
}

void ToolOptionsSurface::DrawSlider(NVGcontext* vg, const wxRect& rect, const std::string& label, int value, int min, int max, bool active) {
	float x = static_cast<float>(rect.x);
	float y = static_cast<float>(rect.y);
	float w = static_cast<float>(rect.width);
	float h = static_cast<float>(rect.height);

	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	// Label
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgText(vg, x, y + h * 0.5f, label.c_str(), nullptr);

	// Track
	float label_w = static_cast<float>(FromDIP(SLIDER_LABEL_WIDTH));
	float track_x = x + label_w;
	float track_w = w - label_w - static_cast<float>(FromDIP(SLIDER_TEXT_MARGIN));
	float track_h = 4.0f;
	float track_y = y + (h - track_h) * 0.5f;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, track_x, track_y, track_w, track_h, 2.0f);
	nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
	nvgFill(vg);

	// Fill
	if (value >= min && max > min) {
		float pct = std::clamp(static_cast<float>(value - min) / static_cast<float>(max - min), 0.0f, 1.0f);
		float fill_w = track_w * pct;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, track_x, track_y, fill_w, track_h, 2.0f);
		nvgFillColor(vg, nvgRGBA(100, 150, 255, 255));
		nvgFill(vg);

		// Thumb
		nvgBeginPath(vg);
		nvgCircle(vg, track_x + fill_w, track_y + track_h * 0.5f, 6.0f);
		nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
		nvgFill(vg);

		if (active) {
			nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 128));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);
		}
	}

	// Value Text
	std::string val_str = std::format("{}", value);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, track_x + track_w + 10.0f, y + h * 0.5f, val_str.c_str(), nullptr);
}

void ToolOptionsSurface::DrawCheckbox(NVGcontext* vg, const wxRect& rect, const std::string& label, bool value, bool hover) {
	float x = static_cast<float>(rect.x);
	float y = static_cast<float>(rect.y);
	// float w = static_cast<float>(rect.width);
	float h = static_cast<float>(rect.height);

	float box_sz = 16.0f;
	float box_y = y + (h - box_sz) * 0.5f;

	// Box
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, box_y, box_sz, box_sz, 3.0f);
	if (value) {
		nvgFillColor(vg, nvgRGBA(100, 150, 255, 255));
		nvgFill(vg);
	} else {
		nvgFillColor(vg, nvgRGBA(40, 40, 40, 255));
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	if (hover) {
		nvgStrokeColor(vg, nvgRGBA(200, 200, 200, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Checkmark
	if (value) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, x + 3, box_y + 8);
		nvgLineTo(vg, x + 7, box_y + 12);
		nvgLineTo(vg, x + 13, box_y + 4);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgText(vg, x + box_sz + 8.0f, y + h * 0.5f, label.c_str(), nullptr);
}

void ToolOptionsSurface::OnMouse(wxMouseEvent& evt) {
	// Adjust for scrolling
	wxPoint pos = evt.GetPosition();
	int scrollY = GetScrollPosition();
	m_hoverPos = wxPoint(pos.x, pos.y + scrollY);

	// Hit testing
	Brush* prev_hover = hover_brush;
	hover_brush = nullptr;

	bool prev_preview = interactables.hover_preview;
	bool prev_lock = interactables.hover_lock;
	interactables.hover_preview = false;
	interactables.hover_lock = false;

	bool hand_cursor = false;

	// Tools
	for (const auto& tr : tool_rects) {
		if (tr.rect.Contains(m_hoverPos)) {
			hover_brush = tr.brush;
			if (prev_hover != hover_brush) {
				SetToolTip(tr.tooltip);
			}
			hand_cursor = true;
			break;
		}
	}
	if (!hover_brush && prev_hover) {
		UnsetToolTip();
	}

	// Sliders Interaction
	if (evt.LeftDown()) {
		if (interactables.size_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_size = true;
			CaptureMouse();
			int val = CalculateSliderValue(interactables.size_slider_rect, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Brush Size: {}", current_size));
		} else if (interactables.thickness_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_thickness = true;
			CaptureMouse();
			int val = CalculateSliderValue(interactables.thickness_slider_rect, MIN_BRUSH_THICKNESS, MAX_BRUSH_THICKNESS);
			if (val != current_thickness) {
				current_thickness = val;
				g_brush_manager.SetBrushThickness(true, current_thickness, 100);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Thickness: {}", current_thickness));
		} else if (interactables.preview_check_rect.Contains(m_hoverPos)) {
			show_preview = !show_preview;
			g_settings.setInteger(Config::SHOW_AUTOBORDER_PREVIEW, show_preview);
			Refresh();
		} else if (interactables.lock_check_rect.Contains(m_hoverPos)) {
			lock_doors = !lock_doors;
			g_settings.setInteger(Config::DRAW_LOCKED_DOOR, lock_doors);
			g_brush_manager.SetDoorLocked(lock_doors);
			Refresh();
		} else if (hover_brush) {
			SelectBrush(hover_brush);
			g_gui.SetStatusText(std::format("Selected Tool: {}", hover_brush->getName()));
		}
	} else if (evt.LeftDClick()) {
		if (interactables.size_slider_rect.Contains(m_hoverPos)) {
			current_size = MIN_BRUSH_SIZE;
			g_brush_manager.SetBrushSize(current_size);
			Refresh();
			g_gui.SetStatusText(std::format("Brush Size: {}", current_size));
		} else if (interactables.thickness_slider_rect.Contains(m_hoverPos)) {
			current_thickness = MAX_BRUSH_THICKNESS;
			g_brush_manager.SetBrushThickness(true, current_thickness, 100);
			Refresh();
			g_gui.SetStatusText(std::format("Thickness: {}", current_thickness));
		}
	}

	if (evt.Dragging() && evt.LeftIsDown()) {
		if (interactables.dragging_size) {
			int val = CalculateSliderValue(interactables.size_slider_rect, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size); // Assuming square for now
				Refresh();
			}
			g_gui.SetStatusText(std::format("Brush Size: {}", current_size));
		}
		if (interactables.dragging_thickness) {
			int val = CalculateSliderValue(interactables.thickness_slider_rect, MIN_BRUSH_THICKNESS, MAX_BRUSH_THICKNESS);
			if (val != current_thickness) {
				current_thickness = val;
				g_brush_manager.SetBrushThickness(true, current_thickness, 100);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Thickness: {}", current_thickness));
		}
	}

	if (evt.LeftUp()) {
		if (GetCapture() == this) {
			ReleaseMouse();
		}
		interactables.dragging_size = false;
		interactables.dragging_thickness = false;
	}

	// Hover states for checkboxes
	if (interactables.preview_check_rect.Contains(m_hoverPos)) {
		interactables.hover_preview = true;
		hand_cursor = true;
	}
	if (interactables.lock_check_rect.Contains(m_hoverPos)) {
		interactables.hover_lock = true;
		hand_cursor = true;
	}

	if (interactables.size_slider_rect.Contains(m_hoverPos) || interactables.thickness_slider_rect.Contains(m_hoverPos)) {
		hand_cursor = true;
	}

	SetCursor(hand_cursor ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_ARROW));

	if (prev_hover != hover_brush || prev_preview != interactables.hover_preview || prev_lock != interactables.hover_lock) {
		Refresh();
	}
}

void ToolOptionsSurface::OnLeave(wxMouseEvent& evt) {
	hover_brush = nullptr;
	Refresh();
}

void ToolOptionsSurface::OnTimer(wxTimerEvent& evt) {
	// Rotate brush
	m_brushRotation += 0.05f;
	if (m_brushRotation > 3.14159f * 2.0f) {
		m_brushRotation -= 3.14159f * 2.0f;
	}

	// Only refresh if active brush is set (optimization)
	if (active_brush || hover_brush) {
		Refresh();
	}
}

void ToolOptionsSurface::OnSize(wxSizeEvent& evt) {
	RebuildLayout();
	Refresh();
	evt.Skip();
}

void ToolOptionsSurface::SetPaletteType(PaletteType type) {
	if (current_type == type) {
		return;
	}
	current_type = type;
	RebuildLayout();
	Refresh();
}

void ToolOptionsSurface::UpdateBrushSize(BrushShape shape, int size) {
	if (current_size != size) {
		current_size = size;
		Refresh();
	}
}

void ToolOptionsSurface::ReloadSettings() {
	show_preview = g_settings.getInteger(Config::SHOW_AUTOBORDER_PREVIEW);
	lock_doors = g_settings.getInteger(Config::DRAW_LOCKED_DOOR);
	RebuildLayout();
	Refresh();
}

void ToolOptionsSurface::SelectBrush(Brush* brush) {
	if (active_brush == brush) {
		return;
	}
	active_brush = brush;
	g_brush_manager.SelectBrush(brush);
	Refresh();
}

int ToolOptionsSurface::CalculateSliderValue(const wxRect& sliderRect, int min, int max) const {
	int label_w = FromDIP(SLIDER_LABEL_WIDTH);
	int track_x = sliderRect.GetLeft() + label_w;
	int track_w = sliderRect.width - label_w - FromDIP(SLIDER_TEXT_MARGIN);

	int rel_x = m_hoverPos.x - track_x;
	float pct = std::clamp(static_cast<float>(rel_x) / static_cast<float>(track_w), 0.0f, 1.0f);

	return min + static_cast<int>(pct * (max - min));
}
