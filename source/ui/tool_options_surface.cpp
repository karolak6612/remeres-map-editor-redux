#include "ui/tool_options_surface.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "ui/artprovider.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/brush.h"
#include "game/sprites.h"

// Bring in specific brushes for identification/selection
#include "brushes/border/optional_border_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"
#include "util/nvg_utils.h"
#include "rendering/core/text_renderer.h"

ToolOptionsSurface::ToolOptionsSurface(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxBORDER_NONE | wxWANTS_CHARS | wxVSCROLL) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_animTimer.SetOwner(this);

	Bind(wxEVT_LEFT_DOWN, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEFT_UP, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_MOTION, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEAVE_WINDOW, &ToolOptionsSurface::OnLeave, this);
	Bind(wxEVT_SIZE, &ToolOptionsSurface::OnSize, this);
	Bind(wxEVT_TIMER, &ToolOptionsSurface::OnTimer, this);
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

void ToolOptionsSurface::DoSetSizeHints(int minW, int minH, int maxW, int maxH, int incW, int incH) {
	wxWindow::DoSetSizeHints(minW, minH, maxW, maxH, incW, incH);
}

void ToolOptionsSurface::RebuildLayout() {
	tool_rects.clear();
	interactables.size_slider_rect = wxRect();
	interactables.thickness_slider_rect = wxRect();
	interactables.preview_check_rect = wxRect();
	interactables.lock_check_rect = wxRect();

	if (current_type == TILESET_UNKNOWN) {
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
		for (Brush* b : brushes) {
			if (cur_x + icon_sz > w) {
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
	int slider_w = w - FromDIP(8);

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
		DrawSlider(vg, interactables.size_slider_rect, "Size", current_size, 1, 15, true);
	}
	if (interactables.thickness_slider_rect.height > 0) {
		DrawSlider(vg, interactables.thickness_slider_rect, "Thinkness", current_thickness, 1, 100, true);
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

	float r_x = static_cast<float>(tr.rect.x);
	float r_y = static_cast<float>(tr.rect.y);
	float r_w = static_cast<float>(tr.rect.width);
	float r_h = static_cast<float>(tr.rect.height);

	// Background
	if (is_selected) {
		nvgBeginPath(vg);
		nvgRect(vg, r_x, r_y, r_w, r_h);
		nvgFillColor(vg, nvgRGBA(51, 153, 255, 180)); // Highlight
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(51, 153, 255, 255));
		nvgStroke(vg);
	} else if (is_hover) {
		nvgBeginPath(vg);
		nvgRect(vg, r_x, r_y, r_w, r_h);
		nvgFillColor(vg, nvgRGBA(200, 200, 200, 100));
		nvgFill(vg);
	}

	// Draw Brush Sprite
	if (tr.brush) {
		Sprite* s = tr.brush->getSprite();
		int spriteId = tr.brush->getLookID();

		if (!s && spriteId != 0) {
			s = g_gui.gfx.getSprite(spriteId);
		}

		int img = 0;
		if (s) {
			uint32_t key;
			if (spriteId != 0) {
				key = static_cast<uint32_t>(spriteId) | 0x40000000;
			} else {
				// Pointer hash for dynamic/unregistered sprites
				uint64_t ptr = reinterpret_cast<uint64_t>(s);
				key = static_cast<uint32_t>(ptr ^ (ptr >> 32));
				key |= 0x80000000;
			}
			img = GetOrCreateSpriteTexture(s, key);
		}

		if (img > 0) {
			// Assuming 32x32 sprite content
			int sw, sh;
			nvgImageSize(vg, img, &sw, &sh);

			// Center
			float draw_x = r_x + (r_w - 32) / 2;
			float draw_y = r_y + (r_h - 32) / 2;

			nvgBeginPath(vg);
			nvgRect(vg, draw_x, draw_y, 32, 32);
			nvgFillPaint(vg, nvgImagePattern(vg, draw_x, draw_y, 32, 32, 0, img, 1.0f));
			nvgFill(vg);
		} else {
			// Fallback text
			nvgFontSize(vg, 16.0f);
			nvgFontFace(vg, "sans");
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, r_x + r_w / 2, r_y + r_h / 2, tr.tooltip.Left(1).c_str(), nullptr);
		}
	}

	// Border
	nvgBeginPath(vg);
	nvgRect(vg, r_x, r_y, r_w, r_h);
	if (is_selected) {
		nvgStrokeColor(vg, nvgRGBA(51, 153, 255, 255));
	} else {
		nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 255));
	}
	nvgStroke(vg);
}

