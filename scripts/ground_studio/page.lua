local Common = dofile("common.lua")
local Model = dofile("model.lua")
local LibraryPanel = dofile("library_panel.lua")
local PreviewPanel = dofile("preview_panel.lua")
local PropertiesPanel = dofile("properties_panel.lua")
local SavePanel = dofile("save_panel.lua")
local Logger = dofile("logger.lua")
local XmlTarget = dofile("xml_target_helper.lua")

local GroundStudioPage = {}

local function create_page(options)
    options = options or {}
    if options.reset_logger then
        Logger.reset("startup")
    end
    Logger.log("page", "Ground Studio page create")

    local settings = Model.load_settings()
    local session = options.session or {}
    local listener_id = nil
    local dlg
    local actions = {}

    local state = {
        filter_text = "",
        target_path = Model.resolve_target_path(settings.target_path or ""),
        session_grounds = {},
        repository = nil,
        library_filtered_items = {},
        library_filtered_names = {},
        library_items = {},
        library_visible_names = {},
        library_selection = 0,
        library_selected_name = "",
        library_page = 1,
        library_page_size = 18,
        library_page_count = 1,
        draft = nil,
        variant_selection = 0,
        selected_variant_chance = "",
        placement_chance = tostring(settings.main_chance or 1),
        selected_raw_id = Common.active_raw_brush_id(),
        recent_raw_ids = Common.clone(settings.recent_raw_ids or {}),
        status_message = "",
        dirty = false,
        rebuilding = false,
        library_skip_changes = 0,
    }

local function refresh_repository()
    Logger.log("main", "refresh_repository")
    state.repository = Model.read_repository(state.target_path, state.session_grounds)
end

local function sync_library(preferred_name, allow_draft_fallback)
    local filtered_items, filtered_names = Model.library_items(state.repository, state.filter_text)
    state.library_filtered_items = filtered_items
    state.library_filtered_names = filtered_names
    local explicit_preferred = preferred_name ~= nil
    if type(preferred_name) ~= "string" then
        preferred_name = ""
    end
    preferred_name = Common.trim(preferred_name)
    if preferred_name == "" and not explicit_preferred then
        preferred_name = Common.trim(state.library_selected_name or "")
    end
    if allow_draft_fallback ~= false and preferred_name == "" and state.draft and not state.draft.isNew then
        preferred_name = state.draft.name
    end

    local filtered_count = #state.library_filtered_names
    local page_size = math.max(1, tonumber(state.library_page_size or 18) or 18)
    state.library_page_count = math.max(1, math.ceil(filtered_count / page_size))

    if preferred_name ~= "" then
        for absolute_index, name in ipairs(state.library_filtered_names) do
            if name == preferred_name then
                state.library_page = math.ceil(absolute_index / page_size)
                break
            end
        end
    end

    if state.library_page < 1 then
        state.library_page = 1
    end
    if state.library_page > state.library_page_count then
        state.library_page = state.library_page_count
    end

    local page_start = ((state.library_page - 1) * page_size) + 1
    local page_end = math.min(filtered_count, page_start + page_size - 1)

    state.library_items = {}
    state.library_visible_names = {}
    for absolute_index = page_start, page_end do
        table.insert(state.library_items, state.library_filtered_items[absolute_index])
        table.insert(state.library_visible_names, state.library_filtered_names[absolute_index])
    end

    state.library_selection = 0
    if preferred_name ~= "" then
        for index, name in ipairs(state.library_visible_names) do
            if name == preferred_name then
                state.library_selection = index
                break
            end
        end
    end
    Logger.log("main", string.format("sync_library filter='%s' filtered=%d page=%d/%d visible=%d selection=%d selected='%s'", tostring(state.filter_text or ""), filtered_count, tonumber(state.library_page or 1), tonumber(state.library_page_count or 1), #state.library_visible_names, tonumber(state.library_selection or 0), tostring(state.library_selected_name or "")))
end

local function soft_refresh_library()
    if not dlg then
        return
    end

    state.library_skip_changes = 12
    dlg:modify({
        ground_library = {
            items = state.library_items,
            selection = state.library_selection,
        },
        ground_library_page_label = {
            text = string.format("Strona %d / %d", tonumber(state.library_page or 1), tonumber(state.library_page_count or 1)),
        },
    })
    dlg:layout()
    dlg:repaint()
end

local function persist_settings()
    Model.save_settings(state)
end

local function sync_session_out()
    session.lastSelectedRaw = tonumber(state.selected_raw_id or 0) or 0
    session.recentItems = session.recentItems or {}
    session.recentItems.groundRawIds = state.recent_raw_ids
    session.pageDirty = session.pageDirty or {}
    session.pageDirty.grounds = state.dirty
    if state.draft and not state.draft.isNew and Common.trim(state.draft.name or "") ~= "" then
        session.selectedGroundForComposer = state.draft.name
    end
end

local function sync_session_in()
    local shared_raw = tonumber(session.lastSelectedRaw or 0) or 0
    if shared_raw > 0 then
        state.selected_raw_id = shared_raw
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
        text = "The current ground draft has unsaved changes. Continue and discard them?",
        buttons = { "Yes", "No" },
    })
    return choice == 1
