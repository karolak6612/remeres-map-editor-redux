#ifndef RME_GAME_MATERIAL_DATABASE_H_
#define RME_GAME_MATERIAL_DATABASE_H_

#include <string>
#include <vector>

class Brush;

struct DynamicTilesetDefinition {
	std::string name;
	std::vector<Brush*> brushes;

	[[nodiscard]] size_t size() const {
		return brushes.size();
	}

	[[nodiscard]] bool containsBrush(const Brush* brush) const;
};

struct DynamicPaletteDefinition {
	std::string name;
	std::vector<DynamicTilesetDefinition> tilesets;

	[[nodiscard]] bool containsBrush(const Brush* brush) const;
};

class PaletteCatalog {
public:
	void clear();
	void addDynamicPalette(DynamicPaletteDefinition palette);

	[[nodiscard]] const std::vector<DynamicPaletteDefinition>& dynamicPalettes() const {
		return palettes;
	}

	[[nodiscard]] const DynamicPaletteDefinition* findPalette(std::string_view name) const;
	[[nodiscard]] const DynamicPaletteDefinition* findPaletteContainingBrush(const Brush* brush, std::string_view preferredPalette = {}) const;

private:
	std::vector<DynamicPaletteDefinition> palettes;
};

class MaterialDatabase {
public:
	void clear();

	[[nodiscard]] PaletteCatalog& paletteCatalog() {
		return palettes;
	}

	[[nodiscard]] const PaletteCatalog& paletteCatalog() const {
		return palettes;
	}

private:
	PaletteCatalog palettes;
};

#endif
