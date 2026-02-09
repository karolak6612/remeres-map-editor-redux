//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "app/settings.h"

#include "ui/dcbutton.h"
#include "game/sprites.h"
#include "ui/gui.h"
#include "rendering/core/game_sprite.h"

#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>

DCButton::DCButton(wxWindow* parent, wxWindowID id, wxPoint pos, int type, RenderSize sz, int sprite_id) :
	NanoVGCanvas(parent, id, 0),
	type(type),
	state(false),
	size(sz),
	sprite(nullptr),
	overlay(nullptr) {

	wxSize wsize;
	if (sz == RENDER_SIZE_64x64)
		wsize = wxSize(68, 68);
	else if (sz == RENDER_SIZE_32x32)
		wsize = wxSize(36, 36);
	else
		wsize = wxSize(20, 20);

	SetSize(wsize);
	SetPosition(pos);
	SetMinSize(wsize);
	SetMaxSize(wsize);

	Bind(wxEVT_LEFT_DOWN, &DCButton::OnClick, this);
	SetSprite(sprite_id);
}

DCButton::~DCButton() {
	////
}

void DCButton::SetSprite(int _sprid) {
	if (_sprid != 0) {
		sprite = g_gui.gfx.getSprite(_sprid);
	} else {
		sprite = nullptr;
	}
	Refresh();
}

void DCButton::SetSprite(Sprite* _sprite) {
	sprite = _sprite;
	Refresh();
}

void DCButton::SetOverlay(Sprite* espr) {
	overlay = espr;
	Refresh();
}

void DCButton::SetValue(bool val) {
	ASSERT(type == DC_BTN_TOGGLE);
	bool oldval = val;
	state = val;
	if (state == oldval) {
		// Cheap to change value to the old one (which is done ALOT)
		if (GetValue() && g_settings.getInteger(Config::USE_GUI_SELECTION_SHADOW)) {
			SetOverlay(g_gui.gfx.getSprite(EDITOR_SPRITE_SELECTION_MARKER));
		} else {
			SetOverlay(nullptr);
		}
		Refresh();
	}
}

bool DCButton::GetValue() const {
	ASSERT(type == DC_BTN_TOGGLE);
	return state;
}

