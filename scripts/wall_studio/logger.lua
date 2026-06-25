local STORE = app.storage("wall_studio.log")

local Logger = {}

local function append_line(line)
    if not STORE or not STORE.save or not STORE.readText then
        return
    end

    local previous = ""
    local content = STORE:readText()
    if type(content) == "string" then
        previous = content
    elseif type(content) == "table" then
        previous = content[1] or ""
    end

    local next_content = previous ~= "" and (previous .. "\n" .. line) or line
    STORE:save(next_content)
end

function Logger.reset(reason)
    if STORE and STORE.save then
        STORE:save(string.format("[reset] %s", tostring(reason or "")))
    end
end

function Logger.log(scope, message)
    local line = string.format("[%s] %s", tostring(scope or "main"), tostring(message or ""))
    print("[Wall Studio] " .. line)
    append_line(line)
end

return Logger
