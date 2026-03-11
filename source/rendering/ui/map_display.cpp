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

#include "app/main.h"

#include <wx/wfstream.h>
#include <chrono>
#include <format>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <time.h>

#include "app/application.h"
#include "app/settings.h"
#include "brushes/brush.h"
#include "brushes/brush_utility.h"
#include "editor/action_queue.h"
#include "editor/editor.h"
#include "game/animation_timer.h"
#include "game/item.h"
#include "game/sprites.h"
#include "live/live_client.h"
#include "live/live_server.h"
#include "map/map.h"
#include "map/tile.h"
#include "palette/palette_window.h"
#include "rendering/core/coordinate_mapper.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/text_renderer.h"
#include "rendering/core/brush_snapshot.h"
#include "rendering/core/render_context.h"
#include "rendering/core/view_snapshot.h"
#include "rendering/map_drawer.h"
#include "rendering/ui/brush_selector.h"
#include "rendering/ui/clipboard_handler.h"
#include "rendering/ui/drawing_controller.h"
#include "rendering/ui/keyboard_handler.h"
#include "rendering/ui/map_display.h"
#include "rendering/ui/map_menu_handler.h"
#include "rendering/ui/map_status_updater.h"
#include "rendering/ui/navigation_controller.h"
#include "rendering/ui/popup_action_handler.h"
#include "rendering/ui/screenshot_controller.h"
#include "rendering/ui/selection_controller.h"
#include "rendering/ui/zoom_controller.h"
#include "rendering/utilities/tile_describer.h"
#include "ui/browse_tile_window.h"
#include "ui/dialog_helper.h"
#include "ui/gui.h"
#include "ui/map_popup_menu.h"
#include "ui/map_tab.h"
#include "ui/properties/old_properties_window.h"
#include "ui/properties/properties_window.h"
#include "ui/tileset_window.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>

#include "brushes/carpet/carpet_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/waypoint/waypoint_brush.h"

// Helper to create attributes
static wxGLAttributes& GetCoreProfileAttributes()
{
    static wxGLAttributes vAttrs = []() {
        wxGLAttributes a;
        a.PlatformDefaults().Defaults().RGBA().DoubleBuffer().Depth(24).Stencil(8).EndList();
        return a;
    }();
    return vAttrs;
}

