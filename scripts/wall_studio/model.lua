local Common = dofile("common.lua")
local Repository = dofile("repository.lua")

local M = {}

local SETTINGS_STORE = app.storage("settings.json")

function M.load_settings()
    return SETTINGS_STORE:load() or {}
end

function M.save_settings(state)
    SETTINGS_STORE:save({
        target_path = state.target_path or "",
        last_wall_name = state.draft and state.draft.name or "",
        recent_raw_ids = state.recent_raw_ids or {},
    })
end

function M.resolve_target_path(saved_path)
    return Repository.resolve_target_path(saved_path)
end

function M.read_repository(target_path, session_walls)
    return Repository.read(target_path, session_walls)
end

function M.new_draft(repository, session_walls)
    return Repository.new_draft(repository, session_walls)
end

function M.load_draft(repository, name)
    return Repository.load_draft(repository, name)
end

function M.duplicate_draft(repository, session_walls, draft)
    return Repository.duplicate_draft(repository, session_walls, draft)
end

function M.library_items(repository, filter_text)
    return Repository.library_items(repository, filter_text)
end

function M.save_intent(repository, draft, mode)
    return Repository.save_intent(repository, draft, mode)
end

function M.save_draft(repository, session_walls, draft, mode, target_path)
    return Repository.save_draft(repository, session_walls, draft, mode, target_path)
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

return M
