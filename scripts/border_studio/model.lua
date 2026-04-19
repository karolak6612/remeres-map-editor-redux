local Common = dofile("common.lua")
local BorderDraft = dofile("border_draft.lua")
local BorderRepository = dofile("border_repository.lua")
local XmlWriter = dofile("xml_writer.lua")
local Logger = dofile("logger.lua")

local M = {}

local SETTINGS_STORE = app.storage("settings.json")

local function file_exists(path)
    if not path or path == "" then
        return false
    end

    local store = app.storage(path)
    if store and store.exists then
        return store:exists()
    end

    return true
end

local function read_file(path)
    local store = app.storage(path)
    if not store or not store.readText then
        return nil, "This RME build does not expose raw file reads for backup."
    end

    local content, err = store:readText()
    if not content then
        return nil, err or "Could not open file."
    end

    return content
end

local function write_file(path, content)
    local store = app.storage(path)
    if not store or not store.save then
        return false, "This RME build does not expose safe file writes."
    end

    local ok = store:save(content or "")
    if not ok then
        return false, "Could not write file."
    end

    return true
end

function M.load_settings()
    return SETTINGS_STORE:load() or {}
end

function M.save_settings(state)
    SETTINGS_STORE:save({
        target_path = state.target_path or "",
        recent_raw_ids = Common.clone(state.recent_raw_ids or {}),
        selected_raw_id = state.selected_raw_id or 0,
        last_border_id = state.draft and state.draft.borderId or 0,
    })
end

function M.resolve_target_path(saved_path)
    if saved_path and saved_path ~= "" and file_exists(saved_path) then
        return saved_path
    end

    local data_dir = app.getDataDirectory()
    local preferred = tostring(data_dir or "") .. "/1310/borders.xml"
    if file_exists(preferred) then
        return preferred
    end

    return preferred
end

