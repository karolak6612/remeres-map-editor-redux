local Common = dofile("common.lua")

local BorderDraft = {}
BorderDraft.__index = BorderDraft

local function clone_table(value)
    return Common.clone(value)
end

local function read_item_id(raw_item)
    if type(raw_item) == "table" then
        return tonumber(raw_item.itemId or raw_item.item_id or raw_item.id or raw_item.rawId or 0) or 0
    end
    return tonumber(raw_item or 0) or 0
end

local function build_raw_reference(raw_item)
    local item_id = read_item_id(raw_item)
    if item_id <= 0 then
        return nil
    end

    local info = Common.safe_item_info(item_id)
    local source = type(raw_item) == "table" and raw_item or {}

    return {
        itemId = item_id,
        rawId = tonumber(source.rawId or source.raw_id or item_id) or item_id,
        lookId = tonumber(source.lookId or source.look_id or source.clientId or (info and info.clientId) or 0) or 0,
        name = source.name or (info and info.name) or ("Item " .. tostring(item_id)),
    }
end

local function fresh_slots()
    local slots = {}
    for _, slot_name in ipairs(Common.slot_order) do
        slots[slot_name] = nil
    end
    return slots
end

function BorderDraft.new(init)
    local self = setmetatable({}, BorderDraft)
    self.borderId = tonumber((init and (init.borderId or init.id)) or 0) or 0
    self.name = tostring((init and init.name) or "")
    self.group = tonumber((init and init.group) or 0) or 0
    self.isNew = init and (init.isNew == true or init.is_new == true) or false
    self.status = init and init.status or nil
    self.slots = fresh_slots()

    for slot_name, raw_item in pairs((init and init.slots) or {}) do
        self:assignSlot(slot_name, raw_item)
    end

    return self
end

function BorderDraft.isValidSlotName(_, slot_name)
    return Common.is_valid_slot_name(slot_name)
end

function BorderDraft:getSlotAssignment(slot_name)
    if not self:isValidSlotName(slot_name) then
        return nil
    end
    return self.slots[slot_name]
end

function BorderDraft:getSlotItemId(slot_name)
    local assignment = self:getSlotAssignment(slot_name)
    return assignment and assignment.itemId or 0
end

function BorderDraft:clear()
    self.slots = fresh_slots()
end

function BorderDraft:isEmpty()
    for _, slot_name in ipairs(Common.slot_order) do
        if self:getSlotItemId(slot_name) > 0 then
            return false
        end
    end
    return true
end

function BorderDraft:assignSlot(slot_name, raw_item)
    if not self:isValidSlotName(slot_name) then
        return false, "Invalid border slot: " .. tostring(slot_name)
    end

    local reference = build_raw_reference(raw_item)
    if not reference then
        return false, "RAW item is missing or invalid."
    end

    self.slots[slot_name] = reference
    return true
end

function BorderDraft:clearSlot(slot_name)
    if not self:isValidSlotName(slot_name) then
        return false, "Invalid border slot: " .. tostring(slot_name)
    end

    self.slots[slot_name] = nil
    return true
end

function BorderDraft:getAssignedSlots()
    local assigned = {}
    for _, slot_name in ipairs(Common.slot_order) do
        local assignment = self.slots[slot_name]
        if assignment then
            assigned[slot_name] = clone_table(assignment)
        end
    end
    return assigned
end

function BorderDraft:getEmptySlots()
    local empty_slots = {}
    for _, slot_name in ipairs(Common.slot_order) do
        if self:getSlotItemId(slot_name) <= 0 then
            table.insert(empty_slots, slot_name)
        end
    end
    return empty_slots
end

function BorderDraft:getValidation()
    local has_data = not self:isEmpty()
    return {
        hasAssignments = has_data,
        hasData = has_data,
        emptySlots = self:getEmptySlots(),
        invalidSlots = {},
    }
end

function BorderDraft:hasRequiredDataForSave()
    return self.borderId > 0 and not self:isEmpty()
end

function BorderDraft:clone()
    return BorderDraft.new({
        borderId = self.borderId,
        name = self.name,
        group = self.group,
        isNew = self.isNew,
        status = self.status,
        slots = self:getAssignedSlots(),
    })
end

function BorderDraft:toBorderItemEntries()
    local entries = {}
    for _, slot_name in ipairs(Common.slot_order) do
        local assignment = self.slots[slot_name]
        if assignment and assignment.itemId and assignment.itemId > 0 then
            table.insert(entries, {
                slotName = slot_name,
                edge = Common.slot_meta[slot_name].edge,
                itemId = assignment.itemId,
                raw = clone_table(assignment),
            })
        end
    end
    return entries
end

function BorderDraft:toXmlExportData()
    return {
        borderId = self.borderId,
        name = self.name,
        slots = self:getAssignedSlots(),
        borderItems = self:toBorderItemEntries(),
        emptySlots = self:getEmptySlots(),
        hasRequiredDataForSave = self:hasRequiredDataForSave(),
    }
end

return BorderDraft
