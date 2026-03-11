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

#ifndef RME_DISPLAY_WINDOW_H_
#define RME_DISPLAY_WINDOW_H_

#include "editor/action.h"
#include "game/creature.h"
#include "map/tile.h"
#include "rendering/utilities/frame_pacer.h"

#include "game/animation_timer.h"
#include "rendering/core/graphics.h"
#include "rendering/ui/render_loop.h"
#include "rendering/ui/input_state.h"
#include "rendering/ui/view_state_manager.h"
#include "ui/map_popup_menu.h"
#include <chrono>
#include <cstddef>
#include <memory>

struct NVGcontext;
struct RenderSettings;
struct FrameOptions;
struct ViewSnapshot;
class GUI;
class Settings;
class Brush;
class MapTab;
class EditorTab;
class wxGLContext;

class Item;
class Creature;
class MapWindow;
class AnimationTimer;
class MapDrawer;
class SelectionController;
class DrawingController;
class ScreenshotController;
class MapMenuHandler;

namespace rme::rendering {
class GLContextManager;
}

class MapCanvas : public wxGLCanvas, public rme::rendering::RenderLoopHost {

public:
    MapCanvas(MapWindow* parent, Editor& editor, GUI& gui, Settings& settings, int* attriblist);
    ~MapCanvas() override;
    void Reset();

