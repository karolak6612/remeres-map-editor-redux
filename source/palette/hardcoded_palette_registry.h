#ifndef RME_PALETTE_HARDCODED_PALETTE_REGISTRY_H_
#define RME_PALETTE_HARDCODED_PALETTE_REGISTRY_H_

#include <string_view>
#include <vector>

class Map;
class PalettePanel;
class wxWindow;

struct HardcodedPaletteProvider {
	std::string_view name;
	PalettePanel* (*createPanel)(wxWindow* parent);
	void (*updateMap)(PalettePanel* panel, Map* map);
};

[[nodiscard]] const std::vector<HardcodedPaletteProvider>& GetHardcodedPaletteProviders();

#endif
