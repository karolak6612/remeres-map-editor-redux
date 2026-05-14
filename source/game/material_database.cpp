#include "game/material_database.h"

#include "brushes/brush.h"

#include <algorithm>

bool DynamicTilesetDefinition::containsBrush(const Brush* brush) const {
	return brush && std::ranges::find(brushes, brush) != brushes.end();
}

bool DynamicPaletteDefinition::containsBrush(const Brush* brush) const {
	return brush && std::ranges::any_of(tilesets, [brush](const auto& tileset) {
		return tileset.containsBrush(brush);
	});
}

void PaletteCatalog::clear() {
	palettes.clear();
}

void PaletteCatalog::addDynamicPalette(DynamicPaletteDefinition palette) {
	palettes.push_back(std::move(palette));
}

const DynamicPaletteDefinition* PaletteCatalog::findPalette(std::string_view name) const {
	auto it = std::ranges::find_if(palettes, [name](const auto& palette) {
		return palette.name == name;
	});
	return it == palettes.end() ? nullptr : &*it;
}

const DynamicPaletteDefinition* PaletteCatalog::findPaletteContainingBrush(const Brush* brush, std::string_view preferredPalette) const {
	if (!brush) {
		return nullptr;
	}
	if (!preferredPalette.empty()) {
		if (const auto* palette = findPalette(preferredPalette); palette && palette->containsBrush(brush)) {
			return palette;
		}
	}
	auto it = std::ranges::find_if(palettes, [brush](const auto& palette) {
		return palette.containsBrush(brush);
	});
	return it == palettes.end() ? nullptr : &*it;
}

void MaterialDatabase::clear() {
	palettes.clear();
}
