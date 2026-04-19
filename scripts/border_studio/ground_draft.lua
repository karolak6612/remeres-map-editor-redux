local Common = dofile("common.lua")

local GroundDraft = {}
GroundDraft.__index = GroundDraft

local function build_ground_reference(raw_item)
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

local function normalize_align(value)
    value = string.lower(tostring(value or "outer"))
    if value ~= "inner" then
        return "outer"
    end
    return value
end

local function normalize_friend(value)
    local text = tostring(value or ""):match("^%s*(.-)%s*$")
    if text == "" then
        return nil
    end
    return text
end

function GroundDraft.new(init)
    local self = setmetatable({}, GroundDraft)
    self.name = tostring((init and init.name) or "")
    self.centerGroundItem = build_ground_reference(init and (init.centerGroundItem or init.center_ground_item))
    self.linkedBorderId = tonumber((init and (init.linkedBorderId or init.linked_border_id)) or 0) or 0
    self.borderTargetBrushName = tostring((init and (init.borderTargetBrushName or init.border_target_brush_name)) or "")
    self.align = normalize_align(init and (init.align or init.alignMode or init.align_mode))
    self.friends = {}
    self.zOrder = tonumber((init and (init.zOrder or init.z_order)) or 0) or 0

    for _, friend_name in ipairs((init and init.friends) or {}) do
        self:addFriend(friend_name)
    end

    return self
end

function GroundDraft:setName(name)
    self.name = tostring(name or "")
end

function GroundDraft:setCenterGroundItem(raw_item)
    self.centerGroundItem = build_ground_reference(raw_item)
    return self.centerGroundItem ~= nil
end

function GroundDraft:clearCenterGroundItem()
    self.centerGroundItem = nil
end

function GroundDraft:setLinkedBorderId(border_id)
    self.linkedBorderId = tonumber(border_id or 0) or 0
end

function GroundDraft:setBorderTargetBrushName(name)
    self.borderTargetBrushName = tostring(name or "")
end

function GroundDraft:setAlign(mode)
    self.align = normalize_align(mode)
end

function GroundDraft:setFriends(friend_list)
    self.friends = {}
    for _, friend_name in ipairs(friend_list or {}) do
        self:addFriend(friend_name)
    end
end

function GroundDraft:addFriend(friend_name)
    friend_name = normalize_friend(friend_name)
    if not friend_name then
        return false
    end

    for _, existing in ipairs(self.friends) do
        if existing == friend_name then
            return false
        end
    end

    table.insert(self.friends, friend_name)
    return true
end

function GroundDraft:removeFriend(friend_name)
    friend_name = normalize_friend(friend_name)
    if not friend_name then
        return false
    end

    for index, existing in ipairs(self.friends) do
        if existing == friend_name then
            table.remove(self.friends, index)
            return true
        end
    end

    return false
end

function GroundDraft:clearFriends()
    self.friends = {}
end

function GroundDraft:setZOrder(z_order)
    self.zOrder = tonumber(z_order or 0) or 0
end

function GroundDraft:hasCenterGroundItem()
    return self.centerGroundItem ~= nil and (self.centerGroundItem.itemId or 0) > 0
end

function GroundDraft:getValidation()
    return {
        hasCenterGroundItem = self:hasCenterGroundItem(),
        hasLinkedBorder = self.linkedBorderId > 0,
        hasBorderTargetBrush = self.borderTargetBrushName ~= "",
        hasName = self.name ~= "",
        align = self.align,
        friendsCount = #self.friends,
        zOrder = self.zOrder,
    }
end

function GroundDraft:clone()
    return GroundDraft.new({
        name = self.name,
        centerGroundItem = self.centerGroundItem and Common.clone(self.centerGroundItem) or nil,
        linkedBorderId = self.linkedBorderId,
        borderTargetBrushName = self.borderTargetBrushName,
        align = self.align,
        friends = Common.clone(self.friends),
        zOrder = self.zOrder,
    })
end

function GroundDraft:toGroundData()
    return {
        name = self.name,
        centerGroundItem = self.centerGroundItem and Common.clone(self.centerGroundItem) or nil,
        linkedBorderId = self.linkedBorderId,
        borderTargetBrushName = self.borderTargetBrushName,
        align = self.align,
        friends = Common.clone(self.friends),
        zOrder = self.zOrder,
    }
end

return GroundDraft
