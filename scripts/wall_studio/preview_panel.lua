local Common = dofile("common.lua")

local M = {}

function M.render(dlg, state)
    local current_bucket = state.draft and state.draft:getBucket(state.selected_alignment) or nil
    local preview_id = current_bucket and Common.representative_item_id(current_bucket) or 0

    dlg:box({
        label = "2-5. Preview Walla",
        orient = "vertical",
        expand = true,
        min_width = 430,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            id = "wall_preview_help",
            text = "Wybierz alignment i buduj bucket itemow oraz door entries.",
            fgcolor = Common.palette.text,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = true,
        })
            dlg:panel({
                bgcolor = Common.palette.panel_alt,
                padding = 10,
                margin = 4,
                expand = false,
                min_width = 150,
            })
                dlg:label({
                    text = "Wybrany Alignment",
                    fgcolor = Common.palette.text,
                    font_weight = "bold",
                })
                dlg:newrow()
                dlg:image({
                    id = "selected_alignment_preview",
                    image = Common.safe_image(preview_id),
                    width = 80,
                    height = 80,
                    smooth = false,
                })
                dlg:newrow()
                dlg:label({
                    id = "selected_alignment_title",
                    text = Common.alignment_label(state.selected_alignment),
                    fgcolor = Common.palette.text,
                })
                dlg:newrow()
                dlg:label({
                    id = "selected_alignment_meta",
                    text = string.format("Wall items: %d  |  Doors: %d", current_bucket and #(current_bucket.wallItems or {}) or 0, current_bucket and #(current_bucket.doorItems or {}) or 0),
                    fgcolor = Common.palette.muted,
                    font_size = 8,
                })
            dlg:endpanel()
            dlg:panel({
                bgcolor = Common.palette.panel_alt,
                padding = 10,
                margin = 4,
                expand = true,
            })
                dlg:label({
                    text = "Pokrycie Alignmentow",
                    fgcolor = Common.palette.text,
                    font_weight = "bold",
                })
                dlg:newrow()
                dlg:grid({
                    id = "wall_alignment_grid",
                    width = 300,
                    height = 380,
                    cell_width = 96,
                    cell_height = 96,
                    item_width = 56,
                    item_height = 56,
                    icon_width = 56,
                    icon_height = 56,
                    show_text = true,
                    items = Common.alignment_grid_items(state.draft),
                })
                dlg:newrow()
                dlg:label({
                    id = "wall_preview_status",
                    text = "Grid pokazuje representative item dla kazdego alignmentu.",
                    fgcolor = Common.palette.muted,
                })
            dlg:endpanel()
        dlg:endbox()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 10,
            margin = 4,
            expand = true,
        })
            dlg:label({
                text = "Relacje",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "wall_friend_summary",
                text = Common.friend_summary(state.draft),
                fgcolor = Common.palette.muted,
            })
            dlg:newrow()
            dlg:label({
                id = "wall_redirect_summary",
                text = state.draft and state.draft.redirectName ~= "" and ("Redirect: " .. state.draft.redirectName) or "Redirect: brak",
                fgcolor = Common.palette.subtle,
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