MapCanvas::MapCanvas(MapWindow* parent, Editor& editor, int* attriblist) :
    wxGLCanvas(parent, GetCoreProfileAttributes(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
    editor(editor),
    renderer_initialized(false),
    input_ {
        .keyCode = WXK_NONE,
        .cursor_x = -1,
        .cursor_y = -1,
        .dragging = false,
        .boundbox_selection = false,
        .screendragging = false,
        .last_cursor_map_x = -1,
        .last_cursor_map_y = -1,
        .last_cursor_map_z = -1,
        .last_click_map_x = -1,
        .last_click_map_y = -1,
        .last_click_map_z = -1,
        .last_click_abs_x = -1,
        .last_click_abs_y = -1,
        .last_click_x = -1,
        .last_click_y = -1,
        .last_mmb_click_x = -1,
        .last_mmb_click_y = -1
    }
{
    view_state_ = std::make_unique<ViewStateManager>(this);

    // Context creation must happen on the main/UI thread
    m_glContext = std::make_unique<wxGLContext>(this, g_gui.GetGLContext(this));
    if (!m_glContext->IsOK()) {
        spdlog::error("MapCanvas: Failed to create wxGLContext");
        m_glContext.reset();
    }

    popup_menu = std::make_unique<MapPopupMenu>(editor);
    animation_timer = std::make_unique<AnimationTimer>(this);
    drawer = std::make_unique<MapDrawer>(editor, RenderContext { g_gui.gfx });
    selection_controller = std::make_unique<SelectionController>(this, editor);
    drawing_controller = std::make_unique<DrawingController>(this, editor);
    screenshot_controller = std::make_unique<ScreenshotController>(this);
    menu_handler = std::make_unique<MapMenuHandler>(this, editor);
    menu_handler->BindEvents();
    Bind(wxEVT_KEY_DOWN, &MapCanvas::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &MapCanvas::OnKeyUp, this);

    Bind(wxEVT_MOTION, &MapCanvas::OnMouseMove, this);
    Bind(wxEVT_LEFT_UP, &MapCanvas::OnMouseLeftRelease, this);
    Bind(wxEVT_LEFT_DOWN, &MapCanvas::OnMouseLeftClick, this);
    Bind(wxEVT_LEFT_DCLICK, &MapCanvas::OnMouseLeftDoubleClick, this);
    Bind(wxEVT_MIDDLE_DOWN, &MapCanvas::OnMouseCenterClick, this);
    Bind(wxEVT_MIDDLE_UP, &MapCanvas::OnMouseCenterRelease, this);
    Bind(wxEVT_RIGHT_DOWN, &MapCanvas::OnMouseRightClick, this);
    Bind(wxEVT_RIGHT_UP, &MapCanvas::OnMouseRightRelease, this);
    Bind(wxEVT_MOUSEWHEEL, &MapCanvas::OnWheel, this);
    Bind(wxEVT_ENTER_WINDOW, &MapCanvas::OnGainMouse, this);
    Bind(wxEVT_LEAVE_WINDOW, &MapCanvas::OnLoseMouse, this);

    Bind(wxEVT_PAINT, &MapCanvas::OnPaint, this);
    Bind(wxEVT_ERASE_BACKGROUND, &MapCanvas::OnEraseBackground, this);
}

MapCanvas::~MapCanvas()
{
    bool context_ok = false;
    if (m_glContext) {
        context_ok = g_gl_context.EnsureContextCurrent(*m_glContext, this);
    } else if (auto context = g_gui.GetGLContext(this)) {
        context_ok = g_gl_context.EnsureContextCurrent(*context, this);
    }

    if (!context_ok) {
        spdlog::warn("MapCanvas: Destroying canvas without a current OpenGL context. Cleanup might fail or assert.");
    }

    drawer.reset();
    m_nvg.reset();

    g_gl_context.UnregisterCanvas(this);
}

void MapCanvas::Refresh()
{
    if (refresh_watch.Time() > g_settings.getInteger(Config::HARD_REFRESH_RATE)) {
        refresh_watch.Start();
        wxGLCanvas::Update();
    }
    wxGLCanvas::Refresh();
}

void MapCanvas::SetZoom(double value)
{
    ZoomController::SetZoom(this, value);
}

void MapCanvas::GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) const
{
    GetMapWindow()->GetViewSize(screensize_x, screensize_y);
    GetMapWindow()->GetViewStart(view_scroll_x, view_scroll_y);
}

MapWindow* MapCanvas::GetMapWindow() const
{
    return static_cast<MapWindow*>(GetParent());
}

void MapCanvas::EnsureNanoVG()
{
    if (!m_nvg) {
        if (!gladLoadGL()) {
            spdlog::error("MapCanvas: Failed to initialize GLAD");
        }
        m_nvg.reset(nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES));
        if (m_nvg) {
            TextRenderer::LoadFont(m_nvg.get());
        } else {
            spdlog::error("MapCanvas: Failed to initialize NanoVG");
        }
    }
}

void MapCanvas::DrawOverlays(NVGcontext* vg, const RenderSettings& settings, const FrameOptions& frame)
{
    if (!vg) {
        return;
    }

    // Sanitize state before handover to NanoVG
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glClear(GL_STENCIL_BUFFER_BIT);
    TextRenderer::BeginFrame(vg, GetSize().x, GetSize().y, GetContentScaleFactor());

    if (settings.show_creatures) {
        drawer->DrawCreatureNames(vg);
    }
    if (settings.show_tooltips) {
        drawer->DrawTooltips(vg);
    }
    if (settings.show_hooks) {
        drawer->DrawHookIndicators(vg);
    }
    if (settings.highlight_locked_doors) {
        drawer->DrawDoorIndicators(vg);
    }

    TextRenderer::EndFrame(vg);

    // Sanitize state after NanoVG to avoid polluting the next frame or other tabs
    glUseProgram(0);
    glBindVertexArray(0);
}

