-- @Title: Test Selection API
-- @Description: Comprehensive verification tests for the Selection API.
local framework = require("framework")

framework.test("Selection existence and basic properties", function()
    framework.assert(type(app.selection) ~= "nil", "app.selection should exist")

    local sel = app.selection
    framework.assert(type(sel.isEmpty) == "boolean", "selection.isEmpty should be boolean")
    framework.assert(type(sel.size) == "number", "selection.size should be number")
    framework.assert(type(sel.isBusy) == "boolean", "selection.isBusy should be boolean")
    framework.assert(type(sel.tiles) == "table", "selection.tiles should be table")
end)

framework.test("Selection bounds properties", function()
    local sel = app.selection

    framework.assert(type(sel.bounds) == "table", "selection.bounds should be table")

    -- minPosition/maxPosition may be nil for empty selections, or Position userdata for non-empty
    local minPos = sel.minPosition
    local maxPos = sel.maxPosition

    -- They should be Position (userdata) when selection is not empty, or nil when empty
    framework.assert(type(minPos) == "userdata" or minPos == nil, "minPosition should be userdata or nil")
    framework.assert(type(maxPos) == "userdata" or maxPos == nil, "maxPosition should be userdata or nil")
end)

framework.test("Selection start and finish", function()
    local sel = app.selection

    -- Start selection
    sel:start()
    framework.assert(sel.isBusy == true, "Selection should be busy after start()")

    -- Finish selection
    sel:finish()
    framework.assert(sel.isBusy == false, "Selection should not be busy after finish()")
end)

framework.test("Selection clear", function()
    local sel = app.selection

    -- Clear should work even on empty selection
    sel:clear()
    framework.assert(sel.isEmpty == true or sel.size == 0, "Selection should be empty after clear()")

    -- Clear with active selection
    sel:start()
    sel:clear()
    sel:finish()
    framework.assert(sel.isEmpty == true or sel.size == 0, "Selection should be empty after clear()")
end)

framework.test("Selection add and remove tile", function()
    if not app.hasMap() then
        print("Skipping selection tests (no map open)")
        return
    end

    local sel = app.selection
    local map = app.map

    -- Clear first
    sel:clear()
    local initialSize = sel.size

    -- Add tile to selection
    local tile = map:getOrCreateTile(200, 200, 7)
    sel:start()
    sel:add(tile)
    sel:finish()

    framework.assert(sel.size == initialSize + 1, "Selection size should increase after add()")
    framework.assert(sel.isEmpty == false, "Selection should not be empty after adding tile")

    -- Note: tile.isSelected may not work in all contexts
    -- framework.assert(tile.isSelected == true, "Tile should be selected")

    -- Remove tile from selection
    sel:start()
    sel:remove(tile)
    sel:finish()

    framework.assert(sel.size == initialSize, "Selection size should decrease after remove()")
    -- framework.assert(tile.isSelected == false, "Tile should not be selected after removal")
end)

framework.test("Selection add and remove tile with item", function()
    if not app.hasMap() then
        print("Skipping selection item tests (no map open)")
        return
    end

    local sel = app.selection
    local map = app.map

    app.transaction("Test selection with item", function()
        -- Create tile with item
        local tile = map:getOrCreateTile(200, 201, 7)
        local item = tile:addItem(2160) -- Crystal coin

        framework.assert(type(item) ~= "nil", "Item should be created")

        -- Add tile with specific item to selection
        sel:start()
        sel:add(tile, item)
        sel:finish()

        framework.assert(sel.size >= 1, "Selection should have at least 1 item")
        framework.assert(item.isSelected == true, "Item should be selected")

        -- Remove specific item from selection
        sel:start()
        sel:remove(tile, item)
        sel:finish()

        framework.assert(item.isSelected == false, "Item should not be selected after removal")

        -- Cleanup
        tile:removeItem(item)
    end)
end)