void DCButton::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (g_gui.gfx.isUnloaded()) {
		return;
	}

	// Draw Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgFill(vg);

	// Determine border colors
	NVGcolor highlight = nvgRGBA(255, 255, 255, 255);
	NVGcolor dark_highlight = nvgRGBA(212, 208, 200, 255); // 0xD4, 0xD0, 0xC8
	NVGcolor light_shadow = nvgRGBA(128, 128, 128, 255);
	NVGcolor shadow = nvgRGBA(64, 64, 64, 255);

	bool pressed = (type == DC_BTN_TOGGLE && GetValue());

	float w = static_cast<float>(width);
	float h = static_cast<float>(height);

	nvgBeginPath(vg);
	if (pressed) {
		// Pressed state borders
		// Outer Top/Left - Shadow
		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, h - 1);
		nvgLineTo(vg, 0, 0);
		nvgLineTo(vg, w - 1, 0);
		nvgStrokeColor(vg, shadow);
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Inner Top/Left - Light Shadow
		nvgBeginPath(vg);
		nvgMoveTo(vg, 1, h - 2);
		nvgLineTo(vg, 1, 1);
		nvgLineTo(vg, w - 2, 1);
		nvgStrokeColor(vg, light_shadow);
		nvgStroke(vg);

		// Inner Bottom/Right - Dark Highlight
		nvgBeginPath(vg);
		nvgMoveTo(vg, w - 2, 1);
		nvgLineTo(vg, w - 2, h - 2);
		nvgLineTo(vg, 1, h - 2);
		nvgStrokeColor(vg, dark_highlight);
		nvgStroke(vg);

		// Outer Bottom/Right - Highlight
		nvgBeginPath(vg);
		nvgMoveTo(vg, w - 1, 0);
		nvgLineTo(vg, w - 1, h - 1);
		nvgLineTo(vg, 0, h - 1);
		nvgStrokeColor(vg, highlight);
		nvgStroke(vg);

	} else {
		// Normal state borders
		// Outer Top/Left - Highlight
		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, h - 1);
		nvgLineTo(vg, 0, 0);
		nvgLineTo(vg, w - 1, 0);
		nvgStrokeColor(vg, highlight);
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Inner Top/Left - Dark Highlight
		nvgBeginPath(vg);
		nvgMoveTo(vg, 1, h - 2);
		nvgLineTo(vg, 1, 1);
		nvgLineTo(vg, w - 2, 1);
		nvgStrokeColor(vg, dark_highlight);
		nvgStroke(vg);

		// Inner Bottom/Right - Light Shadow
		nvgBeginPath(vg);
		nvgMoveTo(vg, w - 2, 1);
		nvgLineTo(vg, w - 2, h - 2);
		nvgLineTo(vg, 1, h - 2);
		nvgStrokeColor(vg, light_shadow);
		nvgStroke(vg);

		// Outer Bottom/Right - Shadow
		nvgBeginPath(vg);
		nvgMoveTo(vg, w - 1, 0);
		nvgLineTo(vg, w - 1, h - 1);
		nvgLineTo(vg, 0, h - 1);
		nvgStrokeColor(vg, shadow);
		nvgStroke(vg);
	}

	// Draw Sprite
	if (sprite) {
		int tex = GetOrCreateSpriteTexture(vg, sprite);
		if (tex > 0) {
			float ix = 2.0f;
			float iy = 2.0f;
			float isz = 0.0f;

			if (size == RENDER_SIZE_16x16) isz = 16.0f;
			else if (size == RENDER_SIZE_32x32) isz = 32.0f;
			else if (size == RENDER_SIZE_64x64) isz = 64.0f;

			if (isz > 0) {
				NVGpaint imgPaint = nvgImagePattern(vg, ix, iy, isz, isz, 0.0f, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, ix, iy, isz, isz);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}

			// Overlay
			if (overlay && pressed) {
				int ovTex = GetOrCreateSpriteTexture(vg, overlay);
				if (ovTex > 0) {
					NVGpaint ovPaint = nvgImagePattern(vg, ix, iy, isz, isz, 0.0f, ovTex, 1.0f);
					nvgBeginPath(vg);
					nvgRect(vg, ix, iy, isz, isz);
					nvgFillPaint(vg, ovPaint);
					nvgFill(vg);
				}
			}
		}
	}
}

