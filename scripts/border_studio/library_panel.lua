local Common = dofile("common.lua")
local Logger = dofile("logger.lua")

local M = {}

function M.render(dlg, state, actions)
    dlg:box({
        label = "1. Wybierz Border",
        orient = "vertical",
        width = 300,
        min_width = 280,
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            text = "Wybierz gotowy border albo utworz nowy draft.",
            fgcolor = Common.palette.muted,
        })
        dlg:newrow()
        dlg:input({
            id = "library_filter",
            label = "Search",
            text = state.filter_text or "",
            expand = true,
            onchange = function(d)
                actions.set_filter(d.data.library_filter or "")
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
                id = "library_page_label",
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
            id = "border_library",
            width = 460,
            height = 440,
            expand = true,
            icon_size = 28,
            item_height = 34,
            items = state.library_items or {},
            selection = tonumber(state.library_selection or 0) or 0,
            onchange = function(d)
                actions.handle_library_change(d.data.border_library or 0)
            end,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "Nowy Border",
                bgcolor = Common.palette.accent,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.new_border()
                end,
            })
            dlg:button({
                text = "Duplikuj Border",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.duplicate_current()
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
                id = "current_border_title",
                text = Common.format_border_title(state.draft),
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "current_border_status",
                text = "Status: " .. Common.border_status(state.draft),
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
