-- @Title: Test Creature & Spawn API
-- @Description: Comprehensive tests for Creature, Spawn, and related global functions
local framework = require("framework")

-- ============================================================================
-- Direction Enum Tests
-- ============================================================================

framework.test("Direction enum exists", function()
    framework.assert(type(Direction) == "table", "Direction enum should exist")
    framework.assert(type(Direction.NORTH) == "number", "Direction.NORTH should be number")
    framework.assert(type(Direction.EAST) == "number", "Direction.EAST should be number")
    framework.assert(type(Direction.SOUTH) == "number", "Direction.SOUTH should be number")
    framework.assert(type(Direction.WEST) == "number", "Direction.WEST should be number")
end)

framework.test("Direction enum values", function()
    -- Verify all directions are distinct
    framework.assert(Direction.NORTH ~= Direction.EAST, "NORTH should differ from EAST")
    framework.assert(Direction.NORTH ~= Direction.SOUTH, "NORTH should differ from SOUTH")
    framework.assert(Direction.NORTH ~= Direction.WEST, "NORTH should differ from WEST")
    framework.assert(Direction.EAST ~= Direction.SOUTH, "EAST should differ from SOUTH")
    framework.assert(Direction.EAST ~= Direction.WEST, "EAST should differ from WEST")
    framework.assert(Direction.SOUTH ~= Direction.WEST, "SOUTH should differ from WEST")
end)

-- ============================================================================
-- Global Creature Functions Tests
-- ============================================================================

framework.test("creatureExists function", function()
    framework.assert(type(creatureExists) == "function", "creatureExists should be function")
    
    -- Test with known creature names (these should exist in standard OTB)
    local ratExists = creatureExists("Rat")
    framework.assert(type(ratExists) == "boolean", "creatureExists should return boolean")
    
    -- Test with non-existent creature
    local fakeExists = creatureExists("FakeCreature_That_Does_Not_Exist_12345")
    framework.assert(fakeExists == false, "creatureExists should return false for unknown creature")
end)

framework.test("isNpcType function", function()
    framework.assert(type(isNpcType) == "function", "isNpcType should be function")
    
    -- Test with a creature name
    local result = isNpcType("Rat")
    framework.assert(type(result) == "boolean", "isNpcType should return boolean")
    
    -- Test with non-existent creature (should return false or nil)
    local fakeResult = isNpcType("FakeNpc_That_Does_Not_Exist_12345")
    framework.assert(fakeResult == false or fakeResult == nil, "isNpcType should return false/nil for unknown")
end)

-- ============================================================================
-- Creature Property Tests
-- ============================================================================

