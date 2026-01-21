//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_CAMERA_HANDLER_H_
#define RME_CAMERA_HANDLER_H_

#include "input_dispatcher.h"

namespace rme {
	namespace canvas {
		class MapCanvas;
	}
}

// using MapCanvas = rme::canvas::MapCanvas;

namespace rme {
	namespace input {

		/// Handles camera-related input (scrolling and zooming)
		class CameraInputHandler : public InputReceiver {
		public:
			CameraInputHandler(rme::canvas::MapCanvas* canvas);
			virtual ~CameraInputHandler() = default;

			void onMouseDrag(const MouseEvent& event, const DragState& drag) override;
			void onMouseWheel(const MouseEvent& event) override;

		private:
			rme::canvas::MapCanvas* canvas_;
		};

	} // namespace input
} // namespace rme

#endif // RME_CAMERA_HANDLER_H_
