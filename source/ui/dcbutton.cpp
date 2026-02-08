//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "app/settings.h"

#include "ui/dcbutton.h"
#include "ui/gui.h"

#include "rendering/core/game_sprite.h"
#include "rendering/core/editor_sprite.h"
#include <wx/rawbmp.h>

#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>

IMPLEMENT_DYNAMIC_CLASS(DCButton, NanoVGCanvas)

DCButton::DCButton() :
	NanoVGCanvas(nullptr, wxID_ANY, 0),
	type(DC_BTN_NORMAL),
	state(false),
	sprite(nullptr),
	overlay(nullptr) {
	Bind(wxEVT_LEFT_DOWN, &DCButton::OnClick, this);
	SetSprite(0);
}

DCButton::DCButton(wxWindow* parent, wxWindowID id, wxPoint pos, int type, RenderSize sz, int sprite_id) :
	NanoVGCanvas(parent, id, wxWANTS_CHARS),
	type(type),
	state(false),
	sprite(nullptr),
	overlay(nullptr) {

	// Set initial size based on RenderSize
	wxSize winSize;
	if (sz == RENDER_SIZE_64x64) winSize = wxSize(68, 68);
	else if (sz == RENDER_SIZE_32x32) winSize = wxSize(36, 36);
	else winSize = wxSize(20, 20); // 16x16 icon + padding

	SetSize(winSize);
	SetMinSize(winSize);

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

int DCButton::GetOrCreateSpriteTexture(NVGcontext* vg, Sprite* spr) {
	if (!spr) {
		return 0;
	}

	// Use pointer as ID
	uint32_t id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(spr) & 0xFFFFFFFF);

	int existing = GetCachedImage(id);
	if (existing > 0) {
		return existing;
	}

	// Try GameSprite (Optimized path)
	if (GameSprite* gs = dynamic_cast<GameSprite*>(spr)) {
		if (gs->spriteList.empty()) return 0;

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
		return GetOrCreateImage(id, composite.data(), w, h);
	}

	// Fallback for EditorSprite or other types using wxBitmap via wxMemoryDC
	{
		// Determine size
		int w = 32;
		int h = 32;

		// Create bitmap with alpha
		wxBitmap bmp(w, h, 32);
		#ifdef __WXMSW__
			bmp.UseAlpha();
		#endif

		{
			wxMemoryDC dc(bmp);
			dc.SetBackground(wxBrush(wxColor(0, 0, 0, 0), wxTRANSPARENT));
			dc.Clear();

			// Draw sprite centered if possible, or just draw it
			// EditorSprite DrawTo usually expects generic drawing
			spr->DrawTo(&dc, SPRITE_SIZE_32x32, 0, 0, w, h);
		}

		wxImage img = bmp.ConvertToImage();
		if (!img.HasAlpha()) img.InitAlpha();

		int iw = img.GetWidth();
		int ih = img.GetHeight();
		std::vector<uint8_t> rgba(iw * ih * 4);

		uint8_t* data = img.GetData();
		uint8_t* alpha = img.GetAlpha();

		for (int i = 0; i < iw * ih; ++i) {
			rgba[i*4 + 0] = data[i*3 + 0];
			rgba[i*4 + 1] = data[i*3 + 1];
			rgba[i*4 + 2] = data[i*3 + 2];
			rgba[i*4 + 3] = alpha ? alpha[i] : 255;
		}

		return GetOrCreateImage(id, rgba.data(), iw, ih);
	}
}

void DCButton::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Draw Button Background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 0.5f, 0.5f, width - 1.0f, height - 1.0f, 3.0f);

	if (type == DC_BTN_TOGGLE && GetValue()) {
		// Pressed State
		nvgFillColor(vg, nvgRGBA(80, 100, 120, 255));
		nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
	} else {
		// Normal State
		NVGpaint bgPaint = nvgLinearGradient(vg, 0, 0, 0, height, nvgRGBA(60, 60, 65, 255), nvgRGBA(50, 50, 55, 255));
		nvgFillPaint(vg, bgPaint);
		nvgStrokeColor(vg, nvgRGBA(30, 30, 35, 255));
		nvgStrokeWidth(vg, 1.0f);
	}

	nvgFill(vg);
	nvgStroke(vg);

	// Draw Sprite
	if (sprite) {
		int tex = GetOrCreateSpriteTexture(vg, sprite);
		if (tex > 0) {
			int iconSize = std::min(width, height) - 4;
			int iconX = (width - iconSize) / 2;
			int iconY = (height - iconSize) / 2;

			NVGpaint imgPaint = nvgImagePattern(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 0.0f, tex, 1.0f);

			nvgBeginPath(vg);
			nvgRect(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize));
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);

			// Overlay (e.g. selection marker)
			if (overlay && type == DC_BTN_TOGGLE && GetValue()) {
				int ovTex = GetOrCreateSpriteTexture(vg, overlay);
				if (ovTex > 0) {
					NVGpaint ovPaint = nvgImagePattern(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 0.0f, ovTex, 1.0f);
					nvgBeginPath(vg);
					nvgRect(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize));
					nvgFillPaint(vg, ovPaint);
					nvgFill(vg);
				}
			}
		}
	}
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
