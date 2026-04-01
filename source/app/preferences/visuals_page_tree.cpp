#include "app/preferences/visuals_page.h"

#include "app/preferences/visuals_page_helpers.h"

#include <algorithm>
#include <array>
#include <map>
#include <ranges>

#include <wx/clntdata.h>
#include <wx/srchctrl.h>
#include <wx/treelist.h>

using namespace VisualsPageHelpers;

namespace {

class VisualTreeItemData final : public wxClientData {
public:
	explicit VisualTreeItemData(std::string value = {}) : key(std::move(value)) {}

	[[nodiscard]] bool isLeaf() const {
		return !key.empty();
	}

	std::string key;
};

constexpr std::array<const char*, 5> CategoryTitles {
	"Items",
	"Markers",
	"Overlays",
	"Tile Zone",
	"Custom Items",
};

wxTreeListItem findItemByKey(const wxTreeListCtrl* tree, wxTreeListItem parent, const std::string& key) {
	for (auto item = tree->GetFirstChild(parent); item.IsOk(); item = tree->GetNextSibling(item)) {
		if (const auto* data = dynamic_cast<VisualTreeItemData*>(tree->GetItemData(item)); data && data->key == key) {
			return item;
		}
		if (const auto child = findItemByKey(tree, item, key); child.IsOk()) {
			return child;
		}
	}

	return {};
}

}

std::vector<VisualsPage::DisplayEntry> VisualsPage::BuildDisplayEntries() const {
	std::map<std::string, bool> keys;
	for (const auto& entry : g_visuals.BuildCatalog()) {
		if (const auto* rule = entry.effective()) {
			keys[rule->key] = true;
		}
	}
	for (const auto& entry : working_copy_.BuildCatalog()) {
		if (const auto* rule = entry.effective()) {
			keys[rule->key] = true;
		}
	}
	for (const auto& [key, _] : draft_rules_) {
		keys[key] = true;
	}

	const std::string filter = search_ctrl_ ? LowercaseCopy(search_ctrl_->GetValue().ToStdString()) : std::string {};

	std::vector<DisplayEntry> entries;
	for (const auto& [key, _] : keys) {
		const DisplayEntry entry = MakeDisplayEntry(key);
		if (filter.empty() || SearchTextFor(entry).find(filter) != std::string::npos) {
			entries.push_back(entry);
		}
	}

	std::ranges::sort(entries, [](const DisplayEntry& left, const DisplayEntry& right) {
		if (left.category != right.category) {
			return left.category < right.category;
		}
		if (left.section != right.section) {
			return left.section < right.section;
		}
		return left.seed_rule.label < right.seed_rule.label;
	});

	return entries;
}

std::optional<VisualRule> VisualsPage::RuleForKey(const Visuals& visuals, const std::string& key) const {
	if (const auto* rule = visuals.GetEffectiveRule(key)) {
		return *rule;
	}

	if (const auto item_id = ParseItemId(key); item_id.has_value()) {
		return Visuals::MakeItemRule(*item_id);
	}
	return std::nullopt;
}

std::optional<uint16_t> VisualsPage::ParseItemId(const std::string& key) const {
	static constexpr std::string_view Prefix = "item.id.";
	if (!key.starts_with(Prefix)) {
		return std::nullopt;
	}

	try {
		return static_cast<uint16_t>(std::stoi(key.substr(Prefix.size())));
	} catch (...) {
		return std::nullopt;
	}
}

VisualRule VisualsPage::NormalizeRule(VisualRule rule) const {
	Visuals scratch = working_copy_;
	scratch.SetUserRule(rule);
	if (const auto* normalized = scratch.GetUserRule(rule.key.empty() ? Visuals::MakeKeyForItemId(static_cast<uint16_t>(rule.match_id)) : rule.key)) {
		return *normalized;
	}
	return rule;
}

VisualsPage::DisplayEntry VisualsPage::MakeDisplayEntry(const std::string& key) const {
	DisplayEntry entry;
	entry.key = key;
	entry.current_rule = RuleForKey(g_visuals, key);
	entry.preview_rule = draft_rules_.contains(key) ? std::optional<VisualRule>(draft_rules_.at(key)) : RuleForKey(working_copy_, key);
	entry.current_has_override = g_visuals.GetUserRule(key) != nullptr;
	entry.preview_has_override = working_copy_.GetUserRule(key) != nullptr || draft_rules_.contains(key);
	const bool preview_is_fallback_item = !working_copy_.GetEffectiveRule(key) && ParseItemId(key).has_value();
	entry.seed_rule = preview_is_fallback_item && entry.current_rule ? *entry.current_rule :
		entry.preview_rule ? *entry.preview_rule :
		entry.current_rule ? *entry.current_rule :
		VisualRule { .key = key };
	entry.category = ClassifyCategory(entry.seed_rule, entry.preview_has_override || entry.current_has_override);
	entry.section = ClassifySection(entry.seed_rule, entry.category, entry.preview_has_override || entry.current_has_override);
	return entry;
}

