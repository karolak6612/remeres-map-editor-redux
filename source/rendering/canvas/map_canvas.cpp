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

#include "../../main.h"
#include "../../logging/logger.h"
#include "map_canvas.h"
#include "../../editor.h"
#include "../../editor.h"
#include "../../map_window.h"
#include "../../gui.h"
#include "../../map.h"
#include "../../ground_brush.h"
#include "../opengl/gl_context.h"
#include "../opengl/gl_includes.h"
#include "../opengl/gl_primitives.h"
#include "../renderers/tile_renderer.h"
#include "../renderers/item_renderer.h"
#include "../renderers/creature_renderer.h"
#include "../renderers/selection_renderer.h"
#include "../renderers/brush_renderer.h"
#include "../renderers/grid_renderer.h"
#include "../renderers/light_renderer.h"
#include "../renderers/tooltip_renderer.h"
#include "../renderers/ui_renderer.h"
#include "../renderers/font_renderer.h"
// Forward declarations for wxWidgets types
class wxMouseEvent;
class wxKeyEvent;

// Forward declarations for wxWidgets types
class wxMouseEvent;
class wxKeyEvent;

// Renderers for dependency injection
#include "../renderers/tile_renderer.h"
#include "../renderers/item_renderer.h"
#include "../renderers/creature_renderer.h"
#include "../renderers/selection_renderer.h"
#include "../renderers/brush_renderer.h"
#include "../renderers/grid_renderer.h"
#include "../renderers/light_renderer.h"
#include "../renderers/tooltip_renderer.h"
#include "../renderers/ui_renderer.h"
#include "../renderers/font_renderer.h"

namespace rme {
	namespace canvas {

		BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
		EVT_SIZE(MapCanvas::OnSize)
		EVT_PAINT(MapCanvas::OnPaint)
		EVT_ERASE_BACKGROUND(MapCanvas::OnEraseBackground)

		EVT_MOTION(MapCanvas::OnMouseMove)
		EVT_LEFT_DOWN(MapCanvas::OnMouseLeftClick)
		EVT_LEFT_UP(MapCanvas::OnMouseLeftRelease)
		EVT_LEFT_DCLICK(MapCanvas::OnMouseLeftDoubleClick)
		EVT_MIDDLE_DOWN(MapCanvas::OnMouseCenterClick)
		EVT_MIDDLE_UP(MapCanvas::OnMouseCenterRelease)
		EVT_RIGHT_DOWN(MapCanvas::OnMouseRightClick)
		EVT_RIGHT_UP(MapCanvas::OnMouseRightRelease)
		EVT_MOUSEWHEEL(MapCanvas::OnMouseWheel)

		EVT_KEY_DOWN(MapCanvas::OnKeyDown)
		EVT_KEY_UP(MapCanvas::OnKeyUp)
		EVT_TIMER(ANIMATION_TIMER_ID, MapCanvas::OnTimer)
		END_EVENT_TABLE()

		MapCanvas::MapCanvas(MapWindow* parent, Editor& editor, int* attribList) :
			wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
			editor_(editor), parent_(parent) {
			// Create renderers via dependency injection
			renderCoordinator_ = std::make_unique<render::RenderCoordinator>(
				std::make_unique<render::TileRenderer>(),
				std::make_unique<render::ItemRenderer>(),
				std::make_unique<render::CreatureRenderer>(),
				std::make_unique<render::SelectionRenderer>(),
				std::make_unique<render::BrushRenderer>(),
				std::make_unique<render::GridRenderer>(),
				std::make_unique<render::LightRenderer>(),
				std::make_unique<render::TooltipRenderer>(),
				std::make_unique<render::UIRenderer>(),
				std::make_unique<render::FontRenderer>()
			);

			// Create handlers
			brushHandler_ = std::make_unique<input::BrushInputHandler>(this, editor_);
			cameraHandler_ = std::make_unique<input::CameraInputHandler>(this);
			selectionHandler_ = std::make_unique<input::SelectionInputHandler>(this, editor_);

			LOG_RENDER_INFO("[CANVAS] MapCanvas created - Parent: {}, Editor: {}", (void*)parent, (void*)&editor);

			// Connect ViewManager to canvas redraw
			viewManager_.addViewChangeListener([this]() {
				syncRenderState(); // Ensure RenderState/InputDispatcher are in sync with ViewManager
				requestRedraw();
			});
		}

