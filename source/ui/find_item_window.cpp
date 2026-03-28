#include "app/main.h"
#include "ui/find_item_window.h"

#include "brushes/creature/creature_brush.h"
#include "ui/dialogs/find_dialog.h"
#include "ui/gui.h"
#include "ui/theme.h"
#include "util/image_manager.h"
#include "util/nanovg_listbox.h"

#include <glad/glad.h>
#include <nanovg.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <array>
#include <string_view>

namespace {
	template <typename Enum>
	constexpr size_t toIndex(Enum value) {
		return static_cast<size_t>(value);
	}

	constexpr auto kFindByButtons = std::to_array<std::pair<AdvancedFinderFindByMode, std::string_view>>({
		{ AdvancedFinderFindByMode::FuzzyName, "Name (fuzzy)" },
		{ AdvancedFinderFindByMode::ServerId, "Server ID" },
		{ AdvancedFinderFindByMode::ClientId, "Client ID" },
	});

	constexpr auto kTypeCheckboxes = std::to_array<std::pair<AdvancedFinderTypeFilter, std::string_view>>({
		{ AdvancedFinderTypeFilter::Depot, "Depot" },
		{ AdvancedFinderTypeFilter::Mailbox, "Mailbox" },
		{ AdvancedFinderTypeFilter::TrashHolder, "Trash Holder" },
		{ AdvancedFinderTypeFilter::Container, "Container" },
		{ AdvancedFinderTypeFilter::Door, "Door" },
		{ AdvancedFinderTypeFilter::MagicField, "Magic Field" },
		{ AdvancedFinderTypeFilter::Teleport, "Teleport" },
		{ AdvancedFinderTypeFilter::Bed, "Bed" },
		{ AdvancedFinderTypeFilter::Key, "Key" },
		{ AdvancedFinderTypeFilter::Podium, "Podium" },
		{ AdvancedFinderTypeFilter::Weapon, "Weapon" },
		{ AdvancedFinderTypeFilter::Ammo, "Ammo" },
		{ AdvancedFinderTypeFilter::Armor, "Armor" },
		{ AdvancedFinderTypeFilter::Rune, "Rune" },
		{ AdvancedFinderTypeFilter::Creature, "Creature" },
	});

	constexpr auto kPropertyCheckboxes = std::to_array<std::pair<AdvancedFinderPropertyFilter, std::string_view>>({
		{ AdvancedFinderPropertyFilter::Unpassable, "Unpassable" },
		{ AdvancedFinderPropertyFilter::Unmovable, "Unmovable" },
		{ AdvancedFinderPropertyFilter::BlockMissiles, "Block Missiles" },
		{ AdvancedFinderPropertyFilter::BlockPathfinder, "Block Pathfinder" },
		{ AdvancedFinderPropertyFilter::HasElevation, "Has Elevation" },
		{ AdvancedFinderPropertyFilter::FloorChange, "Floor Change" },
		{ AdvancedFinderPropertyFilter::FullTile, "Full Tile" },
	});

	constexpr auto kInteractionCheckboxes = std::to_array<std::pair<AdvancedFinderInteractionFilter, std::string_view>>({
		{ AdvancedFinderInteractionFilter::Readable, "Readable" },
		{ AdvancedFinderInteractionFilter::Writeable, "Writeable" },
		{ AdvancedFinderInteractionFilter::Pickupable, "Pickupable" },
		{ AdvancedFinderInteractionFilter::ForceUse, "Force Use" },
		{ AdvancedFinderInteractionFilter::DistRead, "Dist Read" },
		{ AdvancedFinderInteractionFilter::Rotatable, "Rotatable" },
		{ AdvancedFinderInteractionFilter::Hangable, "Hangable" },
	});

	constexpr auto kVisualCheckboxes = std::to_array<std::pair<AdvancedFinderVisualFilter, std::string_view>>({
		{ AdvancedFinderVisualFilter::HasLight, "Has Light" },
		{ AdvancedFinderVisualFilter::Animation, "Animation" },
		{ AdvancedFinderVisualFilter::AlwaysTop, "Always Top" },
		{ AdvancedFinderVisualFilter::IgnoreLook, "Ignore Look" },
		{ AdvancedFinderVisualFilter::HasCharges, "Has Charges" },
		{ AdvancedFinderVisualFilter::ClientCharges, "Client Charges" },
		{ AdvancedFinderVisualFilter::Decays, "Decays" },
		{ AdvancedFinderVisualFilter::HasSpeed, "Has Speed" },
	});

