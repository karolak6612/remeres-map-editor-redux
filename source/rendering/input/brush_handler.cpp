//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "../../main.h"
#include "../../position.h"
#include "brush_handler.h"
#include "../canvas/map_canvas.h"
#include "../../editor.h"
#include "../../gui.h"
#include "../../brush.h"
#include "../../doodad_brush.h"
#include "../../waypoint_brush.h"

namespace rme {
	namespace input {

		BrushInputHandler::BrushInputHandler(MapCanvas* canvas, Editor& editor) :
			canvas_(canvas), editor_(editor) {
		}

		void BrushInputHandler::onMouseDown(const MouseEvent& event) {
			if (!g_gui.IsSelectionMode() && event.isLeftButton()) {
				canvas_->drawing_ = true;
			}
		}

		void BrushInputHandler::onMouseUp(const MouseEvent& event) {
			if (event.isLeftButton()) {
				canvas_->drawing_ = false;
			}
		}

		void BrushInputHandler::onMouseMove(const MouseEvent& event) {
			if (!g_gui.IsSelectionMode() && canvas_->drawing_) {
				handleDrawing(event.mapPos, event);
			}
		}

		void BrushInputHandler::onMouseClick(const MouseEvent& event) {
			if (!g_gui.IsSelectionMode() && event.isLeftButton()) {
				handleDrawing(event.mapPos, event);
			}
		}

		void BrushInputHandler::onMouseDrag(const MouseEvent& event, const DragState& drag) {
			// Brush dragging logic if any
		}

		void BrushInputHandler::handleDrawing(const MapCoord& pos, const MouseEvent& event) {
			Brush* brush = g_gui.GetCurrentBrush();
			if (!brush) {
				return;
			}

			Position mapPos(pos.x, pos.y, pos.z);

			if (brush->isDoodad()) {
				if (event.modifiers.ctrl) {
					PositionVector tilestodraw;
					canvas_->getTilesToDraw(pos.x, pos.y, pos.z, &tilestodraw, nullptr);
					editor_.undraw(tilestodraw, event.modifiers.shift || event.modifiers.alt);
				} else {
					editor_.draw(mapPos, event.modifiers.shift || event.modifiers.alt);
				}
			} else if (brush->isDoor()) {
				if (brush->canDraw(&editor_.map, mapPos)) {
					PositionVector tilestodraw, tilestoborder;
					tilestodraw.push_back(mapPos);

					tilestoborder.push_back(Position(pos.x, pos.y - 1, pos.z));
					tilestoborder.push_back(Position(pos.x - 1, pos.y, pos.z));
					tilestoborder.push_back(Position(pos.x, pos.y + 1, pos.z));
					tilestoborder.push_back(Position(pos.x + 1, pos.y, pos.z));

					if (event.modifiers.ctrl) {
						editor_.undraw(tilestodraw, tilestoborder, event.modifiers.alt);
					} else {
						editor_.draw(tilestodraw, tilestoborder, event.modifiers.alt);
					}
				}
			} else if (brush->needBorders()) {
				PositionVector tilestodraw, tilestoborder;
				canvas_->getTilesToDraw(pos.x, pos.y, pos.z, &tilestodraw, &tilestoborder);

				if (event.modifiers.ctrl) {
					editor_.undraw(tilestodraw, tilestoborder, event.modifiers.alt);
				} else {
					editor_.draw(tilestodraw, tilestoborder, event.modifiers.alt);
				}
			} else if (brush->oneSizeFitsAll()) {
				PositionVector tilestodraw;
				tilestodraw.push_back(mapPos);

				if (event.modifiers.ctrl) {
					editor_.undraw(tilestodraw, event.modifiers.alt);
				} else {
					editor_.draw(tilestodraw, event.modifiers.alt);
				}
			} else { // No borders
				PositionVector tilestodraw;
				int brushSize = g_gui.GetBrushSize();
				int brushShape = g_gui.GetBrushShape();

				for (int y = -brushSize; y <= brushSize; y++) {
					for (int x = -brushSize; x <= brushSize; x++) {
						if (brushShape == BRUSHSHAPE_SQUARE) {
							tilestodraw.push_back(Position(pos.x + x, pos.y + y, pos.z));
						} else if (brushShape == BRUSHSHAPE_CIRCLE) {
							double distance = sqrt(double(x * x) + double(y * y));
							if (distance < brushSize + 0.005) {
								tilestodraw.push_back(Position(pos.x + x, pos.y + y, pos.z));
							}
						}
					}
				}
				if (event.modifiers.ctrl) {
					editor_.undraw(tilestodraw, event.modifiers.alt);
				} else {
					editor_.draw(tilestodraw, event.modifiers.alt);
				}
			}

			g_gui.FillDoodadPreviewBuffer();
			g_gui.RefreshView();
		}

	} // namespace input
} // namespace rme
