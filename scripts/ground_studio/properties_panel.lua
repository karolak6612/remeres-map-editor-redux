local Common = dofile("common.lua")

local M = {}

function M.render(dlg, state, actions)
    local active_raw_id = state.selected_raw_id or 0
    local raw_info = Common.safe_item_info(active_raw_id)
    local main_item_id = state.draft and state.draft.mainGroundItem and state.draft.mainGroundItem.itemId or 0
    local selected_variant = state.draft and state.draft.variants and state.draft.variants[tonumber(state.variant_selection or 0) or 0] or nil

    dlg:box({
        label = "3-4. Wlasciwosci",
        orient = "vertical",
        width = 360,
        min_width = 340,
        expand = true,
        fgcolor = Common.palette.muted,
    })
        dlg:label({
            text = "Aktualny RAW z glownej palety sluzy jako zrodlo main tile i wariantow.",
            fgcolor = Common.palette.muted,
        })
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Aktualny RAW",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:image({
                id = "active_raw_preview",
                image = Common.safe_image(active_raw_id),
                width = 48,
                height = 48,
                smooth = false,
            })
            dlg:newrow()
            dlg:label({
                id = "active_raw_title",
                text = raw_info and (raw_info.name or ("Item " .. tostring(active_raw_id))) or "Najpierw wybierz RAW tile",
                fgcolor = Common.palette.text,
            })
            dlg:newrow()
            dlg:label({
                id = "active_raw_meta",
                text = raw_info and string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0) or "Brak aktywnego RAW.",
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
            dlg:newrow()
            dlg:label({
                id = "active_raw_hint",
                text = raw_info and "Ten RAW mozesz ustawic jako Main Ground albo dodac jako wariant." or "Najpierw wybierz RAW tile w glownej palecie RME.",
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
            dlg:newrow()
            dlg:input({
                id = "placement_chance",
                label = "Set chance",
                text = tostring(state.placement_chance or (state.draft and state.draft.mainChance) or 1),
                expand = true,
                onchange = function(d)
                    actions.set_placement_chance(d.data.placement_chance or "")
                end,
            })
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:button({
                    text = "Ustaw jako Main Ground",
                    bgcolor = Common.palette.accent,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.use_active_raw_as_main()
                    end,
                })
                dlg:button({
                    text = "Dodaj jako Wariant",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.add_variant_from_active_raw()
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
        dlg:endpanel()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Main Ground",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:image({
                id = "main_ground_preview_side",
                image = Common.safe_image(main_item_id),
                width = 56,
                height = 56,
                smooth = false,
            })
            dlg:newrow()
            dlg:label({
                id = "main_ground_title",
                text = main_item_id > 0 and (state.draft.mainGroundItem.name or ("Item " .. tostring(main_item_id))) or "Brak Main Ground",
                fgcolor = Common.palette.text,
            })
            dlg:newrow()
            dlg:label({
                id = "main_ground_meta",
                text = main_item_id > 0 and string.format("Item ID %d  |  chance %d", main_item_id, tonumber(state.draft.mainChance or 0) or 0) or "Wybierz RAW i kliknij 'Ustaw jako Main Ground'.",
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
        dlg:endpanel()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = true,
        })
            dlg:label({
                text = "Warianty Grounda",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:label({
                id = "variant_hint_label",
                text = #state.draft.variants > 0 and "Warianty sa oddzielone od Main Ground. Wybierz wariant z listy, aby edytowac chance." or "Brak wariantow. Wybierz RAW i kliknij 'Dodaj Wariant'.",
                fgcolor = Common.palette.muted,
                font_size = 8,
            })
            dlg:newrow()
            dlg:list({
                id = "ground_variant_list",
                label = "Lista wariantow",
                height = 180,
                expand = true,
                icon_size = 24,
                item_height = 34,
                items = Common.variant_list_items(state.draft),
                selection = tonumber(state.variant_selection or 0) or 0,
                onchange = function(d)
                    actions.set_variant_selection(d.data.ground_variant_list or 0)
                end,
            })
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:image({
                    id = "selected_variant_preview",
                    image = Common.safe_image(selected_variant and selected_variant.itemId or 0),
                    width = 40,
                    height = 40,
                    smooth = false,
                })
                dlg:box({
                    orient = "vertical",
                    expand = true,
                })
                    dlg:label({
                        id = "selected_variant_title",
                        text = selected_variant and (selected_variant.name or ("Item " .. tostring(selected_variant.itemId or 0))) or "Nie wybrano wariantu",
                        fgcolor = Common.palette.text,
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "selected_variant_meta",
                        text = selected_variant and string.format("Item ID %d  |  chance %d", tonumber(selected_variant.itemId or 0) or 0, tonumber(selected_variant.chance or 0) or 0) or "Wybierz wariant z listy, aby edytowac chance.",
                        fgcolor = Common.palette.subtle,
                        font_size = 8,
                    })
                dlg:endbox()
            dlg:endbox()
            dlg:newrow()
            dlg:input({
                id = "ground_variant_chance",
                label = "Chance wariantu",
                text = state.selected_variant_chance or "",
                expand = true,
                onchange = function(d)
                    actions.set_variant_chance(d.data.ground_variant_chance or "")
                end,
            })
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:button({
                    text = "Dodaj Wariant",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.add_variant_from_active_raw()
                    end,
                })
                dlg:button({
                    text = "Usun Wybrany Wariant",
                    bgcolor = Common.palette.panel_alt,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.remove_selected_variant()
                    end,
                })
            dlg:endbox()
        dlg:endpanel()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Tile w Tym Groundzie",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:list({
                id = "ground_set_list",
                height = 130,
                expand = true,
                icon_size = 28,
                item_height = 34,
                items = Common.ground_set_items(state.draft),
            })
        dlg:endpanel()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "Ostatnio Uzyte",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:list({
                id = "recent_raw_list",
                height = 120,
                expand = true,
                icon_size = 24,
                item_height = 30,
                items = Common.recent_raw_items(state.recent_raw_ids),
            })
        dlg:endpanel()
        dlg:newrow()
        dlg:input({
            id = "ground_name_input",
            label = "Nazwa",
            text = state.draft and state.draft.name or "",
            expand = true,
            onchange = function(d)
                actions.set_name(d.data.ground_name_input or "")
            end,
        })
        dlg:input({
            id = "ground_lookid_input",
            label = "lookid",
            text = tostring(state.draft and state.draft.lookId or 0),
            expand = true,
            onchange = function(d)
                actions.set_look_id(d.data.ground_lookid_input or "")
            end,
        })
        dlg:newrow()
        dlg:input({
            id = "ground_server_lookid_input",
            label = "server_lookid",
            text = tostring(state.draft and state.draft.serverLookId or 0),
            expand = true,
            onchange = function(d)
                actions.set_server_lookid(d.data.ground_server_lookid_input or "")
            end,
        })
        dlg:newrow()
        dlg:input({
            id = "ground_z_order_input",
            label = "z-order",
            text = tostring(state.draft and state.draft.zOrder or 0),
            expand = true,
            onchange = function(d)
                actions.set_z_order(d.data.ground_z_order_input or "")
            end,
        })
        dlg:newrow()
        dlg:box({
            orient = "horizontal",
            expand = false,
        })
            dlg:button({
                id = "toggle_randomize_button",
                text = state.draft and (state.draft.randomize and "Randomize: ON" or "Randomize: OFF") or "Randomize: ON",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.toggle_randomize()
                end,
            })
            dlg:button({
                id = "toggle_solo_optional_button",
                text = state.draft and (state.draft.soloOptional and "Solo Optional: ON" or "Solo Optional: OFF") or "Solo Optional: OFF",
                bgcolor = Common.palette.panel_alt,
                fgcolor = Common.palette.text,
                onclick = function()
                    actions.toggle_solo_optional()
                end,
            })
        dlg:endbox()
    dlg:endbox()
end

return M