	wxStaticText* makeSubtleText(wxWindow* parent, std::string_view text, bool bold = false) {
		auto* label = newd wxStaticText(parent, wxID_ANY, wxString::FromUTF8(text.data(), text.size()));
		label->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
		label->SetFont(Theme::GetFont(9, bold));
		return label;
	}

	wxStaticText* makeSectionLabel(wxWindow* parent, std::string_view text) {
		auto* label = newd wxStaticText(parent, wxID_ANY, wxString::FromUTF8(text.data(), text.size()));
		label->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
		label->SetFont(Theme::GetFont(9, true));
		return label;
	}

}

class AdvancedFinderResultListBox final : public NanoVGListBox {
public:
	AdvancedFinderResultListBox(wxWindow* parent, wxWindowID id) :
		NanoVGListBox(parent, id, wxLB_SINGLE) {
		Bind(wxEVT_LEFT_DCLICK, &AdvancedFinderResultListBox::OnLeftDoubleClick, this);
		SetPrompt("No matching items", "Enter search term or select filters");
	}

	void SetPrompt(std::string primary, std::string secondary) {
		state_ = State::Prompt;
		primary_message_ = std::move(primary);
		secondary_message_ = std::move(secondary);
		rows_.clear();
		SetItemCount(1);
		SetSelection(-1);
		Refresh();
	}

	void SetNoMatches(std::string primary, std::string secondary) {
		state_ = State::NoMatches;
		primary_message_ = std::move(primary);
		secondary_message_ = std::move(secondary);
		rows_.clear();
		SetItemCount(1);
		SetSelection(-1);
		Refresh();
	}

	void SetRows(std::vector<const AdvancedFinderCatalogRow*> rows) {
		state_ = rows.empty() ? State::Prompt : State::Rows;
		rows_ = std::move(rows);
		if (state_ == State::Rows) {
			SetItemCount(rows_.size());
		} else {
			SetItemCount(1);
		}
		SetSelection(-1);
		Refresh();
	}

	[[nodiscard]] const AdvancedFinderCatalogRow* GetSelectedRow() const {
		if (state_ != State::Rows) {
			return nullptr;
		}

		const int selection = GetSelection();
		if (selection < 0 || selection >= static_cast<int>(rows_.size())) {
			return nullptr;
		}

		return rows_[selection];
	}

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override {
		if (state_ != State::Rows) {
			drawEmptyState(vg, rect);
			return;
		}

		ASSERT(index < rows_.size());
		const AdvancedFinderCatalogRow& row = *rows_[index];

		if (Sprite* sprite = spriteForRow(row)) {
			if (const int texture = GetOrCreateSpriteTexture(vg, sprite); texture > 0) {
				const int icon_size = rect.height - 10;
				const int icon_x = rect.x + 6;
				const int icon_y = rect.y + 5;
				NVGpaint img_paint = nvgImagePattern(vg, icon_x, icon_y, icon_size, icon_size, 0.0f, texture, 1.0f);
				nvgBeginPath(vg);
				nvgRoundedRect(vg, icon_x, icon_y, icon_size, icon_size, 4.0f);
				nvgFillPaint(vg, img_paint);
				nvgFill(vg);
			}
		}

		const wxColour primary_colour = IsSelected(static_cast<int>(index)) ? Theme::Get(Theme::Role::TextOnAccent) : Theme::Get(Theme::Role::Text);
		const wxColour secondary_colour = IsSelected(static_cast<int>(index)) ? Theme::Get(Theme::Role::TextOnAccent) : Theme::Get(Theme::Role::TextSubtle);
		const float text_x = static_cast<float>(rect.x + rect.height + 10);
		const float title_y = static_cast<float>(rect.y + 18);
		const float subtitle_y = static_cast<float>(rect.y + rect.height - 14);

		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

		nvgFontSize(vg, 12.0f);
		nvgFillColor(vg, nvgRGBA(primary_colour.Red(), primary_colour.Green(), primary_colour.Blue(), primary_colour.Alpha()));
		nvgText(vg, text_x, title_y, row.label.c_str(), nullptr);

		nvgFontSize(vg, 11.0f);
		nvgFillColor(vg, nvgRGBA(secondary_colour.Red(), secondary_colour.Green(), secondary_colour.Blue(), secondary_colour.Alpha()));
		nvgText(vg, text_x, subtitle_y, row.secondary_label.c_str(), nullptr);
	}

