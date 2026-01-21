//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_BRUSH_HANDLER_H_
#define RME_BRUSH_HANDLER_H_

#include "input_dispatcher.h"
#include "../../position.h"

namespace rme {
	namespace canvas {
		class MapCanvas;
	}
}
class Editor;
class Brush;

using MapCanvas = rme::canvas::MapCanvas;

namespace rme {
	namespace input {

		/// Handles brush-related input (drawing, smearing, etc.)
		class BrushInputHandler : public InputReceiver {
		public:
			BrushInputHandler(MapCanvas* canvas, Editor& editor);
			virtual ~BrushInputHandler() = default;

			void onMouseDown(const MouseEvent& event) override;
			void onMouseUp(const MouseEvent& event) override;
			void onMouseMove(const MouseEvent& event) override;
			void onMouseClick(const MouseEvent& event) override;
			void onMouseDrag(const MouseEvent& event, const DragState& drag) override;

		private:
			void handleDrawing(const MapCoord& pos, const MouseEvent& event);

			MapCanvas* canvas_;
			Editor& editor_;
		};

	} // namespace input
} // namespace rme

#endif // RME_BRUSH_HANDLER_H_
