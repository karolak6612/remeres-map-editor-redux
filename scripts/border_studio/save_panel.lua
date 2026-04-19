local Common = dofile("common.lua")
local Model = dofile("model.lua")

local M = {}

function M.render(dlg, state, actions)
    local is_existing = state.draft and not state.draft.isNew
    local dirty_label = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie"
    local mode_label = is_existing
        and string.format("Tryb: edycja istniejacego wpisu #%d", state.draft.borderId or 0)
        or "Tryb: nowy border"
    local intent_new = Model.save_intent(state.repository, state.draft, state.save_border_id, "new")
    local intent_overwrite = Model.save_intent(state.repository, state.draft, state.save_border_id, "overwrite")
    local target_label = state.target_path ~= "" and state.target_path or "No borders.xml target resolved."

    dlg:box({
        orient = "vertical",
        label = "5. Zapisz Border",
        expand = false,
        fgcolor = Common.palette.muted,
    })
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                id = "save_mode_label",
                text = mode_label,
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "save_dirty_label",
                text = dirty_label,
                fgcolor = state.dirty and Common.palette.accent_soft or Common.palette.subtle,
                font_size = 8,
            })
            dlg:newrow()
            dlg:label({
                id = "save_target_label",
                text = "Target: " .. target_label,
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
        dlg:endpanel()

        dlg:number({
            id = "save_border_id",
            label = "Border ID",
            value = tonumber(state.save_border_id or 0) or 0,
            min = 0,
            max = 65535,
            onchange = function(d)
                local next_id = d.data.save_border_id or 0
                if tonumber(next_id or 0) ~= tonumber(state.save_border_id or 0) then
                    actions.set_save_border_id(next_id)
                end
            end,
        })
        dlg:newrow()
        dlg:input({
            id = "save_border_name",
            label = "Robocza nazwa",
            text = state.save_name or "",
            expand = true,
            onchange = function(d)
                local next_name = d.data.save_border_name or ""
                if tostring(next_name or "") ~= tostring(state.save_name or "") then
                    actions.set_save_name(next_name)
                end
            end,
        })
        dlg:newrow()
        dlg:label({
            id = "save_hint_label",
            text = "Podaj ID, aby zapisac border.",
            fgcolor = Common.palette.muted,
            font_weight = "bold",
        })
        dlg:newrow()
        dlg:label({
            id = "save_intent_new",
            text = intent_new,
            fgcolor = Common.palette.subtle,
            font_size = 8,
        })
        dlg:newrow()
        dlg:label({
            id = "save_intent_overwrite",
            text = intent_overwrite,
            fgcolor = Common.palette.subtle,
            font_size = 8,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "Zapisz Jako Nowy",
                bgcolor = Common.palette.success,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.save_as_new()
                end,
            })
            dlg:button({
                text = "Nadpisz Istniejacy",
                bgcolor = Common.palette.accent,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.overwrite_existing()
                end,
            })
        dlg:endbox()
    dlg:endbox()
end

return M
