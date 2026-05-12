#ifndef RME_EDITOR_PERSISTENCE_MINIMAP_EXPORTER_INTERNAL_H_
#define RME_EDITOR_PERSISTENCE_MINIMAP_EXPORTER_INTERNAL_H_

#include "app/definitions.h"
#include "editor/persistence/minimap_exporter.h"

#include <array>
#include <limits>
#include <span>
#include <vector>
#include <wx/filename.h>

class Map;
class Tile;

namespace minimap_export {

struct Bounds {
	int minX = std::numeric_limits<int>::max();
	int minY = std::numeric_limits<int>::max();
	int maxX = std::numeric_limits<int>::min();
	int maxY = std::numeric_limits<int>::min();

	[[nodiscard]] bool valid() const {
		return minX <= maxX && minY <= maxY;
	}
};

struct Chunk {
	int x = 0;
	int y = 0;
};

struct FloorSelection {
	std::array<int, MAP_LAYERS> values {};
	size_t count = 0;

	void push(int floor);
	[[nodiscard]] std::span<const int> floors() const;
};

[[nodiscard]] bool isTileExportable(const Tile* tile);
[[nodiscard]] bool containsFloor(std::span<const int> floors, int floor);
[[nodiscard]] FloorSelection allFloors();
[[nodiscard]] FloorSelection singleFloor(int floor);
[[nodiscard]] FloorSelection showAllFloorsForTarget(int targetFloor);
[[nodiscard]] FloorSelection selectedFloorsForBounds(const MinimapExportOptions& options);
[[nodiscard]] FloorSelection selectedFloorsForOtmm(const MinimapExportOptions& options);
[[nodiscard]] Bounds findContentBounds(Map& map, std::span<const int> floors);
[[nodiscard]] std::vector<Chunk> buildChunks(const Bounds& bounds, int imageSize);
[[nodiscard]] wxFileName outputFile(const wxFileName& directory, const std::string& baseName, const std::string& extension);
[[nodiscard]] wxFileName chunkFile(const wxFileName& directory, const std::string& baseName, MinimapExportFormat format, int floor, const Chunk& chunk);

[[nodiscard]] MinimapExportResult exportOtmm(Editor& editor, const MinimapExportOptions& options, const MinimapExporter::ProgressCallback& progress);
[[nodiscard]] MinimapExportResult exportImages(Editor& editor, const MinimapExportOptions& options, const MinimapExporter::ProgressCallback& progress);

} // namespace minimap_export

#endif
