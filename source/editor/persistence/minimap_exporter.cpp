#include "editor/persistence/minimap_exporter.h"

#include "editor/persistence/minimap_exporter_internal.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/tile.h"

#include <algorithm>
#include <stdexcept>

namespace minimap_export {

void FloorSelection::push(int floor) {
	if (floor >= 0 && floor < MAP_LAYERS && count < values.size()) {
		values[count++] = floor;
	}
}

std::span<const int> FloorSelection::floors() const {
	return { values.data(), count };
}

bool isTileExportable(const Tile* tile) {
	return tile && !tile->empty();
}

bool containsFloor(std::span<const int> floors, int floor) {
	return std::ranges::find(floors, floor) != floors.end();
}

FloorSelection allFloors() {
	FloorSelection selection;
	for (int floor = 0; floor < MAP_LAYERS; ++floor) {
		selection.push(floor);
	}
	return selection;
}

FloorSelection singleFloor(int floor) {
	FloorSelection selection;
	selection.push(std::clamp(floor, 0, MAP_MAX_LAYER));
	return selection;
}

FloorSelection showAllFloorsForTarget(int targetFloor) {
	FloorSelection selection;
	const int floor = std::clamp(targetFloor, 0, MAP_MAX_LAYER);
	if (floor <= GROUND_LAYER) {
		for (int visibleFloor = GROUND_LAYER; visibleFloor >= floor; --visibleFloor) {
			selection.push(visibleFloor);
		}
		return selection;
	}

	for (int visibleFloor = std::min(MAP_MAX_LAYER, floor + 2); visibleFloor >= floor; --visibleFloor) {
		selection.push(visibleFloor);
	}
	return selection;
}

FloorSelection selectedFloorsForBounds(const MinimapExportOptions& options) {
	if (options.floorMode == MinimapExportFloorMode::AllFloors) {
		return allFloors();
	}

	const int floor = options.floorMode == MinimapExportFloorMode::GroundFloor ? GROUND_LAYER : options.selectedFloor;
	if (options.format != MinimapExportFormat::Otmm && options.showAllFloors) {
		return showAllFloorsForTarget(floor);
	}
	return singleFloor(floor);
}

FloorSelection selectedFloorsForOtmm(const MinimapExportOptions& options) {
	if (options.floorMode == MinimapExportFloorMode::AllFloors) {
		return allFloors();
	}
	return singleFloor(options.floorMode == MinimapExportFloorMode::GroundFloor ? GROUND_LAYER : options.selectedFloor);
}

Bounds findContentBounds(Map& map, std::span<const int> floors) {
	Bounds bounds;
	for (TileLocation& location : map.tiles()) {
		if (!containsFloor(floors, location.getZ())) {
			continue;
		}

		const Tile* tile = location.get();
		if (!isTileExportable(tile)) {
			continue;
		}

		bounds.minX = std::min(bounds.minX, location.getX());
		bounds.minY = std::min(bounds.minY, location.getY());
		bounds.maxX = std::max(bounds.maxX, location.getX());
		bounds.maxY = std::max(bounds.maxY, location.getY());
	}
	return bounds;
}

std::vector<Chunk> buildChunks(const Bounds& bounds, int imageSize) {
	std::vector<Chunk> chunks;
	if (!bounds.valid()) {
		return chunks;
	}

	const int startX = (bounds.minX / imageSize) * imageSize;
	const int startY = (bounds.minY / imageSize) * imageSize;
	const int endX = (bounds.maxX / imageSize) * imageSize;
	const int endY = (bounds.maxY / imageSize) * imageSize;
	chunks.reserve(static_cast<size_t>(((endX - startX) / imageSize + 1) * ((endY - startY) / imageSize + 1)));

	for (int y = startY; y <= endY; y += imageSize) {
		for (int x = startX; x <= endX; x += imageSize) {
			chunks.push_back({ .x = x, .y = y });
		}
	}
	return chunks;
}

wxFileName outputFile(const wxFileName& directory, const std::string& baseName, const std::string& extension) {
	wxFileName file(directory);
	const std::string fullName = baseName + extension;
	file.SetFullName(wxString::FromUTF8(fullName.c_str()));
	return file;
}

wxFileName chunkFile(const wxFileName& directory, const std::string& baseName, MinimapExportFormat format, int floor, const Chunk& chunk) {
	const std::string extension = format == MinimapExportFormat::Jpg ? ".jpg" : ".webp";
	wxFileName file(directory);
	file.SetFullName(wxString::FromUTF8(baseName.c_str()) + wxString::Format("_z%d_x%d_y%d", floor, chunk.x, chunk.y) + wxString::FromUTF8(extension.c_str()));
	return file;
}

} // namespace minimap_export

namespace {

[[nodiscard]] MinimapExportResult validateOptions(const MinimapExportOptions& options) {
	if (options.fileBaseName.empty()) {
		return { .ok = false, .error = "File name is required." };
	}
	if (options.imageSize <= 0) {
		return { .ok = false, .error = "Image size must be greater than zero." };
	}
	if (options.floorMode == MinimapExportFloorMode::SelectedFloor && (options.selectedFloor < 0 || options.selectedFloor > MAP_MAX_LAYER)) {
		return { .ok = false, .error = "Selected floor is out of range." };
	}
	if (!options.outputDirectory.DirExists()) {
		return { .ok = false, .error = "Output folder not found." };
	}
	if (!options.outputDirectory.IsDirWritable()) {
		return { .ok = false, .error = "Output folder is not writable." };
	}
	return { .ok = true };
}

} // namespace

MinimapExportResult MinimapExporter::Export(Editor& editor, const MinimapExportOptions& options, ProgressCallback progress) {
	if (MinimapExportResult validation = validateOptions(options); !validation.ok) {
		return validation;
	}

	try {
		if (options.format == MinimapExportFormat::Otmm) {
			return minimap_export::exportOtmm(editor, options, progress);
		}
		return minimap_export::exportImages(editor, options, progress);
	} catch (const std::bad_alloc&) {
		return { .ok = false, .error = "There is not enough memory available to complete the operation." };
	} catch (const std::exception& exception) {
		return { .ok = false, .error = exception.what() };
	}
}