framework.test("Creature name property (readonly)", function()
    if not app.hasMap() then
        print("Skipping creature tests (no map open)")
        return
    end
    
    app.transaction("Test creature name", function()
        local tile = app.map:getOrCreateTile(100, 100, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        framework.assert(type(creature) ~= "nil", "creature should exist on tile")
        framework.assert(type(creature.name) == "string", "creature.name should be string")
        framework.assert(creature.name == "Rat", "creature.name should be 'Rat'")
        framework.assert(#creature.name > 0, "creature.name should not be empty")
        
        tile:removeCreature()
    end)
end)

framework.test("Creature isNpc property (readonly)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature isNpc", function()
        local tile = app.map:getOrCreateTile(100, 101, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        framework.assert(type(creature.isNpc) == "boolean", "creature.isNpc should be boolean")
        framework.assert(creature.isNpc == false, "Rat should not be an NPC")
        
        tile:removeCreature()
    end)
end)

framework.test("Creature spawnTime property (read/write)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature spawnTime", function()
        local tile = app.map:getOrCreateTile(100, 102, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        -- Test read
        framework.assert(type(creature.spawnTime) == "number", "creature.spawnTime should be number")
        local originalTime = creature.spawnTime
        
        -- Test write
        creature.spawnTime = 30
        framework.assert(creature.spawnTime == 30, "spawnTime should be settable to 30")
        
        creature.spawnTime = 5
        framework.assert(creature.spawnTime == 5, "spawnTime should be settable to 5")
        
        -- Test edge cases
        creature.spawnTime = 0
        framework.assert(creature.spawnTime == 0, "spawnTime should accept 0")
        
        creature.spawnTime = 9999
        framework.assert(creature.spawnTime == 9999, "spawnTime should accept large values")
        
        -- Restore original
        creature.spawnTime = originalTime
        
        tile:removeCreature()
    end)
end)

framework.test("Creature direction property (read/write)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature direction", function()
        local tile = app.map:getOrCreateTile(100, 103, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        -- Test read
        framework.assert(type(creature.direction) == "number", "creature.direction should be number")
        
        -- Test write with enum values
        creature.direction = Direction.NORTH
        framework.assert(creature.direction == Direction.NORTH, "direction should be NORTH")
        
        creature.direction = Direction.EAST
        framework.assert(creature.direction == Direction.EAST, "direction should be EAST")
        
        creature.direction = Direction.SOUTH
        framework.assert(creature.direction == Direction.SOUTH, "direction should be SOUTH")
        
        creature.direction = Direction.WEST
        framework.assert(creature.direction == Direction.WEST, "direction should be WEST")
        
        -- Test with raw integer values
        creature.direction = 0
        framework.assert(creature.direction == 0, "direction should accept integer 0")
        
        tile:removeCreature()
    end)
end)

framework.test("Creature isSelected property (readonly)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature isSelected", function()
        local tile = app.map:getOrCreateTile(100, 104, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        framework.assert(type(creature.isSelected) == "boolean", "creature.isSelected should be boolean")
        framework.assert(creature.isSelected == false, "new creature should not be selected")
        
        tile:removeCreature()
    end)
end)

-- ============================================================================
-- Creature Method Tests
-- ============================================================================

framework.test("Creature select method", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature select", function()
        local tile = app.map:getOrCreateTile(100, 105, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        framework.assert(type(creature.select) == "function", "creature.select should be function")
        
        creature:select()
        framework.assert(creature.isSelected == true, "creature should be selected after select()")
        
        tile:removeCreature()
    end)
end)

framework.test("Creature deselect method", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature deselect", function()
        local tile = app.map:getOrCreateTile(100, 106, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        framework.assert(type(creature.deselect) == "function", "creature.deselect should be function")
        
        creature:select()
        framework.assert(creature.isSelected == true, "creature should be selected")
        
        creature:deselect()
        framework.assert(creature.isSelected == false, "creature should not be selected after deselect()")
        
        -- Test deselect on already deselected creature
        creature:deselect()
        framework.assert(creature.isSelected == false, "deselect on deselected creature should not error")
        
        tile:removeCreature()
    end)
end)

framework.test("Creature select/deselect cycle", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature select/deselect cycle", function()
        local tile = app.map:getOrCreateTile(100, 107, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        -- Multiple cycles
        for i = 1, 3 do
            creature:select()
            framework.assert(creature.isSelected == true, "cycle " .. i .. ": should be selected")
            creature:deselect()
            framework.assert(creature.isSelected == false, "cycle " .. i .. ": should be deselected")
        end
        
        tile:removeCreature()
    end)
end)

-- ============================================================================
-- Creature String Representation
-- ============================================================================

framework.test("Creature to_string", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature to_string", function()
        local tile = app.map:getOrCreateTile(100, 108, 7)
        tile:setCreature("Rat")
        local creature = tile.creature
        
        local str = tostring(creature)
        framework.assert(type(str) == "string", "tostring(creature) should return string")
        framework.assert(str:find("Creature"), "tostring should contain 'Creature'")
        framework.assert(str:find("Rat"), "tostring should contain creature name")
        
        tile:removeCreature()
    end)
end)

-- ============================================================================
-- Spawn Property Tests
-- ============================================================================

framework.test("Spawn size property (read/write)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn size", function()
        local tile = app.map:getOrCreateTile(100, 110, 7)
        tile:setSpawn(5)
        local spawn = tile.spawn
        
        framework.assert(type(spawn) ~= "nil", "spawn should exist on tile")
        framework.assert(type(spawn.size) == "number", "spawn.size should be number")
        framework.assert(spawn.size == 5, "spawn.size should be 5")
        
        -- Test write
        spawn.size = 10
        framework.assert(spawn.size == 10, "spawn.size should be settable to 10")
        
        spawn.size = 1
        framework.assert(spawn.size == 1, "spawn.size should accept minimum value 1")
        
        -- Test edge: large value (should be capped at < 100 per C++ code)
        spawn.size = 50
        framework.assert(spawn.size == 50, "spawn.size should accept 50")
        
        tile:removeSpawn()
    end)
end)