ViewSnapshot MapCanvas::BuildViewSnapshot() const
{
    ViewSnapshot snapshot;
    snapshot.zoom = static_cast<float>(view_state_->getZoom());
    snapshot.floor = view_state_->getFloor();

    int mouse_map_x, mouse_map_y;
    view_state_->screenToMap(input_.cursor_x, input_.cursor_y, &mouse_map_x, &mouse_map_y);
    snapshot.mouse_map_x = mouse_map_x;
    snapshot.mouse_map_y = mouse_map_y;

    int vsx, vsy, ssx, ssy;
    GetViewBox(&vsx, &vsy, &ssx, &ssy);
    snapshot.view_scroll_x = vsx;
    snapshot.view_scroll_y = vsy;
    snapshot.screensize_x = ssx;
    snapshot.screensize_y = ssy;

    snapshot.last_click_map_x = input_.last_click_map_x;
    snapshot.last_click_map_y = input_.last_click_map_y;
    snapshot.last_cursor_map_x = input_.last_cursor_map_x;
    snapshot.last_cursor_map_y = input_.last_cursor_map_y;

    snapshot.last_click_abs_x = input_.last_click_abs_x;
    snapshot.last_click_abs_y = input_.last_click_abs_y;
    snapshot.cursor_x = input_.cursor_x;
    snapshot.cursor_y = input_.cursor_y;

    // Only use the drag-preview overlay for explicit drag-draw gestures.
    snapshot.is_dragging_draw = drawing_controller->IsDraggingDraw();
    snapshot.drag_start = selection_controller->GetDragStartPosition();

    if (auto* mapTab = g_gui.GetCurrentMapTab()) {
        snapshot.secondary_map = mapTab->GetSession()->secondary_map;
    }
    snapshot.is_pasting = isPasting();

    return snapshot;
}

void MapCanvas::ConfigureRenderSettings(RenderSettings& settings) const
{
    if (screenshot_controller->IsCapturing()) {
        settings.SetIngame();
    } else {
        settings = RenderSettings::FromSettings(g_settings, g_gui.GetLightIntensity(), g_gui.GetAmbientLightLevel());
    }
}

void MapCanvas::ConfigureFrameOptions(FrameOptions& frame) const
{
    frame.dragging = selection_controller->IsDragging();
    frame.boundbox_selection = selection_controller->IsBoundboxSelection();
}

void MapCanvas::PerformGarbageCollection()
{
    // Only run GC if this is the active tab to prevent multiple tabs from fighting over resources
    if (g_gui.gfx.shouldCollectGarbage() && g_gui.GetCurrentMapTab() == GetParent()) {
        g_gui.gfx.garbageCollection();
        g_gui.gfx.markGarbageCollected();
    }
}

