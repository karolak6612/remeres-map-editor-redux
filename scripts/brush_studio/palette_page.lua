local Paths = dofile("module_paths.lua")
local GroundCommon = Paths.load("ground_studio/common.lua")
local PaletteDraft = dofile("palette_draft.lua")
local PaletteRepository = dofile("palette_repository.lua")
local XmlTarget = dofile("xml_target_helper.lua")

local PalettePage = {}
local SETTINGS_STORE = app.storage("palette_settings.json")

local palette = {
    header = "#233246",
    panel = "#243447",
    panel_alt = "#2C3E50",
    sidebar = "#26282d",
    accent = "#2563EB",
    success = "#2F855A",
    warning = "#D97706",
    text = "#F8FAFC",
    muted = "#CBD5E1",
    subtle = "#94A3B8",
}

local PALETTE_SECTION_OPTIONS = {
    { value = "terrain", title = "Terrain Palette" },
    { value = "doodad", title = "Doodad Palette" },
    { value = "items", title = "Item Palette" },
    { value = "doodad_and_raw", title = "Creature Palette" },
    { value = "items_and_raw", title = "RAW Palette" },
}

local function create_page(options)
    options = options or {}
    local session = options.session or {}
    local settings = SETTINGS_STORE:load() or {}
    local dlg

    local state = {
        target_path = PaletteRepository.resolve_target_path(settings.target_path or ""),
        repository = nil,
        brush_filter = "",
        brush_items = {},
        brush_names = {},
        brush_page = 1,
        brush_page_size = 12,
        preview_page = 1,
        preview_page_size = 12,
        tileset_items = {},
        tileset_names = {},
        draft = PaletteDraft.new({}),
        status_message = "Najpierw wybierz gotowy brush.",
        dirty = false,
    }

    local actions = {}

    local function persist_settings()
        SETTINGS_STORE:save({
            target_path = state.target_path or "",
        })
    end

    local function sync_session_out()
        session.pageDirty = session.pageDirty or {}
        session.pageDirty.palette = state.dirty
        if state.draft.selectedBrush then
            session.lastPaletteBrush = state.draft.selectedBrush.name
        end
    end

    local function request_host_render()
        sync_session_out()
        if options.onRequestRender then
            options.onRequestRender()
            return true
        end
        return false
    end

    local function confirm_discard()
        if not state.dirty then
            return true
        end
        local choice = app.alert({
            title = "Discard changes?",
            text = "The current palette draft has unsaved changes. Continue and discard them?",
            buttons = { "Yes", "No" },
        })
        return choice == 1
    end

    local function refresh_repository()
        state.repository = PaletteRepository.read(state.target_path)
    end

    local function page_count(total, page_size)
        total = tonumber(total or 0) or 0
        page_size = tonumber(page_size or 1) or 1
        if total <= 0 then
            return 1
        end
        return math.max(1, math.ceil(total / page_size))
    end

    local function clamp_page(page, total, page_size)
        local max_page = page_count(total, page_size)
        page = tonumber(page or 1) or 1
        if page < 1 then
            return 1
        end
        if page > max_page then
            return max_page
        end
        return page
    end

    local function page_bounds(page, total, page_size)
        page = clamp_page(page, total, page_size)
        local first = ((page - 1) * page_size) + 1
        local last = math.min(total, first + page_size - 1)
        return first, last, page_count(total, page_size)
    end

    local function palette_title_from_section(section_name)
        section_name = tostring(section_name or "")
        for _, option in ipairs(PALETTE_SECTION_OPTIONS) do
            if option.value == section_name then
                return option.title
            end
        end
        return "Terrain Palette"
    end

    local function palette_section_from_title(title)
        title = tostring(title or "")
        for _, option in ipairs(PALETTE_SECTION_OPTIONS) do
            if option.title == title then
                return option.value
            end
        end
        return "terrain"
    end

    local function palette_option_titles()
        local titles = {}
        for _, option in ipairs(PALETTE_SECTION_OPTIONS) do
            table.insert(titles, option.title)
        end
        return titles
    end

    local function tileset_options_for_section(section_name)
        local options = { "-- wybierz tileset --" }
        for _, tileset in ipairs(state.repository and state.repository.ordered_tilesets or {}) do
            if tileset.sections_by_name and tileset.sections_by_name[section_name] then
                table.insert(options, tileset.name)
            end
        end
        return options
    end

    local function sync_tilesets()
        state.tileset_items, state.tileset_names = PaletteRepository.tileset_items(state.repository)
    end

    local function sync_brushes()
        state.brush_items, state.brush_names = PaletteRepository.brush_items(state.repository, state.brush_filter)
        state.brush_page = clamp_page(state.brush_page, #state.brush_names, state.brush_page_size)
    end

    local function visible_brush_entry(slot)
        local first, last = page_bounds(state.brush_page, #state.brush_names, state.brush_page_size)
        local index = first + ((tonumber(slot or 1) or 1) - 1)
        if index < first or index > last then
            return nil, nil, nil
        end
        return index, state.brush_names[index], state.brush_items[index]
    end

    local function helper_text()
        if not state.draft.selectedBrush then
            return "1. Wybierz gotowy brush. 2. Wybierz tileset i sekcje. 3. Sprawdz preview. 4. Zapisz."
        end
        if state.draft.targetTileset == "" or state.draft.targetCategory == "" then
            return "Wybierz miejsce w palecie, aby opublikowac brush."
        end
        return "Palette publikuje gotowy brush do tilesets.xml bez pracy na surowym XML."
    end

    local function save_hint_text()
        local validation = state.draft:getValidation()
        if not validation.isValid then
            return validation.errors[1] or "Uzupelnij draft."
        end
        if state.draft.displayName == "" and state.draft.selectedBrush then
            return "Paleta uzyje fallbacku z nazwy brusha, bo tilesets.xml nie ma osobnego labela."
        end
        return "Zapis utworzy nowy wpis albo zaktualizuje istniejacy wpis palety."
    end

    local function effective_display_name()
        return state.draft:effectiveDisplayName()
    end

    local function preview_entries()
        return PaletteRepository.preview_entries(state.repository, state.draft)
    end

    local function soft_refresh()
        sync_session_out()
        if request_host_render() then
            return
        end
    end

    local function refresh_brush_picker_local()
        if not dlg or not dlg.modify then
            return false
        end

        local selected_brush = state.draft.selectedBrush
        local _, _, brush_pages = page_bounds(state.brush_page, #state.brush_names, state.brush_page_size)

        dlg:modify({
            id = "palette_brush_filter",
            text = state.brush_filter or "",
        })
        dlg:modify({
            id = "palette_brush_page_label",
            text = string.format("Strona %d / %d", state.brush_page, brush_pages),
        })
        dlg:modify({
            id = "palette_brush_prev",
            bgcolor = state.brush_page > 1 and palette.panel or palette.sidebar,
        })
        dlg:modify({
            id = "palette_brush_next",
            bgcolor = state.brush_page < brush_pages and palette.panel or palette.sidebar,
        })

        for slot = 1, tonumber(state.brush_page_size or 12) or 12 do
            local _, name, item = visible_brush_entry(slot)
            local is_selected = name and selected_brush and selected_brush.name == name or false
            dlg:modify({
                id = "palette_brush_icon_" .. slot,
                image = item and item.image or GroundCommon.safe_image(0),
            })
            dlg:modify({
                id = "palette_brush_label_" .. slot,
                text = item and item.text or " ",
                fgcolor = name and palette.text or palette.subtle,
            })
            dlg:modify({
                id = "palette_brush_open_" .. slot,
                text = name and "Open" or "",
                bgcolor = name and (is_selected and palette.accent or palette.panel) or palette.sidebar,
                fgcolor = name and palette.text or palette.subtle,
            })
        end

        dlg:layout()
        dlg:repaint()
        return true
    end

    local function refresh_preview_local()
        if not dlg or not dlg.modify then
            return false
        end

        local selected_brush = state.draft.selectedBrush
        local preview = preview_entries()
        state.preview_page = clamp_page(state.preview_page, #preview, state.preview_page_size)
        local preview_first, preview_last, preview_pages = page_bounds(state.preview_page, #preview, state.preview_page_size)

        dlg:modify({
            id = "palette_preview_prev",
            bgcolor = state.preview_page > 1 and palette.panel or palette.sidebar,
        })
        dlg:modify({
            id = "palette_preview_page_label",
            text = string.format(
                "Pozycje %d-%d / %d  |  Strona %d / %d",
                #preview > 0 and preview_first or 0,
                #preview > 0 and preview_last or 0,
                #preview,
                state.preview_page,
                preview_pages
            ),
        })
        dlg:modify({
            id = "palette_preview_next",
            bgcolor = state.preview_page < preview_pages and palette.panel or palette.sidebar,
        })

        for slot = 1, tonumber(state.preview_page_size or 12) or 12 do
            local index = preview_first + slot - 1
            local entry = index <= preview_last and preview[index] or nil
            dlg:modify({
                id = "palette_preview_slot_" .. slot,
                bgcolor = entry and (entry.isInserted and palette.accent or (entry.isSelected and palette.success or palette.sidebar)) or palette.sidebar,
            })
            dlg:modify({
                id = "palette_preview_slot_label_" .. slot,
                text = entry and string.format("%d. %s", index, entry.name) or " ",
                fgcolor = entry and palette.text or palette.muted,
            })
        end

        dlg:modify({
            id = "palette_preview_empty_hint",
            text = #preview == 0 and "Wybierz tileset i sekcje, aby zobaczyc podglad publikacji." or " ",
        })

        dlg:layout()
        dlg:repaint()
        return true
    end

    local function use_brush_by_name(name)
        local ref = PaletteRepository.make_brush_reference(state.repository, name)
        if not ref then
            state.status_message = "Wybrany brush nie jest dostepny."
            soft_refresh()
            return
        end
        state.draft:setBrush(ref)
        state.preview_page = 1
        state.dirty = true
        state.status_message = string.format("Wybrano brush '%s'.", ref.name)
        sync_brushes()
        soft_refresh()
    end

    local function finalize_save(mode)
        local ok, result = PaletteRepository.save_draft(state.repository, state.draft, mode, state.target_path)
        if not ok then
            app.alert({
                title = "Save failed",
                text = result,
                buttons = { "OK" },
            })
            return
        end

        state.dirty = false
        session.lastSavedPaletteEntry = state.draft.selectedBrush and state.draft.selectedBrush.name or ""
        state.status_message = result.message
        refresh_repository()
        sync_brushes()
        sync_tilesets()
        soft_refresh()
        app.alert({
            title = "Palette updated",
            text = result.message,
            buttons = { "OK" },
        })
    end

    function actions.set_brush_filter_text(text)
        state.brush_filter = tostring(text or "")
    end

    function actions.set_brush_filter(text)
        text = tostring(text or "")
        if text == tostring(state.brush_filter or "") then
            return
        end
        state.brush_filter = text
        state.brush_page = 1
        sync_brushes()
        soft_refresh()
    end

    function actions.clear_brush_filter()
        state.brush_filter = ""
        state.brush_page = 1
        sync_brushes()
        soft_refresh()
    end

    function actions.prev_brush_page()
        state.brush_page = math.max(1, (tonumber(state.brush_page or 1) or 1) - 1)
        if not refresh_brush_picker_local() then
            soft_refresh()
        end
    end

    function actions.next_brush_page()
        state.brush_page = clamp_page((tonumber(state.brush_page or 1) or 1) + 1, #state.brush_names, state.brush_page_size)
        if not refresh_brush_picker_local() then
            soft_refresh()
        end
    end

    function actions.handle_brush_change(index)
        local name = state.brush_names[tonumber(index or 0) or 0]
        if name then
            use_brush_by_name(name)
        end
    end

    function actions.open_brush_visible_slot(slot)
        local _, name = visible_brush_entry(slot)
        if not name then
            return
        end
        use_brush_by_name(name)
    end

    function actions.use_last_saved_brush()
        local name = GroundCommon.trim(session.lastSavedComposedBrush or "")
        if name == "" then
            state.status_message = "Brak ostatnio zapisanego composed brusha."
            soft_refresh()
            return
        end
        use_brush_by_name(name)
    end

    function actions.use_last_saved_ground()
        local name = GroundCommon.trim(session.lastSavedGround or "")
        if name == "" then
            state.status_message = "Brak ostatnio zapisanego grounda."
            soft_refresh()
            return
        end
        use_brush_by_name(name)
    end

    function actions.set_tileset(index)
        local name = state.tileset_names[tonumber(index or 0) or 0]
        if not name then
            return
        end
        state.draft:setTileset(name)
        state.draft:setCategory("")
        state.dirty = true
        sync_tilesets()
        state.status_message = string.format("Wybrano tileset '%s'.", name)
        soft_refresh()
    end

    function actions.set_tileset_by_name(name)
        name = tostring(name or "")
        for index, candidate in ipairs(state.tileset_names or {}) do
            if candidate == name then
                actions.set_tileset(index)
                return
            end
        end
    end

    function actions.set_category(index)
        local name = state.category_names and state.category_names[tonumber(index or 0) or 0] or nil
        if name then
            state.draft:setCategory(name)
            state.dirty = true
            state.status_message = string.format("Wybrano sekcje '%s'.", name)
            soft_refresh()
        end
    end

    function actions.set_category_by_name(name)
        local section_name = tostring(name or "")
        if section_name == "" then
            return
        end
        state.draft:setCategory(section_name)
        if state.draft.targetTileset ~= "" then
            local tileset = state.repository and state.repository.by_tileset and state.repository.by_tileset[state.draft.targetTileset] or nil
            if not (tileset and tileset.sections_by_name and tileset.sections_by_name[section_name]) then
                state.draft:setTileset("")
            end
        end
        state.preview_page = 1
        state.dirty = true
        state.status_message = string.format("Wybrano palete '%s'.", palette_title_from_section(section_name))
        soft_refresh()
    end

    function actions.set_palette_title(title)
        local section_name = palette_section_from_title(title)
        actions.set_category_by_name(section_name)
    end

    function actions.set_tileset_title(title)
        local name = tostring(title or "")
        if name == "" or name == "-- wybierz tileset --" then
            state.draft:setTileset("")
            state.preview_page = 1
            state.dirty = true
            state.status_message = "Wyczyszczono tileset docelowy."
            soft_refresh()
            return
        end
        state.draft:setTileset(name)
        state.preview_page = 1
        state.dirty = true
        state.status_message = string.format("Wybrano tileset '%s'.", name)
        soft_refresh()
    end

    function actions.set_display_name(value)
        state.draft:setDisplayName(value)
        state.dirty = true
        sync_session_out()
    end

    function actions.set_order(value)
        state.draft:setOrder(value)
        state.preview_page = 1
        state.dirty = true
        sync_session_out()
    end

    function actions.prev_preview_page()
        state.preview_page = math.max(1, (tonumber(state.preview_page or 1) or 1) - 1)
        if not refresh_preview_local() then
            soft_refresh()
        end
    end

    function actions.next_preview_page(total_entries)
        state.preview_page = clamp_page((tonumber(state.preview_page or 1) or 1) + 1, tonumber(total_entries or 0) or 0, state.preview_page_size)
        if not refresh_preview_local() then
            soft_refresh()
        end
    end

    function actions.save_as_new()
        finalize_save("new")
    end

    function actions.update_existing()
        finalize_save("overwrite")
    end

    function actions.choose_target_path()
        local chosen, err = XmlTarget.pick_xml_file("tilesets.xml", state.target_path, "Wybierz tilesets.xml")
        if err then
            app.alert({
                title = "Nie mozna ustawic targetu",
                text = err,
                buttons = { "OK" },
            })
            return
        end
        if not chosen then
            return
        end

        state.target_path = chosen
        persist_settings()
        refresh_repository()
        sync_brushes()
        sync_tilesets()
        state.status_message = "Ustawiono nowy target tilesets.xml."
        soft_refresh()
    end

    function actions.use_default_target_path()
        state.target_path = PaletteRepository.resolve_target_path("")
        persist_settings()
        refresh_repository()
        sync_brushes()
        sync_tilesets()
        state.status_message = "Przywrocono domyslny target tilesets.xml."
        soft_refresh()
    end

    refresh_repository()
    sync_brushes()
    sync_tilesets()
    if state.draft.targetCategory == "" then
        state.draft:setCategory("terrain")
        state.draft.dirty = false
    end
    if state.repository.api_missing then
        state.status_message = "Brak API do odczytu tilesets.xml: " .. tostring(state.repository.api_missing)
    elseif #(state.repository.warnings or {}) > 0 then
        state.status_message = state.repository.warnings[1]
    end

    local page = {}

    function page.getTitle()
        return "Palette"
    end

    function page.isDirty()
        return state.dirty
    end

    function page.confirmLeave()
        return confirm_discard()
    end

    function page.onEnter(shared_session)
        session = shared_session or session
        sync_session_out()
    end

    function page.onLeave(shared_session)
        session = shared_session or session
        sync_session_out()
        persist_settings()
    end

    function page.render_into(dialog)
        dlg = dialog
        sync_brushes()
        sync_tilesets()
        local selected_brush = state.draft.selectedBrush
        local preview = preview_entries()
        local brush_first, brush_last, brush_pages = page_bounds(state.brush_page, #state.brush_names, state.brush_page_size)
        state.preview_page = clamp_page(state.preview_page, #preview, state.preview_page_size)
        local preview_first, preview_last, preview_pages = page_bounds(state.preview_page, #preview, state.preview_page_size)

        dlg:panel({
            bgcolor = palette.header,
            padding = 10,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "PALETTE",
                fgcolor = palette.text,
                font_size = 14,
                font_weight = "bold",
                align = "center",
            })
            dlg:newrow()
            dlg:label({
                text = helper_text(),
                fgcolor = palette.muted,
                align = "center",
            })
        dlg:endpanel()

        dlg:panel({
            bgcolor = palette.panel_alt,
            padding = 8,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = state.status_message ~= "" and state.status_message or helper_text(),
                fgcolor = palette.text,
            })
        dlg:endpanel()

        dlg:box({
            orient = "horizontal",
            expand = true,
        })
            dlg:box({
                label = "1. Wybierz Brush",
                orient = "vertical",
                width = 360,
                min_width = 340,
                expand = true,
                fgcolor = palette.muted,
            })
                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = false,
                })
                    dlg:label({
                        text = "Selected Brush",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:label({
                        text = selected_brush and selected_brush.title or "Brak wybranego brusha",
                        fgcolor = palette.text,
                    })
                    dlg:newrow()
                    dlg:label({
                        text = selected_brush and ("Source: " .. tostring(selected_brush.sourceType or "brush")) or "Najpierw wybierz gotowy brush",
                        fgcolor = palette.muted,
                        font_size = 8,
                    })
                    dlg:newrow()
                    dlg:image({
                        image = GroundCommon.safe_image(selected_brush and selected_brush.sampleItemId or 0),
                        width = 72,
                        height = 72,
                        smooth = false,
                    })
                dlg:endpanel()
                dlg:newrow()
                dlg:box({
                    orient = "horizontal",
                    expand = false,
                })
                    dlg:button({
                        text = "Use last saved brush",
                        bgcolor = palette.accent,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.use_last_saved_brush()
                        end,
                    })
                    dlg:button({
                        text = "Use last saved ground",
                        bgcolor = palette.panel_alt,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.use_last_saved_ground()
                        end,
                    })
                dlg:endbox()
                dlg:newrow()
                dlg:input({
                    id = "palette_brush_filter",
                    label = "Search",
                    text = state.brush_filter or "",
                    expand = true,
                    onchange = function(d)
                        actions.set_brush_filter_text(d.data.palette_brush_filter or "")
                    end,
                })
                dlg:newrow()
                dlg:box({
                    orient = "horizontal",
                    expand = false,
                })
                    dlg:button({
                        text = "Search",
                        bgcolor = palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.set_brush_filter(state.brush_filter or "")
                        end,
                    })
                    dlg:button({
                        text = "Clear",
                        bgcolor = palette.sidebar,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.clear_brush_filter()
                        end,
                    })
                dlg:endbox()
                dlg:newrow()
                dlg:box({
                    orient = "horizontal",
                    expand = false,
                })
                    dlg:button({
                        id = "palette_brush_prev",
                        text = "<",
                        bgcolor = state.brush_page > 1 and palette.panel or palette.sidebar,
                        fgcolor = palette.text,
                        onclick = function()
                            if state.brush_page > 1 then
                                actions.prev_brush_page()
                            end
                        end,
                    })
                    dlg:label({
                        id = "palette_brush_page_label",
                        text = string.format("Strona %d / %d", state.brush_page, brush_pages),
                        fgcolor = palette.muted,
                    })
                    dlg:button({
                        id = "palette_brush_next",
                        text = ">",
                        bgcolor = state.brush_page < brush_pages and palette.panel or palette.sidebar,
                        fgcolor = palette.text,
                        onclick = function()
                            if state.brush_page < brush_pages then
                                actions.next_brush_page()
                            end
                        end,
                    })
                dlg:endbox()
                dlg:newrow()
                dlg:panel({
                    bgcolor = palette.sidebar,
                    padding = 8,
                    margin = 2,
                    expand = true,
                })
                    for slot = 1, tonumber(state.brush_page_size or 12) or 12 do
                        local _, name, item = visible_brush_entry(slot)
                        local is_selected = name and selected_brush and selected_brush.name == name or false
                        dlg:box({
                            orient = "horizontal",
                            expand = true,
                        })
                            dlg:image({
                                id = "palette_brush_icon_" .. slot,
                                image = item and item.image or GroundCommon.safe_image(0),
                                width = 22,
                                height = 22,
                                smooth = false,
                            })
                            dlg:label({
                                id = "palette_brush_label_" .. slot,
                                text = item and item.text or " ",
                                fgcolor = name and palette.text or palette.subtle,
                                expand = true,
                            })
                            dlg:button({
                                id = "palette_brush_open_" .. slot,
                                text = name and "Open" or "",
                                bgcolor = name and (is_selected and palette.accent or palette.panel) or palette.sidebar,
                                fgcolor = name and palette.text or palette.subtle,
                                onclick = function()
                                    actions.open_brush_visible_slot(slot)
                                end,
                            })
                        dlg:endbox()
                        dlg:newrow()
                    end
                dlg:endpanel()
            dlg:endbox()

            dlg:box({
                label = "2-3. Preview Publikacji",
                orient = "vertical",
                min_width = 560,
                expand = true,
                fgcolor = palette.muted,
            })
                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = false,
                })
                    dlg:label({
                        text = "Podglad wpisu palety",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:label({
                        text = selected_brush and effective_display_name() or "Najpierw wybierz gotowy brush",
                        fgcolor = palette.text,
                    })
                    dlg:newrow()
                    dlg:label({
                        text = state.draft.targetTileset ~= "" and (state.draft.targetTileset .. " / " .. (state.draft.targetCategory ~= "" and state.draft.targetCategory or "?")) or "Wybierz miejsce w palecie",
                        fgcolor = palette.muted,
                    })
                dlg:endpanel()
                dlg:newrow()
                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = false,
                })
                    dlg:image({
                        image = GroundCommon.safe_image(selected_brush and selected_brush.sampleItemId or 0),
                        width = 96,
                        height = 96,
                        smooth = false,
                    })
                dlg:endpanel()
                dlg:newrow()
                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = true,
                })
                    dlg:label({
                        text = "Mock widoku palety",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = false,
                    })
                        dlg:button({
                            id = "palette_preview_prev",
                            text = "<",
                            bgcolor = state.preview_page > 1 and palette.panel or palette.sidebar,
                            fgcolor = palette.text,
                            onclick = function()
                                if state.preview_page > 1 then
                                    actions.prev_preview_page()
                                end
                            end,
                        })
                        dlg:label({
                            id = "palette_preview_page_label",
                            text = string.format(
                                "Pozycje %d-%d / %d  |  Strona %d / %d",
                                #preview > 0 and preview_first or 0,
                                #preview > 0 and preview_last or 0,
                                #preview,
                                state.preview_page,
                                preview_pages
                            ),
                            fgcolor = palette.muted,
                        })
                        dlg:button({
                            id = "palette_preview_next",
                            text = ">",
                            bgcolor = state.preview_page < preview_pages and palette.panel or palette.sidebar,
                            fgcolor = palette.text,
                            onclick = function()
                                if state.preview_page < preview_pages then
                                    actions.next_preview_page(#preview)
                                end
                            end,
                        })
                    dlg:endbox()
                    for slot = 1, tonumber(state.preview_page_size or 12) or 12 do
                        local index = preview_first + slot - 1
                        local entry = index <= preview_last and preview[index] or nil
                        dlg:newrow()
                        dlg:panel({
                            id = "palette_preview_slot_" .. slot,
                            bgcolor = entry and (entry.isInserted and palette.accent or (entry.isSelected and palette.success or palette.sidebar)) or palette.sidebar,
                            padding = 6,
                            margin = 2,
                            expand = true,
                        })
                            dlg:label({
                                id = "palette_preview_slot_label_" .. slot,
                                text = entry and string.format("%d. %s", index, entry.name) or " ",
                                fgcolor = entry and palette.text or palette.muted,
                            })
                        dlg:endpanel()
                    end
                    dlg:newrow()
                    dlg:label({
                        id = "palette_preview_empty_hint",
                        text = #preview == 0 and "Wybierz tileset i sekcje, aby zobaczyc podglad publikacji." or " ",
                        fgcolor = palette.muted,
                    })
                dlg:endpanel()
            dlg:endbox()

            dlg:box({
                label = "4. Ustawienia i Save",
                orient = "vertical",
                width = 380,
                min_width = 360,
                expand = true,
                fgcolor = palette.muted,
            })
                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = false,
                })
                    dlg:label({
                        text = "Docelowe miejsce",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:combobox({
                        id = "palette_section_combo",
                        label = "Palette",
                        options = palette_option_titles(),
                        option = palette_title_from_section(state.draft.targetCategory),
                        onchange = function(d)
                            actions.set_palette_title(d.data.palette_section_combo or "")
                        end,
                    })
                    dlg:newrow()
                    dlg:combobox({
                        id = "palette_tileset_combo",
                        label = "Tileset",
                        options = tileset_options_for_section(state.draft.targetCategory),
                        option = (state.draft.targetTileset ~= "" and state.draft.targetTileset) or "-- wybierz tileset --",
                        onchange = function(d)
                            actions.set_tileset_title(d.data.palette_tileset_combo or "")
                        end,
                    })
                    dlg:newrow()
                    dlg:input({
                        id = "palette_display_name",
                        label = "Display name",
                        text = state.draft.displayName or "",
                        expand = true,
                        onchange = function(d)
                            actions.set_display_name(d.data.palette_display_name or "")
                        end,
                    })
                    dlg:newrow()
                    dlg:input({
                        id = "palette_order_input",
                        label = "Order",
                        text = tostring(state.draft.order or 0),
                        expand = true,
                        onchange = function(d)
                            actions.set_order(d.data.palette_order_input or "")
                        end,
                    })
                    dlg:newrow()
                    dlg:button({
                        text = "Save as New Entry",
                        bgcolor = palette.success,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.save_as_new()
                        end,
                    })
                    dlg:newrow()
                    dlg:button({
                        text = "Update Existing Entry",
                        bgcolor = palette.accent,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.update_existing()
                        end,
                    })
                dlg:endpanel()
                dlg:newrow()
                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = false,
                })
                    dlg:label({
                        text = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie",
                        fgcolor = state.dirty and palette.warning or palette.muted,
                    })
                    dlg:newrow()
                    dlg:label({
                        text = "Target: " .. tostring(state.target_path or ""),
                        fgcolor = palette.subtle,
                        font_size = 8,
                    })
                    dlg:newrow()
                    dlg:label({
                        text = save_hint_text(),
                        fgcolor = palette.muted,
                    })
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = false,
                    })
                        dlg:button({
                            text = "Zmien XML",
                            bgcolor = palette.panel,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.choose_target_path()
                            end,
                        })
                        dlg:button({
                            text = "Domyslny XML",
                            bgcolor = palette.panel,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.use_default_target_path()
                            end,
                        })
                    dlg:endbox()
                dlg:endpanel()
            dlg:endbox()
        dlg:endbox()
    end

    function page.getTitle()
        return "Palette"
    end

    function page.isDirty()
        return state.dirty
    end

    function page.confirmLeave()
        return confirm_discard()
    end

    function page.onEnter(shared_session)
        session = shared_session or session
        sync_session_out()
    end

    function page.onLeave(shared_session)
        session = shared_session or session
        sync_session_out()
    end

    function page.getSessionState()
        sync_session_out()
        return session
    end

    function page.getStatusText()
        return state.status_message ~= "" and state.status_message or helper_text()
    end

    function page.getState()
        return state
    end

    return page
end

function PalettePage.create(options)
    return create_page(options)
end

return PalettePage