		MapCanvas::~MapCanvas() {
			shutdown();
		}

		void MapCanvas::initialize() {
			LOG_RENDER_INFO("[CANVAS] Initializing MapCanvas...");
			if (initialized_) {
				LOG_RENDER_WARN("[CANVAS] MapCanvas already initialized");
				return;
			}

			// Create GL context
			glContext_ = new wxGLContext(this);
			SetCurrent(*glContext_);

			// Initialize GLAD
			LOG_RENDER_INFO("[CANVAS] Initializing GLAD...");
			render::gl::initializeGLAD();

			// Initialize render coordinator
			renderCoordinator_->initialize();

			// Initialize input dispatcher and register receivers
			// Share ViewManager's coordinate mapper to ensure consistent state
			inputDispatcher_.initialize(&viewManager_.getCoordinateMapper());
			inputDispatcher_.addReceiver(this);
			inputDispatcher_.addReceiver(brushHandler_.get());
			inputDispatcher_.addReceiver(cameraHandler_.get());
			inputDispatcher_.addReceiver(selectionHandler_.get());

			// Initialize animation
			animationTimer_.SetOwner(this, ANIMATION_TIMER_ID);
			animationTimer_.Start(10); // 100 FPS target for smooth animations
			animationManager_.start();
			animationManager_.setRedrawCallback([this]() {
				requestRedraw();
			});

			// Set initial viewport
			int w, h;
			GetClientSize(&w, &h);
			viewManager_.setViewportSize(w, h);

			// InputDispatcher shares state via pointer, so no manual sync needed here

			// Sync render state
			syncRenderState();

			initialized_ = true;
			LOG_RENDER_INFO("[CANVAS] MapCanvas initialization complete");
		}

		void MapCanvas::shutdown() {
			LOG_RENDER_INFO("[CANVAS] Shutting down MapCanvas...");
			if (!initialized_) {
				return;
			}

			inputDispatcher_.shutdown();
			renderCoordinator_->shutdown();

			animationTimer_.Stop();
			animationManager_.stop();

			delete glContext_;
			glContext_ = nullptr;

			initialized_ = false;
			LOG_RENDER_INFO("[CANVAS] MapCanvas shutdown complete");
		}

		void MapCanvas::OnSize(wxSizeEvent& event) {
			int w, h;
			GetClientSize(&w, &h);
			LOG_RENDER_INFO("[CANVAS] Resize event - New Size: {}x{}", w, h);
			viewManager_.setViewportSize(w, h);
			event.Skip();
		}

		void MapCanvas::OnTimer(wxTimerEvent& WXUNUSED(event)) {
			// Update animation manager, which may trigger redraw
			animationManager_.update();
		}

		void MapCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
			// LOG_TRACE("MapCanvas::OnPaint");
			if (!initialized_) {
				LOG_RENDER_INFO("[CANVAS] First paint - Triggering initialization");
				initialize();
			}

			wxPaintDC dc(this);
			SetCurrent(*glContext_);

			render();
		}