void MapCanvas::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this); // validates the paint event
    if (m_glContext) {
        g_gl_context.EnsureContextCurrent(*m_glContext, this);
        g_gl_context.SetFallbackCanvas(this);
    }

    EnsureNanoVG();

    if (g_gui.IsRenderingEnabled()) {
        g_gui.gfx.updateTime();

        RenderSettings& settings = drawer->getRenderSettings();
        FrameOptions& frame = drawer->getFrameOptions();
        ConfigureRenderSettings(settings);
        ConfigureFrameOptions(frame);

        if (settings.show_preview) {
            animation_timer->Start();
            g_gui.gfx.resumeAnimation();
        } else {
            animation_timer->Stop();
            g_gui.gfx.pauseAnimation();
        }

        ViewSnapshot snapshot = BuildViewSnapshot();
        const BrushSnapshot brush_snap {
            .current_brush = g_gui.GetCurrentBrush(),
            .brush_shape = g_gui.GetBrushShape(),
            .brush_size = g_gui.GetBrushSize(),
            .is_drawing_mode = g_gui.IsDrawingMode()
        };

        drawer->SetupVars(snapshot, brush_snap);
        drawer->SetupGL();
        drawer->Draw();
        drawer->DrainPendingNodeRequests();

        if (screenshot_controller->IsCapturing()) {
            drawer->TakeScreenshot(screenshot_controller->GetBuffer());
        }

        drawer->Release();

        DrawOverlays(m_nvg.get(), settings, frame);
        drawer->BeginFrame();
    }

    PerformGarbageCollection();

    SwapBuffers();

    // FPS tracking and limiting
    frame_pacer.UpdateAndLimit(g_settings.getInteger(Config::FRAME_RATE_LIMIT), g_settings.getBoolean(Config::SHOW_FPS_COUNTER));

    // Send newd node requests
    if (editor.live_manager.GetClient()) {
        editor.live_manager.GetClient()->sendNodeRequests();
    }
}

void MapCanvas::TakeScreenshot(wxFileName path, wxString format)
{
    screenshot_controller->TakeScreenshot(path, format);
}

void MapCanvas::ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y)
{
    view_state_->screenToMap(screen_x, screen_y, map_x, map_y);
}
#if 0

*map_y = int(start_y + (screen_y * zoom)) / TILE_SIZE;
}

if (floor <= GROUND_LAYER) {
	*map_x += GROUND_LAYER - floor;
	*map_y += GROUND_LAYER - floor;
} /* else {
	 *map_x += MAP_MAX_LAYER - floor;
	 *map_y += MAP_MAX_LAYER - floor;
 }*/
}

#endif
void MapCanvas::GetScreenCenter(int* map_x, int* map_y)
{
    int width, height;
    GetMapWindow()->GetViewSize(&width, &height);
    ScreenToMap(width / 2, height / 2, map_x, map_y);
}

Position MapCanvas::GetCursorPosition() const
{
    return Position(input_.last_cursor_map_x, input_.last_cursor_map_y, view_state_->getFloor());
}

void MapCanvas::UpdatePositionStatus(int x, int y)
{
    if (x == -1) {
        x = input_.cursor_x;
    }
    if (y == -1) {
        y = input_.cursor_y;
    }

    int map_x, map_y;
    ScreenToMap(x, y, &map_x, &map_y);

    MapStatusUpdater::Update(editor, map_x, map_y, view_state_->getFloor());
}

void MapCanvas::UpdateZoomStatus()
{
    ZoomController::UpdateStatus(this);
}

void MapCanvas::OnMouseMove(wxMouseEvent& event)
{
    NavigationController::HandleMouseDrag(this, event);

    input_.cursor_x = event.GetX();
    input_.cursor_y = event.GetY();

    int mouse_map_x, mouse_map_y;
    MouseToMap(&mouse_map_x, &mouse_map_y);

    bool map_update = false;
    if (input_.last_cursor_map_x != mouse_map_x || input_.last_cursor_map_y != mouse_map_y || input_.last_cursor_map_z != view_state_->getFloor()) {
        map_update = true;
    }
    input_.last_cursor_map_x = mouse_map_x;
    input_.last_cursor_map_y = mouse_map_y;
    input_.last_cursor_map_z = view_state_->getFloor();

    if (map_update) {
        g_gui.UpdateAutoborderPreview(Position(mouse_map_x, mouse_map_y, view_state_->getFloor()));
        UpdatePositionStatus(input_.cursor_x, input_.cursor_y);
        UpdateZoomStatus();
        Refresh();
    }

    if (g_gui.IsSelectionMode()) {
        selection_controller->HandleDrag(
            Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown()
        );
    } else { // Drawing mode
        drawing_controller->HandleDrag(Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown());
    }
}

