#ifndef RME_EDITOR_PERSISTENCE_MINIMAP_EXPORTER_H_
#define RME_EDITOR_PERSISTENCE_MINIMAP_EXPORTER_H_

#include <cstdint>
#include <functional>
#include <string>
#include <wx/filename.h>

class Editor;

enum class MinimapExportFormat : uint8_t {
	Otmm,
	Jpg,
	Webp,
};

enum class MinimapExportFloorMode : uint8_t {
	AllFloors,
	GroundFloor,
	SelectedFloor,
};

struct MinimapExportOptions {
	wxFileName outputDirectory;
	std::string fileBaseName;
	MinimapExportFormat format = MinimapExportFormat::Otmm;
	MinimapExportFloorMode floorMode = MinimapExportFloorMode::AllFloors;
	int selectedFloor = 7;
	int imageSize = 1024;
	bool showAllFloors = false;
};

struct MinimapExportResult {
	bool ok = false;
	size_t filesWritten = 0;
	std::string error;
};

class MinimapExporter {
public:
	using ProgressCallback = std::function<void(uint64_t completed, uint64_t total)>;

	[[nodiscard]] static MinimapExportResult Export(Editor& editor, const MinimapExportOptions& options, ProgressCallback progress = {});
};

#endif
