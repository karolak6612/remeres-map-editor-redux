local Common = dofile("common.lua")
local SavePanel = dofile("save_panel.lua")

local M = {}

function M.render(dlg, state, actions)
    local active_raw_id = state.selected_raw_id or 0
    local raw_info = Common.safe_item_info(active_raw_id)
    local bucket = state.draft and state.draft:getBucket(state.selected_alignment) or Common.make_bucket(Common.alignment_order[1].token)
    local selected_wall_item = bucket.wallItems[tonumber(state.selected_wall_item_index or 0) or 0]
    local selected_door = bucket.doorItems[tonumber(state.selected_door_index or 0) or 0]

    dlg:box({
        label = "3-5. Wlasciwosci",
        orient = "vertical",
        width = 390,
        min_width = 360,
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
                text = "Aktualny RAW",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:image({
                id = "wall_active_raw_preview",
                image = Common.safe_image(active_raw_id),
                width = 48,
                height = 48,
                smooth = false,
            })
            dlg:newrow()
            dlg:label({
                id = "wall_active_raw_title",
                text = raw_info and (raw_info.name or ("Item " .. tostring(active_raw_id))) or "Najpierw wybierz RAW tile",
                fgcolor = Common.palette.text,
            })
            dlg:newrow()
            dlg:label({
                id = "wall_active_raw_meta",
                text = raw_info and string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0) or "Brak aktywnego RAW.",
                fgcolor = Common.palette.subtle,
                font_size = 8,
            })
        dlg:endpanel()
        dlg:newrow()
        dlg:panel({
            bgcolor = Common.palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:combobox({
                id = "wall_alignment_combo",
                label = "Alignment",
                options = (function()
                    local options = {}
                    for _, alignment in ipairs(Common.alignment_order) do
                        table.insert(options, alignment.label)
                    end
                    return options
                end)(),
                option = Common.alignment_label(state.selected_alignment),
                onchange = function(d)
                    actions.set_alignment_title(d.data.wall_alignment_combo or "")
                end,
            })
            dlg:newrow()
            dlg:input({
                id = "wall_name_input",
                label = "Nazwa",
                text = state.draft and state.draft.name or "",
                expand = true,
                onchange = function(d)
                    actions.set_name(d.data.wall_name_input or "")
                end,
            })
            dlg:newrow()
            dlg:input({
                id = "wall_lookid_input",
                label = "lookid",
                text = tostring(state.draft and state.draft.lookId or 0),
                expand = true,
                onchange = function(d)
                    actions.set_lookid(d.data.wall_lookid_input or "")
                end,
            })
            dlg:newrow()
            dlg:input({
                id = "wall_server_lookid_input",
                label = "server_lookid",
                text = tostring(state.draft and state.draft.serverLookId or 0),
                expand = true,
                onchange = function(d)
                    actions.set_server_lookid(d.data.wall_server_lookid_input or "")
                end,
            })
            dlg:newrow()
            dlg:input({
                id = "wall_friends_input",
                label = "Friends (comma)",
                text = table.concat(state.draft and state.draft.friends or {}, ", "),
                expand = true,
                onchange = function(d)
                    actions.set_friends_text(d.data.wall_friends_input or "")
                end,
            })
            dlg:newrow()
            dlg:input({
                id = "wall_redirect_input",
                label = "Redirect",
                text = state.draft and state.draft.redirectName or "",
                expand = true,
                onchange = function(d)
                    actions.set_redirect_name(d.data.wall_redirect_input or "")
                end,
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
                text = "Wall Items w Buckecie",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:list({
                id = "wall_bucket_item_list",
                height = 130,
                expand = true,
                icon_size = 24,
                item_height = 30,
                items = Common.bucket_wall_list_items(bucket),
                selection = tonumber(state.selected_wall_item_index or 0) or 0,
                onchange = function(d)
                    actions.set_wall_item_selection(d.data.wall_bucket_item_list or 0)
                end,
            })
            dlg:newrow()
            dlg:input({
                id = "wall_item_chance_input",
                label = "Chance",
                text = selected_wall_item and tostring(selected_wall_item.chance or 0) or tostring(state.wall_item_chance or 1),
                expand = true,
                onchange = function(d)
                    actions.set_wall_item_chance_text(d.data.wall_item_chance_input or "")
                end,
            })
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:button({
                    text = "Dodaj RAW jako Wall",
                    bgcolor = Common.palette.accent,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.add_wall_item_from_active_raw()
                    end,
                })
                dlg:button({
                    text = "Aktualizuj Chance",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.apply_selected_wall_item_chance()
                    end,
                })
                dlg:button({
                    text = "Usun",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.remove_selected_wall_item()
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
                text = "Door / Window Entries",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:list({
                id = "wall_bucket_door_list",
                height = 130,
                expand = true,
                icon_size = 24,
                item_height = 30,
                items = Common.bucket_door_list_items(bucket),
                selection = tonumber(state.selected_door_index or 0) or 0,
                onchange = function(d)
                    actions.set_door_selection(d.data.wall_bucket_door_list or 0)
                end,
            })
            dlg:newrow()
            dlg:combobox({
                id = "wall_door_type_combo",
                label = "Door type",
                options = Common.door_type_options,
                option = state.door_form.type or "normal",
                onchange = function(d)
                    actions.set_door_form_type(d.data.wall_door_type_combo or "normal")
                end,
            })
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:check({
                    id = "wall_door_open_check",
                    text = "Open",
                    selected = state.door_form.open and true or false,
                    onclick = function(d)
                        actions.set_door_form_open(d.data.wall_door_open_check)
                    end,
                })
                dlg:check({
                    id = "wall_door_locked_check",
                    text = "Locked",
                    selected = state.door_form.locked and true or false,
                    onclick = function(d)
                        actions.set_door_form_locked(d.data.wall_door_locked_check)
                    end,
                })
                dlg:check({
                    id = "wall_door_hate_check",
                    text = "Hate",
                    selected = state.door_form.hate and true or false,
                    onclick = function(d)
                        actions.set_door_form_hate(d.data.wall_door_hate_check)
                    end,
                })
            dlg:endbox()
            dlg:newrow()
            dlg:box({
                orient = "horizontal",
                expand = false,
            })
                dlg:button({
                    text = "Dodaj RAW jako Door",
                    bgcolor = Common.palette.accent_soft,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.add_door_from_active_raw()
                    end,
                })
                dlg:button({
                    text = "Aktualizuj Door",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.apply_selected_door()
                    end,
                })
                dlg:button({
                    text = "Usun Door",
                    bgcolor = Common.palette.panel,
                    fgcolor = Common.palette.text,
                    onclick = function()
                        actions.remove_selected_door()
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
                text = "Ostatnio Uzyte RAW",
                fgcolor = Common.palette.text,
                font_weight = "bold",
            })
            dlg:newrow()
            dlg:list({
                id = "wall_recent_raw_list",
                height = 110,
                expand = true,
                icon_size = 24,
                item_height = 30,
                items = Common.recent_raw_items(state.recent_raw_ids),
            })
        dlg:endpanel()
        dlg:newrow()
        SavePanel.render(dlg, state, actions)
    dlg:endbox()
end

return M
