local Common = dofile("common.lua")
local WallDraft = dofile("wall_draft.lua")
local XmlCodec = dofile("xml_codec.lua")
local Logger = dofile("logger.lua")

local M = {}

local function file_exists(path)
    if not path or path == "" then
        return false
    end
    local store = app.storage(path)
    return store and store.exists and store:exists() or false
end

local function read_file(path)
    local store = app.storage(path)
    if not store or not store.readText then
        return nil, "This RME build does not expose raw file reads for walls.xml."
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

local function build_repository_from_app_walls(app_walls, session_walls)
    local repository = {
        ordered = {},
        by_name = {},
        warnings = {},
        target_path = "",
        api_missing = nil,
    }

    for _, wall_data in pairs(app_walls or {}) do
        local name = Common.trim(wall_data.name or "")
        if name ~= "" then
            local alignments = {}
            for _, alignment in ipairs(Common.alignment_order) do
                local bucket_data = wall_data.alignments and wall_data.alignments[alignment.token] or {}
                local bucket = Common.make_bucket(alignment.token)
                for _, item in ipairs(bucket_data.items or {}) do
                    local item_id = tonumber(item.id or item.itemId or 0) or 0
                    if item_id > 0 then
                        table.insert(bucket.wallItems, {
                            itemId = item_id,
                            chance = tonumber(item.chance or 1) or 1,
                            name = Common.safe_item_name(item_id),
                            lookId = (Common.safe_item_info(item_id) or {}).clientId or 0,
                        })
                    end
                end
                for _, door in ipairs(bucket_data.doors or {}) do
                    local item_id = tonumber(door.id or door.itemId or 0) or 0
                    if item_id > 0 then
                        table.insert(bucket.doorItems, {
                            itemId = item_id,
                            doorType = Common.trim(door.type or door.doorType or "normal"),
                            isOpen = Common.normalize_bool(door.open, false),
                            isLocked = Common.normalize_bool(door.locked, false),
                            hate = Common.normalize_bool(door.hate, false),
                            name = Common.safe_item_name(item_id),
                            lookId = (Common.safe_item_info(item_id) or {}).clientId or 0,
                        })
                    end
                end
                alignments[alignment.token] = bucket
            end

            local draft = WallDraft.new({
                name = name,
                lookId = tonumber(wall_data.lookid or 0) or 0,
                serverLookId = tonumber(wall_data.server_lookid or 0) or 0,
                isNew = false,
                sourceName = name,
                friends = Common.clone(wall_data.friends or {}),
                redirectName = Common.trim(wall_data.redirect_to or ""),
                alignments = alignments,
            })

            if draft.serverLookId <= 0 then
                draft.serverLookId = draft:getPreviewItemId()
            end

            repository.by_name[name] = draft
        end
    end

    for name, overlay in pairs(session_walls or {}) do
        repository.by_name[name] = overlay:clone()
        repository.by_name[name].isNew = false
    end

    for _, draft in pairs(repository.by_name) do
        table.insert(repository.ordered, draft)
    end

    table.sort(repository.ordered, function(a, b)
        return string.lower(a.name or "") < string.lower(b.name or "")
    end)

    return repository
end

function M.resolve_target_path(saved_path)
    if saved_path and saved_path ~= "" and file_exists(saved_path) then
        return saved_path
    end

    local preferred = tostring(app.getDataDirectory() or "") .. "/1310/walls.xml"
    return preferred
end

function M.read(target_path, session_walls)
    local repository = nil
    if app and app.walls then
        repository = build_repository_from_app_walls(app.walls, session_walls)
        repository.target_path = target_path or ""
    end

    local content, err = read_file(target_path)
    if not content then
        repository = repository or {
            ordered = {},
            by_name = {},
            warnings = {},
            target_path = target_path or "",
            api_missing = err,
        }
        table.insert(repository.warnings, err)
        return repository
    end

    local parsed = XmlCodec.parse_walls(content)
    if not repository then
        repository = parsed
    else
        for name, draft in pairs(parsed.by_name or {}) do
            if not repository.by_name[name] then
                repository.by_name[name] = draft
            else
                repository.by_name[name].serverLookId = tonumber(draft.serverLookId or repository.by_name[name].serverLookId or 0) or 0
                repository.by_name[name].lookId = tonumber(repository.by_name[name].lookId or draft.lookId or 0) or 0
            end
        end
        repository.warnings = repository.warnings or {}
        for _, warning in ipairs(parsed.warnings or {}) do
            table.insert(repository.warnings, warning)
        end
    end

    for name, overlay in pairs(session_walls or {}) do
        repository.by_name[name] = overlay:clone()
        repository.by_name[name].isNew = false
    end

    repository.ordered = {}
    for _, draft in pairs(repository.by_name) do
        table.insert(repository.ordered, draft)
    end
    table.sort(repository.ordered, function(a, b)
        return string.lower(a.name or "") < string.lower(b.name or "")
    end)
    repository.target_path = target_path or ""
    return repository
end