end

local function helper_text()
    local step = Common.current_step(state)
    local labels = {
        "Krok 1: wybierz gotowy ground albo utworz nowy.",
        "Krok 2: wybierz RAW tile z glownej palety RME.",
        "Krok 3: ustaw glowny ground tile.",
        "Krok 4: opcjonalnie dodaj warianty z chance.",
        "Krok 5: ustaw wlasciwosci i sprawdz preview.",
        "Krok 6: zapisz ground.",
    }
    return labels[step] or labels[1]
end

local function current_raw_id()
    return Common.active_raw_brush_id()
end

local function sync_selected_raw_from_brush(reason)
    local active = current_raw_id()
    local current = tonumber(state.selected_raw_id or 0) or 0
    if active == current then
        return false
    end
    state.selected_raw_id = active
    Logger.log("main", string.format("sync_selected_raw_from_brush reason=%s raw=%d previous=%d", tostring(reason or "unknown"), active, current))
    return true
end

local function selected_variant()
    local index = tonumber(state.variant_selection or 0) or 0
    if index <= 0 or not state.draft or index > #state.draft.variants then
        return nil
    end
    return state.draft.variants[index]
end

local function remember_recent_raw(item_id)
    item_id = tonumber(item_id or 0) or 0
    if item_id <= 0 then
        return
    end

    local next_items = { item_id }
    for _, existing in ipairs(state.recent_raw_ids or {}) do
        existing = tonumber(existing or 0) or 0
        if existing > 0 and existing ~= item_id then
            table.insert(next_items, existing)
        end
        if #next_items >= 8 then
            break
        end
    end
    state.recent_raw_ids = next_items
end