void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event)
{
    OnMouseActionRelease(event);
}

void MapCanvas::OnMouseLeftClick(wxMouseEvent& event)
{
    OnMouseActionClick(event);
}

void MapCanvas::OnMouseLeftDoubleClick(wxMouseEvent& event)
{
    int mouse_map_x, mouse_map_y;
    ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
    selection_controller->HandleDoubleClick(Position(mouse_map_x, mouse_map_y, view_state_->getFloor()));
}

void MapCanvas::OnMouseCenterClick(wxMouseEvent& event)
{
    if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
        OnMousePropertiesClick(event);
    } else {
        OnMouseCameraClick(event);
    }
}

void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event)
{
    if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
        OnMousePropertiesRelease(event);
    } else {
        OnMouseCameraRelease(event);
    }
}

void MapCanvas::OnMouseRightClick(wxMouseEvent& event)
{
    if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
        OnMouseCameraClick(event);
    } else {
        OnMousePropertiesClick(event);
    }
}

void MapCanvas::OnMouseRightRelease(wxMouseEvent& event)
{
    if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
        OnMouseCameraRelease(event);
    } else {
        OnMousePropertiesRelease(event);
    }
}

void MapCanvas::OnMouseActionClick(wxMouseEvent& event)
{
    SetFocus();

    int mouse_map_x, mouse_map_y;
    ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

    if (event.ControlDown() && event.AltDown()) {
        Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, view_state_->getFloor());
        BrushSelector::SelectSmartBrush(editor, tile);
    } else if (g_gui.IsSelectionMode()) {
        selection_controller->HandleClick(
            Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown()
        );
    } else if (g_gui.GetCurrentBrush()) { // Drawing mode
        drawing_controller->HandleClick(Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown());
    }
    input_.last_click_x = int(event.GetX() * view_state_->getZoom());
    input_.last_click_y = int(event.GetY() * view_state_->getZoom());

    int start_x, start_y;
    static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);
    input_.last_click_abs_x = input_.last_click_x + start_x;
    input_.last_click_abs_y = input_.last_click_y + start_y;

    input_.last_click_map_x = mouse_map_x;
    input_.last_click_map_y = mouse_map_y;
    input_.last_click_map_z = view_state_->getFloor();
    g_gui.RefreshView();
    g_gui.UpdateMinimap();
}

void MapCanvas::OnMouseActionRelease(wxMouseEvent& event)
{
    int mouse_map_x, mouse_map_y;
    ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

    int move_x = input_.last_click_map_x - mouse_map_x;
    int move_y = input_.last_click_map_y - mouse_map_y;
    int move_z = input_.last_click_map_z - view_state_->getFloor();

    if (g_gui.IsSelectionMode()) {
        selection_controller->HandleRelease(
            Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown()
        );
    } else if (g_gui.GetCurrentBrush()) { // Drawing mode
        drawing_controller->HandleRelease(
            Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown()
        );
    }
    g_gui.RefreshView();
    g_gui.UpdateMinimap();
}

void MapCanvas::OnMouseCameraClick(wxMouseEvent& event)
{
    SetFocus();

    input_.last_mmb_click_x = event.GetX();
    input_.last_mmb_click_y = event.GetY();
    if (event.ControlDown()) {
        ZoomController::ApplyRelativeZoom(this, 1.0 - view_state_->getZoom());
    } else {
        NavigationController::HandleCameraClick(this, event);
    }
}

void MapCanvas::OnMouseCameraRelease(wxMouseEvent& event)
{
    NavigationController::HandleCameraRelease(this, event);
}

