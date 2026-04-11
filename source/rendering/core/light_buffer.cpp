#include "light_buffer.h"

#include <cmath>

#include "app/definitions.h"
#include "rendering/core/render_view.h"

namespace {
	[[nodiscard]] int floorDiv(int value, int divisor) noexcept {
		const int quotient = value / divisor;
		const int remainder = value % divisor;
		return (remainder != 0 && ((remainder < 0) != (divisor < 0))) ? quotient - 1 : quotient;
	}
}

void LightBuffer::Prepare(const RenderView& view) {
	constexpr int buffer_margin_tiles = 1;

	origin_x = floorDiv(view.view_scroll_x, TILE_SIZE) - buffer_margin_tiles;
	origin_y = floorDiv(view.view_scroll_y, TILE_SIZE) - buffer_margin_tiles;
	width = std::max(1, static_cast<int>(std::ceil((view.view_scroll_x + view.logical_width) / static_cast<float>(TILE_SIZE))) - origin_x + buffer_margin_tiles);
	height = std::max(1, static_cast<int>(std::ceil((view.view_scroll_y + view.logical_height) / static_cast<float>(TILE_SIZE))) - origin_y + buffer_margin_tiles);
	tiles.assign(static_cast<size_t>(width * height), TileLight {});
}

void LightBuffer::AddLight(int pixel_x, int pixel_y, const SpriteLight& light) {
	const uint8_t intensity = std::min(light.intensity, static_cast<uint8_t>(255));

	if (!lights.empty()) {
		Light& previous = lights.back();
		if (previous.pixel_x == pixel_x && previous.pixel_y == pixel_y && previous.color == light.color) {
			previous.intensity = std::max(previous.intensity, intensity);
			return;
		}
	}

	lights.push_back(Light {
		.pixel_x = pixel_x,
		.pixel_y = pixel_y,
		.color = light.color,
		.intensity = intensity
	});
}

void LightBuffer::AddTileLight(int tile_x, int tile_y, const SpriteLight& light) {
	AddLight(tile_x * TILE_SIZE + TILE_SIZE / 2, tile_y * TILE_SIZE + TILE_SIZE / 2, light);
}

void LightBuffer::AddScreenLight(int screen_x, int screen_y, const RenderView& view, const SpriteLight& light) {
	AddLight(screen_x + view.view_scroll_x, screen_y + view.view_scroll_y, light);
}

void LightBuffer::SetFieldBrightness(int tile_x, int tile_y, size_t start, uint8_t color) {
	const int index = IndexOf(tile_x, tile_y);
	if (index < 0) {
		return;
	}

	tiles[static_cast<size_t>(index)] = TileLight {
		.start = static_cast<uint32_t>(start),
		.color = color
	};
}

void LightBuffer::Clear() {
	lights.clear();
	tiles.clear();
	origin_x = 0;
	origin_y = 0;
	width = 0;
	height = 0;
}

bool LightBuffer::ContainsTile(int tile_x, int tile_y) const noexcept {
	return tile_x >= origin_x && tile_y >= origin_y && tile_x < origin_x + width && tile_y < origin_y + height;
}

int LightBuffer::IndexOf(int tile_x, int tile_y) const noexcept {
	if (!ContainsTile(tile_x, tile_y)) {
		return -1;
	}
	return (tile_y - origin_y) * width + (tile_x - origin_x);
}
