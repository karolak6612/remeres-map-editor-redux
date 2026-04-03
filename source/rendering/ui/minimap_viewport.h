#ifndef RME_RENDERING_UI_MINIMAP_VIEWPORT_H_
#define RME_RENDERING_UI_MINIMAP_VIEWPORT_H_

#include "app/definitions.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace MinimapViewport {

inline constexpr std::array<double, 9> ZoomFactors = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
inline constexpr std::array<std::string_view, ZoomFactors.size()> ZoomLabels = { "1:16", "1:8", "1:4", "1:2", "1:1", "2x", "4x", "8x", "16x" };
inline constexpr int DefaultZoomStep = 4;

inline int ClampZoomStep(int step) {
	return std::clamp(step, 0, static_cast<int>(ZoomFactors.size()) - 1);
}

inline double GetZoomFactor(int step) {
	return ZoomFactors[ClampZoomStep(step)];
}

inline std::string_view GetZoomLabel(int step) {
	return ZoomLabels[ClampZoomStep(step)];
}

inline int ClampFloor(int floor) {
	return std::clamp(floor, 0, MAP_MAX_LAYER);
}

} // namespace MinimapViewport

struct MinimapViewportState {
	bool initialized = false;
	bool tracking_main_camera = true;
	double center_x = 0.0;
	double center_y = 0.0;
	int zoom_step = MinimapViewport::DefaultZoomStep;
	int floor = GROUND_LAYER;
};

#endif