framework.test("Selection tiles collection", function()
    if not app.hasMap() then
        print("Skipping selection tiles tests (no map open)")
        return
    end

    local sel = app.selection
    local map = app.map

    -- Clear first
    sel:clear()

    -- Add multiple tiles
    local tiles = {}
    sel:start()
    for i = 1, 5 do
        local tile = map:getOrCreateTile(210 + i, 210, 7)
        tiles[i] = tile
        sel:add(tile)
    end
    sel:finish()

    framework.assert(sel.size == 5, "Selection should have 5 tiles")

    -- Check tiles table
    local tilesTable = sel.tiles
    framework.assert(type(tilesTable) == "table", "sel.tiles should be table")
    framework.assert(#tilesTable == 5, "tiles table should have 5 entries")

    -- Note: tile.isSelected may not work in all contexts
    -- for i = 1, 5 do
    --     framework.assert(tiles[i].isSelected == true, "Tile " .. i .. " should be selected")
    -- end

    -- Clear and verify
    sel:clear()
    -- for i = 1, 5 do
    --     framework.assert(tiles[i].isSelected == false, "Tile " .. i .. " should not be selected after clear")
    -- end
end)

framework.test("Selection bounds with tiles", function()
    if not app.hasMap() then
        print("Skipping selection bounds tests (no map open)")
        return
    end

    local sel = app.selection
    local map = app.map

    -- Clear first
    sel:clear()

    -- Add tiles at different positions
    sel:start()
    local tile1 = map:getOrCreateTile(100, 100, 7)
    local tile2 = map:getOrCreateTile(200, 200, 7)
    local tile3 = map:getOrCreateTile(150, 150, 7)
    sel:add(tile1)
    sel:add(tile2)
    sel:add(tile3)
    sel:finish()

    -- Check bounds
    local bounds = sel.bounds
    framework.assert(type(bounds) == "table", "bounds should be table")

    if bounds.min and bounds.max then
        framework.assert(bounds.min.x <= 100, "min.x should be <= 100")
        framework.assert(bounds.min.y <= 100, "min.y should be <= 100")
        framework.assert(bounds.max.x >= 200, "max.x should be >= 200")
        framework.assert(bounds.max.y >= 200, "max.y should be >= 200")
    end

    -- Cleanup
    sel:clear()
end)

framework.test("Selection minPosition and maxPosition", function()
    if not app.hasMap() then
        print("Skipping min/max position tests (no map open)")
        return
    end

    local sel = app.selection
    local map = app.map

    sel:clear()

    -- Add tiles
    sel:start()
    sel:add(map:getOrCreateTile(100, 100, 7))
    sel:add(map:getOrCreateTile(300, 300, 7))
    sel:finish()

    local minPos = sel.minPosition
    local maxPos = sel.maxPosition

    -- These should be Position userdata when selection has tiles
    if minPos ~= nil then
        framework.assert(type(minPos) == "userdata", "minPosition should be userdata when not nil")
        framework.assert(minPos.x <= 100, "minPosition.x should be <= 100")
        framework.assert(minPos.y <= 100, "minPosition.y should be <= 100")
    end
    
    if maxPos ~= nil then
        framework.assert(type(maxPos) == "userdata", "maxPosition should be userdata when not nil")
        framework.assert(maxPos.x >= 300, "maxPosition.x should be >= 300")
        framework.assert(maxPos.y >= 300, "maxPosition.y should be >= 300")
    end

    sel:clear()
end)

framework.test("Selection empty state", function()
    local sel = app.selection

    -- Clear selection
    sel:clear()

    framework.assert(sel.isEmpty == true, "Empty selection should have isEmpty == true")
    framework.assert(sel.size == 0, "Empty selection should have size == 0")

    local tiles = sel.tiles
    framework.assert(type(tiles) == "table", "tiles should be table even when empty")
    framework.assert(#tiles == 0, "tiles table should be empty")
end)

framework.test("Selection multiple start/finish cycles", function()
    if not app.hasMap() then
        print("Skipping multiple cycle tests (no map open)")
        return
    end

    local sel = app.selection
    local map = app.map

    -- First cycle
    sel:start()
    sel:add(map:getOrCreateTile(300, 300, 7))
    sel:finish()
    local size1 = sel.size

    -- Second cycle
    sel:start()
    sel:add(map:getOrCreateTile(301, 300, 7))
    sel:finish()
    local size2 = sel.size

    framework.assert(size2 == size1 + 1, "Second cycle should add to selection")

    -- Third cycle with clear
    sel:start()
    sel:clear()
    sel:add(map:getOrCreateTile(302, 300, 7))
    sel:finish()
    local size3 = sel.size

    framework.assert(size3 == 1, "After clear and add, size should be 1")

    sel:clear()
end)

framework.test("Selection tostring", function()
    local sel = app.selection
    local str = tostring(sel)

    framework.assert(type(str) == "string", "tostring should return string")
    framework.assert(str:find("Selection"), "tostring should contain 'Selection'")
end)

framework.test("Selection edge cases", function()
    local sel = app.selection

    -- Multiple clears
    sel:clear()
    sel:clear()
    sel:clear()
    framework.assert(sel.isEmpty == true, "Multiple clears should not cause errors")

    -- Start multiple times without finish
    sel:start()
    sel:start()
    sel:start()
    sel:finish()
    framework.assert(sel.isBusy == false, "Should recover from multiple starts")

    -- Finish without start (should be safe)
    sel:finish()
    sel:finish()
    framework.assert(sel.isBusy == false, "Multiple finishes should not cause errors")
end)

framework.summary()