VisualsPage::Category VisualsPage::ClassifyCategory(const VisualRule& seed_rule, bool preview_has_override) const {
	switch (seed_rule.match_type) {
		case VisualMatchType::ItemId:
			return preview_has_override && !working_copy_.GetDefaultRule(seed_rule.key) ? Category::CustomItems : Category::Items;
		case VisualMatchType::Marker:
			return Category::Markers;
		case VisualMatchType::Overlay:
			return Category::Overlays;
		case VisualMatchType::Tile:
			return Category::TileZone;
		case VisualMatchType::ClientId:
			return Category::Items;
	}
	return Category::Items;
}

std::string VisualsPage::ClassifySection(const VisualRule& seed_rule, Category category, bool) const {
	if (category == Category::Items) {
		if (seed_rule.label.contains("Primal")) {
			return "Primal";
		}
		if (seed_rule.label.contains("Invisible")) {
			return "Technical";
		}
		return "Defaults";
	}
	if (category == Category::Markers) {
		return "Map Markers";
	}
	if (category == Category::Overlays) {
		return "Indicators";
	}
	if (category == Category::TileZone) {
		if (seed_rule.match_value == "house_overlay") {
			return "House";
		}
		return "Zone & Tile";
	}
	return "Custom Item Overrides";
}

std::string VisualsPage::SearchTextFor(const DisplayEntry& entry) const {
	return LowercaseCopy(entry.seed_rule.label + " " + entry.seed_rule.key + " " + entry.section);
}

void VisualsPage::RebuildTree() {
	if (shutting_down_ || !tree_) {
		return;
	}

	const auto entries = BuildDisplayEntries();
	const std::string previous_selection = selected_key_;
	std::string first_leaf_key;

	tree_->Freeze();
	tree_->DeleteAllItems();

	const auto root = tree_->GetRootItem();
	for (size_t index = 0; index < CategoryTitles.size(); ++index) {
		const auto category = static_cast<Category>(index);
		const auto count = static_cast<int>(std::count_if(entries.begin(), entries.end(), [category](const DisplayEntry& entry) {
			return entry.category == category;
		}));

		auto category_item = tree_->AppendItem(root, wxString::Format("%s (%d)", CategoryTitles[index], count), -1, -1, new VisualTreeItemData);
		tree_->Expand(category_item);

		std::string current_section;
		wxTreeListItem section_item;
		for (const auto& entry : entries) {
			if (entry.category != category) {
				continue;
			}

			if (first_leaf_key.empty()) {
				first_leaf_key = entry.key;
			}

			if (entry.section != current_section) {
				current_section = entry.section;
				section_item = tree_->AppendItem(category_item, wxString::FromUTF8(current_section), -1, -1, new VisualTreeItemData);
				tree_->Expand(section_item);
			}

			auto leaf = tree_->AppendItem(section_item, BuildVisualName(entry), -1, -1, new VisualTreeItemData(entry.key));
			tree_->SetItemText(leaf, 1, BuildTypeLabel(entry.preview_rule));
		}
	}

	tree_->Thaw();

	if (!focus_key_.empty()) {
		SelectKey(focus_key_);
		focus_key_.clear();
		return;
	}
	if (!previous_selection.empty()) {
		SelectKey(previous_selection);
		if (selected_key_ == previous_selection) {
			return;
		}
	}
	if (!first_leaf_key.empty()) {
		SelectKey(first_leaf_key);
		return;
	}

	selected_key_.clear();
	RefreshDetail();
}

void VisualsPage::SelectKey(const std::string& key) {
	if (!tree_) {
		selected_key_.clear();
		RefreshDetail();
		return;
	}

	const auto item = findItemByKey(tree_, tree_->GetRootItem(), key);
	if (!item.IsOk()) {
		selected_key_.clear();
		tree_->UnselectAll();
		RefreshDetail();
		return;
	}

	suppress_events_ = true;
	selected_key_ = key;
	tree_->Select(item);
	tree_->EnsureVisible(item);
	suppress_events_ = false;
	RefreshDetail();
}

void VisualsPage::FocusRow(const std::string& key) {
	SelectKey(key);
}

void VisualsPage::ActivateSelectedEntry() {
	RefreshDetail();
}

void VisualsPage::OnTreeSelectionChanged(wxTreeListEvent& event) {
	if (suppress_events_) {
		event.Skip();
		return;
	}

	if (!tree_) {
		return;
	}

	const auto item = event.GetItem();
	const auto* data = dynamic_cast<VisualTreeItemData*>(tree_->GetItemData(item));
	if (!data || !data->isLeaf()) {
		if (!selected_key_.empty()) {
			FocusRow(selected_key_);
		}
		return;
	}

	selected_key_ = data->key;
	ActivateSelectedEntry();
	event.Skip();
}