framework.test("Spawn radius property (alias for size)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn radius", function()
        local tile = app.map:getOrCreateTile(100, 111, 7)
        tile:setSpawn(7)
        local spawn = tile.spawn
        
        framework.assert(type(spawn.radius) == "number", "spawn.radius should be number")
        framework.assert(spawn.radius == 7, "spawn.radius should equal size")
        
        -- Test that radius and size are linked
        spawn.radius = 15
        framework.assert(spawn.size == 15, "setting radius should update size")
        
        spawn.size = 20
        framework.assert(spawn.radius == 20, "setting size should update radius")
        
        tile:removeSpawn()
    end)
end)

framework.test("Spawn isSelected property (readonly)", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn isSelected", function()
        local tile = app.map:getOrCreateTile(100, 112, 7)
        tile:setSpawn(5)
        local spawn = tile.spawn
        
        framework.assert(type(spawn.isSelected) == "boolean", "spawn.isSelected should be boolean")
        framework.assert(spawn.isSelected == false, "new spawn should not be selected")
        
        tile:removeSpawn()
    end)
end)

-- ============================================================================
-- Spawn Method Tests
-- ============================================================================

framework.test("Spawn select method", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn select", function()
        local tile = app.map:getOrCreateTile(100, 113, 7)
        tile:setSpawn(5)
        local spawn = tile.spawn
        
        framework.assert(type(spawn.select) == "function", "spawn.select should be function")
        
        spawn:select()
        framework.assert(spawn.isSelected == true, "spawn should be selected after select()")
        
        tile:removeSpawn()
    end)
end)

framework.test("Spawn deselect method", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn deselect", function()
        local tile = app.map:getOrCreateTile(100, 114, 7)
        tile:setSpawn(5)
        local spawn = tile.spawn
        
        framework.assert(type(spawn.deselect) == "function", "spawn.deselect should be function")
        
        spawn:select()
        framework.assert(spawn.isSelected == true, "spawn should be selected")
        
        spawn:deselect()
        framework.assert(spawn.isSelected == false, "spawn should not be selected after deselect()")
        
        -- Test deselect on already deselected spawn
        spawn:deselect()
        framework.assert(spawn.isSelected == false, "deselect on deselected spawn should not error")
        
        tile:removeSpawn()
    end)
end)

framework.test("Spawn select/deselect cycle", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn select/deselect cycle", function()
        local tile = app.map:getOrCreateTile(100, 115, 7)
        tile:setSpawn(5)
        local spawn = tile.spawn
        
        -- Multiple cycles
        for i = 1, 3 do
            spawn:select()
            framework.assert(spawn.isSelected == true, "cycle " .. i .. ": should be selected")
            spawn:deselect()
            framework.assert(spawn.isSelected == false, "cycle " .. i .. ": should be deselected")
        end
        
        tile:removeSpawn()
    end)
end)

-- ============================================================================
-- Spawn String Representation
-- ============================================================================

framework.test("Spawn to_string", function()
    if not app.hasMap() then return end
    
    app.transaction("Test spawn to_string", function()
        local tile = app.map:getOrCreateTile(100, 116, 7)
        tile:setSpawn(8)
        local spawn = tile.spawn
        
        local str = tostring(spawn)
        framework.assert(type(str) == "string", "tostring(spawn) should return string")
        framework.assert(str:find("Spawn"), "tostring should contain 'Spawn'")
        framework.assert(str:find("8"), "tostring should contain radius value")
        
        tile:removeSpawn()
    end)
end)

-- ============================================================================
-- Creature and Spawn Coexistence Tests
-- ============================================================================