void MapCanvas::OnMousePropertiesClick(wxMouseEvent& event)
{
    SetFocus();

    int mouse_map_x, mouse_map_y;
    ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
    Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, view_state_->getFloor());

    if (g_gui.IsDrawingMode()) {
        g_gui.SetSelectionMode();
    }

    selection_controller->HandlePropertiesClick(
        Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown()
    );
    input_.last_click_y = int(event.GetY() * view_state_->getZoom());

    int start_x, start_y;
    static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);
    input_.last_click_abs_x = input_.last_click_x + start_x;
    input_.last_click_abs_y = input_.last_click_y + start_y;

    input_.last_click_map_x = mouse_map_x;
    input_.last_click_map_y = mouse_map_y;
    g_gui.RefreshView();
}

void MapCanvas::OnMousePropertiesRelease(wxMouseEvent& event)
{
    int mouse_map_x, mouse_map_y;
    ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

    if (g_gui.IsDrawingMode()) {
        g_gui.SetSelectionMode();
    }

    selection_controller->HandlePropertiesRelease(
        Position(mouse_map_x, mouse_map_y, view_state_->getFloor()), event.ShiftDown(), event.ControlDown(), event.AltDown()
    );

    popup_menu->Update();
    PopupMenu(popup_menu.get());

    editor.actionQueue->resetTimer();
    input_.dragging = false;
    input_.boundbox_selection = false;

    input_.last_cursor_map_x = mouse_map_x;
    input_.last_cursor_map_y = mouse_map_y;
    input_.last_cursor_map_z = view_state_->getFloor();

    g_gui.RefreshView();
}

void MapCanvas::OnWheel(wxMouseEvent& event)
{
    if (event.ControlDown()) {
        NavigationController::HandleWheel(this, event);
    } else if (event.AltDown()) {
        drawing_controller->HandleWheel(event.GetWheelRotation(), event.AltDown(), event.ControlDown());
    } else {
        ZoomController::OnWheel(this, event);
    }

    Refresh();
}

void MapCanvas::OnLoseMouse(wxMouseEvent& event)
{
    Refresh();
}

void MapCanvas::OnGainMouse(wxMouseEvent& event)
{
    if (!event.LeftIsDown()) {
        input_.dragging = false;
        input_.boundbox_selection = false;
        drawing_controller->Reset();
    }
    if (!event.MiddleIsDown()) {
        input_.screendragging = false;
    }

    Refresh();
}

void MapCanvas::OnKeyDown(wxKeyEvent& event)
{
    KeyboardHandler::OnKeyDown(this, event);
}

void MapCanvas::OnKeyUp(wxKeyEvent& event)
{
    KeyboardHandler::OnKeyUp(this, event);
}

void MapCanvas::ChangeFloor(int new_floor)
{
    NavigationController::ChangeFloor(this, new_floor);
}

void MapCanvas::EnterDrawingMode()
{
    input_.dragging = false;
    input_.boundbox_selection = false;
    EndPasting();
    Refresh();
}

void MapCanvas::EnterSelectionMode()
{
    drawing_controller->Reset();
    editor.replace_brush = nullptr;
    Refresh();
}

bool MapCanvas::isPasting() const
{
    return g_gui.IsPasting();
}

void MapCanvas::StartPasting()
{
    g_gui.StartPasting();
}

void MapCanvas::EndPasting()
{
    g_gui.EndPasting();
}

void MapCanvas::Reset()
{
    input_.cursor_x = 0;
    input_.cursor_y = 0;

    view_state_->setZoom(1.0);
    view_state_->setFloor(GROUND_LAYER);

    input_.dragging = false;
    input_.boundbox_selection = false;
    input_.screendragging = false;
    drawing_controller->Reset();

    editor.replace_brush = nullptr;

    input_.last_click_map_x = -1;
    input_.last_click_map_y = -1;
    input_.last_click_map_z = -1;

    input_.last_mmb_click_x = -1;
    input_.last_mmb_click_y = -1;

    editor.selection.clear();
    editor.actionQueue->clear();
}
