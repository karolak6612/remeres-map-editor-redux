local Common = dofile("common.lua")
local DraftAdapter = dofile("draft_adapter.lua")
local TestPreviewPanel = dofile("test_preview_panel.lua")

local M = {}

local function render_slot_matrix(dlg, state, actions)
    dlg:grid({
        id = "slot_preview_grid",
        width = 320,
        height = 320,
        expand = false,
        icon_size = 42,
        cell_size = 62,
        show_text = true,
        label_wrap = false,
        items = DraftAdapter.slot_grid_items(state.draft, state.selected_raw_id, { showImages = true }),
        selection = tonumber(state.highlighted_slot_index or 0) or 0,
        onleftclick = function(_, info)
            local index = tonumber(info and info.index or 0) or 0
            if actions and actions.assign_layout_cell then
                actions.assign_layout_cell(index)
            end
        end,
        onrightclick = function(_, info)
            local index = tonumber(info and info.index or 0) or 0
            if actions and actions.clear_layout_cell then
                actions.clear_layout_cell(index)
            end
        end,
    })
end

function M.render(dlg, state, actions)
    local preview_state = DraftAdapter.preview_state(state.draft)
    local summary_text = preview_state.hasAssignments
        and string.format(
            "Ustawione sloty: %d. Puste sloty: %s",
            preview_state.assignedCount,
            #preview_state.emptySlots > 0 and table.concat(preview_state.emptySlots, ", ") or "none"
        )
        or "Kliknij slot w podgladzie, aby przypisac tile."

    dlg:box({
        label = "3. Kliknij Sloty",
        orient = "vertical",
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            id = "preview_help_label",
            text = "Najpierw wybierz RAW tile. LPM przypisuje, PPM czysci slot.",
            fgcolor = Common.palette.muted,
        })
        dlg:newrow()
        render_slot_matrix(dlg, state, actions)
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Status slotow",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "selected_slot_status_label",
                text = "Nie wybrano zadnego slotu.",
                fgcolor = Common.palette.muted,
            })
            dlg:newrow()
            dlg:label({
                id = "slot_summary_label",
                text = summary_text,
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:button({
                    text = "Wyczysc Slot",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        if actions and actions.clear_selected_slot then
                            actions.clear_selected_slot()
                        end
                    end,
                })
                dlg:button({
                    text = "Wyczysc Caly Draft",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        if actions and actions.clear_draft then
                            actions.clear_draft()
                        end
                    end,
                })
            dlg:endbox()
        dlg:endpanel()
        dlg:newrow()
        TestPreviewPanel.render(dlg, state.draft)
    dlg:endbox()
end

return M
