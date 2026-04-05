local MCP_PORT = 8080

local mcp = {}
mcp.tools = {}

-- Register a tool
function mcp.registerTool(name, description, inputSchema, callback)
    mcp.tools[name] = {
        name = name,
        description = description,
        inputSchema = inputSchema,
        callback = callback
    }
end

-- Handle incoming JSON-RPC
function mcp.handleRequest(requestStr)
    local ok, req = pcall(json.decode, requestStr)
    if not ok or type(req) ~= "table" then
        return json.encode({
            jsonrpc = "2.0",
            error = { code = -32700, message = "Parse error" },
            id = nil
        })
    end

    local id = req.id

    if req.method == "initialize" then
        return json.encode({
            jsonrpc = "2.0",
            result = {
                protocolVersion = "2024-11-05",
                capabilities = { tools = {} },
                serverInfo = { name = "RME-MCP", version = "1.0.0" }
            },
            id = id
        })
    elseif req.method == "notifications/initialized" then
        return "" -- No response needed for notifications
    elseif req.method == "tools/list" then
        local toolList = {}
        for _, t in pairs(mcp.tools) do
            table.insert(toolList, {
                name = t.name,
                description = t.description,
                inputSchema = t.inputSchema
            })
        end
        return json.encode({
            jsonrpc = "2.0",
            result = { tools = toolList },
            id = id
        })
    elseif req.method == "tools/call" then
        if not req.params or not req.params.name then
            return json.encode({
                jsonrpc = "2.0",
                error = { code = -32602, message = "Invalid params: name is required" },
                id = id
            })
        end

        local tool = mcp.tools[req.params.name]
        if not tool then
            return json.encode({
                jsonrpc = "2.0",
                error = { code = -32601, message = "Tool not found" },
                id = id
            })
        end

        local args = req.params.arguments or {}
        local success, result = pcall(tool.callback, args)

        if not success then
            return json.encode({
                jsonrpc = "2.0",
                result = {
                    content = {{ type = "text", text = "Error: " .. tostring(result) }},
                    isError = true
                },
                id = id
            })
        end

        return json.encode({
            jsonrpc = "2.0",
            result = {
                content = {{ type = "text", text = type(result) == "string" and result or json.encode(result) }}
            },
            id = id
        })
    else
        return json.encode({
            jsonrpc = "2.0",
            error = { code = -32601, message = "Method not found" },
            id = id
        })
    end
end

--------------------------------------------------------------------------------
-- Helper: get a tile from map (read-only, returns nil if not found)
--------------------------------------------------------------------------------
local function getTile(x, y, z)
    if not app.hasMap() then return nil end
    return app.map:getTile(x, y, z)
end

-- Helper: get or create a tile (for modifications)
local function getOrCreateTile(x, y, z)
    if not app.hasMap() then return nil end
    return app.map:getOrCreateTile(x, y, z)
end

--------------------------------------------------------------------------------
-- MCP Tools Implementation
--------------------------------------------------------------------------------

mcp.registerTool("get_map_info", "Returns basic map information like width, height, and description.", {
    type = "object", properties = {}
}, function()
    if not app.hasMap() then return "No map loaded." end
    local m = app.map
    return json.encode({
        name = m.name,
        width = m.width,
        height = m.height,
        description = m.description,
        tileCount = m.tileCount
    })
end)

