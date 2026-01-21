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

#ifndef RME_MAP_CANVAS_H_
#define RME_MAP_CANVAS_H_

#include "../../main.h"
#include "../pipeline/render_coordinator.h"
#include "render_state.h"
#include "../input/input_dispatcher.h"
#include "../input/input_types.h"
#include "../../position.h"

// Forward declarations
class Editor;
class MapWindow;

namespace rme {
	namespace input {
		class BrushInputHandler;
		class CameraInputHandler;
		class SelectionInputHandler;
	}

	namespace canvas {

		/// Modern map canvas that integrates the new rendering and input systems
		/// This class bridges the wxWidgets GL canvas with the new architecture
		class MapCanvas : public wxGLCanvas, public input::InputReceiver {
		public:
			MapCanvas(MapWindow* parent, Editor& editor, int* attribList = nullptr);
			virtual ~MapCanvas();

			// Viewport management (Forwarded from MapWindow)
			void OnSize(wxSizeEvent& event);
			void OnPaint(wxPaintEvent& event);
			void OnEraseBackground(wxEraseEvent& event);

			/// Initialize the canvas
			void initialize();

			/// Shutdown and cleanup
			void shutdown();

			/// Check if canvas is ready
			[[nodiscard]] bool isInitialized() const noexcept {
				return initialized_;
			}

			// Viewport management

			/// Update viewport size (call on resize)
			void setViewportSize(int width, int height);

			/// Set zoom level
			void setZoom(float zoom);

			/// Set current floor
			void setFloor(int floor);

			/// Set scroll position
			void setScroll(int scrollX, int scrollY);

			/// Get current zoom
			[[nodiscard]] float zoom() const noexcept {
				return zoom_;
			}

			/// Get current floor
			[[nodiscard]] int floor() const noexcept {
				return floor_;
			}

			// Legacy compatibility
			int GetFloor() const {
				return floor_;
			}
			void ChangeFloor(int floor) {
				setFloor(floor);
			}
			void GetScreenCenter(int* x, int* y);

			// Rendering

			/// Render the map
			/// Called from the wxWidgets paint handler
			void render();

			/// Request a redraw
			void requestRedraw();

			void OnMouseMove(wxMouseEvent& event);
			void OnMouseLeftClick(wxMouseEvent& event);
			void OnMouseLeftRelease(wxMouseEvent& event);
			void OnMouseLeftDoubleClick(wxMouseEvent& event);
			void OnMouseCenterClick(wxMouseEvent& event);
			void OnMouseCenterRelease(wxMouseEvent& event);
			void OnMouseRightClick(wxMouseEvent& event);
			void OnMouseRightRelease(wxMouseEvent& event);
			void OnMouseWheel(wxMouseEvent& event);
			void OnKeyDown(wxKeyEvent& event);
			void OnKeyUp(wxKeyEvent& event);

			// InputReceiver interface
			void onMouseMove(const input::MouseEvent& event) override;
			void onMouseClick(const input::MouseEvent& event) override;
			void onMouseDoubleClick(const input::MouseEvent& event) override;
			void onMouseDrag(const input::MouseEvent& event, const input::DragState& drag) override;
			void onMouseWheel(const input::MouseEvent& event) override;
			void onKeyDown(const input::KeyEvent& event) override;

			// Accessors
			render::RenderCoordinator& renderCoordinator() {
				return *renderCoordinator_;
			}
			input::InputDispatcher& inputDispatcher() {
				return inputDispatcher_;
			}

			/// Get mouse position in map coordinates
			[[nodiscard]] const input::MapCoord& mouseMapPos() const;

			/// Check if currently dragging
			[[nodiscard]] bool isDragging() const;

			void EnterSelectionMode();
			void EnterDrawingMode();

			void getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor, PositionVector* tilestodraw, PositionVector* tilestoborder, bool fill = false);

		protected:
			enum {
				BLOCK_SIZE = 100
			};

			inline int getFillIndex(int x, int y) const {
				return x + BLOCK_SIZE * y;
			}

			static bool processed[BLOCK_SIZE * BLOCK_SIZE];
			int countMaxFills_ = 0;

			bool floodFill(Map* map, const Position& center, int x, int y, GroundBrush* brush, PositionVector* positions);

			bool initialized_ = false;
			Editor& editor_;
			MapWindow* parent_ = nullptr;

			// Viewport state
			int viewportWidth_ = 800;
			int viewportHeight_ = 600;
			float zoom_ = 1.0f;
			int floor_ = 7;
			int scrollX_ = 0;
			int scrollY_ = 0;

			// Core systems
			std::unique_ptr<render::RenderCoordinator> renderCoordinator_;
			render::RenderState renderState_;
			input::InputDispatcher inputDispatcher_;
			std::unique_ptr<input::BrushInputHandler> brushHandler_;
			std::unique_ptr<input::CameraInputHandler> cameraHandler_;
			std::unique_ptr<input::SelectionInputHandler> selectionHandler_;

			// Dirty flag for redraw requests
			bool needsRedraw_ = true;

			// Context management
			wxGLContext* glContext_ = nullptr;

			// Update render state from current canvas state
			void syncRenderState();

			DECLARE_EVENT_TABLE()
		};

	} // namespace canvas
} // namespace rme

#endif // RME_MAP_CANVAS_H_
