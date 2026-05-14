local Common = dofile("common.lua")
local Logger = dofile("logger.lua")

local M = {}

function M.render(dlg, state, actions)
    dlg:box({
        label = "1. Wybierz Ground",
        orient = "vertical",
        width = 320,
        min_width = 300,
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            text = "Wybierz gotowy ground albo utworz nowy draft.",
            fgcolor = Common.palette.muted,
        })
        dlg:newrow()
        dlg:input({
            id = "ground_library_filter",
            label = "Search",
            text = state.filter_text or "",
            expand = true,
            onchange = function(d)
                actions.set_filter(d.data.ground_library_filter or "")
            end,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "<",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.previous_library_page()
                end,
            })
            dlg:label({
                id = "ground_library_page_label",
                text = string.format("Strona %d / %d", tonumber(state.library_page or 1), tonumber(state.library_page_count or 1)),
                fgcolor = Common.palette.subtle,
                min_width = 120,
                align = "center",
            })
            dlg:button({
                text = ">",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.next_library_page()
                end,
            })
        dlg:endbox()
        dlg:newrow()
        dlg:label({
            text = "Zaznacz wpis z listy, potem kliknij 'Otworz Zaznaczony'.",
            fgcolor = Common.palette.subtle,
            font_size = 8,
        })
        dlg:newrow()
        dlg:list({
            id = "ground_library",
            height = 720,
            expand = true,
            icon_size = 32,
            item_height = 40,
            items = state.library_items,
            selection = state.library_selection,
            onchange = function(d)
                local index = d.data.ground_library or 0
                Logger.log("library", string.format("onchange index=%s selected_name=%s", tostring(index), tostring(state.library_visible_names[index] or "")))
                actions.handle_library_change(index)
            end,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "Nowy Ground",
                bgcolor = Common.palette.accent,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.new_ground()
                end,
            })
            dlg:button({
                text = "Duplikuj Ground",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.duplicate_current()
                end,
            })
            dlg:button({
                text = "Otworz Zaznaczony",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.open_selected_ground()
                end,
            })
        dlg:endbox()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Aktualnie edytujesz",
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
            dlg:newrow()
            dlg:label({
                id = "current_ground_title",
                text = Common.format_ground_title(state.draft),
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "current_ground_status",
                text = "Status: " .. Common.ground_status(state.draft),
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