local function preview_status_text()
    if not state.draft or not state.draft:hasMainItem() then
        return "Wybierz glowny ground tile, aby zobaczyc preview."
    end
    if #state.draft.variants == 0 then
        return "Preview pokazuje glowny tile. Warianty sa opcjonalne."
    end
    return string.format("Preview pokazuje glowny tile i %d wariant(y).", #state.draft.variants)
end

local function save_hint_text()
    if not state.draft or Common.trim(state.draft.name or "") == "" then
        return "Podaj nazwe grounda, aby zapisac."
    end
    if not state.draft:hasMainItem() then
        return "Wybierz glowny ground tile przed zapisem."
    end
    return "Zapis utworzy nowy wpis albo nadpisze istniejacy ground."
end

local function save_panel_state()
    local is_existing = state.draft and not state.draft.isNew
    return {
        mode_label = state.draft and (state.draft.isNew and "Tryb: nowy ground" or "Tryb: edycja istniejacego grounda") or "Tryb: nowy ground",
        dirty_label = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie",
        target_label = "Target: " .. tostring(state.target_path or ""),
        hint_label = save_hint_text(),
        intent_new = Model.save_intent(state.repository, state.draft, "new"),
        intent_overwrite = is_existing and Model.save_intent(state.repository, state.draft, "overwrite") or Model.save_intent(state.repository, state.draft, "overwrite"),
    }
end

local function soft_refresh()
    if not dlg or not state.draft then
        return
    end

    sync_session_out()
    sync_selected_raw_from_brush("soft_refresh")
    state.rebuilding = true

    local raw_info = Common.safe_item_info(state.selected_raw_id)
    local main_item_id = state.draft.mainGroundItem and state.draft.mainGroundItem.itemId or 0
    local selected_variant = selected_variant()
    local save_state = save_panel_state()

    state.library_skip_changes = 12
    dlg:modify({
        ground_library_page_label = {
            text = string.format("Strona %d / %d", tonumber(state.library_page or 1), tonumber(state.library_page_count or 1)),
        },
        current_ground_title = {
            text = Common.format_ground_title(state.draft),
        },
        current_ground_status = {
            text = "Status: " .. Common.ground_status(state.draft),
        },
        ground_preview_help = {
            text = helper_text(),
        },
        ground_main_preview = {
            image = Common.safe_image(main_item_id),
            width = 80,
            height = 80,
            smooth = false,
        },
        ground_main_label = {
            text = main_item_id > 0 and string.format("Item ID %d  |  chance %d", main_item_id, tonumber(state.draft.mainChance or 0) or 0) or "Brak glownego tile",
        },
        ground_surface_preview = {
            items = Common.surface_preview_items(state.draft),
        },
        ground_preview_status = {
            text = preview_status_text(),
        },
        ground_variant_preview = {
            items = Common.variant_preview_items(state.draft),
        },
        active_raw_preview = {
            image = Common.safe_image(state.selected_raw_id),
            width = 48,
            height = 48,
            smooth = false,
        },
        active_raw_title = {
            text = raw_info and (raw_info.name or ("Item " .. tostring(state.selected_raw_id))) or "Najpierw wybierz RAW tile",
        },
        active_raw_meta = {
            text = raw_info and string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0) or "Brak aktywnego RAW.",
        },
        active_raw_hint = {
            text = raw_info and "Ten RAW mozesz ustawic jako Main Ground albo dodac jako wariant." or "Najpierw wybierz RAW tile w glownej palecie RME.",
        },
        main_ground_preview_side = {
            image = Common.safe_image(main_item_id),
            width = 56,
            height = 56,
            smooth = false,
        },
        main_ground_title = {
            text = main_item_id > 0 and ((state.draft.mainGroundItem and state.draft.mainGroundItem.name) or ("Item " .. tostring(main_item_id))) or "Brak Main Ground",
        },
        main_ground_meta = {
            text = main_item_id > 0 and string.format("Item ID %d  |  chance %d", main_item_id, tonumber(state.draft.mainChance or 0) or 0) or "Wybierz RAW i kliknij 'Ustaw jako Main Ground'.",
        },
        ground_name_input = {
            text = state.draft.name or "",
        },
        ground_variant_list = {
            items = Common.variant_list_items(state.draft),
            selection = tonumber(state.variant_selection or 0) or 0,
        },
        ground_variant_chance = {
            text = selected_variant and tostring(selected_variant.chance or "") or "",
        },
        placement_chance = {
            text = tostring(state.placement_chance or state.draft.mainChance or 1),
        },
        selected_variant_preview = {
            image = Common.safe_image(selected_variant and selected_variant.itemId or 0),
            width = 40,
            height = 40,
            smooth = false,
        },
        selected_variant_title = {
            text = selected_variant and (selected_variant.name or ("Item " .. tostring(selected_variant.itemId or 0))) or "Nie wybrano wariantu",
        },
        selected_variant_meta = {
            text = selected_variant and string.format("Item ID %d  |  chance %d", tonumber(selected_variant.itemId or 0) or 0, tonumber(selected_variant.chance or 0) or 0) or "Wybierz wariant z listy, aby edytowac chance.",
        },
        variant_hint_label = {
            text = #state.draft.variants > 0 and "Warianty sa oddzielone od Main Ground. Wybierz wariant z listy, aby edytowac chance." or "Brak wariantow. Wybierz RAW i kliknij 'Dodaj Wariant'.",
        },
        recent_raw_list = {
            items = Common.recent_raw_items(state.recent_raw_ids),
        },
        ground_set_list = {
            items = Common.ground_set_items(state.draft),
        },
        ground_lookid_input = {
            text = tostring(state.draft.lookId or 0),
        },
        ground_server_lookid_input = {
            text = tostring(state.draft.serverLookId or 0),
        },
        ground_z_order_input = {
            text = tostring(state.draft.zOrder or 0),
        },
        toggle_randomize_button = {
            text = state.draft.randomize and "Randomize: ON" or "Randomize: OFF",
        },
        toggle_solo_optional_button = {
            text = state.draft.soloOptional and "Solo Optional: ON" or "Solo Optional: OFF",
        },
        save_mode_label = {
            text = save_state.mode_label,
        },
        save_dirty_label = {
            text = save_state.dirty_label,
        },
        save_target_label = {
            text = save_state.target_label,
        },
        save_hint_label = {
            text = save_state.hint_label,
        },
        save_intent_new = {
            text = save_state.intent_new,
        },
        save_intent_overwrite = {
            text = save_state.intent_overwrite,
        },
    })
    dlg:repaint()
    state.rebuilding = false
