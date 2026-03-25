//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/zoom_controller.h"
#include "app/main.h"
#include "rendering/ui/map_display.h"
#include "ui/gui.h"
#include "ui/map_window.h"
#include <algorithm>
#include <cmath>

void ZoomController::OnWheel(MapCanvas* canvas, wxMouseEvent& event)
{
    double diff = -event.GetWheelRotation() * canvas->GetSettings().getFloat(Config::ZOOM_SPEED) / 640.0;
    ApplyRelativeZoom(canvas, diff);
}

void ZoomController::SetZoom(MapCanvas* canvas, double value)
{
    if (value < 0.125) {
        value = 0.125;
    }
    if (value > 25.00) {
        value = 25.0;
    }

    if (canvas->GetZoom() != value) {
        int center_x, center_y;
        canvas->GetScreenCenter(&center_x, &center_y);

        canvas->SetZoomDirect(value);
        static_cast<MapWindow*>(canvas->GetParent())->SetScreenCenterPosition(Position(center_x, center_y, canvas->GetFloor()));

        canvas->UpdatePositionStatus();
        UpdateStatus(canvas);
        canvas->Refresh();
    }
}

void ZoomController::ApplyRelativeZoom(MapCanvas* canvas, double diff)
{
    double oldzoom = canvas->GetZoom();
    canvas->SetZoomDirect(oldzoom + diff);

    if (canvas->GetZoom() < 0.125) {
        diff = 0.125 - oldzoom;
        canvas->SetZoomDirect(0.125);
    }
    if (canvas->GetZoom() > 25.00) {
        diff = 25.00 - oldzoom;
        canvas->SetZoomDirect(25.0);
    }

    UpdateStatus(canvas);

    int screensize_x, screensize_y;
    static_cast<MapWindow*>(canvas->GetParent())->GetViewSize(&screensize_x, &screensize_y);

    // This took a day to figure out!
    // Using cursor position from canvas
    int scroll_x = static_cast<int>(screensize_x * diff * (std::max(canvas->GetCursorX(), 1) / double(screensize_x)))
        * canvas->GetContentScaleFactor();
    int scroll_y = static_cast<int>(screensize_y * diff * (std::max(canvas->GetCursorY(), 1) / double(screensize_y)))
        * canvas->GetContentScaleFactor();

    static_cast<MapWindow*>(canvas->GetParent())->ScrollRelative(-scroll_x, -scroll_y);

    canvas->Refresh();
}

void ZoomController::UpdateStatus(MapCanvas* canvas)
{
    int percentage = static_cast<int>((1.0 / canvas->GetZoom()) * 100);
    wxString ss;
    ss << "zoom: " << percentage << "%";
    canvas->GetGui().root->SetStatusText(ss, 3);
}
