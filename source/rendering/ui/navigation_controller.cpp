//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/navigation_controller.h"
#include "app/main.h"
#include "rendering/ui/map_display.h"
#include "ui/gui.h"
#include "ui/map_window.h"
#include <algorithm>
#include <cmath>

void NavigationController::HandleArrowKeys(MapCanvas* canvas, wxKeyEvent& event)
{
    int start_x, start_y;
    static_cast<MapWindow*>(canvas->GetParent())->GetViewStart(&start_x, &start_y);

    int tiles = 3;
    if (event.ControlDown()) {
        tiles = 10;
    } else if (canvas->GetZoom() == 1.0) {
        tiles = 1;
    }

    int keycode = event.GetKeyCode();
    if (keycode == WXK_NUMPAD_UP || keycode == WXK_UP) {
        static_cast<MapWindow*>(canvas->GetParent())->Scroll(start_x, int(start_y - TILE_SIZE * tiles * canvas->GetZoom()));
    } else if (keycode == WXK_NUMPAD_DOWN || keycode == WXK_DOWN) {
        static_cast<MapWindow*>(canvas->GetParent())->Scroll(start_x, int(start_y + TILE_SIZE * tiles * canvas->GetZoom()));
    } else if (keycode == WXK_NUMPAD_LEFT || keycode == WXK_LEFT) {
        static_cast<MapWindow*>(canvas->GetParent())->Scroll(int(start_x - TILE_SIZE * tiles * canvas->GetZoom()), start_y);
    } else if (keycode == WXK_NUMPAD_RIGHT || keycode == WXK_RIGHT) {
        static_cast<MapWindow*>(canvas->GetParent())->Scroll(int(start_x + TILE_SIZE * tiles * canvas->GetZoom()), start_y);
    }

    canvas->UpdatePositionStatus();
    canvas->Refresh();
}

bool NavigationController::HandleMouseDrag(MapCanvas* canvas, wxMouseEvent& event)
{
    if (canvas->IsScreenDragging()) {
        static_cast<MapWindow*>(canvas->GetParent())
            ->ScrollRelative(
                int(canvas->GetSettings().getFloat(Config::SCROLL_SPEED) * canvas->GetZoom() * (event.GetX() - canvas->GetCursorX())),
                int(canvas->GetSettings().getFloat(Config::SCROLL_SPEED) * canvas->GetZoom() * (event.GetY() - canvas->GetCursorY()))
            );
        return true;
    }
    return false;
}

void NavigationController::HandleCameraClick(MapCanvas* canvas, wxMouseEvent& event)
{
    canvas->SetFocus();

    canvas->SetLastMmbClickX(event.GetX());
    canvas->SetLastMmbClickY(event.GetY());

    // Control logic is handled by ZoomController in OnMouseCameraClick wrapper usually, but if extracted here:
    // But MapCanvas::OnMouseCameraClick has explicit Control check for Reset Zoom.
    // If NavigationController handles Click, it should know about Zoom Reset?
    // Or MapCanvas delegates only "Navigation logic" here?
    // MapCanvas::OnMouseCameraClick implementation:
    /*
    if (event.ControlDown()) {
        // Zoom Reset logic ...
    } else {
        screendragging = true;
    }
    */
    // So purely navigation part is just screendragging = true.
    // But HandleCameraClick implies handling the event.
    // I'll put the screendragging = true logic here.
    // MapCanvas should check ControlDown first.

    canvas->SetScreenDragging(true);
}

void NavigationController::HandleCameraRelease(MapCanvas* canvas, wxMouseEvent& event)
{
    canvas->SetFocus();
    canvas->SetScreenDragging(false);
    if (event.ControlDown()) {
        // ...
        // Haven't moved much, it's a click!
    } else if (
        canvas->GetLastMmbClickX() > event.GetX() - 3 && canvas->GetLastMmbClickX() < event.GetX() + 3
        && canvas->GetLastMmbClickY() > event.GetY() - 3 && canvas->GetLastMmbClickY() < event.GetY() + 3
    ) {
        int screensize_x, screensize_y;
        static_cast<MapWindow*>(canvas->GetParent())->GetViewSize(&screensize_x, &screensize_y);
        static_cast<MapWindow*>(canvas->GetParent())
            ->ScrollRelative(
                int(canvas->GetZoom() * (2 * canvas->GetCursorX() - screensize_x)),
                int(canvas->GetZoom() * (2 * canvas->GetCursorY() - screensize_y))
            );
        canvas->Refresh();
    }
}

void NavigationController::ChangeFloor(MapCanvas* canvas, int new_floor)
{
    new_floor = std::clamp(new_floor, 0, MAP_LAYERS - 1);
    int old_floor = canvas->GetFloor();
    canvas->SetFloorDirect(new_floor);
    if (old_floor != new_floor) {
        canvas->UpdatePositionStatus();
        canvas->GetGui().root->UpdateFloorMenu();
        canvas->UpdateMinimap(true);
    }
    canvas->Refresh();
}

void NavigationController::HandleWheel(MapCanvas* canvas, wxMouseEvent& event)
{
    if (event.ControlDown()) {
        static double diff = 0.0;
        diff += event.GetWheelRotation();
        if (diff <= 1.0 || diff >= 1.0) {
            if (diff < 0.0) {
                ChangeFloor(canvas, canvas->GetFloor() - 1);
            } else {
                ChangeFloor(canvas, canvas->GetFloor() + 1);
            }
            diff = 0.0;
        }
        canvas->UpdatePositionStatus();
    }
}