end

local function soft_refresh_active_raw()
    if not dlg then
        return
    end

    sync_session_out()
    sync_selected_raw_from_brush("soft_refresh_raw")

    local raw_info = Common.safe_item_info(state.selected_raw_id)
    dlg:modify({
        active_raw_preview = {
            image = Common.safe_image(state.selected_raw_id),
            width = 48,
            height = 48,
            smooth = false,
        },
        active_raw_title = {
            text = raw_info and (raw_info.name or ("Item " .. tostring(state.selected_raw_id))) or "Najpierw wybierz RAW tile",
        },
        active_raw_meta = {
            text = raw_info and string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0) or "Brak aktywnego RAW.",
        },
        active_raw_hint = {
            text = raw_info and "Ten RAW mozesz ustawic jako Main Ground albo dodac jako wariant." or "Najpierw wybierz RAW tile w glownej palecie RME.",
        },
    })
    dlg:repaint()
end

local function render_page_content()
    state.rebuilding = true
    Logger.log("main", string.format("rebuild start ground='%s' dirty=%s", state.draft and state.draft.name or "nil", tostring(state.dirty)))
    refresh_repository()
    sync_library()
    sync_selected_raw_from_brush("rebuild")
    dlg:panel({
        bgcolor = Common.palette.header,
        padding = 10,
        margin = 4,
        expand = false,
    })
        dlg:label({
            text = "GROUND STUDIO",
            fgcolor = Common.palette.text,
            font_size = 14,
            font_weight = "bold",
            align = "center",
        })
    dlg:endpanel()

    dlg:box({
        orient = "horizontal",
        expand = true,
    })
        LibraryPanel.render(dlg, state, actions)
        PreviewPanel.render(dlg, state)
        dlg:box({
            orient = "vertical",
            width = 380,
            min_width = 360,
            expand = true,
        })
            PropertiesPanel.render(dlg, state, actions)
            dlg:newrow()
            SavePanel.render(dlg, state, actions)
        dlg:endbox()
    dlg:endbox()
    state.library_skip_changes = 8
    state.rebuilding = false
    Logger.log("main", "rebuild done")
end

local function rebuild()
    if not dlg then
        return
    end
    if request_host_render() then
        return
    end

    dlg:clear()
    render_page_content()
    dlg:layout()
    dlg:repaint()
end

function actions.set_filter(text)
    state.filter_text = text or ""
    state.library_page = 1
    sync_library("", false)
    soft_refresh_library()
