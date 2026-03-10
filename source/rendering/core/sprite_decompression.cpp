//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/core/sprite_decompression.h"
#include "rendering/core/outfit_colorizer.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <ranges>
#include <span>

constexpr int RGB_COMPONENTS = 3;
constexpr int RGBA_COMPONENTS = 4;

namespace {

struct DecompressionContext {
	int id;
	uint8_t bpp;
	bool use_alpha;
	bool& non_zero_alpha_found;
	bool& non_black_pixel_found;
};

bool ProcessTransparencyRun(std::span<const uint8_t> dump, size_t& read, std::span<uint8_t> data, size_t& write, DecompressionContext ctx) {
	if (read + 1 >= dump.size()) {
		return false;
	}
	int transparent = dump[read] | dump[read + 1] << 8;

	if (write + (transparent * RGBA_COMPONENTS) > data.size()) {
		spdlog::warn("Sprite {}: Transparency run overrun (transparent={}, write={}, max={})", ctx.id, transparent, write, data.size());
		transparent = (data.size() - write) / RGBA_COMPONENTS;
	}

	read += 2;
	std::ranges::fill(data.subspan(write, transparent * RGBA_COMPONENTS), 0);
	write += transparent * RGBA_COMPONENTS;
	return true;
}

bool ProcessColoredRun(std::span<const uint8_t> dump, size_t& read, std::span<uint8_t> data, size_t& write, DecompressionContext ctx) {
	if (read + 1 >= dump.size()) {
		return false;
	}
	int colored = dump[read] | dump[read + 1] << 8;
	read += 2;

	if (write + (colored * RGBA_COMPONENTS) > data.size()) {
		spdlog::warn("Sprite {}: Colored run overrun (colored={}, write={}, max={})", ctx.id, colored, write, data.size());
		colored = (data.size() - write) / RGBA_COMPONENTS;
	}

	if (read + (colored * ctx.bpp) > dump.size()) {
		spdlog::warn("Sprite {}: Read buffer overrun (colored={}, bpp={}, read={}, size={})", ctx.id, colored, ctx.bpp, read, dump.size());
		return false;
	}

	for (int cnt = 0; cnt < colored; ++cnt) {
		uint8_t r = dump[read + 0];
		uint8_t g = dump[read + 1];
		uint8_t b = dump[read + 2];
		uint8_t a = ctx.use_alpha ? dump[read + 3] : 0xFF;

		data[write + 0] = r;
		data[write + 1] = g;
		data[write + 2] = b;
		data[write + 3] = a;

		if (a > 0) {
			ctx.non_zero_alpha_found = true;
		}
		if (r > 0 || g > 0 || b > 0) {
			ctx.non_black_pixel_found = true;
		}

		write += RGBA_COMPONENTS;
		read += ctx.bpp;
	}
	return true;
}

} // namespace

namespace SpriteDecompression {

std::unique_ptr<uint8_t[]> Decompress(std::span<const uint8_t> dump, bool use_alpha, int id) {
	const int pixels_data_size = SPRITE_PIXELS_SIZE * RGBA_COMPONENTS;
	auto data_buffer = std::make_unique<uint8_t[]>(pixels_data_size);

	std::span<uint8_t> data(data_buffer.get(), pixels_data_size);

	uint8_t bpp = use_alpha ? 4 : 3;
	size_t write = 0;
	size_t read = 0;
	bool non_zero_alpha_found = false;
	bool non_black_pixel_found = false;

	DecompressionContext ctx {
		.id = id,
		.bpp = bpp,
		.use_alpha = use_alpha,
		.non_zero_alpha_found = non_zero_alpha_found,
		.non_black_pixel_found = non_black_pixel_found
	};

	while (read < dump.size() && write < data.size()) {
		if (!ProcessTransparencyRun(dump, read, data, write, ctx)) {
			break;
		}

		if (read >= dump.size() || write >= data.size()) {
			break;
		}

		if (!ProcessColoredRun(dump, read, data, write, ctx)) {
			break;
		}
	}

	while (write < data.size()) {
		data[write + 0] = 0x00;
		data[write + 1] = 0x00;
		data[write + 2] = 0x00;
		data[write + 3] = 0x00;
		write += RGBA_COMPONENTS;
	}

	if (!non_zero_alpha_found && id > 100) {
		static int empty_log_count = 0;
		if (empty_log_count++ < 10) {
			spdlog::info("Sprite {}: Decoded fully transparent sprite. bpp used: {}, dump size: {}", id, bpp, dump.size());
		}
	} else if (!non_black_pixel_found && non_zero_alpha_found && id > 100) {
		static int black_log_count = 0;
		if (black_log_count++ < 10) {
			spdlog::warn("Sprite {}: Decoded PURE BLACK sprite (Alpha > 0, RGB = 0). bpp used: {}, dump size: {}. Check hasTransparency() config!", id, bpp, dump.size());
		}
	}

	return data_buffer;
}

void ColorizeTemplatePixels(uint8_t* dest, const uint8_t* mask, size_t pixelCount,
	int lookHead, int lookBody, int lookLegs, int lookFeet, bool destHasAlpha) {
	const int dest_step = destHasAlpha ? RGBA_COMPONENTS : RGB_COMPONENTS;
	const int mask_step = RGB_COMPONENTS;

	std::span<uint8_t> destSpan(dest, pixelCount * dest_step);
	std::span<const uint8_t> maskSpan(mask, pixelCount * mask_step);

	for (size_t i : std::views::iota(0u, pixelCount)) {
		uint8_t& red = destSpan[i * dest_step + 0];
		uint8_t& green = destSpan[i * dest_step + 1];
		uint8_t& blue = destSpan[i * dest_step + 2];

		const uint8_t& tred = maskSpan[i * mask_step + 0];
		const uint8_t& tgreen = maskSpan[i * mask_step + 1];
		const uint8_t& tblue = maskSpan[i * mask_step + 2];

		if (tred && tgreen && !tblue) {
			OutfitColorizer::ColorizePixel(lookHead, red, green, blue);
		} else if (tred && !tgreen && !tblue) {
			OutfitColorizer::ColorizePixel(lookBody, red, green, blue);
		} else if (!tred && tgreen && !tblue) {
			OutfitColorizer::ColorizePixel(lookLegs, red, green, blue);
		} else if (!tred && !tgreen && tblue) {
			OutfitColorizer::ColorizePixel(lookFeet, red, green, blue);
		}
	}
}

} // namespace SpriteDecompression
