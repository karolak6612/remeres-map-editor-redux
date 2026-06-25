local Common = dofile("common.lua")

local WallDraft = {}
WallDraft.__index = WallDraft

local function build_item_reference(raw_item)
    if type(raw_item) == "table" then
        local item_id = tonumber(raw_item.itemId or raw_item.item_id or raw_item.id or 0) or 0
        if item_id <= 0 then
            return nil
        end
        local info = Common.safe_item_info(item_id)
        return {
            itemId = item_id,
            rawId = tonumber(raw_item.rawId or raw_item.raw_id or item_id) or item_id,
            lookId = tonumber(raw_item.lookId or raw_item.look_id or raw_item.clientId or (info and info.clientId) or 0) or 0,
            name = raw_item.name or (info and info.name) or ("Item " .. tostring(item_id)),
        }
    end

    local item_id = tonumber(raw_item or 0) or 0
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

local function normalize_chance(value, default)
    local chance = tonumber(value or default or 1) or tonumber(default or 1) or 1
    chance = math.floor(chance)
    if chance < 0 then
        chance = 0
    end
    return chance
end

local function normalize_door_type(value)
    local text = Common.trim(value or "normal")
    for _, option in ipairs(Common.door_type_options) do
        if option == text then
            return option
        end
    end
    return "normal"
end

local function clone_bucket(source, token)
    local bucket = Common.make_bucket(token)
    for _, item in ipairs(source and source.wallItems or {}) do
        table.insert(bucket.wallItems, Common.clone(item))
    end
    for _, door in ipairs(source and source.doorItems or {}) do
        table.insert(bucket.doorItems, Common.clone(door))
    end
    return bucket
end

function WallDraft.new(init)
    local self = setmetatable({}, WallDraft)
    self.name = Common.trim(init and init.name or "")
    self.lookId = tonumber((init and (init.lookId or init.look_id)) or 0) or 0
    self.serverLookId = tonumber((init and (init.serverLookId or init.server_lookid)) or 0) or 0
    self.isNew = Common.normalize_bool(init and init.isNew, true)
    self.sourceName = Common.trim(init and (init.sourceName or init.source_name or init.name) or "")
    self.friends = Common.clone(init and init.friends or {})
    self.redirectName = Common.trim(init and (init.redirectName or init.redirect_to) or "")
    self.extraChildrenXml = tostring((init and (init.extraChildrenXml or init.extra_children_xml)) or "")
    self.alignments = {}

    for _, alignment in ipairs(Common.alignment_order) do
        self.alignments[alignment.token] = clone_bucket(init and init.alignments and init.alignments[alignment.token], alignment.token)
    end

    if self.lookId <= 0 then
        self.lookId = self:getPreviewLookId()
    end
    if self.serverLookId <= 0 then
        self.serverLookId = self:getPreviewItemId()
    end

    return self
end

function WallDraft:getBucket(token)
    token = Common.trim(token)
    if token == "" or not self.alignments[token] then
        token = Common.alignment_order[1].token
    end
    return self.alignments[token]
end

function WallDraft:getPreviewItemId()
    for _, alignment in ipairs(Common.alignment_order) do
        local item_id = Common.representative_item_id(self.alignments[alignment.token])
        if item_id > 0 then
            return item_id
        end
    end
    return 0
end

function WallDraft:getPreviewLookId()
    for _, alignment in ipairs(Common.alignment_order) do
        local bucket = self.alignments[alignment.token]
        for _, item in ipairs(bucket.wallItems or {}) do
            if tonumber(item.lookId or 0) > 0 then
                return tonumber(item.lookId or 0) or 0
            end
        end
        for _, door in ipairs(bucket.doorItems or {}) do
            if tonumber(door.lookId or 0) > 0 then
                return tonumber(door.lookId or 0) or 0
            end
        end
    end
    return 0
end

function WallDraft:setName(name)
    self.name = Common.trim(name)
end

function WallDraft:setLookId(value)
    self.lookId = tonumber(value or 0) or 0
end

function WallDraft:setServerLookId(value)
    self.serverLookId = tonumber(value or 0) or 0
end

function WallDraft:setFriendsFromText(text)
    local names = {}
    local seen = {}
    for raw in tostring(text or ""):gmatch("[^,]+") do
        local name = Common.trim(raw)
        local key = string.lower(name)
        if name ~= "" and not seen[key] then
            seen[key] = true
            table.insert(names, name)
        end
    end
    self.friends = names
end

function WallDraft:setRedirectName(name)
    self.redirectName = Common.trim(name)
end

function WallDraft:addWallItem(token, raw_item, chance)
    local bucket = self:getBucket(token)
    local reference = build_item_reference(raw_item)
    if not reference then
        return false, "Invalid wall item."
    end
    for _, item in ipairs(bucket.wallItems) do
        if tonumber(item.itemId or 0) == reference.itemId then
            return false, "This wall item already exists in the selected alignment."
        end
    end

    reference.chance = normalize_chance(chance, 1)
    table.insert(bucket.wallItems, reference)
    if self.lookId <= 0 then
        self.lookId = reference.lookId or 0
    end
    if self.serverLookId <= 0 then
        self.serverLookId = reference.itemId or 0
    end
    return true
end