end

function actions.previous_library_page()
    if state.library_page <= 1 then
        return
    end
    state.library_page = state.library_page - 1
    sync_library("", false)
    soft_refresh_library()
end

function actions.next_library_page()
    if state.library_page >= state.library_page_count then
        return
    end
    state.library_page = state.library_page + 1
    sync_library("", false)
    soft_refresh_library()
end

function actions.set_library_selection(index)
    index = tonumber(index or 0) or 0
    state.library_selection = index
    state.library_selected_name = state.library_visible_names[index] or ""
end

function actions.handle_library_change(index)
    index = tonumber(index or 0) or 0
    if index <= 0 then
        Logger.log("library", "handle_library_change ignored empty index")
        return
    end
    if state.library_skip_changes > 0 then
        state.library_skip_changes = state.library_skip_changes - 1
        Logger.log("library", string.format("handle_library_change skipped programmatic index=%d remaining=%d", index, state.library_skip_changes))
        return
    end

    local previous_index = tonumber(state.library_selection or 0) or 0
    local previous_name = tostring(state.library_selected_name or "")
    actions.set_library_selection(index)

    if previous_index == index and previous_name == tostring(state.library_selected_name or "") then
        Logger.log("library", string.format("handle_library_change ignored duplicate index=%d selected_name=%s", index, tostring(state.library_selected_name or "")))
        return
    end

    Logger.log("library", string.format("handle_library_change open index=%d selected_name=%s", index, tostring(state.library_selected_name or "")))
    if state.library_selected_name ~= "" then
        actions.open_ground(state.library_selected_name)
    end
end

function actions.open_selected_ground()
    if state.library_selected_name == "" then
        Logger.log("library", "open_selected_ground ignored empty selection")
        return
    end
    Logger.log("library", string.format("open_selected_ground name=%s", tostring(state.library_selected_name or "")))
    actions.open_ground(state.library_selected_name)
end