    // All events
    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event) { }

    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeftRelease(wxMouseEvent& event);
    void OnMouseLeftClick(wxMouseEvent& event);
    void OnMouseLeftDoubleClick(wxMouseEvent& event);
    void OnMouseCenterClick(wxMouseEvent& event);
    void OnMouseCenterRelease(wxMouseEvent& event);
    void OnMouseRightClick(wxMouseEvent& event);
    void OnMouseRightRelease(wxMouseEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnWheel(wxMouseEvent& event);
    void OnGainMouse(wxMouseEvent& event);
    void OnLoseMouse(wxMouseEvent& event);

    // Mouse events handlers (called by the above)
    void OnMouseActionRelease(wxMouseEvent& event);
    void OnMouseActionClick(wxMouseEvent& event);
    void OnMouseCameraClick(wxMouseEvent& event);
    void OnMouseCameraRelease(wxMouseEvent& event);
    void OnMousePropertiesClick(wxMouseEvent& event);
    void OnMousePropertiesRelease(wxMouseEvent& event);

    void Refresh();

    void ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y);
    void MouseToMap(int* map_x, int* map_y)
    {
        ScreenToMap(input_.cursor_x, input_.cursor_y, map_x, map_y);
    }
    void GetScreenCenter(int* map_x, int* map_y);

    void StartPasting();
    void EndPasting();
    void EnterSelectionMode();
    void EnterDrawingMode();

    void UpdatePositionStatus(int x = -1, int y = -1);
    void UpdatePositionStatus(int map_x, int map_y, int map_z);
    void UpdateZoomStatus();

    void ChangeFloor(int new_floor);
    int GetFloor() const
    {
        return view_state_->getFloor();
    }
    double GetZoom() const
    {
        return view_state_->getZoom();
    }
    void SetZoom(double value);
    ViewStateManager& GetViewState() { return *view_state_; }
    void GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) const;

    Position GetCursorPosition() const;

    void TakeScreenshot(wxFileName path, wxString format);

    bool isPasting() const;

    // Public members — widely referenced by framework and event handlers
    Editor& editor;
    std::unique_ptr<MapDrawer> drawer;
    std::unique_ptr<rme::rendering::GLContextManager> gl_context_;
    std::unique_ptr<rme::rendering::RenderLoop> render_loop_;
    std::unique_ptr<ScreenshotController> screenshot_controller;

    wxStopWatch refresh_watch;
    std::unique_ptr<MapPopupMenu> popup_menu;
    std::unique_ptr<AnimationTimer> animation_timer;

    FramePacer frame_pacer;

    std::unique_ptr<SelectionController> selection_controller;
    std::unique_ptr<DrawingController> drawing_controller;
    std::unique_ptr<MapMenuHandler> menu_handler;

    MapWindow* GetMapWindow() const;
    GUI& GetGui() const { return gui_; }
    Settings& GetSettings() const { return settings_; }
    GraphicManager& GetGraphics() const;
    wxGLContext* GetSharedGLContext() const;
    MapTab* GetCurrentMapTab() const;
    EditorTab* GetCurrentTab() const;
    Editor* GetCurrentEditor() const;
    bool IsEditorOpen() const;
    Brush* GetCurrentBrush() const;
    BrushShape GetBrushShape() const;
    int GetBrushSize() const;
    int GetBrushVariation() const;
    void SetBrushSize(int size);
    void SetBrushVariation(int variation);
    void IncreaseBrushSize(bool wrap = false);
    void DecreaseBrushSize(bool wrap = false);
    bool SelectBrush(const Brush* brush, PaletteType palette = TILESET_UNKNOWN);
    void SelectPreviousBrush();
    void FillDoodadPreviewBuffer();
    void UpdateAutoborderPreview(Position pos);
    void RefreshView();
    void UpdateMinimap(bool immediate = false);
    void UpdateMenubar();
    void SetStatusText(const wxString& text);
    void SetSelectionMode();
    void SetDrawingMode();
    void SwitchMode();
    bool IsSelectionMode() const;
    bool IsDrawingMode() const;
    bool IsRenderingEnabledViaGui() const;
    void CycleTab(bool forward = true);
    void SetScreenCenterPosition(Position pos);
    void RebuildPalettes();
    float GetLightIntensity() const;
    float GetAmbientLightLevel() const;
    wxGLCanvas& canvas() override
    {
        return *this;
    }
    bool isRenderingEnabled() const override;
    bool isThreadedRenderingEnabled() const override;
    size_t planningWorkerCount() const override;
    GraphicManager& graphics() const override;
    RenderSettings buildRenderSettings() const override;
    FrameOptions buildFrameOptions() const override;
    ViewSnapshot buildViewSnapshot() const override;
    BrushSnapshot buildBrushSnapshot() const override;
    BrushVisualSettings buildBrushVisualSettings() const override;
    void updateAnimationState(bool show_preview) override;
    bool isCapturingScreenshot() const override;
    uint8_t* screenshotBuffer() const override;
    bool shouldCollectGarbage() const override;
    void collectGarbage() override;
    void updateFramePacing() override;
    void sendNodeRequests() override;

    // --- Accessors for privatized fields ---
    int GetKeyCode() const
    {
        return input_.keyCode;
    }
    void SetKeyCode(int code)
    {
        input_.keyCode = code;
    }
    int GetCursorX() const
    {
        return input_.cursor_x;
    }
    int GetCursorY() const
    {
        return input_.cursor_y;
    }
    bool IsScreenDragging() const
    {
        return input_.screendragging;
    }
    void SetScreenDragging(bool v)
    {
        input_.screendragging = v;
    }
    int GetLastClickMapX() const
    {
        return input_.last_click_map_x;
    }
    int GetLastClickMapY() const
    {
        return input_.last_click_map_y;
    }
    int GetLastClickMapZ() const
    {
        return input_.last_click_map_z;
    }
    int GetLastMmbClickX() const
    {
        return input_.last_mmb_click_x;
    }
    int GetLastMmbClickY() const
    {
        return input_.last_mmb_click_y;
    }
    void SetLastMmbClickX(int v)
    {
        input_.last_mmb_click_x = v;
    }
    void SetLastMmbClickY(int v)
    {
        input_.last_mmb_click_y = v;
    }
    void SetFloorDirect(int f)
    {
        view_state_->setFloor(f);
    }
    void SetZoomDirect(double z)
    {
        view_state_->setZoom(z);
    }

private:
    GUI& gui_;
    Settings& settings_;
    std::unique_ptr<ViewStateManager> view_state_;
    InputState input_;
    std::chrono::steady_clock::time_point next_hover_ui_update_ {};

    ViewSnapshot BuildViewSnapshot() const;
    void ConfigureRenderSettings(RenderSettings& settings) const;
    void ConfigureFrameOptions(FrameOptions& frame) const;
    [[nodiscard]] bool shouldRefreshHoverUi() const;
};

#endif