function WallDraft:removeWallItem(token, index)
    local bucket = self:getBucket(token)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #bucket.wallItems then
        return false
    end
    table.remove(bucket.wallItems, index)
    return true
end

function WallDraft:setWallItemChance(token, index, chance)
    local bucket = self:getBucket(token)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #bucket.wallItems then
        return false
    end
    bucket.wallItems[index].chance = normalize_chance(chance, bucket.wallItems[index].chance)
    return true
end

function WallDraft:addDoorItem(token, raw_item, config)
    local bucket = self:getBucket(token)
    local reference = build_item_reference(raw_item)
    if not reference then
        return false, "Invalid door item."
    end
    for _, door in ipairs(bucket.doorItems) do
        if tonumber(door.itemId or 0) == reference.itemId then
            return false, "This door item already exists in the selected alignment."
        end
    end

    reference.doorType = normalize_door_type(config and config.doorType or "normal")
    reference.isOpen = Common.normalize_bool(config and config.isOpen, false)
    reference.isLocked = Common.normalize_bool(config and config.isLocked, false)
    reference.hate = Common.normalize_bool(config and config.hate, false)
    table.insert(bucket.doorItems, reference)
    if self.lookId <= 0 then
        self.lookId = reference.lookId or 0
    end
    if self.serverLookId <= 0 then
        self.serverLookId = reference.itemId or 0
    end
    return true
end

function WallDraft:removeDoorItem(token, index)
    local bucket = self:getBucket(token)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #bucket.doorItems then
        return false
    end
    table.remove(bucket.doorItems, index)
    return true
end

function WallDraft:updateDoorItem(token, index, config)
    local bucket = self:getBucket(token)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #bucket.doorItems then
        return false
    end
    local door = bucket.doorItems[index]
    door.doorType = normalize_door_type(config and config.doorType or door.doorType)
    door.isOpen = Common.normalize_bool(config and config.isOpen, door.isOpen)
    door.isLocked = Common.normalize_bool(config and config.isLocked, door.isLocked)
    door.hate = Common.normalize_bool(config and config.hate, door.hate)
    return true
end

function WallDraft:getValidation()
    local errors = {}
    local warnings = {}
    local has_any_items = false
    local has_base_wall = false

    if self.name == "" then
        table.insert(errors, "Nazwa wall brusha nie moze byc pusta.")
    end
    if self.redirectName ~= "" and self.redirectName == self.name then
        table.insert(errors, "Redirect nie moze wskazywac na ten sam wall brush.")
    end

    local friend_seen = {}
    for index, friend_name in ipairs(self.friends or {}) do
        local key = string.lower(friend_name or "")
        if key == "" then
            table.insert(errors, string.format("Friend link %d ma pusta nazwe.", index))
        elseif friend_seen[key] then
            table.insert(errors, string.format("Friend link '%s' wystepuje wiecej niz raz.", friend_name))
        else
            friend_seen[key] = true
        end
    end
    if self.redirectName ~= "" and not friend_seen[string.lower(self.redirectName)] then
        table.insert(warnings, "Redirect brush nie jest na liscie friends. Zostanie dopisany przy eksporcie.")
    end

    for _, alignment in ipairs(Common.alignment_order) do
        local bucket = self.alignments[alignment.token]
        if bucket then
            local seen_items = {}
            for index, item in ipairs(bucket.wallItems or {}) do
                has_any_items = true
                has_base_wall = true
                local item_id = tonumber(item.itemId or 0) or 0
                local chance = tonumber(item.chance or 0) or 0
                if item_id <= 0 then
                    table.insert(errors, string.format("%s wall item %d ma niepoprawny item id.", alignment.label, index))
                end
                if chance < 0 then
                    table.insert(errors, string.format("%s wall item %d ma ujemny chance.", alignment.label, index))
                end
                if item_id > 0 then
                    if seen_items[item_id] then
                        table.insert(errors, string.format("%s duplikuje wall item %d.", alignment.label, item_id))
                    end
                    seen_items[item_id] = true
                end
            end

            for index, door in ipairs(bucket.doorItems or {}) do
                has_any_items = true
                local item_id = tonumber(door.itemId or 0) or 0
                if item_id <= 0 then
                    table.insert(errors, string.format("%s door %d ma niepoprawny item id.", alignment.label, index))
                end
                if door.doorType == "undefined" then
                    table.insert(errors, string.format("%s door %d ma niepoprawny type.", alignment.label, index))
                end
            end
        end
    end

    if not has_any_items then
        table.insert(errors, "Wall brush jest pusty.")
    end
    if not has_base_wall then
        table.insert(errors, "Wall brush musi miec przynajmniej jeden zwykly wall item.")
    end

    return {
        hasData = has_any_items,
        isValid = #errors == 0,
        errors = errors,
        warnings = warnings,
    }
end

function WallDraft:clone()
    return WallDraft.new({
        name = self.name,
        lookId = self.lookId,
        serverLookId = self.serverLookId,
        isNew = self.isNew,
        sourceName = self.sourceName,
        friends = Common.clone(self.friends),
        redirectName = self.redirectName,
        extraChildrenXml = self.extraChildrenXml,
        alignments = Common.clone(self.alignments),
    })
end

return WallDraft