void ToolOptionsSurface::DrawSlider(NVGcontext* vg, const wxRect& rect, const wxString& label, int value, int min, int max, bool active) {
	// Label
	nvgFontSize(vg, 12.0f); // Default font size
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	float r_x = static_cast<float>(rect.x);
	float r_y = static_cast<float>(rect.y);
	float r_h = static_cast<float>(rect.height);

	nvgText(vg, r_x, r_y + r_h / 2, label.c_str(), nullptr);

	// Track
	int label_w = FromDIP(70);
	float track_x = r_x + label_w;
	float track_w = static_cast<float>(rect.width - label_w - FromDIP(40));
	float track_h = static_cast<float>(FromDIP(4));
	float track_y = r_y + (r_h - track_h) / 2;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, track_x, track_y, track_w, track_h, 2.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgFill(vg);

	// Fill
	if (value > min && max > min) {
		float pct = (float)(value - min) / (float)(max - min);
		float fill_w = track_w * pct;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, track_x, track_y, fill_w, track_h, 2.0f);
		nvgFillColor(vg, nvgRGBA(51, 153, 255, 255));
		nvgFill(vg);

		// Thumb
		float thumb_x = track_x + fill_w;
		float thumb_y = track_y + track_h / 2;
		nvgBeginPath(vg);
		nvgCircle(vg, thumb_x, thumb_y, static_cast<float>(FromDIP(5)));
		nvgFillColor(vg, nvgRGBA(50, 50, 50, 255));
		nvgFill(vg);
	}

	// Value Text
	std::string val_str = std::to_string(value);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(vg, track_x + track_w + FromDIP(8), r_y + r_h / 2, val_str.c_str(), nullptr);
}

void ToolOptionsSurface::DrawCheckbox(NVGcontext* vg, const wxRect& rect, const wxString& label, bool value, bool hover) {
	float box_sz = static_cast<float>(FromDIP(14));
	float box_y = static_cast<float>(rect.y) + (static_cast<float>(rect.height) - box_sz) / 2;
	float box_x = static_cast<float>(rect.x);

	nvgBeginPath(vg);
	nvgRect(vg, box_x, box_y, box_sz, box_sz);

	if (hover) {
		nvgStrokeColor(vg, nvgRGBA(51, 153, 255, 255));
	} else {
		nvgStrokeColor(vg, nvgRGBA(150, 150, 150, 255));
	}
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);

	if (value) {
		nvgFillColor(vg, nvgRGBA(51, 153, 255, 255));
		nvgFill(vg);

		// Checkmark
		nvgBeginPath(vg);
		nvgMoveTo(vg, box_x + 3, box_y + 7);
		nvgLineTo(vg, box_x + 6, box_y + 10);
		nvgLineTo(vg, box_x + box_sz - 3, box_y + 4);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 1.5f);
		nvgStroke(vg);
	}

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(vg, box_x + box_sz + FromDIP(8), box_y + box_sz / 2, label.c_str(), nullptr);
}

int ToolOptionsSurface::GetOrCreateSpriteTexture(Sprite* s, uint32_t key) {
	int tex = GetCachedImage(key);
	if (tex > 0) return tex;

	if (!s) return 0;
	GameSprite* gs = dynamic_cast<GameSprite*>(s);
	if (!gs) return 0;

	int w, h;
	auto data = NvgUtils::CreateCompositeRGBA(*gs, w, h);
	if (!data) return 0;

	return GetOrCreateImage(key, data.get(), w, h);
}

void ToolOptionsSurface::OnEraseBackground(wxEraseEvent& evt) {
	// No-op
}

void ToolOptionsSurface::OnMouse(wxMouseEvent& evt) {
	m_hoverPos = evt.GetPosition();
	m_hoverPos.y += GetScrollPosition();

	// Hit testing
	Brush* prev_hover = hover_brush;
	hover_brush = nullptr;

	bool prev_preview = interactables.hover_preview;
	bool prev_lock = interactables.hover_lock;
	interactables.hover_preview = false;
	interactables.hover_lock = false;

	// Tools
	for (const auto& tr : tool_rects) {
		if (tr.rect.Contains(m_hoverPos)) {
			hover_brush = tr.brush;
			// Tooltip handling could go here (delayed)
			break;
		}
	}

	// Sliders Interaction
	if (evt.LeftDown()) {
		if (interactables.size_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_size = true;
			CaptureMouse();
		} else if (interactables.thickness_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_thickness = true;
			CaptureMouse();
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
		}
	}

	if (evt.Dragging() && evt.LeftIsDown()) {
		if (interactables.dragging_size) {
			// Calculate value
			// In a real impl, refactor the math to a helper
			int label_w = FromDIP(70);
			int track_x = interactables.size_slider_rect.GetLeft() + label_w;
			int track_w = interactables.size_slider_rect.width - label_w - FromDIP(40);

			int rel_x = m_hoverPos.x - track_x;
			float pct = (float)rel_x / (float)track_w;
			if (pct < 0) {
				pct = 0;
			}
			if (pct > 1) {
				pct = 1;
			}

			int min = 1, max = 15;
			int val = min + (int)(pct * (max - min));
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size); // Assuming square for now
				Refresh();
			}
		}
		// Thickness similar...
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
	}
	if (interactables.lock_check_rect.Contains(m_hoverPos)) {
		interactables.hover_lock = true;
	}

	if (prev_hover != hover_brush || prev_preview != interactables.hover_preview || prev_lock != interactables.hover_lock) {
		Refresh();
	}
}

void ToolOptionsSurface::OnLeave(wxMouseEvent& evt) {
	hover_brush = nullptr;
	Refresh();
}

void ToolOptionsSurface::OnTimer(wxTimerEvent& evt) {
	// Anim logic if needed
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
