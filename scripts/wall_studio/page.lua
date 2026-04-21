local Common = dofile("common.lua")
local Model = dofile("model.lua")
local LibraryPanel = dofile("library_panel.lua")
local PreviewPanel = dofile("preview_panel.lua")
local PropertiesPanel = dofile("properties_panel.lua")
local Logger = dofile("logger.lua")
local XmlTarget = dofile("xml_target_helper.lua")

local WallStudioPage = {}

local function create_page(options)
    options = options or {}
    local settings = Model.load_settings()
    local session = options.session or {}
    local listener_id = nil
    local dlg
    local actions = {}

    local state = {
        filter_text = "",
        target_path = Model.resolve_target_path(settings.target_path or ""),
        session_walls = {},
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
        selected_alignment = Common.alignment_order[1].token,
        selected_wall_item_index = 0,
        selected_door_index = 0,
        selected_raw_id = Common.active_raw_brush_id(),
        recent_raw_ids = Common.clone(settings.recent_raw_ids or {}),
        wall_item_chance = "1",
        door_form = {
            type = "normal",
            open = false,
            locked = false,
            hate = false,
        },
        status_message = "",
        dirty = false,
        rebuilding = false,
        library_ignore_index = 0,
        library_ignore_budget = 0,
        library_skip_changes = 0,
    }

    local function refresh_repository()
        state.repository = Model.read_repository(state.target_path, state.session_walls)
    end

    local function selected_bucket()
        return state.draft and state.draft:getBucket(state.selected_alignment) or Common.make_bucket(state.selected_alignment)
    end

    local function sync_library(preferred_name)
        local filtered_items, filtered_names = Model.library_items(state.repository, state.filter_text)
        state.library_filtered_items = filtered_items
        state.library_filtered_names = filtered_names

        local page_size = math.max(1, tonumber(state.library_page_size or 18) or 18)
        state.library_page_count = math.max(1, math.ceil(#filtered_names / page_size))
        local desired = Common.trim(preferred_name or state.library_selected_name)

        if desired ~= "" then
            for absolute_index, name in ipairs(filtered_names) do
                if name == desired then
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
        local page_end = math.min(#filtered_names, page_start + page_size - 1)
        state.library_items = {}
        state.library_visible_names = {}
        for absolute_index = page_start, page_end do
            table.insert(state.library_items, filtered_items[absolute_index])
            table.insert(state.library_visible_names, filtered_names[absolute_index])
        end

        state.library_selection = 0
        if desired ~= "" then
            for index, name in ipairs(state.library_visible_names) do
                if name == desired then
                    state.library_selection = index
                    break
                end
            end
        end
    end

    local function soft_refresh_library()
        if not dlg then
            return
        end

        state.rebuilding = true
        state.library_skip_changes = 1
        state.library_ignore_index = tonumber(state.library_selection or 0) or 0
        state.library_ignore_budget = state.library_ignore_index > 0 and 2 or 0
        dlg:modify({
            wall_library_page_label = {
                text = string.format("Strona %d / %d", tonumber(state.library_page or 1), tonumber(state.library_page_count or 1)),
            },
            wall_library = {
                items = state.library_items,
                selection = tonumber(state.library_selection or 0) or 0,
            },
        })
        state.rebuilding = false
        dlg:repaint()
    end

    local function persist_settings()
        Model.save_settings(state)
    end

    local function sync_session_out()
        session.lastSelectedRaw = tonumber(state.selected_raw_id or 0) or 0
        session.recentItems = session.recentItems or {}
        session.recentItems.wallRawIds = state.recent_raw_ids
        session.pageDirty = session.pageDirty or {}
        session.pageDirty.walls = state.dirty
        if state.draft and not state.draft.isNew and Common.trim(state.draft.name or "") ~= "" then
            session.selectedWallCreator = state.draft.name
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
            text = "The current wall draft has unsaved changes. Continue and discard them?",
            buttons = { "Yes", "No" },
        })
        return choice == 1
    end

    local function current_raw_id()
        return Common.active_raw_brush_id()
    end

    local function sync_selected_raw_from_brush()
        local active = current_raw_id()
        if active > 0 then
            state.selected_raw_id = active
        end
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

    local function reset_bucket_selection()
        state.selected_wall_item_index = 0
        state.selected_door_index = 0
        state.wall_item_chance = "1"
        state.door_form = {
            type = "normal",
            open = false,
            locked = false,
            hate = false,
        }
    end

    local function sync_forms_from_selection()
        local bucket = selected_bucket()
        local wall_item = bucket.wallItems[tonumber(state.selected_wall_item_index or 0) or 0]
        if wall_item then
            state.wall_item_chance = tostring(wall_item.chance or 0)
        end
        local door = bucket.doorItems[tonumber(state.selected_door_index or 0) or 0]
        if door then
            state.door_form = {
                type = door.doorType or "normal",
                open = door.isOpen and true or false,
                locked = door.isLocked and true or false,
                hate = door.hate and true or false,
            }
        end
    end

    local function soft_refresh()
        if not dlg or not state.draft then
            return
        end

        sync_selected_raw_from_brush()
        sync_session_out()
        sync_forms_from_selection()
        state.rebuilding = true

        local bucket = selected_bucket()
        local raw_info = Common.safe_item_info(state.selected_raw_id)
        local selected_wall_item = bucket.wallItems[tonumber(state.selected_wall_item_index or 0) or 0]
        local selected_door = bucket.doorItems[tonumber(state.selected_door_index or 0) or 0]

        dlg:modify({
            current_wall_title = { text = Common.format_wall_title(state.draft) },
            current_wall_status = { text = "Status: " .. Common.wall_status(state.draft) },
            wall_alignment_combo = { option = Common.alignment_label(state.selected_alignment) },
            wall_alignment_grid = { items = Common.alignment_grid_items(state.draft) },
            selected_alignment_preview = { image = Common.safe_image(Common.representative_item_id(bucket)), width = 80, height = 80, smooth = false },
            selected_alignment_title = { text = Common.alignment_label(state.selected_alignment) },
            selected_alignment_meta = { text = string.format("Wall items: %d  |  Doors: %d", #(bucket.wallItems or {}), #(bucket.doorItems or {})) },
            wall_friend_summary = { text = Common.friend_summary(state.draft) },
            wall_redirect_summary = { text = state.draft.redirectName ~= "" and ("Redirect: " .. state.draft.redirectName) or "Redirect: brak" },
            wall_active_raw_preview = { image = Common.safe_image(state.selected_raw_id), width = 48, height = 48, smooth = false },
            wall_active_raw_title = { text = raw_info and (raw_info.name or ("Item " .. tostring(state.selected_raw_id))) or "Najpierw wybierz RAW tile" },
            wall_active_raw_meta = { text = raw_info and string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0) or "Brak aktywnego RAW." },
            wall_name_input = { text = state.draft.name or "" },
            wall_lookid_input = { text = tostring(state.draft.lookId or 0) },
            wall_server_lookid_input = { text = tostring(state.draft.serverLookId or 0) },
            wall_friends_input = { text = table.concat(state.draft.friends or {}, ", ") },
            wall_redirect_input = { text = state.draft.redirectName or "" },
            wall_bucket_item_list = { items = Common.bucket_wall_list_items(bucket), selection = tonumber(state.selected_wall_item_index or 0) or 0 },
            wall_bucket_door_list = { items = Common.bucket_door_list_items(bucket), selection = tonumber(state.selected_door_index or 0) or 0 },
            wall_item_chance_input = { text = selected_wall_item and tostring(selected_wall_item.chance or 0) or tostring(state.wall_item_chance or 1) },
            wall_door_type_combo = { option = state.door_form.type or "normal" },
            wall_door_open_check = { selected = state.door_form.open and true or false },
            wall_door_locked_check = { selected = state.door_form.locked and true or false },
            wall_door_hate_check = { selected = state.door_form.hate and true or false },
            wall_recent_raw_list = { items = Common.recent_raw_items(state.recent_raw_ids) },
            wall_save_mode_label = { text = state.draft.isNew and "Tryb: nowy wall brush" or "Tryb: edycja istniejacego wall brusha" },
            wall_save_dirty_label = { text = state.dirty and "Niezapisane zmiany: tak" or "Niezapisane zmiany: nie" },
            wall_save_target_label = { text = "Target: " .. tostring(state.target_path or "") },
            wall_save_hint_label = { text = "Podaj nazwe i co najmniej jeden wall item, aby zapisac wall brush." },
            wall_save_intent_new = { text = Model.save_intent(state.repository, state.draft, "new") },
            wall_save_intent_overwrite = { text = Model.save_intent(state.repository, state.draft, "overwrite") },
        })
        state.rebuilding = false
        dlg:repaint()
    end

    local function soft_refresh_active_raw()
        if not dlg then
            return
        end

        sync_selected_raw_from_brush()
        sync_session_out()
        state.rebuilding = true

        local raw_info = Common.safe_item_info(state.selected_raw_id)
        dlg:modify({
            wall_active_raw_preview = { image = Common.safe_image(state.selected_raw_id), width = 48, height = 48, smooth = false },
            wall_active_raw_title = { text = raw_info and (raw_info.name or ("Item " .. tostring(state.selected_raw_id))) or "Najpierw wybierz RAW tile" },
            wall_active_raw_meta = { text = raw_info and string.format("Item ID %d  |  Look ID %d", raw_info.id or 0, raw_info.clientId or 0) or "Brak aktywnego RAW." },
            wall_recent_raw_list = { items = Common.recent_raw_items(state.recent_raw_ids) },
        })
        state.rebuilding = false
        dlg:repaint()
    end

    local function render_page_content()
        state.rebuilding = true
        refresh_repository()
        sync_library()
        sync_selected_raw_from_brush()

        dlg:panel({
            bgcolor = Common.palette.header,
            padding = 10,
            margin = 4,
            expand = false,
        })
            dlg:label({
                text = "WALL STUDIO",
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
            PropertiesPanel.render(dlg, state, actions)
        dlg:endbox()
        state.library_skip_changes = 1
        state.library_ignore_index = tonumber(state.library_selection or 0) or 0
        state.library_ignore_budget = state.library_ignore_index > 0 and 2 or 0
        state.rebuilding = false
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
        if state.rebuilding then
            return
        end
        state.filter_text = text or ""
        state.library_page = 1
        sync_library("")
        soft_refresh_library()
    end

    function actions.previous_library_page()
        if state.rebuilding then
            return
        end
        if state.library_page > 1 then
            state.library_page = state.library_page - 1
            sync_library("")
            soft_refresh_library()
        end
    end

    function actions.next_library_page()
        if state.rebuilding then
            return
        end
        if state.library_page < state.library_page_count then
            state.library_page = state.library_page + 1
            sync_library("")
            soft_refresh_library()
        end
    end

    function actions.handle_library_change(index)
        if state.rebuilding then
            return
        end
        index = tonumber(index or 0) or 0
        if index <= 0 then
            return
        end
        if state.library_skip_changes > 0 then
            state.library_skip_changes = state.library_skip_changes - 1
            return
        end
        if state.library_ignore_budget > 0 and index == state.library_ignore_index then
            state.library_ignore_budget = state.library_ignore_budget - 1
            return
        end

        local previous_index = tonumber(state.library_selection or 0) or 0
        local previous_name = tostring(state.library_selected_name or "")
        state.library_selection = index
        state.library_selected_name = state.library_visible_names[index] or ""

        if previous_index == index and previous_name == tostring(state.library_selected_name or "") then
            return
        end

        if state.library_selected_name ~= "" then
            actions.open_wall(state.library_selected_name)
        end
    end

    function actions.open_selected_wall()
        if state.library_selected_name ~= "" then
            actions.open_wall(state.library_selected_name)
        end
    end

    function actions.open_wall(name)
        name = Common.trim(name)
        if name == "" then
            return
        end
        if state.draft and state.draft.name == name and not state.dirty then
            return
        end
        if not confirm_discard() then
            return
        end
        local loaded = Model.load_draft(state.repository, name)
        if not loaded then
            app.alert("This wall brush could not be loaded from walls.xml.")
            return
        end
        state.draft = loaded
        state.library_selected_name = loaded.name
        state.selected_alignment = Common.alignment_order[1].token
        reset_bucket_selection()
        state.status_message = string.format("Zaladowano wall brush '%s'.", loaded.name)
        state.dirty = false
        sync_library(loaded.name)
        soft_refresh_library()
        soft_refresh()
    end

    function actions.new_wall()
        if not confirm_discard() then
            return
        end
        state.draft = Model.new_draft(state.repository, state.session_walls)
        state.library_selected_name = ""
        state.selected_alignment = Common.alignment_order[1].token
        reset_bucket_selection()
        state.status_message = string.format("Utworzono nowy draft '%s'.", state.draft.name)
        state.dirty = false
        sync_library("")
        soft_refresh_library()
        soft_refresh()
    end

    function actions.duplicate_current()
        state.draft = Model.duplicate_draft(state.repository, state.session_walls, state.draft)
        state.library_selected_name = ""
        reset_bucket_selection()
        state.status_message = string.format("Zduplikowano wall brush do '%s'.", state.draft.name)
        state.dirty = true
        sync_library("")
        soft_refresh_library()
        soft_refresh()
    end

    function actions.set_alignment_title(title)
        if state.rebuilding then
            return
        end
        for _, alignment in ipairs(Common.alignment_order) do
            if alignment.label == title then
                state.selected_alignment = alignment.token
                reset_bucket_selection()
                soft_refresh()
                return
            end
        end
    end

    function actions.set_name(name)
        if state.rebuilding then
            return
        end
        state.draft:setName(name)
        state.dirty = true
        sync_session_out()
    end

    function actions.set_lookid(value)
        if state.rebuilding then
            return
        end
        state.draft:setLookId(tonumber(value or 0) or 0)
        state.dirty = true
        sync_session_out()
    end

    function actions.set_server_lookid(value)
        if state.rebuilding then
            return
        end
        state.draft:setServerLookId(tonumber(value or 0) or 0)
        state.dirty = true
        sync_session_out()
    end

    function actions.set_friends_text(text)
        if state.rebuilding then
            return
        end
        state.draft:setFriendsFromText(text)
        state.dirty = true
        soft_refresh()
    end

    function actions.set_redirect_name(text)
        if state.rebuilding then
            return
        end
        state.draft:setRedirectName(text)
        state.dirty = true
        soft_refresh()
    end

    function actions.set_wall_item_selection(index)
        if state.rebuilding then
            return
        end
        state.selected_wall_item_index = tonumber(index or 0) or 0
        state.selected_door_index = 0
        sync_forms_from_selection()
        soft_refresh()
    end

    function actions.set_wall_item_chance_text(text)
        if state.rebuilding then
            return
        end
        state.wall_item_chance = tostring(text or "")
    end

    function actions.add_wall_item_from_active_raw()
        local raw_id = current_raw_id()
        if raw_id <= 0 then
            state.status_message = "Najpierw wybierz RAW tile."
            soft_refresh()
            return
        end
        local ok, message = state.draft:addWallItem(state.selected_alignment, Model.make_raw_reference(raw_id), tonumber(state.wall_item_chance or 1) or 1)
        if not ok then
            state.status_message = message
            soft_refresh()
            return
        end
        remember_recent_raw(raw_id)
        state.selected_wall_item_index = #selected_bucket().wallItems
        state.selected_door_index = 0
        state.dirty = true
        state.status_message = string.format("Dodano wall item %d do %s.", raw_id, Common.alignment_label(state.selected_alignment))
        soft_refresh()
    end

    function actions.apply_selected_wall_item_chance()
        if state.selected_wall_item_index <= 0 then
            return
        end
        state.draft:setWallItemChance(state.selected_alignment, state.selected_wall_item_index, tonumber(state.wall_item_chance or 0) or 0)
        state.dirty = true
        soft_refresh()
    end

    function actions.remove_selected_wall_item()
        if state.draft:removeWallItem(state.selected_alignment, state.selected_wall_item_index) then
            state.selected_wall_item_index = 0
            state.dirty = true
            soft_refresh()
        end
    end

    function actions.set_door_selection(index)
        if state.rebuilding then
            return
        end
        state.selected_door_index = tonumber(index or 0) or 0
        state.selected_wall_item_index = 0
        sync_forms_from_selection()
        soft_refresh()
    end

    function actions.set_door_form_type(value)
        if state.rebuilding then
            return
        end
        state.door_form.type = value or "normal"
    end

    function actions.set_door_form_open(value)
        if state.rebuilding then
            return
        end
        state.door_form.open = value and true or false
    end

    function actions.set_door_form_locked(value)
        if state.rebuilding then
            return
        end
        state.door_form.locked = value and true or false
    end

    function actions.set_door_form_hate(value)
        if state.rebuilding then
            return
        end
        state.door_form.hate = value and true or false
    end

    function actions.add_door_from_active_raw()
        local raw_id = current_raw_id()
        if raw_id <= 0 then
            state.status_message = "Najpierw wybierz RAW tile."
            soft_refresh()
            return
        end
        local ok, message = state.draft:addDoorItem(state.selected_alignment, Model.make_raw_reference(raw_id), {
            doorType = state.door_form.type,
            isOpen = state.door_form.open,
            isLocked = state.door_form.locked,
            hate = state.door_form.hate,
        })
        if not ok then
            state.status_message = message
            soft_refresh()
            return
        end
        remember_recent_raw(raw_id)
        state.selected_door_index = #selected_bucket().doorItems
        state.selected_wall_item_index = 0
        state.dirty = true
        state.status_message = string.format("Dodano door entry %d do %s.", raw_id, Common.alignment_label(state.selected_alignment))
        soft_refresh()
    end

    function actions.apply_selected_door()
        if state.selected_door_index <= 0 then
            return
        end
        state.draft:updateDoorItem(state.selected_alignment, state.selected_door_index, {
            doorType = state.door_form.type,
            isOpen = state.door_form.open,
            isLocked = state.door_form.locked,
            hate = state.door_form.hate,
        })
        state.dirty = true
        soft_refresh()
    end

    function actions.remove_selected_door()
        if state.draft:removeDoorItem(state.selected_alignment, state.selected_door_index) then
            state.selected_door_index = 0
            state.dirty = true
            soft_refresh()
        end
    end

    local function finalize_save(mode)
        local ok, result = Model.save_draft(state.repository, state.session_walls, state.draft, mode, state.target_path)
        if not ok then
            app.alert({
                title = "Save failed",
                text = result,
                buttons = { "OK" },
            })
            return
        end

        state.draft = result.draft
        refresh_repository()
        sync_library(result.draft.name)
        state.library_selected_name = result.draft.name
        state.status_message = result.message
        state.dirty = false
        session.lastSavedWall = result.draft.name
        sync_session_out()
        soft_refresh_library()
        soft_refresh()
        app.alert({
            title = "Wall saved",
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

    function actions.get_save_intent(mode)
        return Model.save_intent(state.repository, state.draft, mode)
    end

    function actions.choose_target_path()
        local chosen, err = XmlTarget.pick_xml_file("walls.xml", state.target_path, "Wybierz walls.xml")
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
        sync_library(nil)
        state.status_message = "Ustawiono nowy target walls.xml."
        soft_refresh_library()
        soft_refresh()
    end

    function actions.use_default_target_path()
        state.target_path = Model.resolve_target_path("")
        persist_settings()
        refresh_repository()
        sync_library(nil)
        state.status_message = "Przywrocono domyslny target walls.xml."
        soft_refresh_library()
        soft_refresh()
    end

    local function attach_brush_listener()
        if listener_id or not app.events or not app.events.on then
            return
        end
        listener_id = app.events:on("brushChange", function()
            sync_selected_raw_from_brush()
            if dlg then
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
        state.draft = settings.last_wall_name and Model.load_draft(state.repository, settings.last_wall_name) or nil
        if not state.draft then
            state.draft = Model.new_draft(state.repository, state.session_walls)
        end
        if #state.repository.warnings > 0 then
            state.status_message = state.repository.warnings[1]
        else
            state.status_message = "Wybierz gotowy wall brush albo utworz nowy draft."
        end
        sync_session_in()
        sync_selected_raw_from_brush()
        sync_session_out()
    end

    initialize()

    local page = {}

    function page.getTitle()
        return "Walls"
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
        return state.status_message
    end

    return page
end

function WallStudioPage.create(options)
    return create_page(options)
end

function WallStudioPage.open_standalone()
    local existing_dialog = _G.WALL_STUDIO_DIALOG
    if existing_dialog then
        pcall(function()
            existing_dialog:close()
        end)
        _G.WALL_STUDIO_DIALOG = nil
        _G.WALL_STUDIO_OPEN = false
        app.yield()
    end

    local page = create_page({})

    local dlg = Dialog({
        title = "Wall Studio",
        id = "wall_studio_dock",
        width = 1560,
        height = 920,
        resizable = true,
        dockable = true,
        onclose = function()
            page.onLeave({})
            _G.WALL_STUDIO_OPEN = false
            _G.WALL_STUDIO_DIALOG = nil
        end,
    })

    _G.WALL_STUDIO_OPEN = true
    _G.WALL_STUDIO_DIALOG = dlg
    page.onEnter({})
    dlg:clear()
    page.render_into(dlg)
    dlg:layout()
    dlg:repaint()
    dlg:show({ wait = false, center = "screen" })
    return page
end

return WallStudioPage
