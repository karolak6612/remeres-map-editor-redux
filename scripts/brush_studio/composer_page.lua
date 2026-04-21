local Paths = dofile("module_paths.lua")
local GroundCommon = Paths.load("ground_studio/common.lua")
local BorderCommon = Paths.load("border_studio/common.lua")
local Logger = dofile("logger.lua")
local ComposerDraft = dofile("composer_draft.lua")
local ComposerRepository = dofile("composer_repository.lua")
local XmlTarget = dofile("xml_target_helper.lua")

local ComposerPage = {}
local SETTINGS_STORE = app.storage("composer_settings.json")

local palette = {
    header = "#233246",
    panel = "#243447",
    panel_alt = "#2C3E50",
    sidebar = "#26282d",
    accent = "#8B5CF6",
    success = "#2F855A",
    warning = "#D97706",
    text = "#F8FAFC",
    muted = "#CBD5E1",
    subtle = "#94A3B8",
    danger = "#C53030",
}

local preview_slot_map = {
    n = "s",
    e = "w",
    s = "n",
    w = "e",
    cnw = "cse",
    cne = "csw",
    csw = "cne",
    cse = "cnw",
    dnw = "dse",
    dne = "dsw",
    dsw = "dne",
    dse = "dnw",
}

local GROUND_PAGE_SIZE = 12
local BORDER_PAGE_SIZE = 12
local COMPOSED_PAGE_SIZE = 12

