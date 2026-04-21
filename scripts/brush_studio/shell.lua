local Session = dofile("session.lua")
local Contract = dofile("page_contract.lua")
local PlaceholderPage = dofile("placeholder_page.lua")
local Paths = dofile("module_paths.lua")
local BorderStudioPage = Paths.load("border_studio/page.lua")
local GroundStudioPage = Paths.load("ground_studio/page.lua")
local WallStudioPage = Paths.load("wall_studio/page.lua")
local ComposerPage = dofile("composer_page.lua")
local PalettePage = dofile("palette_page.lua")

local Shell = {}

local palette = {
    bg = "#2f2f33",
    header = "#233246",
    panel = "#2f2f33",
    panel_alt = "#31455f",
    sidebar = "#26282d",
    accent = "#d68000",
    text = "#f1f1f1",
    muted = "#b8bcc7",
    subtle = "#8e97a6",
}

local function nav_button(dlg, label, active, onclick)
    dlg:button({
        text = label,
        min_width = 110,
        bgcolor = active and palette.accent or palette.panel_alt,
        fgcolor = palette.text,
        onclick = onclick,
    })
end

function Shell.open_standalone()
    local existing_dialog = _G.BRUSH_STUDIO_DIALOG
    if existing_dialog then
        pcall(function()
            existing_dialog:close()
        end)
        _G.BRUSH_STUDIO_DIALOG = nil
        _G.BRUSH_STUDIO_OPEN = false
        app.yield()
    end

    if _G.BORDER_STUDIO_DIALOG then
        pcall(function()
            _G.BORDER_STUDIO_DIALOG:close()
        end)
        _G.BORDER_STUDIO_DIALOG = nil
        _G.BORDER_STUDIO_OPEN = false
    end
    if _G.GROUND_STUDIO_DIALOG then
        pcall(function()
            _G.GROUND_STUDIO_DIALOG:close()
        end)
        _G.GROUND_STUDIO_DIALOG = nil
        _G.GROUND_STUDIO_OPEN = false
    end

    local dlg
    local session = Session.new()
    local state = {
        active_page_key = "borders",
        shell_status = "Gotowe.",
    }

    local pages = {}

    local function rebuild()
        if not dlg then
            return
        end

        local active_page = pages[state.active_page_key]
        dlg:clear()

        dlg:panel({
            bgcolor = palette.sidebar,
            padding = 6,
            margin = 4,
            expand = false,
        })
            dlg:box({
                orient = "horizontal",
                expand = true,
            })
                dlg:label({
                    text = "Brush Studio",
                    fgcolor = palette.text,
                    font_weight = "bold",
                    min_width = 120,
                })
                nav_button(dlg, "Borders", state.active_page_key == "borders", function()
                    if state.active_page_key == "borders" then
                        return
                    end
                    local current_page = pages[state.active_page_key]
                    if current_page and not current_page.confirmLeave() then
                        return
                    end
                    if current_page then
                        current_page.onLeave(session)
                    end
                    state.active_page_key = "borders"
                    pages[state.active_page_key].onEnter(session)
                    state.shell_status = "Przelaczono na workflow borderow."
                    rebuild()
                end)
                nav_button(dlg, "Grounds", state.active_page_key == "grounds", function()
                    if state.active_page_key == "grounds" then
                        return
                    end
                    local current_page = pages[state.active_page_key]
                    if current_page and not current_page.confirmLeave() then
                        return
                    end
                    if current_page then
                        current_page.onLeave(session)
                    end
                    state.active_page_key = "grounds"
                    pages[state.active_page_key].onEnter(session)
                    state.shell_status = "Przelaczono na workflow groundow."
                    rebuild()
                end)
                nav_button(dlg, "Composer", state.active_page_key == "composer", function()
                    if state.active_page_key == "composer" then
                        return
                    end
                    local current_page = pages[state.active_page_key]
                    if current_page and not current_page.confirmLeave() then
                        return
                    end
                    if current_page then
                        current_page.onLeave(session)
                    end
                    state.active_page_key = "composer"
                    pages[state.active_page_key].onEnter(session)
                    state.shell_status = "Przelaczono na workflow composera."
                    rebuild()
                end)
                dlg:panel({
                    bgcolor = palette.sidebar,
                    padding = 0,
                    margin = 0,
                    min_width = 44,
                    max_width = 44,
                    expand = false,
                })
                dlg:endpanel()
                nav_button(dlg, "Walls", state.active_page_key == "walls", function()
                    if state.active_page_key == "walls" then
                        return
                    end
                    local current_page = pages[state.active_page_key]
                    if current_page and not current_page.confirmLeave() then
                        return
                    end
                    if current_page then
                        current_page.onLeave(session)
                    end
                    state.active_page_key = "walls"
                    pages[state.active_page_key].onEnter(session)
                    state.shell_status = "Przelaczono na workflow scian."
                    rebuild()
                end)
                nav_button(dlg, "Palette", state.active_page_key == "palette", function()
                    if state.active_page_key == "palette" then
                        return
                    end
                    local current_page = pages[state.active_page_key]
                    if current_page and not current_page.confirmLeave() then
                        return
                    end
                    if current_page then
                        current_page.onLeave(session)
                    end
                    state.active_page_key = "palette"
                    pages[state.active_page_key].onEnter(session)
                    state.shell_status = "Przelaczono na workflow publikacji palety."
                    rebuild()
                end)
                dlg:label({
                    text = string.format(
                        "RAW %d | B %s | G %s | C %s | W %s",
                        tonumber(session.lastSelectedRaw or 0) or 0,
                        session.lastSavedBorder and ("#" .. tostring(session.lastSavedBorder)) or "-",
                        tostring(session.lastSavedGround or "-"),
                        tostring(session.lastSavedComposedBrush or "-"),
                        tostring(session.lastSavedWall or "-")
                    ),
                    fgcolor = palette.muted,
                    min_width = 360,
                })
            dlg:endbox()
        dlg:endpanel()

        if state.shell_status and state.shell_status ~= "" then
            dlg:label({
                text = state.shell_status,
                fgcolor = palette.subtle,
            })
            dlg:newrow()
        end

        active_page.render_into(dlg)

        dlg:label({
            text = string.format(
                "Dirty: B=%s | G=%s | C=%s | P=%s | W=%s",
                session.pageDirty and session.pageDirty.borders and "tak" or "nie",
                session.pageDirty and session.pageDirty.grounds and "tak" or "nie",
                session.pageDirty and session.pageDirty.composer and "tak" or "nie",
                session.pageDirty and session.pageDirty.palette and "tak" or "nie",
                session.pageDirty and session.pageDirty.walls and "tak" or "nie"
            ),
            fgcolor = palette.subtle,
        })

        dlg:layout()
        dlg:repaint()
    end

    pages.borders = Contract.assert_page(BorderStudioPage.create({
        session = session,
        onRequestRender = rebuild,
    }), "Borders")
    pages.grounds = Contract.assert_page(GroundStudioPage.create({
        session = session,
        onRequestRender = rebuild,
    }), "Grounds")
    pages.composer = Contract.assert_page(ComposerPage.create({
        session = session,
        onRequestRender = rebuild,
    }), "Composer")
    pages.walls = Contract.assert_page(WallStudioPage.create({
        session = session,
        onRequestRender = rebuild,
    }), "Walls")
    pages.palette = Contract.assert_page(PalettePage.create({
        session = session,
        onRequestRender = rebuild,
    }), "Palette")

    dlg = Dialog({
        title = "Brush Studio",
        id = "brush_studio_dock",
        width = 1700,
        height = 960,
        resizable = true,
        dockable = true,
        onclose = function()
            local active_page = pages[state.active_page_key]
            if active_page then
                active_page.onLeave(session)
            end
            _G.BRUSH_STUDIO_OPEN = false
            _G.BRUSH_STUDIO_DIALOG = nil
        end,
    })

    _G.BRUSH_STUDIO_OPEN = true
    _G.BRUSH_STUDIO_DIALOG = dlg
    pages[state.active_page_key].onEnter(session)
    rebuild()
    dlg:show({ wait = false, center = "screen" })
end

return Shell
