#ifndef RME_RENDERING_CORE_BRUSH_SNAPSHOT_H_
#define RME_RENDERING_CORE_BRUSH_SNAPSHOT_H_

#include "brushes/brush_enums.h"

class Brush;

// Captures all brush/editor UI state needed by MapDrawer for a single frame.
// Constructed once in MapCanvas::OnPaint(), passed into SetupVars().
// Captures brush state explicitly for rendering.
struct BrushSnapshot {
	Brush* current_brush = nullptr;
	BrushShape brush_shape = BRUSHSHAPE_SQUARE;
	int brush_size = 0;
	bool is_drawing_mode = false;
};

#endif