mcp.registerTool("get_tile", "Returns detailed info of items on a specific tile.", {
    type = "object",
    properties = {
        x = { type = "number" },
        y = { type = "number" },
        z = { type = "number" }
    },
    required = { "x", "y", "z" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    local t = getTile(args.x, args.y, args.z)
    if not t then return "Tile not found." end
    local items = {}
    for _, item in ipairs(t.items) do
        table.insert(items, {id = item.id, name = item.name})
    end
    return json.encode({
        ground = t.ground and t.ground.id or nil,
        items = items,
        houseId = t.houseId,
        hasCreature = t.hasCreature,
        hasSpawn = t.hasSpawn
    })
end)

mcp.registerTool("set_tile_ground", "Replaces the ground item of a tile.", {
    type = "object",
    properties = {
        x = { type = "number" },
        y = { type = "number" },
        z = { type = "number" },
        itemId = { type = "number" }
    },
    required = { "x", "y", "z", "itemId" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.transaction("Set Ground", function()
        local t = getOrCreateTile(args.x, args.y, args.z)
        if t then
            t.ground = args.itemId
        end
    end)
    return "Ground set successfully."
end)

mcp.registerTool("add_item", "Adds an item to a specific tile.", {
    type = "object",
    properties = {
        x = { type = "number" },
        y = { type = "number" },
        z = { type = "number" },
        itemId = { type = "number" }
    },
    required = { "x", "y", "z", "itemId" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.transaction("Add Item", function()
        local t = getOrCreateTile(args.x, args.y, args.z)
        if t then
            t:addItem(args.itemId)
        end
    end)
    return "Item added."
end)

mcp.registerTool("clear_tile", "Removes all items from a tile.", {
    type = "object",
    properties = {
        x = { type = "number" },
        y = { type = "number" },
        z = { type = "number" }
    },
    required = { "x", "y", "z" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    local t = getTile(args.x, args.y, args.z)
    if not t then return "Tile not found (already empty)." end
    app.transaction("Clear Tile", function()
        -- Remove all items
        local items = t.items
        for i = #items, 1, -1 do
            t:removeItem(items[i])
        end
        -- Remove ground
        t.ground = nil
    end)
    return "Tile cleared."
end)

mcp.registerTool("get_selection", "Returns the bounding box of the current selection.", {
    type = "object", properties = {}
}, function()
    if app.selection.isEmpty then return "No selection." end
    local startPos = app.selection.minPosition
    local endPos = app.selection.maxPosition
    return string.format("Selection from (X:%d Y:%d Z:%d) to (X:%d Y:%d Z:%d)", startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z)
end)

mcp.registerTool("fill_selection_ground", "Fills the selected area ground with a specific item ID.", {
    type = "object",
    properties = { itemId = { type = "number" } },
    required = { "itemId" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end
    local map = app.map
    app.transaction("Fill Selection Ground", function()
        local minP = app.selection.minPosition
        local maxP = app.selection.maxPosition
        for x = minP.x, maxP.x do
            for y = minP.y, maxP.y do
                for z = minP.z, maxP.z do
                    local t = map:getOrCreateTile(x, y, z)
                    if t then
                        t.ground = args.itemId
                    end
                end
            end
        end
    end)
    return "Selection filled."
end)

mcp.registerTool("create_castle_template", "Creates a simple castle wall boundary around the current selection.", {
    type = "object",
    properties = {
        wallId = { type = "number" },
        floorId = { type = "number" }
    },
    required = { "wallId", "floorId" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end

    app.transaction("Create Castle", function()
        local minP = app.selection.minPosition
        local maxP = app.selection.maxPosition
        local map = app.map

        for x = minP.x, maxP.x do
            for y = minP.y, maxP.y do
                for z = minP.z, maxP.z do
                    local t = map:getOrCreateTile(x, y, z)
                    if t then
                        t.ground = args.floorId
                        if x == minP.x or x == maxP.x or y == minP.y or y == maxP.y then
                            t:addItem(args.wallId)
                        end
                    end
                end
            end
        end
    end)
    return "Castle template created in selection."
end)

mcp.registerTool("undo", "Undoes the last map action.", {
    type = "object", properties = {}
}, function()
    app.editor:undo()
    return "Action undone."
end)

mcp.registerTool("redo", "Redoes the last undone action.", {
    type = "object", properties = {}
}, function()
    app.editor:redo()
    return "Action redone."
end)

--------------------------------------------------------------------------------
-- Start Server
--------------------------------------------------------------------------------

if not app.isMcpServerRunning() then
    app.setMcpHandler(mcp.handleRequest)
    if app.startMcpServer(MCP_PORT) then
        print("MCP Server successfully started on port " .. tostring(MCP_PORT) .. "!")
    else
        print("Failed to start MCP Server. Port might be in use.")
    end
else
    -- Server already running, just update the handler and tools
    app.setMcpHandler(mcp.handleRequest)
    print("MCP Server tools reloaded successfully.")
end

mcp.registerTool("replace_in_selection", "Replaces one item ID with another within the selected area.", {
    type = "object",
    properties = {
        oldId = { type = "number" },
        newId = { type = "number" }
    },
    required = { "oldId", "newId" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end
    local count = 0
    local map = app.map
    app.transaction("Replace Selection", function()
        local minP = app.selection.minPosition
        local maxP = app.selection.maxPosition
        for x = minP.x, maxP.x do
            for y = minP.y, maxP.y do
                for z = minP.z, maxP.z do
                    local t = map:getTile(x, y, z)
                    if t then
                        -- Check ground
                        if t.ground and t.ground.id == args.oldId then
                            t.ground = args.newId
                            count = count + 1
                        end
                        -- Check top items
                        local items = t.items
                        for i = #items, 1, -1 do
                            local it = items[i]
                            if it.id == args.oldId then
                                t:removeItem(it)
                                t:addItem(args.newId)
                                count = count + 1
                            end
                        end
                    end
                end
            end
        end
    end)
    return "Replaced " .. tostring(count) .. " items."
end)

mcp.registerTool("count_items_in_selection", "Counts the occurrences of a specific item ID in the selection.", {
    type = "object",
    properties = { itemId = { type = "number" } },
    required = { "itemId" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end
    local count = 0
    local map = app.map
    local minP = app.selection.minPosition
    local maxP = app.selection.maxPosition
    for x = minP.x, maxP.x do
        for y = minP.y, maxP.y do
            for z = minP.z, maxP.z do
                local t = map:getTile(x, y, z)
                if t then
                    if t.ground and t.ground.id == args.itemId then count = count + 1 end
                    for _, it in ipairs(t.items) do
                        if it.id == args.itemId then count = count + 1 end
                    end
                end
            end
        end
    end
    return "Found " .. tostring(count) .. " items."
end)

mcp.registerTool("set_selection", "Sets the map selection box.", {
    type = "object",
    properties = {
        x1 = { type = "number" }, y1 = { type = "number" }, z1 = { type = "number" },
        x2 = { type = "number" }, y2 = { type = "number" }, z2 = { type = "number" }
    },
    required = { "x1", "y1", "z1", "x2", "y2", "z2" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.selection:clear()
    local minX = math.min(args.x1, args.x2)
    local maxX = math.max(args.x1, args.x2)
    local minY = math.min(args.y1, args.y2)
    local maxY = math.max(args.y1, args.y2)
    local minZ = math.min(args.z1, args.z2)
    local maxZ = math.max(args.z1, args.z2)
    local map = app.map

    app.selection:start()
    for x = minX, maxX do
        for y = minY, maxY do
            for z = minZ, maxZ do
                local t = map:getTile(x, y, z)
                if t then
                    app.selection:add(t)
                end
            end
        end
    end
    app.selection:finish()
    return "Selection set."
end)

mcp.registerTool("remove_item", "Removes a specific item from a tile by ID.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" },
        itemId = { type = "number" }
    },
    required = { "x", "y", "z", "itemId" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    local t = getTile(args.x, args.y, args.z)
    if not t then return "Tile not found." end
    local removed = false
    app.transaction("Remove Item", function()
        for _, it in ipairs(t.items) do
            if it.id == args.itemId then
                t:removeItem(it)
                removed = true
                break
            end
        end
    end)
    return removed and "Item removed." or "Item not found on tile."
end)

mcp.registerTool("draw_brush", "Simulates drawing with a specific brush at a coordinate.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" },
        brushName = { type = "string" }
    },
    required = { "x", "y", "z", "brushName" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.setBrush(args.brushName)
    app.transaction("Draw Brush " .. args.brushName, function()
        local t = getOrCreateTile(args.x, args.y, args.z)
        if t then
            t:applyBrush(args.brushName)
        end
    end)
    return "Brush drawn."
end)

mcp.registerTool("draw_doodad", "Simulates drawing a doodad at a coordinate.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" },
        doodadName = { type = "string" }
    },
    required = { "x", "y", "z", "doodadName" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.setBrush(args.doodadName)
    app.transaction("Draw Doodad " .. args.doodadName, function()
        local t = getOrCreateTile(args.x, args.y, args.z)
        if t then
            t:applyBrush(args.doodadName)
        end
    end)
    return "Doodad drawn."
end)

mcp.registerTool("draw_wall", "Draws a line of walls between two points.", {
    type = "object",
    properties = {
        x1 = { type = "number" }, y1 = { type = "number" }, z1 = { type = "number" },
        x2 = { type = "number" }, y2 = { type = "number" }, z2 = { type = "number" },
        wallId = { type = "number" }
    },
    required = { "x1", "y1", "z1", "x2", "y2", "z2", "wallId" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    if args.z1 ~= args.z2 then return "Points must be on the same Z level." end
    local map = app.map
    local points = geo.bresenhamLine(args.x1, args.y1, args.x2, args.y2)
    app.transaction("Draw Wall", function()
        for _, pt in ipairs(points) do
            local t = map:getOrCreateTile(pt.x, pt.y, args.z1)
            if t then
                t:addItem(args.wallId)
            end
        end
    end)
    return "Wall drawn."
end)

mcp.registerTool("get_camera_position", "Gets the current map view camera center position.", {
    type = "object", properties = {}
}, function()
    return "Camera positioning query not directly supported via bounds yet."
end)

mcp.registerTool("set_camera_position", "Sets the map view camera to center on a position.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" }
    },
    required = { "x", "y", "z" }
}, function(args)
    app.setCameraPosition(args.x, args.y, args.z)
    return "Camera moved."
end)

mcp.registerTool("create_house", "Assigns a selection of tiles to a new house ID.", {
    type = "object",
    properties = { houseId = { type = "number" } },
    required = { "houseId" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end
    local map = app.map
    app.transaction("Create House", function()
        local minP = app.selection.minPosition
        local maxP = app.selection.maxPosition
        for x = minP.x, maxP.x do
            for y = minP.y, maxP.y do
                for z = minP.z, maxP.z do
                    local t = map:getOrCreateTile(x, y, z)
                    if t then
                        t.houseId = args.houseId
                    end
                end
            end
        end
    end)
    return "Tiles assigned to house " .. tostring(args.houseId)
end)

mcp.registerTool("get_house", "Returns the house ID of a given tile.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" }
    },
    required = { "x", "y", "z" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    local t = getTile(args.x, args.y, args.z)
    if not t then return "Tile not found." end
    return t.houseId > 0 and tostring(t.houseId) or "No house at this tile."
end)

mcp.registerTool("clear_selection", "Clears the current map selection.", {
    type = "object", properties = {}
}, function()
    app.selection:clear()
    return "Selection cleared."
end)

mcp.registerTool("save_map", "Saves the current map.", {
    type = "object", properties = {}
}, function()
    if not app.hasMap() then return "No map loaded." end
    return "Error: Programmatic save not directly exposed. Please ask the user to press CTRL+S."
end)

mcp.registerTool("get_spawn", "Gets spawn details at a given coordinate.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" }
    },
    required = { "x", "y", "z" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    local t = getTile(args.x, args.y, args.z)
    if not t then return "Tile not found." end
    if not t.hasSpawn then return "No spawn found." end
    return "Spawn found with radius " .. tostring(t.spawn.radius)
end)

mcp.registerTool("add_spawn", "Creates a spawn point at a tile.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" },
        radius = { type = "number" }
    },
    required = { "x", "y", "z", "radius" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.transaction("Add Spawn", function()
        local t = getOrCreateTile(args.x, args.y, args.z)
        if t then
            t:setSpawn(args.radius)
        end
    end)
    return "Spawn point created."
end)

mcp.registerTool("add_creature", "Adds a creature to a tile.", {
    type = "object",
    properties = {
        x = { type = "number" }, y = { type = "number" }, z = { type = "number" },
        name = { type = "string" }, spawnTime = { type = "number" }
    },
    required = { "x", "y", "z", "name", "spawnTime" }
}, function(args)
    if not app.hasMap() then return "No map loaded." end
    app.transaction("Add Creature", function()
        local t = getOrCreateTile(args.x, args.y, args.z)
        if t then
            t:setCreature(args.name, args.spawnTime)
        end
    end)
    return "Creature added."
end)

mcp.registerTool("get_item_info", "Returns basic properties of an item by its ID.", {
    type = "object",
    properties = { itemId = { type = "number" } },
    required = { "itemId" }
}, function(args)
    local info = Items.getInfo(args.itemId)
    if not info then return "Item not found." end
    return json.encode(info)
end)

mcp.registerTool("set_light", "Sets the global ambient light.", {
    type = "object",
    properties = { level = { type = "number" }, },
    required = { "level" }
}, function(args)
    app.setAmbientLightLevel(args.level)
    return "Ambient light set."
end)

mcp.registerTool("refresh_view", "Refreshes the editor rendering.", {
    type = "object", properties = {}
}, function()
    app.refresh()
    return "View refreshed."
end)

--------------------------------------------------------------------------------
-- Lookup / Discovery Tools
--------------------------------------------------------------------------------

mcp.registerTool("search_items", "Searches for items by name. Returns matching item IDs, names, and properties. Use this to find proper ground tiles, walls, decorations, etc.", {
    type = "object",
    properties = {
        query = { type = "string", description = "Search term (case-insensitive, partial match)" },
        maxResults = { type = "number", description = "Maximum results to return (default 25)" }
    },
    required = { "query" }
}, function(args)
    local results = Items.findByName(args.query, args.maxResults or 25)
    if #results == 0 then return "No items found matching '" .. args.query .. "'." end

    local detailed = {}
    for _, r in ipairs(results) do
        local info = Items.getInfo(r.id)
        if info then
            table.insert(detailed, {
                id = info.id,
                name = info.name,
                isGround = info.isGroundTile,
                isWall = info.isWall,
                isBorder = info.isBorder,
                isDoor = info.isDoor,
                isTable = info.isTable,
                isCarpet = info.isCarpet,
                isBlocking = not info.isMoveable and not info.isPickupable
            })
        end
    end
    return json.encode(detailed)
end)

mcp.registerTool("list_brushes", "Lists available brushes, optionally filtered by type. Brush types: ground, wall, doodad, door, table, carpet, creature, spawn, raw, house, waypoint, eraser, terrain.", {
    type = "object",
    properties = {
        typeFilter = { type = "string", description = "Filter by brush type (e.g. 'ground', 'wall', 'doodad'). Leave empty for all." },
        nameFilter = { type = "string", description = "Filter by name substring (case-insensitive). Leave empty for all." },
        maxResults = { type = "number", description = "Maximum results to return (default 50)" }
    }
}, function(args)
    local allNames = Brushes.getNames()
    local maxResults = args.maxResults or 50
    local typeFilter = args.typeFilter and args.typeFilter:lower() or nil
    local nameFilter = args.nameFilter and args.nameFilter:lower() or nil
    local results = {}

    for _, name in ipairs(allNames) do
        if #results >= maxResults then break end

        local brush = Brushes.get(name)
        if brush then
            local bType = brush.type
            local bName = brush.name

            local matchType = not typeFilter or bType == typeFilter
            local matchName = not nameFilter or bName:lower():find(nameFilter, 1, true)

            if matchType and matchName then
                table.insert(results, {
                    name = bName,
                    type = bType,
                    lookId = brush.lookId
                })
            end
        end
    end

    if #results == 0 then return "No brushes found." end
    return json.encode(results)
end)

mcp.registerTool("get_brush_info", "Gets detailed information about a specific brush by name.", {
    type = "object",
    properties = {
        name = { type = "string", description = "Exact brush name" }
    },
    required = { "name" }
}, function(args)
    local brush = Brushes.get(args.name)
    if not brush then return "Brush '" .. args.name .. "' not found." end
    return json.encode({
        name = brush.name,
        type = brush.type,
        id = brush.id,
        lookId = brush.lookId,
        needBorders = brush:needBorders(),
        canDrag = brush:canDrag(),
        canSmear = brush:canSmear()
    })
end)

mcp.registerTool("list_creatures", "Lists available creature names, optionally filtered. Use this to find creatures to place on the map.", {
    type = "object",
    properties = {
        nameFilter = { type = "string", description = "Filter by name substring (case-insensitive). Leave empty for all." },
        maxResults = { type = "number", description = "Maximum results to return (default 50)" },
        npcOnly = { type = "boolean", description = "If true, only return NPCs. If false, only return monsters. Omit for both." }
    }
}, function(args)
    -- Use Brushes to find creature brushes (they correspond to creatures)
    local allNames = Brushes.getNames()
    local maxResults = args.maxResults or 50
    local nameFilter = args.nameFilter and args.nameFilter:lower() or nil
    local results = {}

    for _, name in ipairs(allNames) do
        if #results >= maxResults then break end

        local brush = Brushes.get(name)
        if brush and brush.type == "creature" then
            local bName = brush.name
            local matchName = not nameFilter or bName:lower():find(nameFilter, 1, true)

            if matchName then
                local isNpc = isNpcType(bName)
                local matchNpc = args.npcOnly == nil or args.npcOnly == isNpc

                if matchNpc then
                    table.insert(results, {
                        name = bName,
                        isNpc = isNpc,
                        lookId = brush.lookId
                    })
                end
            end
        end
    end

    if #results == 0 then return "No creatures found." end
    return json.encode(results)
end)

mcp.registerTool("find_item_id", "Finds the first item ID matching an exact name (case-insensitive). Falls back to partial match if no exact match.", {
    type = "object",
    properties = {
        name = { type = "string", description = "Item name to search for" }
    },
    required = { "name" }
}, function(args)
    local id = Items.findIdByName(args.name)
    if not id then return "No item found matching '" .. args.name .. "'." end
    local info = Items.getInfo(id)
    if info then
        return json.encode(info)
    end
    return "Item ID " .. tostring(id) .. " found but no info available."
end)

mcp.registerTool("get_brush_count", "Returns the total number of brushes available.", {
    type = "object", properties = {}
}, function()
    return "Total brushes: " .. tostring(Brushes.count())
end)

--------------------------------------------------------------------------------
-- Fill Empty Area (Smart Fill / WFC-lite)
--
-- Analyzes existing tiles in the selection, then fills empty tiles by
-- copying ground types and decorations from nearby neighbors.
--------------------------------------------------------------------------------

mcp.registerTool("fill_empty_area", "Fills empty tiles in the selection by analyzing surrounding tiles and reproducing the same ground types and decorations. Works like a smart-fill / wave function collapse.", {
    type = "object",
    properties = {
        scatterDoodads = { type = "boolean", description = "If true (default), scatter decorative items found in the area onto filled tiles." },
        doodadDensity = { type = "number", description = "Probability (0.0-1.0) of placing a doodad on each filled tile. Default 0.15." }
    }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end

    local map = app.map
    local minP = app.selection.minPosition
    local maxP = app.selection.maxPosition
    local scatterDoodads = args.scatterDoodads ~= false
    local doodadDensity = args.doodadDensity or 0.15

    -- Phase 1: Scan existing tiles
    local groundFreq = {}       -- groundId -> count
    local totalGroundTiles = 0
    local doodadItems = {}      -- list of item IDs found as decorations
    local doodadSet = {}        -- dedup set
    local emptyPositions = {}   -- {x, y, z} of empty tiles

    -- Build a lookup grid for quick neighbor checks
    local groundGrid = {}       -- [x][y] = groundId or nil

    for x = minP.x, maxP.x do
        groundGrid[x] = {}
        for y = minP.y, maxP.y do
            local t = map:getTile(x, y, minP.z)
            if t and t.ground then
                local gid = t.ground.id
                groundGrid[x][y] = gid
                groundFreq[gid] = (groundFreq[gid] or 0) + 1
                totalGroundTiles = totalGroundTiles + 1

                -- Collect decorative items (non-ground, non-wall, non-border)
                if scatterDoodads then
                    for _, item in ipairs(t.items) do
                        local iid = item.id
                        if not doodadSet[iid] then
                            -- Only pick up items that aren't walls/borders
                            if not item.isWall and not item.isBorder then
                                table.insert(doodadItems, iid)
                                doodadSet[iid] = true
                            end
                        end
                    end
                end
            else
                table.insert(emptyPositions, {x = x, y = y, z = minP.z})
            end
        end
    end

    if totalGroundTiles == 0 then
        return "No existing tiles found to learn from."
    end
    if #emptyPositions == 0 then
        return "No empty tiles to fill."
    end

    -- Build sorted frequency list for fallback
    local groundList = {}
    for gid, count in pairs(groundFreq) do
        table.insert(groundList, {id = gid, weight = count})
    end
    table.sort(groundList, function(a, b) return a.weight > b.weight end)

    -- Phase 2: Fill empty tiles using neighbor context
    -- We do multiple passes so newly filled tiles help inform later ones
    local filled = 0
    local seed = os.time()

    -- Simple PRNG (LCG) for speed
    local function rng()
        seed = (seed * 1103515245 + 12345) % 2147483648
        return seed / 2147483648
    end

    -- Weighted random pick from a frequency table
    local function weightedPick(freq)
        local total = 0
        for _, w in pairs(freq) do total = total + w end
        if total == 0 then return groundList[1].id end

        local roll = rng() * total
        local acc = 0
        for gid, w in pairs(freq) do
            acc = acc + w
            if roll <= acc then return gid end
        end
        -- fallback
        return groundList[1].id
    end

    app.transaction("Fill Empty Area", function()
        local maxPasses = 5
        for pass = 1, maxPasses do
            local filledThisPass = 0

            for _, pos in ipairs(emptyPositions) do
                if not groundGrid[pos.x][pos.y] then
                    -- Check 8 neighbors
                    local neighborFreq = {}
                    local hasNeighbor = false
                    for dx = -1, 1 do
                        for dy = -1, 1 do
                            if not (dx == 0 and dy == 0) then
                                local nx = pos.x + dx
                                local ny = pos.y + dy
                                if groundGrid[nx] and groundGrid[nx][ny] then
                                    local nGid = groundGrid[nx][ny]
                                    -- Cardinal neighbors get double weight
                                    local weight = (dx == 0 or dy == 0) and 2 or 1
                                    neighborFreq[nGid] = (neighborFreq[nGid] or 0) + weight
                                    hasNeighbor = true
                                end
                            end
                        end
                    end

                    if hasNeighbor then
                        -- Pick ground based on neighbor frequency
                        local chosenGround = weightedPick(neighborFreq)

                        local t = map:getOrCreateTile(pos.x, pos.y, pos.z)
                        if t then
                            t.ground = chosenGround
                            groundGrid[pos.x][pos.y] = chosenGround

                            -- Maybe scatter a doodad
                            if scatterDoodads and #doodadItems > 0 and rng() < doodadDensity then
                                local doodadId = doodadItems[math.floor(rng() * #doodadItems) + 1]
                                t:addItem(doodadId)
                            end

                            filled = filled + 1
                            filledThisPass = filledThisPass + 1
                        end
                    end
                end
            end

            -- If nothing was filled this pass, remaining tiles have no neighbors yet
            -- Use global frequency as fallback
            if filledThisPass == 0 then
                for _, pos in ipairs(emptyPositions) do
                    if not groundGrid[pos.x][pos.y] then
                        local chosenGround = weightedPick(groundFreq)
                        local t = map:getOrCreateTile(pos.x, pos.y, pos.z)
                        if t then
                            t.ground = chosenGround
                            groundGrid[pos.x][pos.y] = chosenGround

                            if scatterDoodads and #doodadItems > 0 and rng() < doodadDensity then
                                local doodadId = doodadItems[math.floor(rng() * #doodadItems) + 1]
                                t:addItem(doodadId)
                            end

                            filled = filled + 1
                        end
                    end
                end
                break
            end
        end
    end)

    return string.format("Filled %d empty tiles. Learned from %d existing tiles with %d ground types, %d doodad types.",
        filled, totalGroundTiles, #groundList, #doodadItems)
end)

--------------------------------------------------------------------------------
-- Raw Tile Dump
--------------------------------------------------------------------------------

mcp.registerTool("get_selection_tiles", "Dumps all tiles in the current selection, including their ground and items. Useful for learning structural patterns.", {
    type = "object",
    properties = {
        maxTiles = { type = "number", description = "Max number of tiles to return (default 1000)" },
        onlyNonEmpty = { type = "boolean", description = "If true, skips tiles without ground or items" }
    }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end

    local map = app.map
    local minP = app.selection.minPosition
    local maxP = app.selection.maxPosition
    local maxTiles = args.maxTiles or 1000
    local onlyNonEmpty = args.onlyNonEmpty

    local tiles = {}
    local count = 0

    for x = minP.x, maxP.x do
        for y = minP.y, maxP.y do
            if count >= maxTiles then break end

            local t = map:getTile(x, y, minP.z)
            if t then
                local hasContent = t.ground or #t.items > 0
                if not onlyNonEmpty or hasContent then
                    local tileInfo = {
                        x = x, y = y, z = minP.z,
                        items = {}
                    }
                    if t.ground then
                        tileInfo.ground = { id = t.ground.id, name = t.ground.name }
                    end
                    for _, item in ipairs(t.items) do
                        table.insert(tileInfo.items, {
                            id = item.id,
                            name = item.name,
                            isWall = item.isWall,
                            isDoor = item.isDoor,
                            isBorder = item.isBorder,
                            isBlocking = item.isBlocking,
                            isStackable = item.isStackable
                        })
                    end
                    table.insert(tiles, tileInfo)
                    count = count + 1
                end
            elseif not onlyNonEmpty then
                -- Return empty tile
                table.insert(tiles, { x = x, y = y, z = minP.z, ground = nil, items = {} })
                count = count + 1
            end
        end
        if count >= maxTiles then break end
    end

    return json.encode({
        tiles = tiles,
        totalReturned = count,
        selectionSize = {
            width = maxP.x - minP.x + 1,
            height = maxP.y - minP.y + 1
        }
    })
end)

--------------------------------------------------------------------------------
-- Architectural Template System
--------------------------------------------------------------------------------

local activeTemplates = {}

local function getTopWall(t)
    if not t then return nil end
    for i = t.itemCount, 1, -1 do
        local item = t:getItemAt(i)
        if item and item.isWall then
            return item.id
        end
    end
    return nil
end

local function getHighestFreq(freqTable)
    local bestId = nil
    local maxCount = -1
    for id, count in pairs(freqTable) do
        if count > maxCount then
            maxCount = count
            bestId = id
        end
    end
    return bestId
end

mcp.registerTool("capture_building_template", "Analyzes the current selected building and saves its structural footprint (walls, floors, corners) under a given template name.", {
    type = "object",
    properties = {
        templateName = { type = "string", description = "Name to save the template under." }
    },
    required = { "templateName" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end

    local map = app.map
    local minP = app.selection.minPosition
    local maxP = app.selection.maxPosition
    
    if maxP.x - minP.x < 2 or maxP.y - minP.y < 2 then
        return "Selection is too small to capture a template. Need at least 3x3 to identify corners and interior."
    end
    
    local floorFreq = {}
    local nWallFreq = {}
    local sWallFreq = {}
    local wWallFreq = {}
    local eWallFreq = {}
    
    local template = {
        corners = {
            nw = nil, ne = nil, sw = nil, se = nil
        },
        door = nil
    }
    
    -- Extract Corners
    template.corners.nw = getTopWall(map:getTile(minP.x, minP.y, minP.z))
    template.corners.ne = getTopWall(map:getTile(maxP.x, minP.y, minP.z))
    template.corners.sw = getTopWall(map:getTile(minP.x, maxP.y, minP.z))
    template.corners.se = getTopWall(map:getTile(maxP.x, maxP.y, minP.z))
    
    -- Extract Edges and Interior
    for x = minP.x, maxP.x do
        for y = minP.y, maxP.y do
            local t = map:getTile(x, y, minP.z)
            if t then
                -- Interior Floors
                if x > minP.x and x < maxP.x and y > minP.y and y < maxP.y then
                    if t.ground then
                        floorFreq[t.ground.id] = (floorFreq[t.ground.id] or 0) + 1
                    end
                end
                
                -- Check for doors
                for i = 1, t.itemCount do
                    local item = t:getItemAt(i)
                    if item and item.isDoor then
                        template.door = item.id
                    end
                end
                
                -- Edges (exclude corners)
                local wallId = getTopWall(t)
                if wallId then
                    if y == minP.y and x > minP.x and x < maxP.x then
                        nWallFreq[wallId] = (nWallFreq[wallId] or 0) + 1
                    elseif y == maxP.y and x > minP.x and x < maxP.x then
                        sWallFreq[wallId] = (sWallFreq[wallId] or 0) + 1
                    elseif x == minP.x and y > minP.y and y < maxP.y then
                        wWallFreq[wallId] = (wWallFreq[wallId] or 0) + 1
                    elseif x == maxP.x and y > minP.y and y < maxP.y then
                        eWallFreq[wallId] = (eWallFreq[wallId] or 0) + 1
                    end
                end
            end
        end
    end
    
    template.floor = getHighestFreq(floorFreq)
    template.walls = {
        n = getHighestFreq(nWallFreq),
        s = getHighestFreq(sWallFreq),
        e = getHighestFreq(eWallFreq),
        w = getHighestFreq(wWallFreq),
    }
    
    -- basic validation
    if not template.floor and not template.walls.n then
        return "Could not identify floor or walls. Make sure selection tightly wraps the building."
    end
    
    activeTemplates[args.templateName] = template
    
    return json.encode({
        status = "Template captured successfully",
        templateName = args.templateName,
        signature = template
    })
end)

mcp.registerTool("build_house_template", "Uses a previously captured template to draw a building filling the current selection.", {
    type = "object",
    properties = {
        templateName = { type = "string", description = "Name of the captured template to build." }
    },
    required = { "templateName" }
}, function(args)
    if not app.hasMap() or app.selection.isEmpty then return "No map or selection." end
    
    local template = activeTemplates[args.templateName]
    if not template then return "Template '" .. args.templateName .. "' not found. Capture it first." end
    
    local map = app.map
    local minP = app.selection.minPosition
    local maxP = app.selection.maxPosition
    
    if maxP.x - minP.x < 2 or maxP.y - minP.y < 2 then
        return "Selection is too small to build a house (needs at least 3x3)."
    end
    
    local houseZ = minP.z
    local tilesPlaced = 0
    
    app.transaction("Build Template House", function()
        -- 1. Fill Floor
        if template.floor then
            for x = minP.x, maxP.x do
                for y = minP.y, maxP.y do
                    local t = map:getOrCreateTile(x, y, houseZ)
                    if t then
                        t.ground = template.floor
                        tilesPlaced = tilesPlaced + 1
                    end
                end
            end
        end
        
        -- 2. Edges
        local function placeWall(x, y, id)
            if not id then return end
            local t = map:getOrCreateTile(x, y, houseZ)
            if t then
                for i = t.itemCount, 1, -1 do
                    local it = t:getItemAt(i)
                    if it and it.isWall then
                        t:removeItem(it)
                    end
                end
                t:addItem(id)
                tilesPlaced = tilesPlaced + 1
            end
        end
        
        for x = minP.x + 1, maxP.x - 1 do
            placeWall(x, minP.y, template.walls.n)
            placeWall(x, maxP.y, template.walls.s)
        end
        
        for y = minP.y + 1, maxP.y - 1 do
            placeWall(minP.x, y, template.walls.w)
            placeWall(maxP.x, y, template.walls.e)
        end
        
        -- 3. Corners
        placeWall(minP.x, minP.y, template.corners.nw)
        placeWall(maxP.x, minP.y, template.corners.ne)
        placeWall(minP.x, maxP.y, template.corners.sw)
        placeWall(maxP.x, maxP.y, template.corners.se)
        
        -- 4. Door
        if template.door then
            -- Find a spot on south wall middle
            local doorX = math.floor((minP.x + maxP.x) / 2)
            local doorY = maxP.y
            local t = map:getOrCreateTile(doorX, doorY, houseZ)
            if t then
                for i = t.itemCount, 1, -1 do
                    local it = t:getItemAt(i)
                    if it and it.isWall then t:removeItem(it) end
                end
                t:addItem(template.door)
            end
        end
    end)
    
    return "Building generated successfully using template '" .. args.templateName .. "'. Placed " .. tostring(tilesPlaced) .. " components."
end)
