local Common = dofile("common.lua")

local M = {}

function M.render(dlg, state)
    local main_item_id = state.draft and state.draft.mainItem and state.draft.mainItem.itemId or 0

    dlg:box({
        label = "2-5. Preview Grounda",
        orient = "vertical",
        expand = true,
        min_width = 420,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            id = "ground_preview_help",
            text = "Krok 2: wybierz RAW. Krok 3: ustaw glowny tile i warianty. Krok 4: sprawdz preview.",
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
                min_width = 130,
            })
                dlg:label({
                    text = "Main Ground",
                    fgcolor = Common.palette.text,
                    font_weight = "bold",
                })
                dlg:newrow()
                dlg:image({
                    id = "ground_main_preview",
                    image = Common.safe_image(main_item_id),
                    width = 80,
                    height = 80,
                    smooth = false,
                })
                dlg:newrow()
                dlg:label({
                    id = "ground_main_label",
                    text = main_item_id > 0 and ("Item ID " .. tostring(main_item_id)) or "Brak glownego tile",
                    fgcolor = Common.palette.muted,
                })
            dlg:endpanel()

            dlg:panel({
                bgcolor = Common.palette.panel_alt,
                padding = 10,
                margin = 4,
                expand = true,
            })
                dlg:label({
                    text = "Podglad Powierzchni",
                    fgcolor = Common.palette.text,
                    font_weight = "bold",
                })
                dlg:newrow()
                dlg:grid({
                    id = "ground_surface_preview",
                    width = 280,
                    height = 280,
                    cell_width = 54,
                    cell_height = 54,
                    item_width = 48,
                    item_height = 48,
                    icon_width = 48,
                    icon_height = 48,
                    show_text = false,
                    items = Common.surface_preview_items(state.draft),
                })
                dlg:newrow()
                dlg:label({
                    id = "ground_preview_status",
                    text = "Podglad powierzchni pokazuje glowny tile i warianty.",
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
                text = "Warianty Preview",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                text = #((state.draft and state.draft.variants) or {}) > 0 and "To sa tylko dodatkowe warianty. Main Ground pozostaje osobny." or "Brak wariantow. Dodaj je z prawej kolumny, jesli chcesz losowosc.",
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
            dlg:newrow()
            dlg:grid({
                id = "ground_variant_preview",
                height = 170,
                expand = true,
                cell_width = 96,
                cell_height = 120,
                item_width = 72,
                item_height = 72,
                icon_width = 72,
                icon_height = 72,
                show_text = true,
                items = Common.variant_preview_items(state.draft),
            })
        dlg:endpanel()
    dlg:endbox()
end

return M
