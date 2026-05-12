#include "editor/persistence/minimap_exporter_internal.h"

#include "editor/editor.h"
#include "map/map.h"
#include "map/map_region.h"
#include "rendering/core/minimap_colors.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <webp/encode.h>
#include <wx/image.h>

namespace minimap_export {
namespace {

constexpr double kCoveredFloorShade = 0.55;

[[nodiscard]] uint8_t shadedChannel(uint8_t channel, double shade) {
	return static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(channel * shade)), 0, 255));
}

[[nodiscard]] bool renderFloorIntoChunk(const Map& map, int floor, const Chunk& chunk, int imageSize, double shade, std::vector<uint8_t>& pixels) {
	bool wrotePixels = false;
	const int chunkEndX = chunk.x + imageSize;
	const int chunkEndY = chunk.y + imageSize;

	map.visitLeaves(chunk.x, chunk.y, chunkEndX, chunkEndY, [&](const MapNode* node, int nodeMapX, int nodeMapY) {
		const Floor* nodeFloor = node->getFloor(static_cast<uint32_t>(floor));
		if (!nodeFloor) {
			return;
		}

		for (int localNodeX = 0; localNodeX < 4; ++localNodeX) {
			const int mapX = nodeMapX + localNodeX;
			if (mapX < chunk.x || mapX >= chunkEndX) {
				continue;
			}

			for (int localNodeY = 0; localNodeY < 4; ++localNodeY) {
				const int mapY = nodeMapY + localNodeY;
				if (mapY < chunk.y || mapY >= chunkEndY) {
					continue;
				}

				const auto& location = nodeFloor->locs[static_cast<size_t>(localNodeX * 4 + localNodeY)];
				const Tile* tile = location.get();
				if (!isTileExportable(tile)) {
					continue;
				}

				const uint8_t colorIndex = tile->getMiniMapColor();
				if (colorIndex == 0) {
					continue;
				}

				const RGBQuad& color = minimap_color[colorIndex];
				const int pixelX = mapX - chunk.x;
				const int pixelY = mapY - chunk.y;
				const size_t index = (static_cast<size_t>(pixelY) * imageSize + pixelX) * PixelFormatRGB;
				pixels[index] = shadedChannel(color.red, shade);
				pixels[index + 1] = shadedChannel(color.green, shade);
				pixels[index + 2] = shadedChannel(color.blue, shade);
				wrotePixels = true;
			}
		}
	});
	return wrotePixels;
}

[[nodiscard]] std::vector<uint8_t> renderImageChunk(const Map& map, int targetFloor, const Chunk& chunk, const MinimapExportOptions& options, bool& wrotePixels) {
	std::vector<uint8_t> pixels(static_cast<size_t>(options.imageSize) * options.imageSize * PixelFormatRGB, 0);
	wrotePixels = false;

	if (!options.showAllFloors || options.floorMode == MinimapExportFloorMode::AllFloors) {
		wrotePixels = renderFloorIntoChunk(map, targetFloor, chunk, options.imageSize, 1.0, pixels);
		return pixels;
	}

	const auto floors = showAllFloorsForTarget(targetFloor);
	for (size_t index = 0; index < floors.count; ++index) {
		const int floor = floors.values[index];
		const double shade = floor == targetFloor ? 1.0 : kCoveredFloorShade;
		wrotePixels = renderFloorIntoChunk(map, floor, chunk, options.imageSize, shade, pixels) || wrotePixels;
	}
	return pixels;
}

[[nodiscard]] bool saveJpg(const wxFileName& file, int width, int height, const std::vector<uint8_t>& pixels) {
	wxImage image(width, height);
	uint8_t* data = image.GetData();
	if (!data) {
		return false;
	}

	std::memcpy(data, pixels.data(), pixels.size());
	return image.SaveFile(file.GetFullPath(), wxBITMAP_TYPE_JPEG);
}

[[nodiscard]] bool saveWebp(const wxFileName& file, int width, int height, const std::vector<uint8_t>& pixels) {
	uint8_t* encoded = nullptr;
	const size_t encodedSize = WebPEncodeLosslessRGB(pixels.data(), width, height, width * PixelFormatRGB, &encoded);
	if (encodedSize == 0 || !encoded) {
		return false;
	}

	std::ofstream stream(
#ifdef _WIN32
		std::filesystem::path(file.GetFullPath().ToStdWstring()),
#else
		std::filesystem::path(file.GetFullPath().ToStdString()),
#endif
		std::ios::binary);
	const bool opened = stream.good();
	if (opened) {
		stream.write(reinterpret_cast<const char*>(encoded), static_cast<std::streamsize>(encodedSize));
	}

	WebPFree(encoded);
	return opened && stream.good();
}

} // namespace

MinimapExportResult exportImages(Editor& editor, const MinimapExportOptions& options, const MinimapExporter::ProgressCallback& progress) {
	const FloorSelection boundsFloors = selectedFloorsForBounds(options);
	const Bounds bounds = findContentBounds(editor.map, boundsFloors.floors());
	const std::vector<Chunk> chunks = buildChunks(bounds, options.imageSize);
	if (chunks.empty()) {
		return { .ok = true };
	}

	const FloorSelection outputFloors = options.floorMode == MinimapExportFloorMode::AllFloors ? allFloors() : singleFloor(options.floorMode == MinimapExportFloorMode::GroundFloor ? GROUND_LAYER : options.selectedFloor);
	const uint64_t total = static_cast<uint64_t>(chunks.size() * outputFloors.count);
	uint64_t completed = 0;
	size_t filesWritten = 0;

	for (const int floor : outputFloors.floors()) {
		for (const Chunk& chunk : chunks) {
			bool wrotePixels = false;
			std::vector<uint8_t> pixels = renderImageChunk(editor.map, floor, chunk, options, wrotePixels);
			++completed;
			if (progress) {
				progress(completed, total);
			}
			if (!wrotePixels) {
				continue;
			}

			const wxFileName file = chunkFile(options.outputDirectory, options.fileBaseName, options.format, floor, chunk);
			const bool saved = options.format == MinimapExportFormat::Jpg
				? saveJpg(file, options.imageSize, options.imageSize, pixels)
				: saveWebp(file, options.imageSize, options.imageSize, pixels);
			if (!saved) {
				return { .ok = false, .filesWritten = filesWritten, .error = "Failed to write minimap image: " + file.GetFullName().ToStdString() };
			}
			++filesWritten;
		}
	}

	return { .ok = true, .filesWritten = filesWritten };
}

} // namespace minimap_export
