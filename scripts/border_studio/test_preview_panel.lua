local Common = dofile("common.lua")

local M = {}

local PREVIEW_OUTSIDE = "#6B7280"
local PREVIEW_INSIDE = "#65A30D"
local PREVIEW_EMPTY = "#111827"

local function render_patch_cell(dlg, draft, index)
    local visual_key = Common.slot_layout[index]
    local row = math.floor((index - 1) / 5) + 1
    local col = ((index - 1) % 5) + 1
    local inside_center = row == 3 and col == 3

    local background = inside_center and PREVIEW_INSIDE or PREVIEW_OUTSIDE
    if not visual_key then
        background = PREVIEW_EMPTY
    end

    dlg:panel({
        bgcolor = background,
        padding = 2,
        margin = 2,
        width = 58,
        height = 58,
        expand = false,
    })
        if visual_key == "c" then
            dlg:label({
                text = "C",
                fgcolor = Common.palette.text,
                font_weight = "bold",
                align = "center",
            })
        elseif visual_key then
            local stored_key = Common.storage_slot_name(visual_key)
            local item_id = draft and draft:getSlotItemId(stored_key) or 0
            dlg:image({
                image = Common.safe_image(item_id),
                width = 42,
                height = 42,
                smooth = false,
            })
            dlg:newrow()
            dlg:label({
                text = Common.slot_meta[visual_key].short,
                fgcolor = Common.palette.text,
                font_size = 7,
                align = "center",
            })
        end
    dlg:endpanel()
end

local function render_patch(dlg, draft)
    dlg:box({
        orient = "vertical",
        expand = false,
    })
    for row = 1, 5 do
        if row > 1 then
            dlg:newrow()
        end
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
        for col = 1, 5 do
            local index = ((row - 1) * 5) + col
            render_patch_cell(dlg, draft, index)
        end
        dlg:endbox()
    end
    dlg:endbox()
end

function M.render(dlg, draft)
    local empty_slots = draft and draft:getEmptySlots() or Common.clone(Common.slot_order)
    local is_complete = draft and #empty_slots == 0 and not draft:isEmpty()
    local status_label = is_complete and "Podglad mapowy jest kompletny." or "Podglad mapowy pokazuje brakujace sloty."
    local missing_label = #empty_slots > 0 and table.concat(empty_slots, ", ") or "none"

    dlg:box({
        label = "4. Sprawdz Podglad",
        orient = "vertical",
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Podglad mapowy 5x5",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                text = "Ten widok pokazuje border tak, jak czytasz go na mapie: srodek jest polem area, sloty dookola sa odwrocone wzgledem zapisu XML.",
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
        dlg:endpanel()
        dlg:newrow()
        render_patch(dlg, draft)
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = status_label,
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                text = "Brakujace sloty: " .. missing_label,
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
