#include <iostream>
#include "ingame_preview/ingame_preview_window.h"
#include "ingame_preview/ingame_preview_canvas.h"
#include "ui/dialogs/outfit_chooser_dialog.h"
#include "game/preview_preferences.h"
#include "MaterialDesign/wxMaterialDesignArtProvider.hpp"

#include "editor/editor.h"
#include "ui/gui.h"
#include "rendering/ui/map_display.h"
#include "app/main.h" // For FROM_DIP
#include <wx/tglbtn.h>

namespace IngamePreview {

	IngamePreviewWindow::IngamePreviewWindow(wxWindow* parent) :
		wxPanel(parent, wxID_ANY),
		update_timer(this, wxID_ANY),
		follow_selection(true) {

		// Load initial preferences
		g_preview_preferences.load();
		preview_outfit = g_preview_preferences.getOutfit();
		current_name = wxString::FromUTF8(g_preview_preferences.getName());
		current_speed = g_preview_preferences.getSpeed();

		wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

		// Single Toolbar
		wxBoxSizer* toolbar_sizer = new wxBoxSizer(wxHORIZONTAL);

		// Toggles
		follow_btn = new wxToggleButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(28, 24)));
		follow_btn->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_MY_LOCATION, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(76, 175, 80))));
		follow_btn->SetValue(true);
		follow_btn->SetToolTip("Follow Selection / Camera");
		toolbar_sizer->Add(follow_btn, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		follow_btn->Bind(wxEVT_TOGGLEBUTTON, &IngamePreviewWindow::OnToggleFollow, this);

		lighting_btn = new wxToggleButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(28, 24)));
		lighting_btn->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_WB_SUNNY, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(255, 235, 59))));
		lighting_btn->SetValue(true);
		lighting_btn->SetToolTip("Toggle Lighting");
		toolbar_sizer->Add(lighting_btn, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		lighting_btn->Bind(wxEVT_TOGGLEBUTTON, &IngamePreviewWindow::OnToggleLighting, this);

		outfit_btn = new wxButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(28, 24)));
		outfit_btn->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_ACCOUNT_CIRCLE, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(96, 125, 139))));
		outfit_btn->SetToolTip("Change Preview Creature Outfit");
		toolbar_sizer->Add(outfit_btn, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		outfit_btn->Bind(wxEVT_BUTTON, &IngamePreviewWindow::OnChooseOutfit, this);

		// Sliders
		ambient_slider = new wxSlider(this, wxID_ANY, 128, 0, 255, wxDefaultPosition, FromDIP(wxSize(60, -1)));
		ambient_slider->SetToolTip("Ambient Light");
		toolbar_sizer->Add(ambient_slider, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		ambient_slider->Bind(wxEVT_SLIDER, &IngamePreviewWindow::OnAmbientSlider, this);

		intensity_slider = new wxSlider(this, wxID_ANY, 100, 0, 200, wxDefaultPosition, FromDIP(wxSize(60, -1)));
		intensity_slider->SetToolTip("Light Intensity");
		toolbar_sizer->Add(intensity_slider, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		intensity_slider->Bind(wxEVT_SLIDER, &IngamePreviewWindow::OnIntensitySlider, this);

		// Spacer
		toolbar_sizer->AddSpacer(FromDIP(4));
		toolbar_sizer->Add(new wxStaticText(this, wxID_ANY, "|"), wxSizerFlags(0).Border(wxALL, 2).Align(wxALIGN_CENTER_VERTICAL));
		toolbar_sizer->AddSpacer(FromDIP(4));

		// Viewport Controls
		// Width
		viewport_w_down = new wxButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(24, 24)));
		viewport_w_down->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_DO_NOT_DISTURB_ON, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(0, 150, 136))));
		toolbar_sizer->Add(viewport_w_down, wxSizerFlags(0).Border(wxALL, 0).Align(wxALIGN_CENTER_VERTICAL));

		viewport_w_down->Bind(wxEVT_BUTTON, &IngamePreviewWindow::OnViewportWidthDown, this);

		viewport_x_text = new wxTextCtrl(this, wxID_ANY, "15", wxDefaultPosition, FromDIP(wxSize(30, -1)), wxTE_READONLY | wxTE_CENTER);
		toolbar_sizer->Add(viewport_x_text, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		viewport_w_up = new wxButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(24, 24)));
		viewport_w_up->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_ADD_CIRCLE, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(0, 150, 136))));
		toolbar_sizer->Add(viewport_w_up, wxSizerFlags(0).Border(wxALL, 0).Align(wxALIGN_CENTER_VERTICAL));

		viewport_w_up->Bind(wxEVT_BUTTON, &IngamePreviewWindow::OnViewportWidthUp, this);

		toolbar_sizer->Add(new wxStaticText(this, wxID_ANY, "x"), wxSizerFlags(0).Border(wxALL, 2).Align(wxALIGN_CENTER_VERTICAL));

		// Height
		viewport_h_down = new wxButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(24, 24)));
		viewport_h_down->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_DO_NOT_DISTURB_ON, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(0, 150, 136))));
		toolbar_sizer->Add(viewport_h_down, wxSizerFlags(0).Border(wxALL, 0).Align(wxALIGN_CENTER_VERTICAL));

		viewport_h_down->Bind(wxEVT_BUTTON, &IngamePreviewWindow::OnViewportHeightDown, this);

		viewport_y_text = new wxTextCtrl(this, wxID_ANY, "11", wxDefaultPosition, FromDIP(wxSize(30, -1)), wxTE_READONLY | wxTE_CENTER);
		toolbar_sizer->Add(viewport_y_text, wxSizerFlags(0).Border(wxALL, 1).Align(wxALIGN_CENTER_VERTICAL));

		viewport_h_up = new wxButton(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(24, 24)));
		viewport_h_up->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_ADD_CIRCLE, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(0, 150, 136))));
		toolbar_sizer->Add(viewport_h_up, wxSizerFlags(0).Border(wxALL, 0).Align(wxALIGN_CENTER_VERTICAL));

		viewport_h_up->Bind(wxEVT_BUTTON, &IngamePreviewWindow::OnViewportHeightUp, this);

		main_sizer->Add(toolbar_sizer, wxSizerFlags(0).Expand().Border(wxALL, 2));

		// Canvas
		canvas = std::make_unique<IngamePreviewCanvas>(this);
		main_sizer->Add(canvas.get(), wxSizerFlags(1).Expand());

		// Sync UI State to Canvas
		canvas->SetLightingEnabled(lighting_btn->GetValue());
		canvas->SetAmbientLight((uint8_t)ambient_slider->GetValue());
		canvas->SetLightIntensity(intensity_slider->GetValue() / 100.0f);
		canvas->SetPreviewOutfit(preview_outfit);
		canvas->SetName(current_name.ToStdString());
		canvas->SetSpeed(current_speed);

		SetSizer(main_sizer);

		Bind(wxEVT_TIMER, &IngamePreviewWindow::OnUpdateTimer, this, update_timer.GetId());
		update_timer.Start(50);
	}

	IngamePreviewWindow::~IngamePreviewWindow() {
		update_timer.Stop();
	}

	void IngamePreviewWindow::OnUpdateTimer(wxTimerEvent& event) {
		UpdateState();
	}

	void IngamePreviewWindow::UpdateState() {
		if (!IsShownOnScreen()) {
			return;
		}

		MapTab* tab = g_gui.GetCurrentMapTab();
		if (!tab) {
			return;
		}

		Editor* active_editor = tab->GetEditor();
		if (!active_editor) {
			return;
		}

		if (follow_selection && !canvas->IsWalking()) {
			Position target;
			// Prioritize Active Selection
			if (!active_editor->selection.empty()) {
				Position min = active_editor->selection.minPosition();
				Position max = active_editor->selection.maxPosition();
				target = Position(min.x + (max.x - min.x) / 2, min.y + (max.y - min.y) / 2, min.z);
			} else {
				// Fallback to Screen Center (Camera)
				target = tab->GetScreenCenterPosition();
			}
			canvas->SetCameraPosition(target);
		}
	}

	void IngamePreviewWindow::OnToggleFollow(wxCommandEvent& event) {
		SetFollowSelection(follow_btn->GetValue());
	}

	void IngamePreviewWindow::SetFollowSelection(bool follow) {
		follow_selection = follow;
		if (follow_selection) {
			follow_btn->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_MY_LOCATION, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(76, 175, 80))));
		} else {
			follow_btn->SetBitmap(wxBitmapBundle::FromBitmap(wxMaterialDesignArtProvider::GetBitmap(wxART_MY_LOCATION, wxART_CLIENT_MATERIAL_FILLED, FromDIP(wxSize(16, 16)), wxColour(158, 158, 158))));
		}
		follow_btn->SetValue(follow_selection);
	}

	void IngamePreviewWindow::OnToggleLighting(wxCommandEvent& event) {
		canvas->SetLightingEnabled(lighting_btn->GetValue());
	}

	void IngamePreviewWindow::OnAmbientSlider(wxCommandEvent& event) {
		canvas->SetAmbientLight((uint8_t)ambient_slider->GetValue());
	}

	void IngamePreviewWindow::OnIntensitySlider(wxCommandEvent& event) {
		canvas->SetLightIntensity(intensity_slider->GetValue() / 100.0f);
	}

	void IngamePreviewWindow::OnViewportWidthUp(wxCommandEvent& event) {
		int w, h;
		canvas->GetViewportSize(w, h);
		w++;
		canvas->SetViewportSize(w, h);
		viewport_x_text->SetValue(wxString::Format("%d", w));
	}

	void IngamePreviewWindow::OnViewportWidthDown(wxCommandEvent& event) {
		int w, h;
		canvas->GetViewportSize(w, h);
		if (w > 15) {
			w--;
			canvas->SetViewportSize(w, h);
			viewport_x_text->SetValue(wxString::Format("%d", w));
		}
	}

	void IngamePreviewWindow::OnViewportHeightUp(wxCommandEvent& event) {
		int w, h;
		canvas->GetViewportSize(w, h);
		h++;
		canvas->SetViewportSize(w, h);
		viewport_y_text->SetValue(wxString::Format("%d", h));
	}

	void IngamePreviewWindow::OnViewportHeightDown(wxCommandEvent& event) {
		int w, h;
		canvas->GetViewportSize(w, h);
		if (h > 11) {
			h--;
			canvas->SetViewportSize(w, h);
			viewport_y_text->SetValue(wxString::Format("%d", h));
		}
	}

	void IngamePreviewWindow::OnChooseOutfit(wxCommandEvent& event) {
		OutfitChooserDialog dialog(this, preview_outfit);
		if (dialog.ShowModal() == wxID_OK) {
			preview_outfit = dialog.GetOutfit();
			current_name = dialog.GetName();
			current_speed = dialog.GetSpeed();

			canvas->SetPreviewOutfit(preview_outfit);
			canvas->SetName(current_name.ToStdString());
			canvas->SetSpeed(current_speed);
		}
	}

} // namespace IngamePreview
