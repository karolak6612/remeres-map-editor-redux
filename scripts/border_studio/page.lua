local Common = dofile("common.lua")
local Model = dofile("model.lua")
local DraftAdapter = dofile("draft_adapter.lua")
local LibraryPanel = dofile("library_panel.lua")
local SlotsPanel = dofile("slots_panel.lua")
local RawPanel = dofile("raw_panel.lua")
local SavePanel = dofile("save_panel.lua")
local Logger = dofile("logger.lua")
local XmlTarget = dofile("xml_target_helper.lua")

local BorderStudioPage = {}

local function create_page(options)
    options = options or {}
    if options.reset_logger then
        Logger.reset("startup")
    end
    Logger.log("page", "Border Studio page create")

    local settings = Model.load_settings()
    local session = options.session or {}
    local listener_id = nil
    local dlg
    local actions = {}

    local state = {
        filter_text = "",
        selected_raw_id = settings.selected_raw_id or 0,
        recent_raw_ids = settings.recent_raw_ids or {},
        target_path = Model.resolve_target_path(settings.target_path or ""),
        session_borders = {},
        repository = nil,
        library_filtered_items = {},
        library_filtered_ids = {},
        library_items = {},
        library_visible_ids = {},
        library_selection = 0,
        library_selected_border_id = 0,
        library_page = 1,
        library_page_size = 18,
        library_page_count = 1,
        draft = nil,
        save_border_id = 0,
        save_name = "",
        export_preview = nil,
        export_checked = false,
        status_message = "",
        dirty = false,
        rebuilding = false,
        library_ignore_index = 0,
        library_ignore_budget = 0,
        library_skip_changes = 0,
        highlighted_slot_index = 0,
    }

local function refresh_repository()
    Logger.log("main", "refresh_repository")
    state.repository = Model.read_repository(state.session_borders)
end