framework.test("Creature and Spawn on same tile", function()
    if not app.hasMap() then return end
    
    app.transaction("Test creature and spawn coexistence", function()
        local tile = app.map:getOrCreateTile(100, 117, 7)
        
        -- Set both creature and spawn
        tile:setCreature("Rat")
        tile:setSpawn(10)
        
        local creature = tile.creature
        local spawn = tile.spawn
        
        framework.assert(type(creature) ~= "nil", "creature should exist")
        framework.assert(type(spawn) ~= "nil", "spawn should exist")
        framework.assert(creature.name == "Rat", "creature name should be Rat")
        framework.assert(spawn.size == 10, "spawn size should be 10")
        
        -- Both should be independently selectable
        creature:select()
        spawn:select()
        framework.assert(creature.isSelected == true, "creature should be selected")
        framework.assert(spawn.isSelected == true, "spawn should be selected")
        
        creature:deselect()
        framework.assert(creature.isSelected == false, "creature should be deselected")
        framework.assert(spawn.isSelected == true, "spawn should remain selected")
        
        tile:removeCreature()
        tile:removeSpawn()
    end)
end)

-- ============================================================================
-- Edge Cases and Error Handling
-- ============================================================================

framework.test("Creature operations on invalid creature", function()
    -- Test that accessing properties on nil creature doesn't crash
    local tile = { creature = nil }
    
    -- These should handle nil gracefully in the C++ code
    -- We just verify the API exists and doesn't crash on valid creatures
    if app.hasMap() then
        app.transaction("Test invalid creature handling", function()
            local tile = app.map:getOrCreateTile(100, 118, 7)
            -- Don't set a creature, just verify tile.creature is nil
            framework.assert(tile.creature == nil, "tile.creature should be nil when no creature set")
        end)
    end
end)

framework.test("Spawn operations on invalid spawn", function()
    if app.hasMap() then
        app.transaction("Test invalid spawn handling", function()
            local tile = app.map:getOrCreateTile(100, 119, 7)
            -- Don't set a spawn, just verify tile.spawn is nil
            framework.assert(tile.spawn == nil, "tile.spawn should be nil when no spawn set")
        end)
    end
end)

framework.test("Multiple creatures on different tiles", function()
    if not app.hasMap() then return end
    
    app.transaction("Test multiple creatures", function()
        local creatures = { "Rat", "Rat", "Rat" }
        local positions = {
            { x = 100, y = 120, z = 7 },
            { x = 101, y = 120, z = 7 },
            { x = 102, y = 120, z = 7 }
        }
        
        for i, pos in ipairs(positions) do
            local tile = app.map:getOrCreateTile(pos.x, pos.y, pos.z)
            tile:setCreature(creatures[i])
            local creature = tile.creature
            framework.assert(creature.name == creatures[i], "creature " .. i .. " should have correct name")
            
            -- Set different directions
            creature.direction = Direction.NORTH + (i - 1)
        end
        
        -- Verify all creatures
        for i, pos in ipairs(positions) do
            local tile = app.map:getTile(pos.x, pos.y, pos.z)
            framework.assert(tile.creature.name == creatures[i], "creature " .. i .. " should persist")
            tile:removeCreature()
        end
    end)
end)

framework.test("Multiple spawns with different sizes", function()
    if not app.hasMap() then return end
    
    app.transaction("Test multiple spawns", function()
        local sizes = { 5, 10, 15 }
        local positions = {
            { x = 100, y = 121, z = 7 },
            { x = 101, y = 121, z = 7 },
            { x = 102, y = 121, z = 7 }
        }
        
        for i, pos in ipairs(positions) do
            local tile = app.map:getOrCreateTile(pos.x, pos.y, pos.z)
            tile:setSpawn(sizes[i])
            local spawn = tile.spawn
            framework.assert(spawn.size == sizes[i], "spawn " .. i .. " should have correct size")
        end
        
        -- Verify all spawns
        for i, pos in ipairs(positions) do
            local tile = app.map:getTile(pos.x, pos.y, pos.z)
            framework.assert(tile.spawn.size == sizes[i], "spawn " .. i .. " should persist")
            tile:removeSpawn()
        end
    end)
end)

-- ============================================================================
-- Summary
-- ============================================================================

framework.summary()