function M.new_draft(repository, session_walls)
    local base = "new wall"
    local index = 1
    local candidate = base
    while (repository.by_name and repository.by_name[candidate]) or (session_walls and session_walls[candidate]) do
        index = index + 1
        candidate = string.format("%s %d", base, index)
    end
    return WallDraft.new({
        name = candidate,
        isNew = true,
    })
end

function M.load_draft(repository, name)
    local draft = repository.by_name[name]
    return draft and draft:clone() or nil
end

function M.duplicate_draft(repository, session_walls, draft)
    local copy = draft:clone()
    local base = Common.trim((draft.name ~= "" and draft.name or "wall") .. " copy")
    local index = 1
    local candidate = base
    while (repository.by_name and repository.by_name[candidate]) or (session_walls and session_walls[candidate]) do
        index = index + 1
        candidate = string.format("%s %d", base, index)
    end
    copy.name = candidate
    copy.sourceName = candidate
    copy.isNew = true
    return copy
end

function M.library_items(repository, filter_text)
    return Common.library_items(repository, filter_text)
end

function M.save_intent(repository, draft, mode)
    local name = Common.trim(draft and draft.name or "")
    local source_name = Common.trim(draft and draft.sourceName or "")
    local exists = name ~= "" and repository.by_name and repository.by_name[name] ~= nil
    local source_exists = source_name ~= "" and repository.by_name and repository.by_name[source_name] ~= nil

    if mode == "new" then
        if name == "" then
            return "Podaj nazwe wall brusha, aby zapisac nowy wpis."
        end
        if exists then
            return string.format("Save as New zablokowany: wall '%s' juz istnieje.", name)
        end
        return string.format("Utworzy nowy wall brush '%s'.", name)
    end

    if not source_exists then
        return "Overwrite wymaga istniejacego wall brusha w bibliotece."
    end
    return string.format("Nadpisze wall brush '%s'.", source_name)
end

function M.validate_save(repository, draft, mode, target_path)
    if not target_path or target_path == "" then
        return false, "Target walls.xml could not be resolved."
    end
    if not Common.path_starts_with(target_path, app.getDataDirectory()) then
        return false, "Target walls.xml must stay inside the RME data directory."
    end
    if not tostring(target_path):lower():match("%.xml$") then
        return false, "Target file must be an XML file."
    end
    if not file_exists(target_path) then
        return false, "Target walls.xml does not exist."
    end

    local validation = draft:getValidation()
    if not validation.isValid then
        return false, validation.errors[1] or "Wall draft jest niepoprawny."
    end

    local current_name = Common.trim(draft.name or "")
    local source_name = Common.trim(draft.sourceName or current_name)
    local exists = repository.by_name and repository.by_name[current_name] ~= nil
    local source_exists = repository.by_name and repository.by_name[source_name] ~= nil

    if mode == "new" and exists then
        return false, string.format("Wall '%s' juz istnieje.", current_name)
    end
    if mode == "overwrite" and not source_exists then
        return false, string.format("Wall '%s' nie istnieje do nadpisania.", source_name)
    end

    return true
end

function M.backup_xml(target_path)
    local original, err = read_file(target_path)
    if not original then
        return false, nil, nil, "Could not read walls.xml before backup: " .. tostring(err)
    end

    local backup_path = string.format("%s.wallstudio.%s.bak", target_path, os.date("%Y%m%d_%H%M%S"))
    local ok, backup_err = write_file(backup_path, original)
    if not ok then
        return false, nil, nil, "Could not write backup file: " .. tostring(backup_err)
    end

    return true, backup_path, original
end

function M.save_draft(repository, session_walls, draft, mode, target_path)
    local ok, validation_message = M.validate_save(repository, draft, mode, target_path)
    if not ok then
        return false, validation_message
    end

    local backup_ok, backup_path, original_content, backup_message = M.backup_xml(target_path)
    if not backup_ok then
        return false, backup_message
    end

    local saved_draft = draft:clone()
    saved_draft.name = Common.trim(saved_draft.name)
    saved_draft.isNew = false

    local writer = app.storage(target_path)
    if not writer or not writer.upsertXml then
        return false, "The current RME build does not expose XML upsert support."
    end

    local match_name = mode == "overwrite" and Common.trim(saved_draft.sourceName or saved_draft.name) or saved_draft.name
    local xml = XmlCodec.serialize_wall(saved_draft)
    local write_ok, write_mode = writer:upsertXml({
        root = "materials",
        parent = "materials",
        tag = "brush",
        match_attr = "name",
        match_value = match_name,
        xml = xml,
    })

    if not write_ok then
        if original_content then
            write_file(target_path, original_content)
        end
        return false, write_mode or "Saving failed while updating walls.xml."
    end

    if match_name ~= saved_draft.name then
        session_walls[match_name] = nil
    end

    saved_draft.sourceName = saved_draft.name
    session_walls[saved_draft.name] = saved_draft:clone()
    Logger.log("repository", string.format("saved wall '%s'", saved_draft.name))

    return true, {
        draft = saved_draft,
        backupPath = backup_path,
        xmlMode = write_mode or mode,
        message = string.format("Saved wall brush '%s' successfully.\nBackup: %s", saved_draft.name, backup_path),
    }
end

return M