	int OnMeasureItem(size_t WXUNUSED(index)) const override {
		return FromDIP(50);
	}

private:
	enum class State : uint8_t {
		Prompt = 0,
		NoMatches,
		Rows,
	};

	void drawEmptyState(NVGcontext* vg, const wxRect& rect) {
		const wxColour primary_colour = Theme::Get(Theme::Role::TextSubtle);
		const wxColour secondary_colour = Theme::Get(Theme::Role::TextSubtle);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

		nvgFontSize(vg, 12.0f);
		nvgFillColor(vg, nvgRGBA(primary_colour.Red(), primary_colour.Green(), primary_colour.Blue(), primary_colour.Alpha()));
		nvgText(vg, rect.x + 18.0f, rect.y + 20.0f, primary_message_.c_str(), nullptr);

		nvgFontSize(vg, 11.0f);
		nvgFillColor(vg, nvgRGBA(secondary_colour.Red(), secondary_colour.Green(), secondary_colour.Blue(), secondary_colour.Alpha()));
		nvgText(vg, rect.x + 18.0f, rect.y + 40.0f, secondary_message_.c_str(), nullptr);
	}

	[[nodiscard]] Sprite* spriteForRow(const AdvancedFinderCatalogRow& row) const {
		if (row.isCreature() && row.creature_brush != nullptr) {
			return row.creature_brush->getSprite();
		}
		if (row.client_id != 0) {
			return g_gui.gfx.getSprite(row.client_id);
		}
		return nullptr;
	}

	void OnLeftDoubleClick(wxMouseEvent& event) {
		const int index = HitTest(event.GetX(), event.GetY() + GetScrollPosition());
		if (state_ != State::Rows || index < 0) {
			return;
		}

		SetSelection(index);
		SendSelectionEvent();

		wxCommandEvent activate_event(wxEVT_LISTBOX_DCLICK, GetId());
		activate_event.SetEventObject(this);
		activate_event.SetInt(index);
		ProcessWindowEvent(activate_event);
		SetFocus();
		Refresh();
	}

	State state_ = State::Prompt;
	std::string primary_message_;
	std::string secondary_message_;
	std::vector<const AdvancedFinderCatalogRow*> rows_;
};

FindItemDialog::FindItemDialog(
	wxWindow* parent,
	const wxString& title,
	bool onlyPickupables,
	ActionSet action_set,
	AdvancedFinderDefaultAction default_action,
	bool include_creatures
) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	input_timer_(this),
	only_pickupables_(onlyPickupables),
	include_creatures_(include_creatures),
	persist_shared_state_(action_set == ActionSet::SearchAndSelect),
	action_set_(action_set),
	default_action_(default_action) {
	buildLayout();
	bindEvents();
	loadInitialState();
	SetIcons(IMAGE_MANAGER.GetIconBundle(ICON_SEARCH));
}

FindItemDialog::~FindItemDialog() {
	savePersistedState();
}

FindItemDialog::SearchMode FindItemDialog::getSearchMode() const {
	return static_cast<SearchMode>(DeriveLegacySearchMode(query_));
}

void FindItemDialog::setSearchMode(SearchMode mode) {
	if (!shouldApplyLegacyFallback()) {
		return;
	}

	ApplyLegacySearchModeFallback(query_, static_cast<int>(mode));
	applyQueryToControls();
	refreshResults();
}

