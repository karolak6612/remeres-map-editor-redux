local M = {}

local STORE = app.storage("brush_studio.log")
local MAX_LINES = 400

local function lines_buffer()
    if type(_G.BRUSH_STUDIO_LOG_LINES) ~= "table" then
        _G.BRUSH_STUDIO_LOG_LINES = {}
    end
    return _G.BRUSH_STUDIO_LOG_LINES
end

local function now_ms()
    if app and app.getTime then
        local ok, value = pcall(app.getTime)
        if ok then
            return tonumber(value or 0) or 0
        end
    end
    return 0
end

local function flush(lines)
    if STORE and STORE.save then
        STORE:save(table.concat(lines, "\n"))
    end
end

function M.log(scope, message)
    local lines = lines_buffer()
    local line = string.format("[%06d] [%s] %s", now_ms(), tostring(scope or "log"), tostring(message or ""))
    table.insert(lines, line)
    while #lines > MAX_LINES do
        table.remove(lines, 1)
    end

    print("[Brush Studio] " .. line)
    flush(lines)
end

return M
