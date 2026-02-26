//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "ui/controls/dat_debug_list_box.h"
#include "ui/theme.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "rendering/core/sprites.h"
#include "util/nvg_utils.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>
#include <format>

DatDebugListBox::DatDebugListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id, wxLB_SINGLE) {
	int maxID = g_gui.gfx.getItemSpriteMaxID();
	sprites.reserve(maxID);
	for (int i = 0; i < maxID; ++i) {
		Sprite* spr = g_gui.gfx.getSprite(i);
		if (spr) {
			sprites.push_back(spr);
		}
	}
	SetItemCount(sprites.size());

	Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent& event) {
		if (GetSelection() != -1) {
			wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
			evt.SetEventObject(this);
			evt.SetInt(GetSelection());
			ProcessWindowEvent(evt);
		}
	});
}

DatDebugListBox::~DatDebugListBox() {
}

void DatDebugListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) {
	if (index >= sprites.size()) {
		return;
	}

	Sprite* sprite = sprites[index];
	if (!sprite) {
		return;
	}

	int x = rect.GetX();
	int y = rect.GetY();
	int w = rect.GetWidth();
	int h = rect.GetHeight();

	// Draw Sprite
	int tex = GetOrCreateSpriteTexture(vg, sprite);
	if (tex > 0) {
		// Center vertically
		float iconY = y + (h - 32) / 2.0f;

		NVGpaint imgPaint = nvgImagePattern(vg, x + 4, iconY, 32, 32, 0, tex, 1.0f);
		nvgBeginPath(vg);
		nvgRect(vg, x + 4, iconY, 32, 32);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
	}

	// Draw Text
	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	if (IsSelected(index)) {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	} else {
		nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));
	}

	std::string text = std::format("ID: {}", index);
	nvgText(vg, x + 40, y + h / 2.0f, text.c_str(), nullptr);
}

int DatDebugListBox::OnMeasureItem(size_t index) const {
	return 40;
}