int DCButton::GetOrCreateSpriteTexture(NVGcontext* vg, Sprite* spr) {
	if (!spr) return 0;

	uint32_t ptrId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(spr) & 0xFFFFFFFF);
	int existing = GetCachedImage(ptrId);
	if (existing > 0) return existing;

	// Try GameSprite optimization
	GameSprite* gs = dynamic_cast<GameSprite*>(spr);
	if (gs && !gs->spriteList.empty()) {
		// Calculate composite size
		int w = gs->width * 32;
		int h = gs->height * 32;
		if (w <= 0 || h <= 0) return 0;

		size_t bufferSize = static_cast<size_t>(w) * h * 4;
		std::vector<uint8_t> composite(bufferSize, 0);

		int px = (gs->pattern_x >= 3) ? 2 : 0;
		for (int l = 0; l < gs->layers; ++l) {
			for (int sw = 0; sw < gs->width; ++sw) {
				for (int sh = 0; sh < gs->height; ++sh) {
					int idx = gs->getIndex(sw, sh, l, px, 0, 0, 0);
					if (idx < 0 || static_cast<size_t>(idx) >= gs->spriteList.size()) continue;

					auto data = gs->spriteList[idx]->getRGBAData();
					if (!data) continue;

					int part_x = (gs->width - sw - 1) * 32;
					int part_y = (gs->height - sh - 1) * 32;

					for (int sy = 0; sy < 32; ++sy) {
						for (int sx = 0; sx < 32; ++sx) {
							int dy = part_y + sy;
							int dx = part_x + sx;
							int di = (dy * w + dx) * 4;
							int si = (sy * 32 + sx) * 4;

							uint8_t sa = data[si + 3];
							if (sa == 0) continue;

							if (sa == 255) {
								composite[di + 0] = data[si + 0];
								composite[di + 1] = data[si + 1];
								composite[di + 2] = data[si + 2];
								composite[di + 3] = 255;
							} else {
								float a = sa / 255.0f;
								float ia = 1.0f - a;
								composite[di + 0] = static_cast<uint8_t>(data[si + 0] * a + composite[di + 0] * ia);
								composite[di + 1] = static_cast<uint8_t>(data[si + 1] * a + composite[di + 1] * ia);
								composite[di + 2] = static_cast<uint8_t>(data[si + 2] * a + composite[di + 2] * ia);
								composite[di + 3] = std::max(composite[di + 3], sa);
							}
						}
					}
				}
			}
		}
		return GetOrCreateImage(ptrId, composite.data(), w, h);
	}

	// Fallback for other sprites (EditorSprite, etc.)
	int w = 32, h = 32;
	SpriteSize szEnum = SPRITE_SIZE_32x32;
	if (size == RENDER_SIZE_16x16) {
		w = 32; h = 32; // Always render at 32x32 for quality, scale down in NanoVG if needed
		szEnum = SPRITE_SIZE_16x16;
	} else if (size == RENDER_SIZE_64x64) {
		w = 64; h = 64;
		szEnum = SPRITE_SIZE_64x64;
	}

	wxBitmap bmp(w, h, 32);
	// Initialize with transparency
	{
		wxMemoryDC mdc(bmp);
		mdc.SetBackground(wxBrush(wxColour(0, 0, 0, 0), wxBRUSHSTYLE_TRANSPARENT));
		mdc.Clear();
		// Some platforms need explicit alpha clearing
		if (bmp.HasAlpha()) {
			// This might not be enough on Windows GDI+ but good effort.
			// Better to just draw.
		}

		spr->DrawTo(&mdc, szEnum, 0, 0);
	}

	wxImage img = bmp.ConvertToImage();
	if (!img.IsOk()) return 0;

	// Convert to RGBA
	int iw = img.GetWidth();
	int ih = img.GetHeight();
	std::vector<uint8_t> rgba(iw * ih * 4);
	uint8_t* data = img.GetData();
	uint8_t* alpha = img.GetAlpha();

	for (int i = 0; i < iw * ih; ++i) {
		rgba[i * 4 + 0] = data[i * 3 + 0];
		rgba[i * 4 + 1] = data[i * 3 + 1];
		rgba[i * 4 + 2] = data[i * 3 + 2];
		if (alpha) {
			rgba[i * 4 + 3] = alpha[i];
		} else {
			// If no alpha channel, check for magic pink or assume opaque?
			// Editor sprites usually have mask.
			// But ConvertToImage might lose mask if not handled.
			// Let's assume opaque if no alpha.
			rgba[i * 4 + 3] = 255;
		}
	}

	// Apply mask if exists and no alpha
	if (!alpha && img.HasMask()) {
		uint8_t mr = img.GetMaskRed();
		uint8_t mg = img.GetMaskGreen();
		uint8_t mb = img.GetMaskBlue();
		for (int i = 0; i < iw * ih; ++i) {
			if (rgba[i * 4 + 0] == mr && rgba[i * 4 + 1] == mg && rgba[i * 4 + 2] == mb) {
				rgba[i * 4 + 3] = 0;
			}
		}
	}

	return GetOrCreateImage(ptrId, rgba.data(), iw, ih);
}

void DCButton::OnClick(wxMouseEvent& WXUNUSED(evt)) {
	wxCommandEvent event(type == DC_BTN_TOGGLE ? wxEVT_COMMAND_TOGGLEBUTTON_CLICKED : wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	event.SetEventObject(this);

	if (type == DC_BTN_TOGGLE) {
		SetValue(!GetValue());
	}
	SetFocus();

	GetEventHandler()->ProcessEvent(event);
}