function M.read_repository(session_borders)
    Logger.log("model", "read_repository start")
    local repository = BorderRepository.read(session_borders, app.borders or {})
    Logger.log("model", string.format("read_repository done count=%d session_overlays=%d", #repository.ordered, session_borders and #session_borders or 0))

    return repository
end

function M.next_border_id(repository, session_borders, extra_id)
    return BorderRepository.next_border_id(repository, session_borders, extra_id)
end

function M.new_draft(repository, session_borders, extra_id)
    local border_id = M.next_border_id(repository, session_borders, extra_id)
    return BorderDraft.new({
        borderId = border_id,
        name = "Border #" .. tostring(border_id),
        group = 0,
        isNew = true,
        status = "partial",
    })
end

function M.load_draft(repository, border_id)
    Logger.log("model", string.format("load_draft request border_id=%d", tonumber(border_id or 0) or 0))
    local border = repository.by_id[border_id]
    if not border then
        Logger.log("model", string.format("load_draft missing border_id=%d", tonumber(border_id or 0) or 0))
        return nil
    end
    local copy = BorderRepository.copy(border)
    Logger.log("model", string.format("load_draft done border_id=%d assigned=%d", copy.borderId or 0, Common.assigned_slot_count(copy)))
    return copy
end

function M.duplicate_draft(repository, session_borders, draft)
    local duplicate = BorderRepository.copy(draft)
    duplicate.borderId = M.next_border_id(repository, session_borders, draft and draft.borderId or 0)
    duplicate.name = "Border #" .. tostring(duplicate.borderId)
    duplicate.isNew = true
    duplicate.status = Common.border_status(duplicate)
    return duplicate
end

function M.make_raw_reference(item_id)
    item_id = tonumber(item_id or 0) or 0
    if item_id <= 0 then
        return nil
    end

    local info = Common.safe_item_info(item_id)
    return {
        itemId = item_id,
        rawId = item_id,
        lookId = info and info.clientId or 0,
        name = info and info.name or ("Item " .. tostring(item_id)),
    }
end

function M.touch_recent(recent_raw_ids, item_id)
    if not item_id or item_id <= 0 then
        return recent_raw_ids
    end

    local next_list = {}
    table.insert(next_list, item_id)

    for _, existing in ipairs(recent_raw_ids or {}) do
        if existing ~= item_id then
            table.insert(next_list, existing)
        end
        if #next_list >= 8 then
            break
        end
    end

    return next_list
end

function M.library_items(repository, filter_text)
    local items = {}
    local visible_ids = {}
    local needle = string.lower(filter_text or "")

    for _, border in ipairs(repository.ordered or {}) do
        local title = Common.format_border_title(border)
        local status = Common.border_status(border)
        local display_name = border.name ~= "" and border.name or ("Border #" .. tostring(border.borderId))
        local sample_name = Common.safe_item_name(Common.primary_item_id(border))
        local search_blob = string.lower(title .. " " .. display_name .. " " .. sample_name .. " " .. status)

        if needle == "" or string.find(search_blob, needle, 1, true) then
            local item_id = Common.primary_item_id(border)
            local tooltip = string.format(
                "Border %d\n%s\nGroup %d\nStatus: %s\nAssigned slots: %d",
                border.borderId,
                display_name,
                border.group or 0,
                status,
                Common.assigned_slot_count(border)
            )

            table.insert(items, {
                text = string.format("#%d  %s  |  %s", border.borderId, display_name, status),
                tooltip = tooltip,
                image = Common.safe_image(item_id),
            })
            table.insert(visible_ids, border.borderId)
        end
    end

    return items, visible_ids
end

function M.prepare_export(draft)
    if not draft then
        return nil
    end

    return {
        borderId = draft.borderId,
        name = draft.name,
        group = draft.group or 0,
        status = Common.border_status(draft),
        hasRequiredDataForSave = draft:hasRequiredDataForSave(),
        emptySlots = draft:getEmptySlots(),
        borderItems = draft:toBorderItemEntries(),
    }
end

function M.save_intent(repository, draft, border_id, mode)
    border_id = tonumber(border_id or 0) or 0
    local exists = repository and repository.by_id and repository.by_id[border_id] ~= nil

    if mode == "new" then
        if border_id <= 0 then
            return "Enter a valid Border ID to create a new border."
        end
        if exists then
            return string.format("Save as new will fail because border #%d already exists.", border_id)
        end
        return string.format("Utworzy nowy border #%d.", border_id)
    end

    if border_id <= 0 then
        return "Enter a valid Border ID to overwrite an existing border."
    end
    if not exists then
        return string.format("Overwrite will fail because border #%d does not exist yet.", border_id)
    end
    return string.format("Nadpisze border #%d.", border_id)
end

function M.validate_save(repository, draft, border_id, mode, target_path)
    border_id = tonumber(border_id or 0) or 0

    if not target_path or target_path == "" then
        return false, "Target borders.xml could not be resolved."
    end
    if not Common.path_starts_with(target_path, app.getDataDirectory()) then
        return false, "Target borders.xml must stay inside the RME data directory."
    end
    if not tostring(target_path):lower():match("%.xml$") then
        return false, "Target file must be an XML file."
    end
    if not file_exists(target_path) then
        return false, "Target borders.xml does not exist."
    end
    if not draft or draft:isEmpty() then
        return false, "Draft is empty. Assign at least one border slot before saving."
    end
    if border_id <= 0 then
        return false, "Border ID must be a positive number."
    end

    local exists = repository and repository.by_id and repository.by_id[border_id] ~= nil
    if mode == "new" and exists then
        return false, string.format("Border #%d already exists, so Save as New is blocked.", border_id)
    end
    if mode == "overwrite" and not exists then
        return false, string.format("Border #%d does not exist, so Overwrite is blocked.", border_id)
    end

    return true
end

function M.backup_xml(target_path)
    local original, err = read_file(target_path)
    if not original then
        return false, nil, nil, "Could not read borders.xml before backup: " .. tostring(err)
    end

    local backup_path = string.format("%s.borderstudio.%s.bak", target_path, os.date("%Y%m%d_%H%M%S"))
    local ok, backup_err = write_file(backup_path, original)
    if not ok then
        return false, nil, nil, "Could not write backup file: " .. tostring(backup_err)
    end

    return true, backup_path, original
end

function M.save_draft(repository, session_borders, draft, border_id, name, mode, target_path)
    local ok, validation_message = M.validate_save(repository, draft, border_id, mode, target_path)
    if not ok then
        return false, validation_message
    end

    local backup_ok, backup_path, original_content, backup_message = M.backup_xml(target_path)
    if not backup_ok then
        return false, backup_message
    end

    local saved_draft = draft:clone()
    saved_draft.borderId = tonumber(border_id or saved_draft.borderId) or saved_draft.borderId
    saved_draft.name = tostring(name or "")
    saved_draft.isNew = false
    saved_draft.status = Common.border_status(saved_draft)

    local writer = app.storage(target_path)
    if not writer or not writer.upsertXml then
        return false, "The current RME build does not expose XML upsert support."
    end

    local xml = XmlWriter.serialize_border(saved_draft)
    local write_ok, write_mode = writer:upsertXml({
        root = "materials",
        parent = "materials",
        tag = "border",
        match_attr = "id",
        match_value = tostring(saved_draft.borderId),
        xml = xml,
    })

    if not write_ok then
        if original_content then
            write_file(target_path, original_content)
        end
        return false, write_mode or "Saving failed while updating borders.xml."
    end

    session_borders[saved_draft.borderId] = saved_draft:clone()
    session_borders[saved_draft.borderId].isNew = false

    return true, {
        draft = saved_draft,
        backupPath = backup_path,
        xmlMode = write_mode or mode,
        message = string.format(
            "Saved border #%d successfully.\nBackup: %s",
            saved_draft.borderId,
            backup_path
        ),
    }
end

return M