void FindItemDialog::buildLayout() {
	SetMinSize(wxSize(1100, 740));

	auto* root_sizer = newd wxBoxSizer(wxVERTICAL);
	auto* content_sizer = newd wxBoxSizer(wxHORIZONTAL);

	auto* find_by_sizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Find By"), wxVERTICAL);
	search_field_ = newd KeyForwardingTextCtrl(find_by_sizer->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field_->SetHint("Name or ID...");
	find_by_sizer->Add(search_field_, 0, wxEXPAND | wxALL, 6);

	for (size_t index = 0; index < kFindByButtons.size(); ++index) {
		const auto [mode, label] = kFindByButtons[index];
		const long style = index == 0 ? wxRB_GROUP : 0;
		auto* button = newd wxRadioButton(find_by_sizer->GetStaticBox(), wxID_ANY, wxString::FromUTF8(label.data(), label.size()), wxDefaultPosition, wxDefaultSize, style);
		find_by_buttons_[toIndex(mode)] = button;
		find_by_sizer->Add(button, 0, wxLEFT | wxRIGHT | wxTOP, 6);
	}

	find_by_sizer->AddSpacer(10);
	find_by_sizer->Add(makeSubtleText(find_by_sizer->GetStaticBox(), "Searches by:", true), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
	find_by_sizer->Add(makeSubtleText(find_by_sizer->GetStaticBox(), "- Name (fuzzy)\n- Server ID\n- Client ID\n\nLeave empty to search by filters only."), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
	content_sizer->Add(find_by_sizer, 0, wxEXPAND | wxALL, 6);

	auto* type_sizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Types"), wxVERTICAL);
	type_sizer->Add(makeSubtleText(type_sizer->GetStaticBox(), "(OR logic)"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
	for (const auto& [type_filter, label] : kTypeCheckboxes) {
		auto* checkbox = newd wxCheckBox(type_sizer->GetStaticBox(), wxID_ANY, wxString::FromUTF8(label.data(), label.size()));
		type_checkboxes_[toIndex(type_filter)] = checkbox;
		type_sizer->Add(checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
	}
	content_sizer->Add(type_sizer, 0, wxEXPAND | wxALL, 6);

	auto* properties_sizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Properties"), wxVERTICAL);
	properties_sizer->Add(makeSubtleText(properties_sizer->GetStaticBox(), "(AND logic)"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
	for (const auto& [property_filter, label] : kPropertyCheckboxes) {
		auto* checkbox = newd wxCheckBox(properties_sizer->GetStaticBox(), wxID_ANY, wxString::FromUTF8(label.data(), label.size()));
		property_checkboxes_[toIndex(property_filter)] = checkbox;
		properties_sizer->Add(checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
	}

	properties_sizer->AddSpacer(6);
	{
		auto* line = newd wxStaticLine(properties_sizer->GetStaticBox(), wxID_ANY);
		properties_sizer->Add(line, 0, wxEXPAND | wxALL, 6);
	}
	properties_sizer->Add(makeSectionLabel(properties_sizer->GetStaticBox(), "Interaction"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
	for (const auto& [interaction_filter, label] : kInteractionCheckboxes) {
		auto* checkbox = newd wxCheckBox(properties_sizer->GetStaticBox(), wxID_ANY, wxString::FromUTF8(label.data(), label.size()));
		interaction_checkboxes_[toIndex(interaction_filter)] = checkbox;
		properties_sizer->Add(checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
	}

	properties_sizer->AddSpacer(6);
	{
		auto* line = newd wxStaticLine(properties_sizer->GetStaticBox(), wxID_ANY);
		properties_sizer->Add(line, 0, wxEXPAND | wxALL, 6);
	}
	properties_sizer->Add(makeSectionLabel(properties_sizer->GetStaticBox(), "Visuals/Misc"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
	for (const auto& [visual_filter, label] : kVisualCheckboxes) {
		auto* checkbox = newd wxCheckBox(properties_sizer->GetStaticBox(), wxID_ANY, wxString::FromUTF8(label.data(), label.size()));
		visual_checkboxes_[toIndex(visual_filter)] = checkbox;
		properties_sizer->Add(checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
	}
	content_sizer->Add(properties_sizer, 0, wxEXPAND | wxALL, 6);

	auto* result_sizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Result (0)"), wxVERTICAL);
	result_box_ = result_sizer->GetStaticBox();
	result_list_ = newd AdvancedFinderResultListBox(result_box_, wxID_ANY);
	result_list_->SetMinSize(FROM_DIP(result_list_, wxSize(380, 620)));
	result_sizer->Add(result_list_, 1, wxEXPAND | wxALL, 6);
	content_sizer->Add(result_sizer, 1, wxEXPAND | wxALL, 6);

	root_sizer->Add(content_sizer, 1, wxEXPAND | wxALL, 4);

	auto* button_sizer = newd wxBoxSizer(wxHORIZONTAL);
	if (action_set_ == ActionSet::SearchAndSelect) {
		search_map_button_ = newd wxButton(this, wxID_FIND, "Search Map");
		search_map_button_->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_SEARCH, wxSize(16, 16)));
		button_sizer->Add(search_map_button_, 0, wxRIGHT, 8);

		select_item_button_ = newd wxButton(this, wxID_OK, "Select Item");
		select_item_button_->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
		button_sizer->Add(select_item_button_, 0, wxRIGHT, 8);
	} else {
		ok_button_ = newd wxButton(this, wxID_OK, "OK");
		ok_button_->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
		button_sizer->Add(ok_button_, 0, wxRIGHT, 8);
	}

	cancel_button_ = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancel_button_->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	button_sizer->Add(cancel_button_, 0);
	root_sizer->Add(button_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	SetSizerAndFit(root_sizer);
}

void FindItemDialog::bindEvents() {
	search_field_->Bind(wxEVT_TEXT, &FindItemDialog::OnTextChanged, this);
	search_field_->Bind(wxEVT_TEXT_ENTER, &FindItemDialog::OnTextEnter, this);

	for (wxRadioButton* button : find_by_buttons_) {
		button->Bind(wxEVT_RADIOBUTTON, &FindItemDialog::OnFindByChanged, this);
	}

	for (wxCheckBox* checkbox : type_checkboxes_) {
		checkbox->Bind(wxEVT_CHECKBOX, &FindItemDialog::OnFilterChanged, this);
	}
	for (wxCheckBox* checkbox : property_checkboxes_) {
		checkbox->Bind(wxEVT_CHECKBOX, &FindItemDialog::OnFilterChanged, this);
	}
	for (wxCheckBox* checkbox : interaction_checkboxes_) {
		checkbox->Bind(wxEVT_CHECKBOX, &FindItemDialog::OnFilterChanged, this);
	}
	for (wxCheckBox* checkbox : visual_checkboxes_) {
		checkbox->Bind(wxEVT_CHECKBOX, &FindItemDialog::OnFilterChanged, this);
	}

	result_list_->Bind(wxEVT_LISTBOX, &FindItemDialog::OnResultSelection, this);
	result_list_->Bind(wxEVT_LISTBOX_DCLICK, &FindItemDialog::OnResultActivate, this);

	if (search_map_button_ != nullptr) {
		search_map_button_->Bind(wxEVT_BUTTON, &FindItemDialog::OnSearchMap, this);
	}
	if (select_item_button_ != nullptr) {
		select_item_button_->Bind(wxEVT_BUTTON, &FindItemDialog::OnSelectItem, this);
	}
	if (ok_button_ != nullptr) {
		ok_button_->Bind(wxEVT_BUTTON, &FindItemDialog::OnSelectItem, this);
	}
	cancel_button_->Bind(wxEVT_BUTTON, &FindItemDialog::OnCancel, this);
	input_timer_.Bind(wxEVT_TIMER, &FindItemDialog::OnInputTimer, this);
}

void FindItemDialog::loadInitialState() {
	catalog_ = BuildAdvancedFinderCatalog(include_creatures_);
	query_ = {};
	current_selection_ = {};

	if (persist_shared_state_) {
		persisted_state_ = LoadAdvancedFinderPersistedState();
		query_ = persisted_state_.query;
		current_selection_ = persisted_state_.selection;
		if (persisted_state_.size.IsFullySpecified()) {
			SetSize(persisted_state_.size);
		}
		if (persisted_state_.position != wxDefaultPosition) {
			Move(persisted_state_.position);
		} else {
			Centre(wxBOTH);
		}
	} else {
		SetSize(wxSize(1480, 860));
		Centre(wxBOTH);
	}

	if (only_pickupables_) {
		query_.interaction_mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Pickupable);
	}

	applyQueryToControls();
	refreshResults();

	if (action_set_ == ActionSet::SearchAndSelect) {
		if (default_action_ == AdvancedFinderDefaultAction::SearchMap) {
			search_map_button_->SetDefault();
		} else {
			select_item_button_->SetDefault();
		}
	} else if (ok_button_ != nullptr) {
		ok_button_->SetDefault();
	}

	search_field_->SetFocus();
	search_field_->SetSelection(-1, -1);
}

void FindItemDialog::savePersistedState() {
	if (!persist_shared_state_) {
		return;
	}

	auto state = persisted_state_;
	state.query = query_;
	state.selection = current_selection_;
	if (result_action_ == ResultAction::SearchMap) {
		state.last_action = AdvancedFinderDefaultAction::SearchMap;
	} else if (result_action_ == ResultAction::SelectItem) {
		state.last_action = AdvancedFinderDefaultAction::SelectItem;
	}
	state.position = GetPosition();
	state.size = GetSize();
	SaveAdvancedFinderPersistedState(state);
}

void FindItemDialog::applyQueryToControls() const {
	search_field_->ChangeValue(wxstr(query_.text));

	for (const auto& [mode, _] : kFindByButtons) {
		find_by_buttons_[toIndex(mode)]->SetValue(query_.find_by == mode);
	}

	for (const auto& [type_filter, _] : kTypeCheckboxes) {
		type_checkboxes_[toIndex(type_filter)]->SetValue((query_.type_mask & advancedFinderBit(type_filter)) != 0);
	}
	for (const auto& [property_filter, _] : kPropertyCheckboxes) {
		property_checkboxes_[toIndex(property_filter)]->SetValue((query_.property_mask & advancedFinderBit(property_filter)) != 0);
	}
	for (const auto& [interaction_filter, _] : kInteractionCheckboxes) {
		interaction_checkboxes_[toIndex(interaction_filter)]->SetValue((query_.interaction_mask & advancedFinderBit(interaction_filter)) != 0);
	}
	for (const auto& [visual_filter, _] : kVisualCheckboxes) {
		visual_checkboxes_[toIndex(visual_filter)]->SetValue((query_.visual_mask & advancedFinderBit(visual_filter)) != 0);
	}

	if (only_pickupables_) {
		auto* pickupable = interaction_checkboxes_[toIndex(AdvancedFinderInteractionFilter::Pickupable)];
		pickupable->SetValue(true);
		pickupable->Enable(false);
	}

	if (!include_creatures_) {
		auto* creature_checkbox = type_checkboxes_[toIndex(AdvancedFinderTypeFilter::Creature)];
		creature_checkbox->SetValue(false);
		creature_checkbox->Enable(false);
	}
}

void FindItemDialog::readQueryFromControls() {
	query_.text = nstr(search_field_->GetValue());

	for (const auto& [mode, _] : kFindByButtons) {
		if (find_by_buttons_[toIndex(mode)]->GetValue()) {
			query_.find_by = mode;
			break;
		}
	}

	query_.type_mask = 0;
	query_.property_mask = 0;
	query_.interaction_mask = 0;
	query_.visual_mask = 0;

	for (const auto& [type_filter, _] : kTypeCheckboxes) {
		if (type_checkboxes_[toIndex(type_filter)]->GetValue()) {
			query_.type_mask |= advancedFinderBit(type_filter);
		}
	}
	for (const auto& [property_filter, _] : kPropertyCheckboxes) {
		if (property_checkboxes_[toIndex(property_filter)]->GetValue()) {
			query_.property_mask |= advancedFinderBit(property_filter);
		}
	}
	for (const auto& [interaction_filter, _] : kInteractionCheckboxes) {
		if (interaction_checkboxes_[toIndex(interaction_filter)]->GetValue()) {
			query_.interaction_mask |= advancedFinderBit(interaction_filter);
		}
	}
	for (const auto& [visual_filter, _] : kVisualCheckboxes) {
		if (visual_checkboxes_[toIndex(visual_filter)]->GetValue()) {
			query_.visual_mask |= advancedFinderBit(visual_filter);
		}
	}

	if (only_pickupables_) {
		query_.interaction_mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Pickupable);
	}
	if (!include_creatures_) {
		query_.type_mask &= ~advancedFinderBit(AdvancedFinderTypeFilter::Creature);
	}
}

void FindItemDialog::refreshResults() {
	readQueryFromControls();
	updateResultTitle(0);

	if (query_.text.empty() && !query_.hasAnyFilter()) {
		result_list_->SetPrompt("No matching items", "Enter search term or select filters");
		filtered_indices_.clear();
		updateCurrentSelection();
		updateButtons();
		return;
	}

	filtered_indices_ = FilterAdvancedFinderCatalog(catalog_, query_);
	if (filtered_indices_.empty()) {
		result_list_->SetNoMatches("No matching items", "Adjust the search term or filters");
		updateResultTitle(0);
		updateCurrentSelection();
		updateButtons();
		return;
	}

	std::vector<const AdvancedFinderCatalogRow*> rows;
	rows.reserve(filtered_indices_.size());
	for (const size_t filtered_index : filtered_indices_) {
		rows.push_back(&catalog_[filtered_index]);
	}

	int selection_index = -1;
	for (size_t index = 0; index < rows.size(); ++index) {
		if (AdvancedFinderSelectionMatches(*rows[index], current_selection_)) {
			selection_index = static_cast<int>(index);
			break;
		}
	}

	result_list_->SetRows(std::move(rows));
	updateResultTitle(filtered_indices_.size());

	if (selection_index < 0) {
		selection_index = 0;
	}

	result_list_->SetSelection(selection_index);
	result_list_->EnsureVisible(selection_index);
	updateCurrentSelection();
	updateButtons();
}

void FindItemDialog::updateButtons() {
	const bool has_selection = result_list_->GetSelectedRow() != nullptr;

	if (search_map_button_ != nullptr) {
		search_map_button_->Enable(has_selection);
	}
	if (select_item_button_ != nullptr) {
		select_item_button_->Enable(has_selection);
	}
	if (ok_button_ != nullptr) {
		ok_button_->Enable(has_selection);
	}
}

void FindItemDialog::updateResultTitle(size_t count) const {
	if (result_box_ != nullptr) {
		result_box_->SetLabel(wxString::Format("Result (%zu)", count));
	}
}

void FindItemDialog::updateCurrentSelection() {
	if (const auto* row = result_list_->GetSelectedRow()) {
		current_selection_ = MakeAdvancedFinderSelectionKey(*row);
	} else {
		current_selection_ = {};
	}
}

void FindItemDialog::triggerDefaultAction() {
	if (action_set_ == ActionSet::SearchAndSelect) {
		handlePositiveAction(default_action_ == AdvancedFinderDefaultAction::SearchMap ? ResultAction::SearchMap : ResultAction::SelectItem);
	} else {
		handlePositiveAction(ResultAction::ConfirmSelection);
	}
}

void FindItemDialog::handlePositiveAction(ResultAction action) {
	const AdvancedFinderCatalogRow* selected_row = result_list_->GetSelectedRow();
	if (selected_row == nullptr) {
		return;
	}

	result_kind_ = selected_row->kind;
	result_brush_ = selected_row->brush;
	result_creature_brush_ = selected_row->creature_brush;
	result_id_ = selected_row->isItem() ? selected_row->server_id : 0;
	result_creature_name_ = selected_row->isCreature() ? selected_row->label : std::string {};
	result_action_ = action;
	updateCurrentSelection();

	EndModal(action == ResultAction::SearchMap ? wxID_FIND : wxID_OK);
}

bool FindItemDialog::shouldApplyLegacyFallback() const {
	return query_.text.empty() && !query_.hasAnyFilter() && current_selection_.server_id == 0 && current_selection_.creature_name.empty();
}

void FindItemDialog::OnFindByChanged(wxCommandEvent& WXUNUSED(event)) {
	refreshResults();
}

void FindItemDialog::OnFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	refreshResults();
}

void FindItemDialog::OnTextChanged(wxCommandEvent& WXUNUSED(event)) {
	query_.text = nstr(search_field_->GetValue());
	input_timer_.Start(250, true);
}

void FindItemDialog::OnInputTimer(wxTimerEvent& WXUNUSED(event)) {
	refreshResults();
}

void FindItemDialog::OnResultSelection(wxCommandEvent& WXUNUSED(event)) {
	updateCurrentSelection();
	updateButtons();
}

void FindItemDialog::OnResultActivate(wxCommandEvent& WXUNUSED(event)) {
	triggerDefaultAction();
}

void FindItemDialog::OnSearchMap(wxCommandEvent& WXUNUSED(event)) {
	handlePositiveAction(ResultAction::SearchMap);
}

void FindItemDialog::OnSelectItem(wxCommandEvent& WXUNUSED(event)) {
	handlePositiveAction(action_set_ == ActionSet::SearchAndSelect ? ResultAction::SelectItem : ResultAction::ConfirmSelection);
}

void FindItemDialog::OnCancel(wxCommandEvent& WXUNUSED(event)) {
	result_action_ = ResultAction::None;
	EndModal(wxID_CANCEL);
}

void FindItemDialog::OnTextEnter(wxCommandEvent& WXUNUSED(event)) {
	triggerDefaultAction();
}
