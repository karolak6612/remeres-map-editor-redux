local Common = dofile("common.lua")

local M = {}

function M.render(dlg, state, actions)
    dlg:box({
        label = "6. Zapis",
        orient = "vertical",
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = true,
        })
            dlg:label({
                id = "wall_save_mode_label",
                text = state.draft and (state.draft.isNew and "Tryb: nowy wall brush" or "Tryb: edycja istniejacego wall brusha"),
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "wall_save_dirty_label",
                text = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie",
                fgcolor = Common.palette.subtle,
            })
            dlg:newrow()
            dlg:label({
                id = "wall_save_target_label",
                text = "Target: " .. tostring(state.target_path or ""),
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
            dlg:newrow()
            dlg:label({
                id = "wall_save_hint_label",
                text = "Podaj nazwe i co najmniej jeden wall item, aby zapisac wall brush.",
                fgcolor = Common.palette.muted,
            })
        dlg:endpanel()
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "Zmien XML",
                bgcolor = Common.palette.panel,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.choose_target_path()
                end,
            })
            dlg:button({
                text = "Domyslny XML",
                bgcolor = Common.palette.panel,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.use_default_target_path()
                end,
            })
        dlg:endbox()
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                text = "Save as New",
                bgcolor = Common.palette.success,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.save_as_new()
                end,
            })
            dlg:button({
                text = "Overwrite Existing",
                bgcolor = Common.palette.accent,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.overwrite_existing()
                end,
            })
        dlg:endbox()
        dlg:newrow()
        dlg:label({
            id = "wall_save_intent_new",
            text = state.draft and actions and actions.get_save_intent and actions.get_save_intent("new") or "",
            fgcolor = Common.palette.muted,
            font_size = 8,
        })
        dlg:newrow()
        dlg:label({
            id = "wall_save_intent_overwrite",
            text = state.draft and actions and actions.get_save_intent and actions.get_save_intent("overwrite") or "",
            fgcolor = Common.palette.muted,
            font_size = 8,
        })
    dlg:endbox()
end

return M
