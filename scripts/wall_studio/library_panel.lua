local Common = dofile("common.lua")

local M = {}

function M.render(dlg, state, actions)
    dlg:box({
        label = "1. Wybierz Wall Brush",
        orient = "vertical",
        width = 320,
        min_width = 300,
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            text = "Wybierz istniejacy wall brush albo utworz nowy draft.",
            fgcolor = Common.palette.muted,
        })
        dlg:newrow()
        dlg:input({
            id = "wall_library_filter",
            label = "Search",
            text = state.filter_text or "",
            expand = true,
            onchange = function(d)
                actions.set_filter(d.data.wall_library_filter or "")
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
                id = "wall_library_page_label",
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
        dlg:list({
            id = "wall_library",
            height = 720,
            expand = true,
            icon_size = 32,
            item_height = 40,
            items = state.library_items,
            selection = state.library_selection,
            onchange = function(d)
                actions.handle_library_change(d.data.wall_library or 0)
            end,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "Nowy Wall",
                bgcolor = Common.palette.accent,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.new_wall()
                end,
            })
            dlg:button({
                text = "Duplikuj",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.duplicate_current()
                end,
            })
            dlg:button({
                text = "Otworz",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.open_selected_wall()
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
                id = "current_wall_title",
                text = Common.format_wall_title(state.draft),
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "current_wall_status",
                text = "Status: " .. Common.wall_status(state.draft),
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
