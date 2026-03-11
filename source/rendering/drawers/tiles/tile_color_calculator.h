#ifndef RME_RENDERING_TILE_COLOR_CALCULATOR_H_
#define RME_RENDERING_TILE_COLOR_CALCULATOR_H_

#include <cstdint>

class Tile;
struct TileRenderSnapshot;
struct RenderSettings;
struct FrameOptions;

class TileColorCalculator {
public:
	static void Calculate(const Tile* tile, const RenderSettings& settings, uint32_t current_house_id, int spawn_count, uint8_t& r, uint8_t& g, uint8_t& b, float highlight_pulse = 0.0f);
	static void Calculate(
		const TileRenderSnapshot& tile, const RenderSettings& settings, uint8_t& r, uint8_t& g, uint8_t& b, float highlight_pulse = 0.0f
	);
	static void GetHouseColor(uint32_t house_id, uint8_t& r, uint8_t& g, uint8_t& b);
	static void GetMinimapColor(const Tile* tile, uint8_t& r, uint8_t& g, uint8_t& b);
	static void GetMinimapColor(const TileRenderSnapshot& tile, uint8_t& r, uint8_t& g, uint8_t& b);
};

#endif