local function create_page(options)
    options = options or {}
    local session = options.session or {}
    local settings = SETTINGS_STORE:load() or {}
    local dlg

    local state = {
        target_path = ComposerRepository.resolve_target_path and ComposerRepository.resolve_target_path(settings.target_path or "") or (app.getDataDirectory() .. "/1310/grounds.xml"),
        repository = nil,
        selected_raw_id = 0,
        ground_filter = "",
        border_filter = "",
        composed_filter = "",
        ground_items = {},
        ground_names = {},
        ground_selection = 0,
        ground_page = 1,
        ground_skip_changes = 0,
        last_ground_open_name = "",
        border_items = {},
        border_ids = {},
        border_selection = 0,
        border_page = 1,
        border_skip_changes = 0,
        last_border_open_id = 0,
        composed_items = {},
        composed_names = {},
        composed_selection = 0,
        composed_page = 1,
        composed_skip_changes = 0,
        last_composed_open_name = "",
        friend_input = "",
        friend_selection = 0,
        source_stage = "ground",
        draft = ComposerDraft.new({
            align = "outer",
            isNew = true,
        }),
        status_message = "Najpierw wybierz ground i border.",
        dirty = false,
    }

    local actions = {}
    local page = {}

    local function sync_session_out()
        session.lastSelectedRaw = tonumber(state.selected_raw_id or 0) or 0
        session.pageDirty = session.pageDirty or {}
        session.pageDirty.composer = state.dirty
        if state.draft and state.draft.name ~= "" then
            session.lastComposerName = state.draft.name
        end
    end

    local function sync_session_in()
        local shared_raw = tonumber(session.lastSelectedRaw or 0) or 0
        if shared_raw > 0 then
            state.selected_raw_id = shared_raw
        else
            state.selected_raw_id = GroundCommon.active_raw_brush_id()
        end
    end

    local function persist_settings()
        SETTINGS_STORE:save({
            target_path = state.target_path or "",
        })
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
            text = "The current composer draft has unsaved changes. Continue and discard them?",
            buttons = { "Yes", "No" },
        })
        return choice == 1
    end

    local function current_ground_ref()
        return state.draft and state.draft.selectedGround or nil
    end

    local function current_border_ref()
        return state.draft and state.draft.selectedBorder or nil
    end

    local function current_ground_name()
        local ref = current_ground_ref()
        return ref and tostring(ref.name or "") or ""
    end

    local function current_border_id()
        local ref = current_border_ref()
        return ref and (tonumber(ref.borderId or 0) or 0) or 0
    end

    local function current_ground_z_order()
        local ref = current_ground_ref()
        if not ref or not ref.draft then
            return 0
        end
        return tonumber(ref.draft.zOrder or 0) or 0
    end

    local function current_composed_name()
        if state.draft and not state.draft.isNew then
            return tostring(state.draft.sourceName or "")
        end
        return ""
    end

    local function effective_source_stage()
        local stage = tostring(state.source_stage or "ground")
        if stage ~= "ground" and stage ~= "border" and stage ~= "existing" then
            stage = "ground"
        end
        if not current_ground_ref() and stage ~= "existing" then
            return "ground"
        end
        if current_ground_ref() and not current_border_ref() and stage ~= "existing" then
            return "border"
        end
        return stage
    end

    local function main_ground_item_id()
        local ground_ref = current_ground_ref()
        return ground_ref and ground_ref.draft and GroundCommon.primary_item_id(ground_ref.draft) or 0
    end

    local function ground_source_preview_item_id(name)
        if not name or name == "" then
            return 0
        end
        local draft = state.repository
            and state.repository.ground_repository
            and state.repository.ground_repository.by_name
            and state.repository.ground_repository.by_name[name]
            or nil
        return GroundCommon.primary_item_id(draft)
    end

    local function border_source_preview_item_id(border_id)
        border_id = tonumber(border_id or 0) or 0
        if border_id <= 0 then
            return 0
        end
        local source = state.repository
            and state.repository.border_repository
            and ComposerRepository.make_border_reference(state.repository, border_id)
            or nil
        return source and source.sampleItemId or 0
    end

    local function composed_source_preview_item_id(name)
        if not name or name == "" then
            return 0
        end
        local draft = state.repository
            and state.repository.composed_by_name
            and state.repository.composed_by_name[name]
            or nil
        local ground_ref = draft and draft.selectedGround or nil
        if ground_ref and tonumber(ground_ref.sampleItemId or 0) > 0 then
            return tonumber(ground_ref.sampleItemId or 0) or 0
        end
        return ground_ref and ground_ref.draft and GroundCommon.primary_item_id(ground_ref.draft) or 0
    end

    local function border_slot_item(slot_name)
        local border_ref = current_border_ref()
        if not border_ref or not border_ref.draft then
            return 0
        end
        return BorderCommon.slot_item_id(border_ref.draft, slot_name)
    end

    local function refresh_repository()
        state.repository = ComposerRepository.read(state.target_path)
        Logger.log("composer", string.format(
            "refresh_repository grounds=%d borders=%d composed=%d warnings=%d",
            #((state.repository and state.repository.ground_repository and state.repository.ground_repository.ordered) or {}),
            #((state.repository and state.repository.border_repository and state.repository.border_repository.ordered) or {}),
            #((state.repository and state.repository.composed_ordered) or {}),
            #((state.repository and state.repository.warnings) or {})
        ))
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

    local function visible_ground_entry(slot)
        local first, last = page_bounds(state.ground_page, #state.ground_names, GROUND_PAGE_SIZE)
        local index = first + ((tonumber(slot or 1) or 1) - 1)
        if index < first or index > last then
            return nil, nil, nil
        end
        return index, state.ground_names[index], state.ground_items[index]
    end

    local function visible_border_entry(slot)
        local first, last = page_bounds(state.border_page, #state.border_ids, BORDER_PAGE_SIZE)
        local index = first + ((tonumber(slot or 1) or 1) - 1)
        if index < first or index > last then
            return nil, nil, nil
        end
        return index, tonumber(state.border_ids[index] or 0) or 0, state.border_items[index]
    end

    local function visible_composed_entry(slot)
        local first, last = page_bounds(state.composed_page, #state.composed_names, COMPOSED_PAGE_SIZE)
        local index = first + ((tonumber(slot or 1) or 1) - 1)
        if index < first or index > last then
            return nil, nil, nil
        end
        return index, state.composed_names[index], state.composed_items[index]
    end

    local function active_stage_page()
        local stage = effective_source_stage()
        if stage == "ground" then
            return tonumber(state.ground_page or 1) or 1, page_count(#state.ground_names, GROUND_PAGE_SIZE)
        elseif stage == "border" then
            return tonumber(state.border_page or 1) or 1, page_count(#state.border_ids, BORDER_PAGE_SIZE)
        end
        return tonumber(state.composed_page or 1) or 1, page_count(#state.composed_names, COMPOSED_PAGE_SIZE)
    end

    local function active_stage_title()
        local stage = effective_source_stage()
        if stage == "ground" then
            return "1. Wybierz Ground", "Ground search"
        elseif stage == "border" then
            return "2. Wybierz Border", "Border search"
        end
        return "3. Otworz gotowy Composer", "Existing composed"
    end

    local function active_stage_filter_text()
        local stage = effective_source_stage()
        if stage == "ground" then
            return state.ground_filter or ""
        elseif stage == "border" then
            return state.border_filter or ""
        end
        return state.composed_filter or ""
    end

    local function active_visible_entry(slot)
        local stage = effective_source_stage()
        if stage == "ground" then
            local _, name, item = visible_ground_entry(slot)
            return {
                text = item and item.text or name or " ",
                fgcolor = name and palette.text or palette.subtle,
                bgcolor = name and (name == current_ground_name() and palette.accent or palette.panel) or palette.sidebar,
                image = GroundCommon.safe_image(ground_source_preview_item_id(name)),
                present = name ~= nil,
            }
        elseif stage == "border" then
            local _, border_id, item = visible_border_entry(slot)
            local present = border_id and border_id > 0
            return {
                text = item and item.text or (present and ("Border #" .. tostring(border_id)) or " "),
                fgcolor = present and palette.text or palette.subtle,
                bgcolor = present and (border_id == current_border_id() and palette.accent or palette.panel) or palette.sidebar,
                image = BorderCommon.safe_image(border_source_preview_item_id(border_id)),
                present = present,
            }
        end
        local _, name, item = visible_composed_entry(slot)
        return {
            text = item and item.text or name or " ",
            fgcolor = name and palette.text or palette.subtle,
            bgcolor = name and (name == current_composed_name() and palette.accent or palette.panel) or palette.sidebar,
            image = GroundCommon.safe_image(composed_source_preview_item_id(name)),
            present = name ~= nil,
        }
    end

    local function sync_sources()
        local previous_ground_name = state.ground_names[tonumber(state.ground_selection or 0) or 0]
        local previous_border_id = state.border_ids[tonumber(state.border_selection or 0) or 0]
        local previous_composed_name = state.composed_names[tonumber(state.composed_selection or 0) or 0]

        state.ground_items, state.ground_names = ComposerRepository.ground_sources(state.repository, state.ground_filter)
        state.border_items, state.border_ids = ComposerRepository.border_sources(state.repository, state.border_filter)
        state.composed_items, state.composed_names = ComposerRepository.composed_sources(state.repository, state.composed_filter)

        state.ground_page = clamp_page(state.ground_page, #state.ground_names, GROUND_PAGE_SIZE)
        state.border_page = clamp_page(state.border_page, #state.border_ids, BORDER_PAGE_SIZE)
        state.composed_page = clamp_page(state.composed_page, #state.composed_names, COMPOSED_PAGE_SIZE)

        state.ground_selection = 0
        if previous_ground_name then
            for index, name in ipairs(state.ground_names) do
                if name == previous_ground_name then
                    state.ground_selection = index
                    break
                end
            end
        end
        if state.ground_selection == 0 and current_ground_ref() then
            for index, name in ipairs(state.ground_names) do
                if name == current_ground_ref().name then
                    state.ground_selection = index
                    break
                end
            end
        end

        state.border_selection = 0
        if previous_border_id then
            for index, border_id in ipairs(state.border_ids) do
                if border_id == previous_border_id then
                    state.border_selection = index
                    break
                end
            end
        end
        if state.border_selection == 0 and current_border_ref() then
            for index, border_id in ipairs(state.border_ids) do
                if border_id == current_border_ref().borderId then
                    state.border_selection = index
                    break
                end
            end
        end

        state.composed_selection = 0
        if previous_composed_name then
            for index, name in ipairs(state.composed_names) do
                if name == previous_composed_name then
                    state.composed_selection = index
                    break
                end
            end
        end
        if state.composed_selection == 0 and not state.draft.isNew and state.draft.sourceName ~= "" then
            for index, name in ipairs(state.composed_names) do
                if name == state.draft.sourceName then
                    state.composed_selection = index
                    break
                end
            end
        end
    end

    local function helper_text()
        if not current_ground_ref() then
            return "1. Wybierz ground. 2. Wybierz border. 3. Ustaw relacje. 4. Sprawdz preview. 5. Zapisz."
        end
        if not current_border_ref() then
            return "Border jest jeszcze pusty. Wybierz border, aby zobaczyc przejscia."
        end
        if state.draft.align == "outer" then
            return "Tryb OUTER rysuje border na sasiednim tile. Z-order tego grounda musi byc wyzszy niz otoczenie."
        end
        if GroundCommon.trim(state.draft.name or "") == "" then
            return "Podaj nazwe, aby zapisac brush."
        end
        return "Composer pokazuje praktyczna symulacje polaczenia ground + border."
    end

    local function preview_status_text()
        if not current_ground_ref() then
            return "Najpierw wybierz ground."
        end
        if not current_border_ref() then
            return "Najpierw wybierz border."
        end
        return string.format(
            "Ground '%s' laczy sie z borderem #%d (%s).",
            tostring(current_ground_ref().name or "?"),
            tonumber(current_border_ref().borderId or 0) or 0,
            tostring(state.draft.align or "outer")
        )
    end

    local function save_hint_text()
        local validation = state.draft:getValidation()
        if not validation.isValid then
            return validation.errors[1] or "Uzupelnij draft."
        end
        if state.draft.align == "outer" and current_ground_z_order() <= 1 then
            return "OUTER zwykle wymaga wyzszego z-order niz otoczenie. Ustaw z-order > 1 albo uzyj preset OUTER."
        end
        return "Utworz nowy brush albo nadpisz istniejacy wpis w grounds.xml."
    end

    local function selected_ground_meta()
        local ref = current_ground_ref()
        if not ref then
            return "Brak wybranego grounda"
        end
        local item_id = ref.sampleItemId or 0
        return string.format("Main item %d  |  z-order %d%s", item_id, tonumber(ref.draft and ref.draft.zOrder or 0) or 0, ref.embedded and "  |  embedded source" or "")
    end

    local function selected_border_meta()
        local ref = current_border_ref()
        if not ref then
            return "Brak wybranego bordera"
        end
        return string.format("Border #%d", tonumber(ref.borderId or 0) or 0)
    end

    local function composer_preview_items()
        local items = {}
        local center_item_id = main_ground_item_id()

        for index = 1, BorderCommon.slot_layout_count do
            local key = BorderCommon.slot_layout[index]
            if not key then
                table.insert(items, {
                    text = "",
                    tooltip = "",
                })
            else
                local meta = BorderCommon.slot_meta[key]
                local item_id = 0
                local tooltip = meta and meta.full or ""

                if key == "c" then
                    item_id = center_item_id
                    tooltip = center_item_id > 0
                        and string.format("Main Ground\nItem ID %d", center_item_id)
                        or "Main Ground\nNajpierw wybierz ground"
                else
                    local preview_key = preview_slot_map[key] or key
                    item_id = border_slot_item(preview_key)
                    if item_id > 0 then
                        tooltip = string.format("%s\nPreview uses slot %s\nItem ID %d", meta.full, string.upper(preview_key), item_id)
                    else
                        tooltip = string.format("%s\nBrak tile w borderze", meta.full)
                    end
                end

                table.insert(items, {
                    text = meta and meta.short or "",
                    tooltip = tooltip,
                    image = GroundCommon.safe_image(item_id),
                })
            end
        end

        return items
    end

    local function soft_refresh_full()
        sync_session_in()
        sync_session_out()
        if request_host_render() then
            return
        end
    end

    local function soft_refresh_local()
        return false
    end

    local function refresh_source_lists()
        if not dlg then
            return
        end

        local updates = {}
        local stage = effective_source_stage()
        local current_page, total_pages = active_stage_page()
        local title, search_label = active_stage_title()

        updates.composer_stage_ground_button = {
            bgcolor = stage == "ground" and palette.accent or palette.panel,
        }
        updates.composer_stage_border_button = {
            bgcolor = stage == "border" and palette.accent or palette.panel,
        }
        updates.composer_stage_existing_button = {
            bgcolor = stage == "existing" and palette.accent or palette.panel,
        }
        updates.composer_picker_title = {
            text = title,
        }
        updates.composer_picker_search_label = {
            text = search_label,
        }
        updates.composer_source_filter = {
            text = active_stage_filter_text(),
        }
        updates.composer_source_prev_button = {
            bgcolor = current_page > 1 and palette.panel or palette.sidebar,
        }
        updates.composer_source_page_label = {
            text = string.format("Strona %d / %d", current_page, total_pages),
        }
        updates.composer_source_next_button = {
            bgcolor = current_page < total_pages and palette.panel or palette.sidebar,
        }

        for slot = 1, math.max(GROUND_PAGE_SIZE, BORDER_PAGE_SIZE, COMPOSED_PAGE_SIZE) do
            local entry = active_visible_entry(slot)
            updates["composer_source_icon_" .. tostring(slot)] = {
                image = entry.image,
                width = 22,
                height = 22,
                smooth = false,
            }
            updates["composer_source_text_" .. tostring(slot)] = {
                text = entry.text,
                fgcolor = entry.fgcolor,
            }
            updates["composer_source_slot_" .. tostring(slot)] = {
                bgcolor = entry.bgcolor,
            }
        end

        dlg:modify(updates)
        dlg:repaint()
    end

    local function soft_refresh(force_full)
        sync_session_in()
        sync_session_out()
        if force_full or not soft_refresh_local() then
            soft_refresh_full()
        end
    end

    local function finalize_save(mode)
        local ok, result = ComposerRepository.save_draft(state.repository, state.draft, mode, state.target_path)
        if not ok then
            app.alert({
                title = "Save failed",
                text = result,
                buttons = { "OK" },
            })
            return
        end

        state.draft = result.draft
        state.draft.dirty = false
        state.dirty = false
        session.lastSavedComposedBrush = result.draft.name
        state.status_message = result.message
        refresh_repository()
        sync_sources()
        soft_refresh(true)
        app.alert({
            title = "Composer saved",
            text = result.message,
            buttons = { "OK" },
        })
    end

    function actions.set_ground_filter(text)
        text = tostring(text or "")
        if text == tostring(state.ground_filter or "") then
            return
        end
        state.ground_filter = text
        state.ground_page = 1
        Logger.log("composer", string.format("set_ground_filter text='%s'", state.ground_filter))
        sync_sources()
        refresh_source_lists()
    end

    function actions.set_border_filter(text)
        text = tostring(text or "")
        if text == tostring(state.border_filter or "") then
            return
        end
        state.border_filter = text
        state.border_page = 1
        Logger.log("composer", string.format("set_border_filter text='%s'", state.border_filter))
        sync_sources()
        refresh_source_lists()
    end

    function actions.set_composed_filter(text)
        text = tostring(text or "")
        if text == tostring(state.composed_filter or "") then
            return
        end
        state.composed_filter = text
        state.composed_page = 1
        Logger.log("composer", string.format("set_composed_filter text='%s'", state.composed_filter))
        sync_sources()
        refresh_source_lists()
    end

    function actions.apply_active_filter(text)
        local stage = effective_source_stage()
        if stage == "ground" then
            actions.set_ground_filter(text)
        elseif stage == "border" then
            actions.set_border_filter(text)
        else
            actions.set_composed_filter(text)
        end
    end

    function actions.clear_active_filter()
        actions.apply_active_filter("")
    end

    function actions.prev_ground_page()
        state.ground_page = math.max(1, (tonumber(state.ground_page or 1) or 1) - 1)
        refresh_source_lists()
    end

    function actions.next_ground_page()
        state.ground_page = clamp_page((tonumber(state.ground_page or 1) or 1) + 1, #state.ground_names, GROUND_PAGE_SIZE)
        refresh_source_lists()
    end

    function actions.prev_border_page()
        state.border_page = math.max(1, (tonumber(state.border_page or 1) or 1) - 1)
        refresh_source_lists()
    end

    function actions.next_border_page()
        state.border_page = clamp_page((tonumber(state.border_page or 1) or 1) + 1, #state.border_ids, BORDER_PAGE_SIZE)
        refresh_source_lists()
    end

    function actions.prev_composed_page()
        state.composed_page = math.max(1, (tonumber(state.composed_page or 1) or 1) - 1)
        refresh_source_lists()
    end

    function actions.next_composed_page()
        state.composed_page = clamp_page((tonumber(state.composed_page or 1) or 1) + 1, #state.composed_names, COMPOSED_PAGE_SIZE)
        refresh_source_lists()
    end

    function actions.prev_active_page()
        local stage = effective_source_stage()
        if stage == "ground" then
            actions.prev_ground_page()
        elseif stage == "border" then
            actions.prev_border_page()
        else
            actions.prev_composed_page()
        end
    end

    function actions.next_active_page()
        local stage = effective_source_stage()
        if stage == "ground" then
            actions.next_ground_page()
        elseif stage == "border" then
            actions.next_border_page()
        else
            actions.next_composed_page()
        end
    end

    function actions.open_ground_visible_slot(slot)
        local index, name = visible_ground_entry(slot)
        if not index or not name then
            return
        end
        state.ground_selection = index
        actions.open_selected_ground()
    end

    function actions.open_border_visible_slot(slot)
        local index, border_id = visible_border_entry(slot)
        if not index or border_id <= 0 then
            return
        end
        state.border_selection = index
        actions.open_selected_border()
    end

    function actions.open_composed_visible_slot(slot)
        local index, name = visible_composed_entry(slot)
        if not index or not name then
            return
        end
        state.composed_selection = index
        actions.open_selected_composed()
    end

    function actions.open_active_visible_slot(slot)
        local stage = effective_source_stage()
        if stage == "ground" then
            actions.open_ground_visible_slot(slot)
        elseif stage == "border" then
            actions.open_border_visible_slot(slot)
        else
            actions.open_composed_visible_slot(slot)
        end
    end

    function actions.handle_ground_change(index)
        index = tonumber(index or 0) or 0
        if index <= 0 then
            Logger.log("composer", "handle_ground_change ignored empty index")
            return
        end
        if state.ground_skip_changes > 0 then
            state.ground_skip_changes = state.ground_skip_changes - 1
            Logger.log("composer", string.format("handle_ground_change skipped programmatic selection=%d remaining=%d", index, state.ground_skip_changes))
            return
        end
        state.ground_selection = index
        local name = tostring(state.ground_names[index] or "")
        Logger.log("composer", string.format("handle_ground_change selection=%d name=%s", index, name))
        if name ~= "" and name ~= current_ground_name() then
            state.status_message = string.format("Zaznaczono ground '%s'. Dwuklik lub przycisk otwiera.", name)
        end
    end

    function actions.open_selected_ground()
        local index = tonumber(state.ground_selection or 0) or 0
        local name = state.ground_names[index]
        if not name then
            Logger.log("composer", string.format("open_selected_ground missing selection index=%d", index))
            state.status_message = "Najpierw zaznacz ground na liscie."
            soft_refresh()
            return
        end
        Logger.log("composer", string.format("open_selected_ground index=%d name='%s'", index, name))
        local ref = ComposerRepository.make_ground_reference(state.repository, name)
        if ref then
            state.last_ground_open_name = name
            state.draft:setGround(ref)
            local linked_border, linked_from = ComposerRepository.find_linked_border_for_ground(state.repository, name)
            if linked_border then
                state.draft:setBorder(linked_border)
                state.last_border_open_id = tonumber(linked_border.borderId or 0) or 0
            else
                state.draft:setBorder(nil)
                state.last_border_open_id = 0
            end
            state.ground_selection = 0
            state.border_selection = 0
            state.dirty = true
            if state.draft.name == "" then
                state.draft:setName(name)
            end
            if linked_border then
                state.status_message = string.format(
                    "Wybrano ground '%s' i dolaczono powiazany border z '%s'.",
                    name,
                    tostring(linked_from or name)
                )
            else
                state.status_message = string.format("Wybrano ground '%s'. Wybierz border, jesli nie ma powiazanego.", name)
            end
            state.source_stage = "border"
            sync_sources()
            soft_refresh()
        else
            Logger.log("composer", string.format("open_selected_ground failed missing name='%s'", name))
        end
    end

    function actions.open_ground_by_name(name)
        name = tostring(name or "")
        for index, candidate in ipairs(state.ground_names or {}) do
            if candidate == name then
                state.ground_selection = index
                actions.open_selected_ground()
                return
            end
        end
        Logger.log("composer", string.format("open_ground_by_name missing name='%s'", name))
    end

    function actions.handle_border_change(index)
        index = tonumber(index or 0) or 0
        if index <= 0 then
            Logger.log("composer", "handle_border_change ignored empty index")
            return
        end
        if state.border_skip_changes > 0 then
            state.border_skip_changes = state.border_skip_changes - 1
            Logger.log("composer", string.format("handle_border_change skipped programmatic selection=%d remaining=%d", index, state.border_skip_changes))
            return
        end
        state.border_selection = index
        local border_id = tonumber(state.border_ids[index] or 0) or 0
        Logger.log("composer", string.format("handle_border_change selection=%d border_id=%s", index, tostring(border_id)))
        if border_id > 0 and border_id ~= current_border_id() then
            state.status_message = string.format("Zaznaczono border #%d. Dwuklik lub przycisk otwiera.", border_id)
        end
    end

    function actions.open_selected_border()
        local index = tonumber(state.border_selection or 0) or 0
        local border_id = state.border_ids[index]
        if not border_id then
            Logger.log("composer", string.format("open_selected_border missing selection index=%d", index))
            state.status_message = "Najpierw zaznacz border na liscie."
            soft_refresh()
            return
        end
        Logger.log("composer", string.format("open_selected_border index=%d border_id=%d", index, border_id))
        local ref = ComposerRepository.make_border_reference(state.repository, border_id)
        if ref then
            state.last_border_open_id = border_id
            state.draft:setBorder(ref)
            state.border_selection = 0
            state.dirty = true
            state.status_message = string.format("Wybrano border #%d.", border_id)
            state.source_stage = "border"
            sync_sources()
            soft_refresh()
        else
            Logger.log("composer", string.format("open_selected_border failed missing border_id=%d", border_id))
        end
    end

    function actions.open_border_by_id(border_id)
        border_id = tonumber(border_id or 0) or 0
        for index, candidate in ipairs(state.border_ids or {}) do
            if tonumber(candidate or 0) == border_id then
                state.border_selection = index
                actions.open_selected_border()
                return
            end
        end
        Logger.log("composer", string.format("open_border_by_id missing border_id=%d", border_id))
    end

    function actions.handle_composed_change(index)
        index = tonumber(index or 0) or 0
        if index <= 0 then
            Logger.log("composer", "handle_composed_change ignored empty index")
            return
        end
        if state.composed_skip_changes > 0 then
            state.composed_skip_changes = state.composed_skip_changes - 1
            Logger.log("composer", string.format("handle_composed_change skipped programmatic selection=%d remaining=%d", index, state.composed_skip_changes))
            return
        end
        state.composed_selection = index
        local name = tostring(state.composed_names[index] or "")
        Logger.log("composer", string.format("handle_composed_change selection=%d name=%s", index, name))
        if name ~= "" and name ~= current_composed_name() then
            state.status_message = string.format("Zaznaczono composed brush '%s'. Dwuklik lub przycisk otwiera.", name)
        end
    end

    function actions.open_selected_composed()
        local index = tonumber(state.composed_selection or 0) or 0
        local name = state.composed_names[index]
        if not name then
            Logger.log("composer", string.format("open_selected_composed missing selection index=%d", index))
            state.status_message = "Najpierw zaznacz composed brush na liscie."
            soft_refresh()
            return
        end
        Logger.log("composer", string.format("open_selected_composed index=%d name='%s'", index, name))
        if not confirm_discard() then
            sync_sources()
            soft_refresh()
            return
        end
        local draft = ComposerRepository.load_composer(state.repository, name)
        if draft then
            state.last_composed_open_name = name
            state.draft = draft
            state.ground_selection = 0
            state.border_selection = 0
            state.composed_selection = 0
            state.dirty = false
            state.friend_selection = 0
            state.status_message = string.format("Zaladowano composed brush '%s' do edycji.", name)
            state.source_stage = "existing"
            sync_sources()
            soft_refresh()
        else
            Logger.log("composer", string.format("open_selected_composed failed name='%s'", name))
        end
    end

    function actions.open_composed_by_name(name)
        name = tostring(name or "")
        for index, candidate in ipairs(state.composed_names or {}) do
            if candidate == name then
                state.composed_selection = index
                actions.open_selected_composed()
                return
            end
        end
        Logger.log("composer", string.format("open_composed_by_name missing name='%s'", name))
    end

    function actions.new_composer()
        if not confirm_discard() then
            return
        end
        state.draft = ComposerRepository.new_draft()
        state.friend_input = ""
        state.friend_selection = 0
        state.source_stage = "ground"
        state.dirty = false
        state.status_message = "Utworzono nowy composer draft."
        sync_sources()
        soft_refresh()
    end

    function actions.duplicate_current()
        state.draft = ComposerRepository.duplicate_draft(state.repository, state.draft)
        state.dirty = true
        state.source_stage = "ground"
        state.status_message = string.format("Zduplikowano composer do '%s'.", state.draft.name)
        sync_sources()
        soft_refresh()
    end

    function actions.use_last_saved_ground()
        local name = GroundCommon.trim(session.lastSavedGround or "")
        if name == "" then
            state.status_message = "Brak ostatnio zapisanego grounda."
            soft_refresh()
            return
        end
        local ref = ComposerRepository.make_ground_reference(state.repository, name)
        if not ref then
            state.status_message = "Ostatnio zapisany ground nie jest dostepny w bibliotece."
            soft_refresh()
            return
        end
        state.draft:setGround(ref)
        local linked_border, linked_from = ComposerRepository.find_linked_border_for_ground(state.repository, name)
        if linked_border then
            state.draft:setBorder(linked_border)
        else
            state.draft:setBorder(nil)
        end
        state.dirty = true
        if state.draft.name == "" then
            state.draft:setName(name)
        end
        if linked_border then
            state.status_message = string.format(
                "Uzyto ostatnio zapisanego grounda '%s' i dolaczono border z '%s'.",
                name,
                tostring(linked_from or name)
            )
        else
            state.status_message = string.format("Uzyto ostatnio zapisanego grounda '%s'. Wybierz border, jesli nie ma powiazanego.", name)
        end
        state.source_stage = "border"
        sync_sources()
        soft_refresh()
    end

    function actions.use_last_saved_border()
        local border_id = tonumber(session.lastSavedBorder or 0) or 0
        if border_id <= 0 then
            state.status_message = "Brak ostatnio zapisanego bordera."
            soft_refresh()
            return
        end
        local ref = ComposerRepository.make_border_reference(state.repository, border_id)
        if not ref then
            state.status_message = "Ostatnio zapisany border nie jest dostepny w bibliotece."
            soft_refresh()
            return
        end
        state.draft:setBorder(ref)
        state.dirty = true
        state.status_message = string.format("Uzyto ostatnio zapisanego bordera #%d.", border_id)
        state.source_stage = "border"
        sync_sources()
        soft_refresh()
    end

    function actions.show_ground_stage()
        state.source_stage = "ground"
        refresh_source_lists()
    end

    function actions.show_border_stage()
        state.source_stage = "border"
        refresh_source_lists()
    end

    function actions.show_existing_stage()
        state.source_stage = "existing"
        refresh_source_lists()
    end

    function actions.set_name(value)
        state.draft:setName(value)
        state.dirty = true
        sync_session_out()
    end

    function actions.set_to_brush(value)
        state.draft:setToBrush(value)
        state.dirty = true
        sync_session_out()
    end

    function actions.set_align(value)
        state.draft:setAlign(value)
        state.dirty = true
        soft_refresh()
    end

    function actions.set_ground_z_order(value)
        local ref = current_ground_ref()
        if not ref or not ref.draft or not ref.draft.setZOrder then
            return
        end
        ref.draft:setZOrder(value)
        state.dirty = true
        sync_session_out()
    end

    function actions.apply_outer_preset()
        state.draft:setAlign("outer")
        local ref = current_ground_ref()
        if ref and ref.draft and ref.draft.setZOrder then
            local current = tonumber(ref.draft.zOrder or 0) or 0
            if current <= 1 then
                ref.draft:setZOrder(2)
            end
        end
        state.dirty = true
        state.status_message = "Preset OUTER ustawiony. Border bedzie rysowany na zewnatrz, jesli z-order przewaza nad otoczeniem."
        soft_refresh()
    end

    function actions.apply_inner_preset()
        state.draft:setAlign("inner")
        state.dirty = true
        state.status_message = "Preset INNER ustawiony. Border bedzie rysowany po stronie aktualnego brusha."
        soft_refresh()
    end

    function actions.set_friend_input(value)
        state.friend_input = tostring(value or "")
    end

    function actions.add_friend()
        local ok, message = state.draft:addFriend(state.friend_input)
        state.status_message = ok and string.format("Dodano friend '%s'.", GroundCommon.trim(state.friend_input or "")) or message
        if ok then
            state.friend_input = ""
            state.dirty = true
        end
        soft_refresh()
    end

    function actions.select_friend(index)
        state.friend_selection = tonumber(index or 0) or 0
        soft_refresh()
    end

    function actions.remove_selected_friend()
        if state.draft:removeFriend(state.friend_selection) then
            state.friend_selection = 0
            state.dirty = true
            state.status_message = "Usunieto friend."
        else
            state.status_message = "Najpierw wybierz friend do usuniecia."
        end
        soft_refresh()
    end

    function actions.save_as_new()
        finalize_save("new")
    end

    function actions.overwrite_existing()
        finalize_save("overwrite")
    end

    function actions.choose_target_path()
        local chosen, err = XmlTarget.pick_xml_file("grounds.xml", state.target_path, "Wybierz grounds.xml dla Composera")
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
        sync_sources()
        soft_refresh()
    end

    function actions.use_default_target_path()
        state.target_path = ComposerRepository.resolve_target_path and ComposerRepository.resolve_target_path("") or (app.getDataDirectory() .. "/1310/grounds.xml")
        persist_settings()
        refresh_repository()
        sync_sources()
        soft_refresh()
    end

    refresh_repository()
    sync_session_in()
    sync_sources()
    if state.repository.api_missing then
        state.status_message = "Brak API do odczytu grounds.xml: " .. tostring(state.repository.api_missing)
    elseif #(state.repository.warnings or {}) > 0 then
        state.status_message = state.repository.warnings[1]
    end

    function page.getTitle()
        return "Composer"
    end

    function page.isDirty()
        return state.dirty
    end

    function page.confirmLeave()
        return confirm_discard()
    end

    function page.onEnter(shared_session)
        session = shared_session or session
        Logger.log("composer", "page onEnter")
        refresh_repository()
        sync_sources()
        sync_session_in()
        sync_session_out()
    end

    function page.onLeave(shared_session)
        session = shared_session or session
        sync_session_out()
        persist_settings()
    end

    function page.render_into(dialog)
        dlg = dialog
        sync_sources()
        local source_stage = effective_source_stage()
        local current_ground = current_ground_ref()
        local current_border = current_border_ref()
        local ground_first, ground_last, ground_pages = page_bounds(state.ground_page, #state.ground_names, GROUND_PAGE_SIZE)
        local border_first, border_last, border_pages = page_bounds(state.border_page, #state.border_ids, BORDER_PAGE_SIZE)
        local composed_first, composed_last, composed_pages = page_bounds(state.composed_page, #state.composed_names, COMPOSED_PAGE_SIZE)
        local friend_items = {}
        for _, friend_name in ipairs(state.draft.friends or {}) do
            table.insert(friend_items, { text = friend_name })
        end

        dlg:panel({
            bgcolor = palette.header,
            padding = 10,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "COMPOSER",
                fgcolor = palette.text,
                font_size = 14,
                font_weight = "bold",
                align = "center",
            })
            dlg:newrow()
            dlg:label({
                id = "composer_helper_text",
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
                id = "composer_status_banner",
                text = state.status_message ~= "" and state.status_message or helper_text(),
                fgcolor = palette.text,
            })
        dlg:endpanel()

        dlg:box({
            orient = "horizontal",
            expand = true,
        })
            dlg:box({
                label = "1-2. Zrodla",
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
                        text = "Wybrane zrodla",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = true,
                    })
                        dlg:panel({
                            bgcolor = palette.panel,
                            padding = 6,
                            margin = 2,
                            min_width = 150,
                            expand = true,
                        })
                            dlg:label({
                                text = "Ground",
                                fgcolor = palette.text,
                                font_weight = "bold",
                            })
                            dlg:newrow()
                            dlg:label({
                                id = "composer_selected_ground_title",
                                text = current_ground and current_ground.title or "Brak wybranego grounda",
                                fgcolor = palette.text,
                            })
                            dlg:newrow()
                            dlg:label({
                                id = "composer_selected_ground_meta",
                                text = selected_ground_meta(),
                                fgcolor = palette.muted,
                                font_size = 8,
                            })
                        dlg:endpanel()
                        dlg:panel({
                            bgcolor = palette.panel,
                            padding = 6,
                            margin = 2,
                            min_width = 150,
                            expand = true,
                        })
                            dlg:label({
                                text = "Border",
                                fgcolor = palette.text,
                                font_weight = "bold",
                            })
                            dlg:newrow()
                            dlg:label({
                                id = "composer_selected_border_title",
                                text = current_border and current_border.title or "Brak wybranego bordera",
                                fgcolor = palette.text,
                            })
                            dlg:newrow()
                            dlg:label({
                                id = "composer_selected_border_meta",
                                text = selected_border_meta(),
                                fgcolor = palette.muted,
                                font_size = 8,
                            })
                        dlg:endpanel()
                    dlg:endbox()
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = false,
                    })
                        dlg:button({
                            text = "Use last saved ground",
                            bgcolor = palette.accent,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.use_last_saved_ground()
                            end,
                        })
                        dlg:button({
                            text = "Use last saved border",
                            bgcolor = palette.accent,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.use_last_saved_border()
                            end,
                        })
                    dlg:endbox()
                dlg:endpanel()

                dlg:box({
                    orient = "horizontal",
                    expand = false,
                })
                    dlg:button({
                        id = "composer_stage_ground_button",
                        text = "Ground",
                        bgcolor = source_stage == "ground" and palette.accent or palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.show_ground_stage()
                        end,
                    })
                    dlg:button({
                        id = "composer_stage_border_button",
                        text = "Border",
                        bgcolor = source_stage == "border" and palette.accent or palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.show_border_stage()
                        end,
                    })
                    dlg:button({
                        id = "composer_stage_existing_button",
                        text = "Existing",
                        bgcolor = source_stage == "existing" and palette.accent or palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.show_existing_stage()
                        end,
                    })
                dlg:endbox()
                dlg:newrow()
                dlg:label({
                    id = "composer_picker_title",
                    text = active_stage_title(),
                    fgcolor = palette.text,
                    font_weight = "bold",
                })
                dlg:newrow()
                dlg:box({
                    orient = "horizontal",
                    expand = true,
                })
                    dlg:label({
                        id = "composer_picker_search_label",
                        text = select(2, active_stage_title()),
                        fgcolor = palette.text,
                        min_width = 110,
                    })
                    dlg:input({
                        id = "composer_source_filter",
                        text = active_stage_filter_text(),
                        expand = true,
                    })
                    dlg:button({
                        text = "Search",
                        bgcolor = palette.panel,
                        fgcolor = palette.text,
                        onclick = function(d)
                            actions.apply_active_filter(d.data.composer_source_filter or "")
                        end,
                    })
                    dlg:button({
                        text = "Clear",
                        bgcolor = palette.sidebar,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.clear_active_filter()
                        end,
                    })
                dlg:endbox()
                dlg:newrow()
                dlg:box({
                    orient = "horizontal",
                    expand = false,
                })
                    dlg:button({
                        id = "composer_source_prev_button",
                        text = "<",
                        bgcolor = palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.prev_active_page()
                        end,
                    })
                    dlg:label({
                        id = "composer_source_page_label",
                        text = string.format("Strona %d / %d", select(1, active_stage_page()), select(2, active_stage_page())),
                        fgcolor = palette.muted,
                    })
                    dlg:button({
                        id = "composer_source_next_button",
                        text = ">",
                        bgcolor = palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.next_active_page()
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
                    for slot = 1, math.max(GROUND_PAGE_SIZE, BORDER_PAGE_SIZE, COMPOSED_PAGE_SIZE) do
                        local entry = active_visible_entry(slot)
                        dlg:box({
                            orient = "horizontal",
                            expand = false,
                        })
                            dlg:image({
                                id = "composer_source_icon_" .. tostring(slot),
                                image = entry.image,
                                width = 22,
                                height = 22,
                                smooth = false,
                            })
                            dlg:panel({
                                bgcolor = palette.sidebar,
                                padding = 0,
                                margin = 0,
                                min_width = 240,
                                max_width = 240,
                                expand = false,
                            })
                                dlg:label({
                                    id = "composer_source_text_" .. tostring(slot),
                                    text = entry.text,
                                    fgcolor = entry.fgcolor,
                                    min_width = 240,
                                    max_width = 240,
                                })
                            dlg:endpanel()
                            dlg:button({
                                id = "composer_source_slot_" .. tostring(slot),
                                text = "Open",
                                bgcolor = entry.bgcolor,
                                fgcolor = palette.text,
                                min_width = 80,
                                max_width = 80,
                                onclick = function()
                                    actions.open_active_visible_slot(slot)
                                end,
                            })
                        dlg:endbox()
                        dlg:newrow()
                    end
                dlg:endpanel()
                dlg:newrow()
                dlg:box({
                    orient = "horizontal",
                    expand = false,
                })
                    dlg:button({
                        text = "New Composer",
                        bgcolor = palette.panel_alt,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.new_composer()
                        end,
                    })
                    dlg:button({
                        text = "Duplicate Composer",
                        bgcolor = palette.panel_alt,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.duplicate_current()
                        end,
                    })
                dlg:endbox()
            dlg:endbox()

            dlg:box({
                label = "3-4. Live Preview",
                orient = "vertical",
                min_width = 540,
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
                        text = "Glowny podglad",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "composer_preview_status",
                        text = preview_status_text(),
                        fgcolor = palette.muted,
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "composer_preview_ground_label",
                        text = current_ground and ("Ground: " .. tostring(current_ground.name or "")) or "Najpierw wybierz ground",
                        fgcolor = palette.subtle,
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "composer_preview_border_label",
                        text = current_border and ("Border: " .. tostring(current_border.title or "")) or "Najpierw wybierz border",
                        fgcolor = palette.subtle,
                    })
                dlg:endpanel()

                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 10,
                    margin = 4,
                    expand = true,
                })
                    dlg:label({
                        text = "Uklad 5x5",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:grid({
                        id = "composer_preview_grid",
                        width = 320,
                        height = 320,
                        expand = false,
                        icon_size = 42,
                        cell_size = 62,
                        show_text = true,
                        label_wrap = false,
                        items = composer_preview_items(),
                    })
                dlg:endpanel()
            dlg:endbox()

            dlg:box({
                label = "5. Relacje i Save",
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
                        text = "Wlasciwosci",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:input({
                        id = "composer_name_input",
                        label = "Brush name",
                        text = state.draft.name or "",
                        expand = true,
                        onchange = function(d)
                            actions.set_name(d.data.composer_name_input or "")
                        end,
                    })
                    dlg:newrow()
                    dlg:input({
                        id = "composer_to_brush_input",
                        label = "To brush",
                        text = state.draft.toBrush or "",
                        expand = true,
                        onchange = function(d)
                            actions.set_to_brush(d.data.composer_to_brush_input or "")
                        end,
                    })
                    dlg:newrow()
                    dlg:input({
                        id = "composer_ground_z_order_input",
                        label = "Ground z-order",
                        text = tostring(current_ground_z_order()),
                        expand = true,
                        onchange = function(d)
                            actions.set_ground_z_order(d.data.composer_ground_z_order_input or 0)
                        end,
                    })
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = false,
                    })
                        dlg:button({
                            id = "composer_align_outer",
                            text = state.draft.align == "outer" and "Align: OUTER" or "Align Outer",
                            bgcolor = state.draft.align == "outer" and palette.accent or palette.panel,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.set_align("outer")
                            end,
                        })
                        dlg:button({
                            id = "composer_align_inner",
                            text = state.draft.align == "inner" and "Align: INNER" or "Align Inner",
                            bgcolor = state.draft.align == "inner" and palette.accent or palette.panel,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.set_align("inner")
                            end,
                        })
                    dlg:endbox()
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = false,
                    })
                        dlg:button({
                            text = "Preset OUTER",
                            bgcolor = palette.panel,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.apply_outer_preset()
                            end,
                        })
                        dlg:button({
                            text = "Preset INNER",
                            bgcolor = palette.panel,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.apply_inner_preset()
                            end,
                        })
                    dlg:endbox()
                    dlg:newrow()
                    dlg:label({
                        text = state.draft.align == "outer"
                            and "OUTER: border na sasiednim tile. Wymaga wyzszego z-order niz otoczenie."
                            or "INNER: border na aktualnym tile, czyli wizualnie do srodka patcha.",
                        fgcolor = palette.subtle,
                    })
                    dlg:newrow()
                    dlg:label({
                        text = "Friends",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:box({
                        orient = "horizontal",
                        expand = true,
                    })
                        dlg:input({
                            id = "composer_friend_input",
                            text = state.friend_input or "",
                            expand = true,
                            onchange = function(d)
                                actions.set_friend_input(d.data.composer_friend_input or "")
                            end,
                        })
                        dlg:button({
                            text = "Dodaj",
                            bgcolor = palette.accent,
                            fgcolor = palette.text,
                            onclick = function()
                                actions.add_friend()
                            end,
                        })
                    dlg:endbox()
                    dlg:newrow()
                    dlg:list({
                        id = "composer_friend_list",
                        height = 160,
                        expand = true,
                        items = friend_items,
                        selection = tonumber(state.friend_selection or 0) or 0,
                        onchange = function(d)
                            actions.select_friend(d.data.composer_friend_list or 0)
                        end,
                    })
                    dlg:newrow()
                    dlg:button({
                        text = "Usun friend",
                        bgcolor = palette.panel,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.remove_selected_friend()
                        end,
                    })
                dlg:endpanel()

                dlg:panel({
                    bgcolor = palette.panel_alt,
                    padding = 8,
                    margin = 4,
                    expand = false,
                })
                    dlg:label({
                        id = "composer_save_mode",
                        text = state.draft.isNew and "Tryb: nowy composed brush" or "Tryb: edycja istniejacego wpisu",
                        fgcolor = palette.text,
                        font_weight = "bold",
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "composer_save_dirty",
                        text = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie",
                        fgcolor = state.dirty and palette.warning or palette.muted,
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "composer_save_target",
                        text = "Target: " .. tostring(state.target_path or ""),
                        fgcolor = palette.subtle,
                        font_size = 8,
                    })
                    dlg:newrow()
                    dlg:label({
                        id = "composer_save_hint",
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
                    dlg:newrow()
                    dlg:button({
                        text = "Save as New",
                        bgcolor = palette.success,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.save_as_new()
                        end,
                    })
                    dlg:newrow()
                    dlg:button({
                        text = "Overwrite Existing",
                        bgcolor = palette.accent,
                        fgcolor = palette.text,
                        onclick = function()
                            actions.overwrite_existing()
                        end,
                    })
                dlg:endpanel()
            dlg:endbox()
        dlg:endbox()
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

function ComposerPage.create(options)
    return create_page(options)
end

return ComposerPage
