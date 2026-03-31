#include "app/visuals.h"

#include "util/file_system.h"
#include "util/image_manager.h"

#include <array>
#include <ranges>

#include <spdlog/spdlog.h>
#include <wx/filefn.h>
#include <wx/filename.h>

namespace {

uint64_t stableHash64(std::string_view value) {
	uint64_t hash = 1469598103934665603ull;
	for (const unsigned char character : value) {
		hash ^= character;
		hash *= 1099511628211ull;
	}
	return hash;
}

bool isImageRule(const VisualRule& rule) {
	return rule.appearance.type == VisualAppearanceType::Png || rule.appearance.type == VisualAppearanceType::Svg;
}

std::string lowercase(std::string value) {
	std::ranges::transform(value, value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

wxString normalizeManagedPath(const wxString& full_path) {
	wxFileName relative(full_path);
	relative.MakeRelativeTo(FileSystem::GetLocalDirectory());
	return relative.GetFullPath();
}

}

wxString Visuals::ResolveAssetPath(const std::string& asset_path) {
	if (asset_path.empty()) {
		return {};
	}

	const wxString resolved = wxString::FromUTF8(IMAGE_MANAGER.ResolvePath(asset_path));
	return wxFileName::FileExists(resolved) ? resolved : wxString {};
}

wxString Visuals::GetManagedAssetsDirectory() {
	wxFileName dir = wxFileName::DirName(FileSystem::GetLocalDirectory());
	dir.AppendDir("visuals_assets");
	dir.Mkdir(0755, wxPATH_MKDIR_FULL);
	return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

bool Visuals::EnsureManagedAssetPath(VisualRule& rule) const {
	if (!isImageRule(rule) || rule.appearance.asset_path.empty()) {
		return true;
	}

	const wxString source_path = ResolveAssetPath(rule.appearance.asset_path);
	if (source_path.empty()) {
		ValidateRule(rule);
		return false;
	}

	wxFileName source_file(source_path);
	const wxString extension = source_file.GetExt().Lower();
	const wxString expected_extension = rule.appearance.type == VisualAppearanceType::Png ? "png" : "svg";
	if (extension != expected_extension) {
		rule.valid = false;
		rule.validation_error = "Image path extension does not match the selected appearance type.";
		return false;
	}

	const auto hash_input = source_path.ToStdString();
	const wxString managed_name = wxString::Format("%016llx.%s", stableHash64(hash_input), extension);
	wxFileName managed_dir = wxFileName::DirName(GetManagedAssetsDirectory());
	managed_dir.Mkdir(0755, wxPATH_MKDIR_FULL);
	wxFileName managed_file = managed_dir;
	managed_file.SetFullName(managed_name);

	if (source_file.GetFullPath() != managed_file.GetFullPath()) {
		if (!wxCopyFile(source_file.GetFullPath(), managed_file.GetFullPath(), true)) {
			spdlog::error("Failed to copy visual asset {} to {}", source_file.GetFullPath().ToStdString(), managed_file.GetFullPath().ToStdString());
			rule.valid = false;
			rule.validation_error = "Failed to copy the selected asset into the managed visuals folder.";
			return false;
		}
	}

	rule.appearance.asset_path = normalizeManagedPath(managed_file.GetFullPath()).ToStdString();
	ValidateRule(rule);
	return rule.valid;
}

bool Visuals::PrepareUserRulesForSerialization() const {
	EnsureServerItemRulesMaterialized();

	for (const auto& [key, rule] : user_rules) {
		base_user_rules[key] = rule;
	}
	legacy_user_client_rules.clear();

	bool success = true;
	for (auto& [_, rule] : base_user_rules) {
		success = EnsureManagedAssetPath(rule) && success;
	}

	auto& self = const_cast<Visuals&>(*this);
	self.InvalidateResolvedRules();
	self.EnsureServerItemRulesMaterialized();
	return success;
}
