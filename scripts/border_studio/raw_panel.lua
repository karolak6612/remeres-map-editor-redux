local Common = dofile("common.lua")

local M = {}

function M.render(dlg, state, actions)
    local raw_info = Common.safe_item_info(state.selected_raw_id)
    local raw_title = "Najpierw wybierz RAW tile"
    local raw_meta = "Wybrany RAW z glownej palety RME pojawi sie tutaj automatycznie."

    if raw_info then
        raw_title = raw_info.name or ("Item " .. tostring(raw_info.id))
        raw_meta = string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0)
    end

    dlg:box({
        label = "2. Wybrany RAW",
        orient = "vertical",
        width = 280,
        min_width = 260,
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            text = "Aktualny RAW z glownej palety.",
            fgcolor = Common.palette.muted,
        })
        dlg:newrow()
        dlg:image({
            id = "selected_raw_preview",
            label = "RAW",
            image = Common.safe_image(state.selected_raw_id),
            width = 48,
            height = 48,
            smooth = false,
        })
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                id = "selected_raw_title",
                text = raw_title,
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "selected_raw_meta",
                text = raw_meta,
                fgcolor = Common.palette.muted,
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
