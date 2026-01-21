//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_SELECTION_HANDLER_H_
#define RME_SELECTION_HANDLER_H_

#include "input_dispatcher.h"

namespace rme {
	namespace canvas {
		class MapCanvas;
	}
}
class Editor;

// using MapCanvas = rme::canvas::MapCanvas; // Removed to avoid conflict with class MapCanvas forward decls

namespace rme {
	namespace input {

		/// Handles selection and dragging input
		class SelectionInputHandler : public InputReceiver {
		public:
			SelectionInputHandler(rme::canvas::MapCanvas* canvas, Editor& editor);
			virtual ~SelectionInputHandler() = default;

			void onMouseDown(const MouseEvent& event) override;
			void onMouseUp(const MouseEvent& event) override;
			void onMouseClick(const MouseEvent& event) override;
			void onMouseDrag(const MouseEvent& event, const DragState& drag) override;
			void onDragEnd(const DragState& drag) override;

		private:
			rme::canvas::MapCanvas* canvas_;
			Editor& editor_;
		};

	} // namespace input
} // namespace rme

#endif // RME_SELECTION_HANDLER_H_