		void MapCanvas::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
			// Do nothing to avoid flicker
		}

		void MapCanvas::GetScreenCenter(int* x, int* y) {
			if (x) {
				*x = viewManager_.getScrollX() + (viewManager_.getViewportWidth() / 2) / viewManager_.getZoom();
			}
			if (y) {
				*y = viewManager_.getScrollY() + (viewManager_.getViewportHeight() / 2) / viewManager_.getZoom();
			}
		}

		void MapCanvas::GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) {
			if (view_scroll_x) {
				*view_scroll_x = viewManager_.getScrollX();
			}
			if (view_scroll_y) {
				*view_scroll_y = viewManager_.getScrollY();
			}
			if (screensize_x) {
				*screensize_x = viewManager_.getViewportWidth();
			}
			if (screensize_y) {
				*screensize_y = viewManager_.getViewportHeight();
			}
		}

		bool MapCanvas::processed[BLOCK_SIZE * BLOCK_SIZE] = { false };

		void MapCanvas::getTilesToDraw(int mouse_map_x, int mouse_map_y, int z, PositionVector* tilestodraw, PositionVector* tilestoborder, bool fill) {
			if (fill) {
				Brush* brush = g_gui.GetCurrentBrush();
				if (!brush || !brush->isGround()) {
					return;
				}

				GroundBrush* newBrush = brush->asGround();
				Position position(mouse_map_x, mouse_map_y, z);

				Tile* tile = editor_.map.getTile(position);
				GroundBrush* oldBrush = nullptr;
				if (tile) {
					oldBrush = tile->getGroundBrush();
				}

				if (oldBrush && oldBrush->getID() == newBrush->getID()) {
					return;
				}

				if ((tile && tile->ground && !oldBrush) || (!tile && oldBrush)) {
					return;
				}

				if (tile && oldBrush) {
					GroundBrush* groundBrush = tile->getGroundBrush();
					if (!groundBrush || groundBrush->getID() != oldBrush->getID()) {
						return;
					}
				}

				std::fill(std::begin(processed), std::end(processed), false);
				countMaxFills_ = 0;
				floodFill(&editor_.map, position, BLOCK_SIZE / 2, BLOCK_SIZE / 2, oldBrush, tilestodraw);
			} else {
				int brushSize = g_gui.GetBrushSize();
				int brushShape = g_gui.GetBrushShape();

				for (int y = -brushSize - 1; y <= brushSize + 1; y++) {
					for (int x = -brushSize - 1; x <= brushSize + 1; x++) {
						if (brushShape == BRUSHSHAPE_SQUARE) {
							if (x >= -brushSize && x <= brushSize && y >= -brushSize && y <= brushSize) {
								if (tilestodraw) {
									tilestodraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, z));
								}
							}
							if (std::abs(x) - brushSize < 2 && std::abs(y) - brushSize < 2) {
								if (tilestoborder) {
									tilestoborder->push_back(Position(mouse_map_x + x, mouse_map_y + y, z));
								}
							}
						} else if (brushShape == BRUSHSHAPE_CIRCLE) {
							double distance = sqrt(double(x * x) + double(y * y));
							if (distance < brushSize + 0.005) {
								if (tilestodraw) {
									tilestodraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, z));
								}
							}
							if (std::abs(distance - brushSize) < 1.5) {
								if (tilestoborder) {
									tilestoborder->push_back(Position(mouse_map_x + x, mouse_map_y + y, z));
								}
							}
						}
					}
				}
			}
		}

		bool MapCanvas::floodFill(Map* map, const Position& center, int x, int y, GroundBrush* brush, PositionVector* positions) {
			countMaxFills_++;
			if (countMaxFills_ > (BLOCK_SIZE * 4 * 4)) {
				countMaxFills_ = 0;
				return true;
			}

			if (x <= 0 || y <= 0 || x >= BLOCK_SIZE || y >= BLOCK_SIZE) {
				return false;
			}

			processed[getFillIndex(x, y)] = true;

			int px = (center.x + x) - (BLOCK_SIZE / 2);
			int py = (center.y + y) - (BLOCK_SIZE / 2);
			if (px <= 0 || py <= 0 || px >= map->getWidth() || py >= map->getHeight()) {
				return false;
			}

			Tile* tile = map->getTile(px, py, center.z);
			if ((tile && tile->ground && !brush) || (!tile && brush)) {
				return false;
			}

			if (tile && brush) {
				GroundBrush* groundBrush = tile->getGroundBrush();
				if (!groundBrush || groundBrush->getID() != brush->getID()) {
					return false;
				}
			}

			positions->push_back(Position(px, py, center.z));

			bool deny = false;
			if (!processed[getFillIndex(x - 1, y)]) {
				deny = floodFill(map, center, x - 1, y, brush, positions);
			}

			if (!deny && !processed[getFillIndex(x, y - 1)]) {
				deny = floodFill(map, center, x, y - 1, brush, positions);
			}

			if (!deny && !processed[getFillIndex(x + 1, y)]) {
				deny = floodFill(map, center, x + 1, y, brush, positions);
			}

			if (!deny && !processed[getFillIndex(x, y + 1)]) {
				deny = floodFill(map, center, x, y + 1, brush, positions);
			}

			return deny;
		}

		void MapCanvas::EnterSelectionMode() {
			drawing_ = false;
			dragging_ = false;
			boundbox_selection_ = false;
			requestRedraw();
		}

		void MapCanvas::EnterDrawingMode() {
			drawing_ = false;
			dragging_ = false;
			boundbox_selection_ = false;
			requestRedraw();
		}

		void MapCanvas::setViewportSize(int width, int height) {
			viewManager_.setViewportSize(width, height);
		}

		void MapCanvas::setZoom(float zoom) {
			viewManager_.setZoom(zoom);
		}

		void MapCanvas::setFloor(int floor) {
			viewManager_.setFloor(floor);
		}

		void MapCanvas::setScroll(int scrollX, int scrollY) {
			viewManager_.setScroll(scrollX, scrollY);
		}

		void MapCanvas::syncRenderState() {
			renderState_.setEditor(&editor_);

			// Pull state from parent window if needed? No, ViewManager is truth now
			// EXCEPT initialization pulling from parent (which usually happens via scrollbars)
			// MapWindow updates scrollbars which calls setScroll on Canvas.
			// So Canvas -> ViewManager is the flow.

			renderState_.setViewport(viewManager_.getViewportWidth(), viewManager_.getViewportHeight(), viewManager_.getZoom());
			renderState_.setFloor(viewManager_.getFloor());
			renderState_.setScroll(viewManager_.getScrollX(), viewManager_.getScrollY());
			renderState_.setTime(animationManager_.getTime());

			// InputDispatcher uses shared mapper pointer, so it is automatically in sync
			// No validation needed here

			// Calculate visible tile range
			const auto* mapper = inputDispatcher_.coordinateMapper();
			if (mapper) {
				renderState_.setVisibleRange(
					mapper->startTileX(), mapper->startTileY(),
					mapper->endTileX(), mapper->endTileY()
				);

				// Update mouse map position (it changes when view scrolls, even if screen pos is static)
				auto screenPos = inputDispatcher_.mouseScreenPos();
				auto mapPos = mapper->screenToMap(screenPos);
				renderState_.setMousePosition(mapPos.x, mapPos.y);
			}
			// LOG_RENDER_TRACE("[CANVAS] Render state synced - Zoom: {}, Scroll: ({},{}), Visible: ({},{}) to ({},{})", zoom_, scrollX_, scrollY_, mapper.startTileX(), mapper.startTileY(), mapper.endTileX(), mapper.endTileY());

			renderState_.options.isDrawing = drawing_;
			renderState_.options.isDragging = dragging_;
			renderState_.options.brushSize = g_gui.GetBrushSize();
			renderState_.options.brushShape = g_gui.GetBrushShape();
		}

		void MapCanvas::render() {
			if (!initialized_) {
				return;
			}

			// Render using the coordinator
			// LOG_RENDER_TRACE("[FRAME] Starting frame render pass");
			renderCoordinator_->render(renderState_);

			SwapBuffers();
			needsRedraw_ = false;
			// LOG_RENDER_TRACE("[FRAME] Frame render complete, buffers swapped");
		}

		void MapCanvas::requestRedraw() {
			needsRedraw_ = true;
			Refresh(false);
		}

		// Input handlers (wxWidgets -> dispatcher)
		void MapCanvas::OnMouseMove(wxMouseEvent& event) {
			inputDispatcher_.handleMouseMove(event.GetX(), event.GetY(), event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseLeftClick(wxMouseEvent& event) {
			inputDispatcher_.handleMouseDown(event.GetX(), event.GetY(), input::MouseButton::Left, event.ControlDown(), event.AltDown(), event.ShiftDown());
			SetFocus();
		}

		void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event) {
			inputDispatcher_.handleMouseUp(event.GetX(), event.GetY(), input::MouseButton::Left, event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseLeftDoubleClick(wxMouseEvent& event) {
			inputDispatcher_.handleMouseDoubleClick(event.GetX(), event.GetY(), input::MouseButton::Left, event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseCenterClick(wxMouseEvent& event) {
			inputDispatcher_.handleMouseDown(event.GetX(), event.GetY(), input::MouseButton::Middle, event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event) {
			inputDispatcher_.handleMouseUp(event.GetX(), event.GetY(), input::MouseButton::Middle, event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseRightClick(wxMouseEvent& event) {
			inputDispatcher_.handleMouseDown(event.GetX(), event.GetY(), input::MouseButton::Right, event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseRightRelease(wxMouseEvent& event) {
			inputDispatcher_.handleMouseUp(event.GetX(), event.GetY(), input::MouseButton::Right, event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnMouseWheel(wxMouseEvent& event) {
			inputDispatcher_.handleMouseWheel(event.GetX(), event.GetY(), event.GetWheelRotation(), event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnKeyDown(wxKeyEvent& event) {
			inputDispatcher_.handleKeyDown(event.GetKeyCode(), event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		void MapCanvas::OnKeyUp(wxKeyEvent& event) {
			inputDispatcher_.handleKeyUp(event.GetKeyCode(), event.ControlDown(), event.AltDown(), event.ShiftDown());
		}

		// InputReceiver callbacks - handle application-level input
		void MapCanvas::onMouseMove(const input::MouseEvent& event) {
			// LOG_RENDER_TRACE("[INPUT] Mouse move - Screen: ({},{}), Map: ({},{})", event.screenPos.x, event.screenPos.y, event.mapPos.x, event.mapPos.y);
			renderState_.setMousePosition(event.mapPos.x, event.mapPos.y);
			requestRedraw();
		}

		void MapCanvas::onMouseClick(const input::MouseEvent& event) {
			requestRedraw();
		}

		void MapCanvas::onMouseDoubleClick(const input::MouseEvent& event) {
			requestRedraw();
		}

		void MapCanvas::onMouseDrag(const input::MouseEvent& event, const input::DragState& drag) {
			requestRedraw();
		}

		void MapCanvas::onMouseWheel(const input::MouseEvent& event) {
			if (event.modifiers.ctrl) {
				// Legacy: Ctrl + Wheel = Change Floor
				// Up (Positive) = Move Up a floor (Decrease Z)
				// Down (Negative) = Move Down a floor (Increase Z)
				int newFloor = viewManager_.getFloor() + (event.wheelDelta > 0 ? -1 : 1);
				newFloor = std::max(0, std::min(15, newFloor));
				setFloor(newFloor);
			} else if (event.modifiers.alt) {
				// Legacy: Alt + Wheel = Change Brush Size
				if (event.wheelDelta > 0) {
					LOG_RENDER_DEBUG("[INPUT] Increasing brush size via Alt+Wheel");
					g_gui.IncreaseBrushSize();
				} else {
					LOG_RENDER_DEBUG("[INPUT] Decreasing brush size via Alt+Wheel");
					g_gui.DecreaseBrushSize();
				}
				requestRedraw();
			}
			// Default (No modifiers) is ignored here, handled by CameraHandler for Zoom.
		}

		void MapCanvas::onKeyDown(const input::KeyEvent& event) {
			switch (event.keyCode) {
				case WXK_NUMPAD_ADD:
				case WXK_PAGEUP:
					g_gui.ChangeFloor(viewManager_.getFloor() - 1);
					break;
				case WXK_NUMPAD_SUBTRACT:
				case WXK_PAGEDOWN:
					g_gui.ChangeFloor(viewManager_.getFloor() + 1);
					break;
				case '[':
				case '+':
					g_gui.IncreaseBrushSize();
					requestRedraw();
					break;
				case ']':
				case '-':
					g_gui.DecreaseBrushSize();
					requestRedraw();
					break;
				case WXK_NUMPAD_UP:
				case WXK_UP:
				case WXK_NUMPAD_DOWN:
				case WXK_DOWN:
				case WXK_NUMPAD_LEFT:
				case WXK_LEFT:
				case WXK_NUMPAD_RIGHT:
				case WXK_RIGHT: {
					int tiles = 3;
					if (event.modifiers.ctrl) {
						tiles = 10;
					}
					int dx = 0, dy = 0;
					if (event.keyCode == WXK_UP || event.keyCode == WXK_NUMPAD_UP) {
						dy = -tiles;
					} else if (event.keyCode == WXK_DOWN || event.keyCode == WXK_NUMPAD_DOWN) {
						dy = tiles;
					} else if (event.keyCode == WXK_LEFT || event.keyCode == WXK_NUMPAD_LEFT) {
						dx = -tiles;
					} else if (event.keyCode == WXK_RIGHT || event.keyCode == WXK_NUMPAD_RIGHT) {
						dx = tiles;
					}

					if (parent_) {
						parent_->ScrollRelative(int(dx * 32), int(dy * 32));
					}
					requestRedraw();
					break;
				}
				default:
					break;
			}
		}

		const input::MapCoord& MapCanvas::mouseMapPos() const {
			return inputDispatcher_.mouseMapPos();
		}

		bool MapCanvas::isDragging() const {
			return inputDispatcher_.isDragging();
		}

	} // namespace canvas
} // namespace rme