function actions.open_ground(name)
    name = Common.trim(name)
    Logger.log("main", string.format("open_ground request name='%s'", tostring(name)))
    if name == "" then
        Logger.log("main", "open_ground ignored empty name")
        return
    end
    if state.draft and state.draft.name == name and not state.dirty then
        Logger.log("main", string.format("open_ground skipped same name='%s'", tostring(name)))
        return
    end
    if not confirm_discard() then
        Logger.log("main", string.format("open_ground cancelled by discard dialog name='%s'", tostring(name)))
        return
    end

    local loaded = Model.load_draft(state.repository, name)
    if not loaded then
        Logger.log("main", string.format("open_ground failed missing name='%s'", tostring(name)))
        app.alert("This ground could not be loaded from grounds.xml.")
        return
    end

    Logger.log("main", string.format("open_ground loaded name='%s' variants=%d", tostring(loaded.name or ""), #(loaded.variants or {})))
    state.draft = loaded
    state.library_selected_name = loaded.name
    state.variant_selection = 0
    state.selected_variant_chance = ""
    state.placement_chance = tostring(loaded.mainChance or state.placement_chance or 1)
    state.status_message = string.format("Zaladowano ground '%s' do edycji.", loaded.name)
    state.dirty = false
    Logger.log("main", string.format("open_ground before rebuild name='%s'", tostring(loaded.name or "")))
    rebuild()
end

function actions.new_ground()
    if not confirm_discard() then
        return
    end
    state.draft = Model.new_draft(state.repository, state.session_grounds)
    state.library_selected_name = ""
    state.variant_selection = 0
    state.selected_variant_chance = ""
    state.placement_chance = tostring(state.draft.mainChance or 1)
    state.status_message = string.format("Utworzono nowy draft '%s'.", state.draft.name)
    state.dirty = false
    rebuild()
end

function actions.duplicate_current()
    state.draft = Model.duplicate_draft(state.repository, state.session_grounds, state.draft)
    state.library_selected_name = ""
    state.variant_selection = 0
    state.selected_variant_chance = ""
    state.placement_chance = tostring(state.draft.mainChance or 1)
    state.status_message = string.format("Zduplikowano ground do nowego draftu '%s'.", state.draft.name)
    state.dirty = true
    rebuild()
end

function actions.use_active_raw_as_main()
    local raw_id = current_raw_id()
    local placement_chance = tonumber(state.placement_chance or state.draft.mainChance or 1) or 1
    if raw_id <= 0 then
        state.status_message = "Najpierw wybierz RAW tile."
        soft_refresh()
        return
    end
    if state.draft.mainGroundItem and tonumber(state.draft.mainGroundItem.itemId or 0) == raw_id then
        state.status_message = string.format("Tile %d jest juz ustawiony jako Main Ground.", raw_id)
        soft_refresh()
        return
    end
    state.draft:setMainGround(Model.make_raw_reference(raw_id))
    state.draft:setMainChance(placement_chance)
    remember_recent_raw(raw_id)
    state.status_message = string.format("Ustawiono glowny tile %d z chance %d.", raw_id, placement_chance)
    state.dirty = true
    soft_refresh()
end

function actions.add_variant_from_active_raw()
    local raw_id = current_raw_id()
    local placement_chance = tonumber(state.placement_chance or 1) or 1
    if raw_id <= 0 then
        state.status_message = "Najpierw wybierz RAW tile."
        soft_refresh()
        return
    end

    local ok, message = state.draft:addVariant(Model.make_raw_reference(raw_id), placement_chance)
    if not ok then
        state.status_message = message
        soft_refresh()
        return
    end

    remember_recent_raw(raw_id)
    state.variant_selection = #state.draft.variants
    state.selected_variant_chance = tostring(state.draft.variants[state.variant_selection].chance or "")
    state.status_message = string.format("Dodano wariant %d z chance %d.", raw_id, placement_chance)
    state.dirty = true
    soft_refresh()
end

function actions.set_name(name)
    if not state.draft then
        return
    end
    state.draft:setName(name)
    state.dirty = true
    sync_session_out()
end

function actions.set_placement_chance(text)
    state.placement_chance = tostring(text or "")
    sync_session_out()
end

function actions.set_variant_selection(index)
    state.variant_selection = tonumber(index or 0) or 0
    local variant = selected_variant()
    state.selected_variant_chance = variant and tostring(variant.chance or "") or ""
    soft_refresh()
end

function actions.set_variant_chance(text)
    local variant = selected_variant()
    if not variant then
        return
    end
    state.selected_variant_chance = tostring(text or "")
    state.draft:setVariantChance(state.variant_selection, tonumber(text or variant.chance or 1) or 1)
    state.dirty = true
    sync_session_out()
end

function actions.remove_selected_variant()
    if state.variant_selection <= 0 then
        state.status_message = "Najpierw wybierz wariant do usuniecia."
        soft_refresh()
        return
    end
    if state.draft:removeVariant(state.variant_selection) then
        state.variant_selection = 0
        state.selected_variant_chance = ""
        state.status_message = "Usunieto wybrany wariant."
        state.dirty = true
        soft_refresh()
    end
end

function actions.set_look_id(value)
    state.draft:setLookId(tonumber(value or 0) or 0)
    state.dirty = true
    sync_session_out()
end

function actions.set_server_lookid(value)
    state.draft:setServerLookId(tonumber(value or 0) or 0)
    state.dirty = true
    sync_session_out()
end

function actions.set_z_order(value)
    state.draft:setZOrder(tonumber(value or 0) or 0)
    state.dirty = true
    sync_session_out()
end

function actions.toggle_randomize()
    state.draft:setRandomize(not state.draft.randomize)
    state.dirty = true
    soft_refresh()
end

function actions.toggle_solo_optional()
    state.draft:setSoloOptional(not state.draft.soloOptional)
    state.dirty = true
    soft_refresh()
end

local function finalize_save(mode)
    local ok, result = Model.save_draft(state.repository, state.session_grounds, state.draft, mode, state.target_path)
    if not ok then
        app.alert({
            title = "Save failed",
            text = result,
            buttons = { "OK" },
        })
        return
    end

    state.draft = result.draft
    state.library_selected_name = result.draft.name
    state.status_message = result.message
    state.dirty = false
    session.lastSavedGround = result.draft.name
    sync_session_out()
    rebuild()
    app.alert({
        title = "Ground saved",
        text = result.message,
        buttons = { "OK" },
    })
end

function actions.save_as_new()
    finalize_save("new")
end

function actions.overwrite_existing()
    finalize_save("overwrite")
end

function actions.choose_target_path()
    local chosen, err = XmlTarget.pick_xml_file("grounds.xml", state.target_path, "Wybierz grounds.xml")
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
    rebuild()
end

function actions.use_default_target_path()
    state.target_path = Model.resolve_target_path("")
    persist_settings()
    rebuild()
end

local function attach_brush_listener()
    if listener_id or not app.events or not app.events.on then
        return
    end

    listener_id = app.events:on("brushChange", function(brush_name)
        Logger.log("main", string.format("brushChange event brush=%s", tostring(brush_name)))
        if sync_selected_raw_from_brush("brushChange") and dlg then
            sync_session_out()
            soft_refresh_active_raw()
        end
    end)
end

local function detach_brush_listener()
    if listener_id and app.events and app.events.off then
        pcall(function()
            app.events:off(listener_id)
        end)
    end
    listener_id = nil
end

local function initialize()
    refresh_repository()
    state.draft = settings.last_ground_name and Model.load_draft(state.repository, settings.last_ground_name) or nil
    if not state.draft then
        state.draft = Model.new_draft(state.repository, state.session_grounds)
        Logger.log("main", "initial draft missing, creating new")
    end

    if state.repository.api_missing then
        state.status_message = "Brak API do odczytu grounds.xml: " .. tostring(state.repository.api_missing)
    elseif #state.repository.warnings > 0 then
        state.status_message = state.repository.warnings[1]
    else
        state.status_message = "Wybierz gotowy ground albo utworz nowy draft."
    end

    sync_session_in()
    sync_selected_raw_from_brush("startup")
    sync_session_out()
end

initialize()

local page = {}

function page.getTitle()
    return "Grounds"
end

function page.isDirty()
    return state.dirty
end

function page.confirmLeave()
    return confirm_discard()
end

function page.onEnter(shared_session)
    session = shared_session or session
    sync_session_in()
    sync_session_out()
    attach_brush_listener()
end

function page.onLeave(shared_session)
    session = shared_session or session
    sync_session_out()
    persist_settings()
    detach_brush_listener()
end

function page.render_into(dialog)
    dlg = dialog
    render_page_content()
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

function GroundStudioPage.create(options)
    return create_page(options)
end

function GroundStudioPage.open_standalone()
    local existing_dialog = _G.GROUND_STUDIO_DIALOG
    if existing_dialog then
        pcall(function()
            existing_dialog:close()
        end)
        _G.GROUND_STUDIO_DIALOG = nil
        _G.GROUND_STUDIO_OPEN = false
        app.yield()
    end

    local page = create_page({
        reset_logger = true,
    })

    local dlg = Dialog({
        title = "Ground Studio",
        id = "ground_studio_dock",
        width = 1540,
        height = 900,
        resizable = true,
        dockable = true,
        onclose = function()
            Logger.log("main", "dialog onclose")
            page.onLeave({})
            _G.GROUND_STUDIO_OPEN = false
            _G.GROUND_STUDIO_DIALOG = nil
        end,
    })

    _G.GROUND_STUDIO_OPEN = true
    _G.GROUND_STUDIO_DIALOG = dlg
    page.onEnter({})
    dlg:clear()
    page.render_into(dlg)
    dlg:layout()
    dlg:repaint()
    Logger.log("main", "show dialog")
    dlg:show({ wait = false, center = "screen" })
    return page
end

return GroundStudioPage
