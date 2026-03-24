-- @Title: Test Tile & Item API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("tile properties", function()
    if not app.hasMap() then return end
    
    local tile = app.map:getOrCreateTile(100, 100, 7)
    framework.assert(tile.x == 100, "tile.x should be 100")
    framework.assert(tile.y == 100, "tile.y should be 100")
    framework.assert(tile.z == 7, "tile.z should be 7")
    framework.assert(type(tile.position) == "table", "tile.position should be table")
    framework.assert(type(tile.itemCount) == "number", "tile.itemCount should be number")
    framework.assert(type(tile.items) == "table", "tile.items should be table")
end)

framework.test("tile methods", function()
    if not app.hasMap() then return end
    
    local tile = app.map:getOrCreateTile(100, 100, 7)
    framework.assert(type(tile.addItem) == "function", "addItem should be function")
    framework.assert(type(tile.removeItem) == "function", "removeItem should be function")
    framework.assert(type(tile.setCreature) == "function", "setCreature should be function")
    framework.assert(type(tile.removeCreature) == "function", "removeCreature should be function")
    framework.assert(type(tile.setSpawn) == "function", "setSpawn should be function")
    framework.assert(type(tile.removeSpawn) == "function", "removeSpawn should be function")
    framework.assert(type(tile.getTopItem) == "function", "getTopItem should be function")
end)

framework.test("tile item operations", function()
    if not app.hasMap() then return end
    
    app.transaction("Test tile item ops", function()
        local tile = app.map:getOrCreateTile(100, 101, 7)
        local countBefore = tile.itemCount
        local item = tile:addItem(2160) -- Crystal coin
        framework.assert(type(item) ~= "nil", "addItem should return item")
        framework.assert(item.id == 2160, "item.id should be 2160")
        framework.assert(tile.itemCount == countBefore + 1, "itemCount should increase")
        
        tile:removeItem(item)
        framework.assert(tile.itemCount == countBefore, "itemCount should decrease after removal")
    end)
end)

framework.test("tile creature/spawn", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature/spawn", function()
        local tile = app.map:getOrCreateTile(100, 103, 7)
        tile:setCreature("Rat")
        local c = tile.creature
        framework.assert(type(c) ~= "nil", "creature should exist")
        framework.assert(c.name == "Rat", "creature name should be Rat")
        framework.assert(type(c.spawnTime) == "number", "spawnTime should be number")
        
        tile:removeCreature()
        framework.assert(type(tile.creature) == "nil" or tile.creature == nil, "creature should be removed")
        
        tile:setSpawn(5)
        local s = tile.spawn
        framework.assert(type(s) ~= "nil", "spawn should exist")
        -- The README says radius (or size)
        framework.assert(s.radius == 5 or s.size == 5, "spawn radius/size should be 5")
        
        tile:removeSpawn()
        framework.assert(type(tile.spawn) == "nil" or tile.spawn == nil, "spawn should be removed")
    end)
end)

framework.test("item properties", function()
    if not app.hasMap() then return end
    
    app.transaction("Test item properties", function()
        local tile = app.map:getOrCreateTile(100, 102, 7)
        local item = tile:addItem(2160, 5)
        framework.assert(item.id == 2160, "item.id should be 2160")
        framework.assert(item.count == 5, "item.count should be 5")
        framework.assert(type(item.name) == "string", "item.name should be string")
        framework.assert(type(item.isStackable) == "boolean", "item.isStackable should be boolean")
        framework.assert(type(item.isMoveable) == "boolean", "item.isMoveable should be boolean")
        
        local clone = item:clone()
        framework.assert(clone.id == item.id, "clone.id should match")
        framework.assert(clone.count == item.count, "clone.count should match")
        
        if type(item.rotate) == "function" then
            item:rotate()
        end
    end)
end)

framework.summary()