local function sync_library(preferred_border_id, allow_draft_fallback)
    local filtered_items, filtered_ids = Model.library_items(state.repository, state.filter_text)
    state.library_filtered_items = filtered_items
    state.library_filtered_ids = filtered_ids
    preferred_border_id = tonumber(preferred_border_id or state.library_selected_border_id or 0) or 0
    if allow_draft_fallback ~= false and (not preferred_border_id or preferred_border_id <= 0) and state.draft and not state.draft.isNew then
        preferred_border_id = state.draft.borderId
    end

    local filtered_count = #state.library_filtered_ids
    local page_size = math.max(1, tonumber(state.library_page_size or 18) or 18)
    state.library_page_count = math.max(1, math.ceil(filtered_count / page_size))

    if preferred_border_id and preferred_border_id > 0 then
        for absolute_index, border_id in ipairs(state.library_filtered_ids) do
            if border_id == preferred_border_id then
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
    state.library_visible_ids = {}
    for absolute_index = page_start, page_end do
        table.insert(state.library_items, state.library_filtered_items[absolute_index])
        table.insert(state.library_visible_ids, state.library_filtered_ids[absolute_index])
    end

    state.library_selection = 0
    if preferred_border_id and preferred_border_id > 0 then
        for index, border_id in ipairs(state.library_visible_ids) do
            if border_id == preferred_border_id then
                state.library_selection = index
                break
            end
        end
    end
    Logger.log("main", string.format("sync_library filter='%s' filtered=%d page=%d/%d visible=%d selected_index=%d selected_border=%d", tostring(state.filter_text or ""), filtered_count, tonumber(state.library_page or 1), tonumber(state.library_page_count or 1), #state.library_visible_ids, tonumber(state.library_selection or 0), tonumber(state.library_selected_border_id or 0)))
end

local function soft_refresh_library()
    if not dlg then
        return
    end

    state.library_skip_changes = 4
    dlg:modify({
        border_library = {
            items = state.library_items or {},
            selection = tonumber(state.library_selection or 0) or 0,
        },
        library_page_label = {
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
    session.recentItems.borderRawIds = state.recent_raw_ids
    session.pageDirty = session.pageDirty or {}
    session.pageDirty.borders = state.dirty
    if state.draft and not state.draft.isNew then
        session.selectedBorderForComposer = state.draft.borderId
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
        text = "The current border draft has unsaved changes. Continue and discard them?",
        buttons = { "Yes", "No" },
    })
    return choice == 1
end

local function helper_text()
    local step = Common.current_step(state)
    if step == 1 then
        return "Krok 1: wybierz gotowy border albo utworz nowy."
    end
    if step == 2 then
        return "Krok 2: wybierz RAW tile w glownej palecie RME."
    end
    if step == 3 then
        return "Krok 3: kliknij slot. LPM przypisuje RAW, PPM czyści."
    end
    if step == 4 then
        return "Krok 4: sprawdz podglad uzycia i brakujace sloty."
    end
    return "Krok 5: zapisz nowy border albo nadpisz istniejacy."
end

local function slot_summary_text()
    local preview_state = DraftAdapter.preview_state(state.draft)
    if preview_state.hasAssignments then
        local empty_label = #preview_state.emptySlots > 0 and table.concat(preview_state.emptySlots, ", ") or "none"
        return string.format("Ustawione sloty: %d. Puste sloty: %s", preview_state.assignedCount, empty_label)
    end
    return "Kliknij slot w podgladzie, aby przypisac tile."
end

local function selected_slot_name()
    local key = DraftAdapter.visual_slot_name_from_layout(state.highlighted_slot_index)
    return key
end

local function selected_slot_status_text()
    local visual_key = selected_slot_name()
    if not visual_key then
        return "Nie wybrano zadnego slotu."
    end

    local meta = Common.slot_meta[visual_key]
    local stored_key = Common.storage_slot_name(visual_key)
    local item_id = state.draft and state.draft:getSlotItemId(stored_key) or 0
    local status = item_id > 0 and "ustawiony" or "pusty"
    return string.format("Wybrany slot: %s (%s) - %s", meta.full, meta.short, status)
end

local function preview_help_text()
    local raw_id = Common.active_raw_brush_id()
    if raw_id <= 0 then
        raw_id = tonumber(state.selected_raw_id or 0) or 0
    end
    if raw_id <= 0 then
        return "Najpierw wybierz RAW tile w glownej palecie."
    end
    if not state.draft or state.draft:isEmpty() then
        return "Kliknij slot w podgladzie, aby przypisac tile."
    end
    return "LPM przypisuje aktualny RAW. PPM czyści wybrany slot."
end

local function save_hint_text()
    if not state.save_border_id or state.save_border_id <= 0 then
        return "Podaj ID, aby zapisac border."
    end
    if not state.draft or state.draft:isEmpty() then
        return "Kliknij slot w podgladzie, aby przypisac tile przed zapisem."
    end

    local has_existing = state.repository and state.repository.by_id and state.repository.by_id[state.save_border_id] ~= nil
    if has_existing then
        return string.format("Nadpiszesz border #%d albo zapisz kopie pod nowym ID.", state.save_border_id)
    end
    return string.format("Ten ID utworzy nowy border #%d.", state.save_border_id)
end

local function current_raw_id()
    local active = Common.active_raw_brush_id()
    if active > 0 then
        return active
    end

    local selected = tonumber(state.selected_raw_id or 0) or 0
    if selected > 0 then
        return selected
    end

    return 0
end

local function sync_selected_raw_from_brush(reason)
    local active = Common.active_raw_brush_id()
    local current = tonumber(state.selected_raw_id or 0) or 0
    if active == current then
        return false
    end

    state.selected_raw_id = active
    Logger.log("main", string.format("sync_selected_raw_from_brush reason=%s raw=%d previous=%d", tostring(reason or "unknown"), active, current))
    return true
end

local function save_panel_state()
    local is_existing = state.draft and not state.draft.isNew
    local dirty_label = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie"
    local mode_label = is_existing
        and string.format("Tryb: edycja istniejacego wpisu #%d", state.draft.borderId or 0)
        or "Tryb: nowy border"
    local target_label = state.target_path ~= "" and state.target_path or "No borders.xml target resolved."
    return {
        mode_label = mode_label,
        dirty_label = dirty_label,
        dirty_color = state.dirty and Common.palette.accent_soft or Common.palette.subtle,
        target_label = "Target: " .. target_label,
        hint_label = save_hint_text(),
        intent_new = Model.save_intent(state.repository, state.draft, state.save_border_id, "new"),
        intent_overwrite = Model.save_intent(state.repository, state.draft, state.save_border_id, "overwrite"),
    }
end

local function soft_refresh_loaded_border()
    if not dlg then
        return
    end

    sync_session_out()
    sync_selected_raw_from_brush("soft_refresh")
    sync_library(state.draft and state.draft.borderId or state.library_selected_border_id, false)
    Logger.log("main", string.format("soft_refresh start draft=%s", state.draft and state.draft.borderId or "nil"))
    state.rebuilding = true
    state.export_preview = Model.prepare_export(state.draft)

    local save_state = save_panel_state()
    local border_title = Common.format_border_title(state.draft)
    local border_status = "Status: " .. Common.border_status(state.draft)
    local slot_snapshot = Common.assignment_snapshot(state.draft)
    local raw_info = Common.safe_item_info(state.selected_raw_id)
    local raw_title = "Najpierw wybierz RAW tile"
    local raw_meta = "Wybrany RAW z glownej palety RME pojawi sie tutaj automatycznie."
    if raw_info then
        raw_title = raw_info.name or ("Item " .. tostring(raw_info.id))
        raw_meta = string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0)
    end
    Logger.log("main", string.format("soft_refresh draft_snapshot border_id=%s %s", state.draft and state.draft.borderId or "nil", slot_snapshot))

    local updates = {
        status_banner = {
            text = state.status_message ~= "" and state.status_message or "",
        },
        border_library = {
            items = state.library_items or {},
            selection = tonumber(state.library_selection or 0) or 0,
        },
        library_page_label = {
            text = string.format("Strona %d / %d", tonumber(state.library_page or 1), tonumber(state.library_page_count or 1)),
        },
        current_border_title = {
            text = border_title,
        },
        current_border_status = {
            text = border_status,
        },
        preview_help_label = {
            text = preview_help_text(),
        },
        selected_slot_status_label = {
            text = selected_slot_status_text(),
        },
        slot_summary_label = {
            text = slot_summary_text(),
        },
        usage_status_label = {
            text = DraftAdapter.usage_preview_state(state.draft).statusLabel,
        },
        usage_missing_label = {
            text = "Brakujace sloty: " .. DraftAdapter.usage_preview_state(state.draft).missingLabel,
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
        save_border_id = {
            value = tonumber(state.save_border_id or 0) or 0,
        },
        save_border_name = {
            text = state.save_name or "",
        },
        save_intent_new = {
            text = save_state.intent_new,
        },
        save_intent_overwrite = {
            text = save_state.intent_overwrite,
        },
        slot_preview_grid = {
            items = DraftAdapter.slot_grid_items(state.draft, state.selected_raw_id, { showImages = true }),
            selection = tonumber(state.highlighted_slot_index or 0) or 0,
        },
        selected_raw_preview = {
            image = Common.safe_image(state.selected_raw_id),
            width = 48,
            height = 48,
            smooth = false,
        },
        selected_raw_title = {
            text = raw_title,
        },
        selected_raw_meta = {
            text = raw_meta,
        },
    }

    state.library_skip_changes = 4
    dlg:modify(updates)
    dlg:repaint()
    state.rebuilding = false
    Logger.log("main", "soft_refresh done")
end

local function soft_refresh_save_panel()
    if not dlg then
        return
    end

    local save_state = save_panel_state()
    Logger.log("main", string.format("soft_refresh_save_panel border_id=%d name='%s'", tonumber(state.save_border_id or 0) or 0, tostring(state.save_name or "")))
    dlg:modify({
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
end

local function render_steps()
    local current = Common.current_step(state)
    local steps = {
        "1. Wybierz Border",
        "2. Wybierz RAW",
        "3. Kliknij Sloty",
        "4. Sprawdz Podglad",
        "5. Zapisz",
    }

    dlg:box({
        orient = "horizontal",
        expand = false,
    })
    for index, label in ipairs(steps) do
        local active = index == current
        dlg:panel({
            bgcolor = active and Common.palette.accent or Common.palette.panel_alt,
            padding = 5,
            margin = 2,
            expand = false,
            min_width = 108,
        })
            dlg:label({
                text = label,
                fgcolor = active and Common.palette.text or Common.palette.muted,
                font_weight = active and "bold" or "normal",
                align = "center",
            })
        dlg:endpanel()
    end
    dlg:endbox()
end

local function render_page_content()
    state.rebuilding = true
    Logger.log("main", string.format("rebuild start draft=%s dirty=%s selected=%d", state.draft and state.draft.borderId or "nil", tostring(state.dirty), tonumber(state.library_selected_border_id or 0)))
    state.filter_text = state.filter_text or ""
    if not state.repository then
        refresh_repository()
    end
    sync_library()
    state.export_preview = Model.prepare_export(state.draft)

    dlg:panel({
        bgcolor = Common.palette.header,
        padding = 8,
        margin = 4,
        expand = false,
    })
        dlg:label({
            text = "BORDER STUDIO",
            fgcolor = Common.palette.text,
            font_size = 14,
            font_weight = "bold",
            align = "center",
        })
        dlg:newrow()
        render_steps()
        dlg:newrow()
        dlg:label({
            id = "status_banner",
            text = state.status_message ~= "" and state.status_message or "",
            fgcolor = Common.palette.muted,
            align = "center",
        })
    dlg:endpanel()

    dlg:box({
        orient = "horizontal",
        expand = true,
    })
        LibraryPanel.render(dlg, state, actions)
        SlotsPanel.render(dlg, state, actions)
        RawPanel.render(dlg, state, actions)
    dlg:endbox()

    dlg:separator()
    SavePanel.render(dlg, state, actions)
    state.library_skip_changes = 3
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
    Logger.log("main", string.format("set_filter text='%s'", tostring(state.filter_text)))
    sync_library(0, false)
    if dlg then
        soft_refresh_library()
    else
        rebuild()
    end
end

function actions.previous_library_page()
    if state.library_page <= 1 then
        return
    end
    state.library_page = state.library_page - 1
    Logger.log("main", string.format("previous_library_page page=%d", tonumber(state.library_page or 1)))
    sync_library(0, false)
    soft_refresh_library()
end

function actions.next_library_page()
    if state.library_page >= state.library_page_count then
        return
    end
    state.library_page = state.library_page + 1
    Logger.log("main", string.format("next_library_page page=%d", tonumber(state.library_page or 1)))
    sync_library(0, false)
    soft_refresh_library()
end

function actions.set_save_border_id(border_id)
    if state.rebuilding then
        return
    end
    state.save_border_id = tonumber(border_id or 0) or 0
    Logger.log("main", string.format("set_save_border_id value=%d", state.save_border_id))
    soft_refresh_save_panel()
end

function actions.set_save_name(name)
    if state.rebuilding then
        return
    end
    state.save_name = tostring(name or "")
    Logger.log("main", string.format("set_save_name value='%s'", state.save_name))
    soft_refresh_save_panel()
end

function actions.set_library_selection(index)
    if state.rebuilding then
        return
    end

    index = tonumber(index or 0) or 0
    state.library_selection = index
    state.library_selected_border_id = state.library_visible_ids[index] or 0
    Logger.log("main", string.format("set_library_selection index=%d resolved_border=%d", index, tonumber(state.library_selected_border_id or 0)))
end

function actions.handle_library_change(index)
    index = tonumber(index or 0) or 0
    if index <= 0 then
        Logger.log("main", "handle_library_change ignored empty selection")
        return
    end
    if state.library_skip_changes > 0 then
        state.library_skip_changes = state.library_skip_changes - 1
        Logger.log("main", string.format("handle_library_change ignored programmatic index=%d remaining=%d", index, state.library_skip_changes))
        return
    end
    if state.library_ignore_budget > 0 and index == state.library_ignore_index then
        state.library_ignore_budget = state.library_ignore_budget - 1
        Logger.log("main", string.format("handle_library_change ignored stale index=%d remaining=%d", index, state.library_ignore_budget))
        return
    end

    actions.set_library_selection(index)
    Logger.log("main", string.format("handle_library_change open index=%d border_id=%d", index, tonumber(state.library_selected_border_id or 0)))

    local border_id = state.library_selected_border_id
    if border_id and border_id > 0 then
        actions.open_border(border_id)
    end
end

function actions.handle_library_leftclick(index)
    index = tonumber(index or 0) or 0
    if index <= 0 then
        Logger.log("main", "handle_library_leftclick ignored empty index")
        return
    end
    Logger.log("main", string.format("handle_library_leftclick open index=%d", index))
    actions.open_visible_border(index)
end

function actions.handle_library_doubleclick(index)
    index = tonumber(index or 0) or 0
    if index <= 0 then
        Logger.log("main", "handle_library_doubleclick ignored empty index")
        return
    end
    Logger.log("main", string.format("handle_library_doubleclick open index=%d", index))
    actions.open_visible_border(index)
end

function actions.open_border(border_id)
    border_id = tonumber(border_id or 0) or 0
    Logger.log("main", string.format("open_border request border_id=%d", border_id))
    if border_id <= 0 then
        Logger.log("main", "open_border ignored invalid id")
        return
    end
    if state.draft and state.draft.borderId == border_id and not state.dirty then
        Logger.log("main", string.format("open_border skipped same border_id=%d", border_id))
        return
    end

    if not confirm_discard() then
        Logger.log("main", string.format("open_border cancelled by discard dialog border_id=%d", border_id))
        return
    end

    local loaded = Model.load_draft(state.repository, border_id)
    if not loaded then
        Logger.log("main", string.format("open_border failed missing draft border_id=%d", border_id))
        app.alert("This border could not be loaded from the current repository.")
        return
    end

    Logger.log("main", string.format("open_border loaded border_id=%d assigned=%d", loaded.borderId or 0, Common.assigned_slot_count(loaded)))
    Logger.log("main", string.format("open_border snapshot border_id=%d %s", loaded.borderId or 0, Common.assignment_snapshot(loaded)))
    state.draft = loaded
    state.draft.status = Common.border_status(state.draft)
    state.library_selected_border_id = state.draft.borderId
    state.save_border_id = state.draft.borderId
    state.save_name = state.draft.name or ""
    state.export_checked = false
    state.status_message = string.format("Zaladowano border #%d do edycji.", state.draft.borderId)
    state.dirty = false
    state.highlighted_slot_index = 0
    Logger.log("main", string.format("open_border before soft refresh border_id=%d", state.draft.borderId))
    soft_refresh_loaded_border()
end

function actions.open_visible_border(index)
    index = tonumber(index or 0) or 0
    Logger.log("main", string.format("open_visible_border index=%d visible_count=%d current_selected=%d", index, #state.library_visible_ids, tonumber(state.library_selected_border_id or 0)))
    actions.set_library_selection(index)
    local border_id = state.library_visible_ids[index]
    if border_id then
        state.library_selected_border_id = border_id
        local current_value = dlg and dlg.data and dlg.data.border_library or "nil"
        Logger.log("main", string.format("open_visible_border resolved border_id=%d dlg_value=%s", border_id, tostring(current_value)))
        actions.open_border(border_id)
    else
        Logger.log("main", string.format("open_visible_border missing index=%d", index))
    end
end

function actions.new_border()
    Logger.log("main", "new_border")
    if not confirm_discard() then
        return
    end

    state.draft = Model.new_draft(state.repository, state.session_borders)
    state.export_preview = Model.prepare_export(state.draft)
    state.export_checked = false
    state.library_selected_border_id = 0
    state.save_border_id = state.draft.borderId
    state.save_name = state.draft.name or ""
    state.status_message = string.format("Utworzono nowy draft bordera #%d.", state.draft.borderId)
    state.dirty = false
    state.highlighted_slot_index = 0
    state.library_ignore_index = 0
    state.library_ignore_budget = 0
    rebuild()
end

function actions.duplicate_current()
    Logger.log("main", "duplicate_current")
    if not state.draft then
        state.draft = Model.new_draft(state.repository, state.session_borders)
    else
        state.draft = Model.duplicate_draft(state.repository, state.session_borders, state.draft)
    end

    state.export_preview = Model.prepare_export(state.draft)
    state.export_checked = false
    state.library_selected_border_id = 0
    state.save_border_id = state.draft.borderId
    state.save_name = state.draft.name or ""
    state.status_message = string.format("Zduplikowano border do nowego draftu #%d.", state.draft.borderId)
    state.dirty = true
    state.highlighted_slot_index = 0
    state.library_ignore_index = 0
    state.library_ignore_budget = 0
    rebuild()
end

function actions.set_raw(item_id)
    item_id = tonumber(item_id or 0) or 0
    Logger.log("main", string.format("set_raw item_id=%d", item_id))
    state.selected_raw_id = item_id
    state.recent_raw_ids = Model.touch_recent(state.recent_raw_ids, item_id)
    if dlg then
        soft_refresh_loaded_border()
    else
        rebuild()
    end
end

function actions.clear_raw()
    Logger.log("main", "clear_raw")
    state.selected_raw_id = 0
    if dlg then
        soft_refresh_loaded_border()
    else
        rebuild()
    end
end

function actions.pick_recent(index)
    local item_id = state.recent_raw_ids[index]
    if item_id then
        actions.set_raw(item_id)
    end
end

function actions.assign_layout_cell(index)
    Logger.log("main", string.format("assign_layout_cell index=%s", tostring(index)))
    local key = DraftAdapter.slot_name_from_layout(index)
    local visual_key = DraftAdapter.visual_slot_name_from_layout(index)
    state.highlighted_slot_index = tonumber(index or 0) or 0
    if not key or not state.draft then
        Logger.log("main", string.format("assign_layout_cell ignored index=%s key=%s has_draft=%s", tostring(index), tostring(key), tostring(state.draft ~= nil)))
        return
    end

    local raw_id = current_raw_id()
    Logger.log("main", string.format("assign_layout_cell visual=%s stored=%s raw_id=%d selected=%d active_brush=%d", tostring(visual_key), tostring(key), raw_id, tonumber(state.selected_raw_id or 0) or 0, Common.active_raw_brush_id()))
    if raw_id <= 0 then
        state.status_message = "Najpierw wybierz RAW tile."
        soft_refresh_loaded_border()
        return
    end

    if raw_id ~= (tonumber(state.selected_raw_id or 0) or 0) then
        state.selected_raw_id = raw_id
        state.recent_raw_ids = Model.touch_recent(state.recent_raw_ids, raw_id)
    end

    local ok, message = state.draft:assignSlot(key, Model.make_raw_reference(raw_id))
    if not ok then
        Logger.log("main", string.format("assign_layout_cell failed key=%s message=%s", tostring(key), tostring(message)))
        app.alert(message)
        return
    end

    state.draft.status = Common.border_status(state.draft)
    state.recent_raw_ids = Model.touch_recent(state.recent_raw_ids, raw_id)
    state.export_checked = false
    state.status_message = string.format("Przypisano RAW %d do slotu %s.", raw_id, string.upper(visual_key or key))
    state.dirty = true
    Logger.log("main", string.format("assign_layout_cell success visual=%s stored=%s raw_id=%d", tostring(visual_key), tostring(key), raw_id))
    soft_refresh_loaded_border()
end

function actions.clear_layout_cell(index)
    Logger.log("main", string.format("clear_layout_cell index=%s", tostring(index)))
    local key = DraftAdapter.slot_name_from_layout(index)
    local visual_key = DraftAdapter.visual_slot_name_from_layout(index)
    state.highlighted_slot_index = tonumber(index or 0) or 0
    if not key or not state.draft then
        Logger.log("main", string.format("clear_layout_cell ignored index=%s key=%s has_draft=%s", tostring(index), tostring(key), tostring(state.draft ~= nil)))
        return
    end

    state.draft:clearSlot(key)
    state.draft.status = Common.border_status(state.draft)
    state.export_checked = false
    state.status_message = string.format("Wyczyszczono slot %s.", string.upper(visual_key or key))
    state.dirty = true
    Logger.log("main", string.format("clear_layout_cell success visual=%s stored=%s", tostring(visual_key), tostring(key)))
    soft_refresh_loaded_border()
end

function actions.clear_selected_slot()
    local index = tonumber(state.highlighted_slot_index or 0) or 0
    if index <= 0 then
        state.status_message = "Najpierw wybierz slot, ktory chcesz wyczyscic."
        soft_refresh_loaded_border()
        return
    end
    actions.clear_layout_cell(index)
end

function actions.clear_draft()
    if not state.draft then
        return
    end
    if state.draft:isEmpty() then
        state.status_message = "Draft jest juz pusty."
        soft_refresh_loaded_border()
        return
    end

    local choice = app.alert({
        title = "Clear draft?",
        text = "This will clear all assigned border slots in the current draft.",
        buttons = { "Yes", "No" },
    })
    if choice ~= 1 then
        return
    end

    state.draft:clear()
    state.draft.status = Common.border_status(state.draft)
    state.export_checked = false
    state.dirty = true
    state.highlighted_slot_index = 0
    state.status_message = "Wyczyszczono caly draft."
    soft_refresh_loaded_border()
end

local function finalize_save(mode)
    local previous_border_id = state.draft and state.draft.borderId or 0
    local ok, result = Model.save_draft(
        state.repository,
        state.session_borders,
        state.draft,
        state.save_border_id,
        state.save_name,
        mode,
        state.target_path
    )

    if not ok then
        app.alert({
            title = "Save failed",
            text = result,
            buttons = { "OK" },
        })
        return
    end

    if previous_border_id ~= result.draft.borderId and state.session_borders[previous_border_id] and not (state.repository.by_id and state.repository.by_id[previous_border_id]) then
        state.session_borders[previous_border_id] = nil
    end

    state.draft = result.draft
    refresh_repository()
    state.library_selected_border_id = result.draft.borderId
    state.save_border_id = result.draft.borderId
    state.save_name = result.draft.name or ""
    state.export_checked = false
    state.status_message = result.message
    state.dirty = false
    session.lastSavedBorder = result.draft.borderId
    sync_session_out()
    rebuild()
    app.alert({
        title = "Border saved",
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
    local chosen, err = XmlTarget.pick_xml_file("borders.xml", state.target_path, "Wybierz borders.xml")
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
    soft_refresh_save_panel()
end

function actions.use_default_target_path()
    state.target_path = Model.resolve_target_path("")
    persist_settings()
    soft_refresh_save_panel()
end

local function attach_brush_listener()
    if listener_id or not app.events or not app.events.on then
        return
    end

    listener_id = app.events:on("brushChange", function(brushName)
        Logger.log("main", string.format("brushChange event brush=%s", tostring(brushName)))
        if sync_selected_raw_from_brush("brushChange") and dlg then
            sync_session_out()
            soft_refresh_loaded_border()
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
    state.draft = settings.last_border_id and Model.load_draft(state.repository, settings.last_border_id) or nil
    if not state.draft then
        Logger.log("main", "initial draft missing, creating new")
        state.draft = Model.new_draft(state.repository, state.session_borders)
    end
    state.save_border_id = state.draft.borderId
    state.save_name = state.draft.name or ""
    sync_session_in()
    sync_selected_raw_from_brush("startup")
    sync_session_out()
    Logger.log("main", string.format("initial draft border_id=%d", state.draft.borderId or 0))
end

initialize()

local page = {}

function page.getTitle()
    return "Borders"
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

function BorderStudioPage.create(options)
    return create_page(options)
end

function BorderStudioPage.open_standalone()
    local existing_dialog = _G.BORDER_STUDIO_DIALOG
    if existing_dialog then
        pcall(function()
            existing_dialog:close()
        end)
        _G.BORDER_STUDIO_DIALOG = nil
        _G.BORDER_STUDIO_OPEN = false
        app.yield()
    end

    local page = create_page({
        reset_logger = true,
    })

    local dlg = Dialog({
        title = "Border Studio",
        id = "border_studio_dock",
        width = 1480,
        height = 860,
        resizable = true,
        dockable = true,
        onclose = function()
            Logger.log("main", "dialog onclose")
            page.onLeave({})
            _G.BORDER_STUDIO_OPEN = false
            _G.BORDER_STUDIO_DIALOG = nil
        end,
    })

    _G.BORDER_STUDIO_OPEN = true
    _G.BORDER_STUDIO_DIALOG = dlg
    page.onEnter({})
    dlg:clear()
    page.render_into(dlg)
    dlg:layout()
    dlg:repaint()
    Logger.log("main", "show dialog")
    dlg:show({ wait = false, center = "screen" })
    return page
end

return BorderStudioPage
