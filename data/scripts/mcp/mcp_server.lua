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
-- MCP Tools Implementation
--------------------------------------------------------------------------------

mcp.registerTool("get_map_info", "Returns basic map information like width, height, and description.", {
    type = "object", properties = {}
}, function()
    if not app.hasMap() then return "No map loaded." end
    local desc = app.map.description
    return "Map Info: " .. desc
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    local items = {}
    for i, item in ipairs(t:getItems()) do
        table.insert(items, {id = item:getID()})
    end
    return json.encode({
        ground = t:getGround() and t:getGround():getID() or nil,
        items = items
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end

    app.transaction("Set Ground", function()
        t:setGround(Item(args.itemId))
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    app.transaction("Add Item", function()
        t:addItem(Item(args.itemId))
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    app.transaction("Clear Tile", function()
        t:clear()
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
    app.transaction("Fill Selection Ground", function()
        for x = app.selection.minPosition.x, app.selection.maxPosition.x do
            for y = app.selection.minPosition.y, app.selection.maxPosition.y do
                for z = app.selection.minPosition.z, app.selection.maxPosition.z do
                    local pos = Position(x, y, z)
                    if app.selection.minPosition and pos.x >= app.selection.minPosition.x and pos.x <= app.selection.maxPosition.x and pos.y >= app.selection.minPosition.y and pos.y <= app.selection.maxPosition.y and pos.z >= app.selection.minPosition.z and pos.z <= app.selection.maxPosition.z then
                        local t = Tile(pos)
                        if t then t:setGround(Item(args.itemId)) end
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

        for x = minP.x, maxP.x do
            for y = minP.y, maxP.y do
                for z = minP.z, maxP.z do
                    local pos = Position(x,y,z)
                if app.selection.minPosition and x >= app.selection.minPosition.x and x <= app.selection.maxPosition.x and y >= app.selection.minPosition.y and y <= app.selection.maxPosition.y and z >= app.selection.minPosition.z and z <= app.selection.maxPosition.z then
                    local t = Tile(pos)
                        if t then
                            t:setGround(Item(args.floorId))
                            if x == minP.x or x == maxP.x or y == minP.y or y == maxP.y then
                                t:addItem(Item(args.wallId))
                            end
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
    app.transaction("Replace Selection", function()
        for x = app.selection.minPosition.x, app.selection.maxPosition.x do
            for y = app.selection.minPosition.y, app.selection.maxPosition.y do
                for z = app.selection.minPosition.z, app.selection.maxPosition.z do
                    local pos = Position(x,y,z)
                    if app.selection.minPosition and pos.x >= app.selection.minPosition.x and pos.x <= app.selection.maxPosition.x and pos.y >= app.selection.minPosition.y and pos.y <= app.selection.maxPosition.y and pos.z >= app.selection.minPosition.z and pos.z <= app.selection.maxPosition.z then
                        local t = Tile(pos)
                        if t then
                            -- Check ground
                            if t:getGround() and t:getGround():getID() == args.oldId then
                                t:setGround(Item(args.newId))
                                count = count + 1
                            end
                            -- Check top items
                            local items = t:getItems()
                            for i = #items, 1, -1 do
                                local it = items[i]
                                if it:getID() == args.oldId then
                                    t:removeItem(it)
                                    t:addItem(Item(args.newId))
                                    count = count + 1
                                end
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
    for x = app.selection.minPosition.x, app.selection.maxPosition.x do
        for y = app.selection.minPosition.y, app.selection.maxPosition.y do
            for z = app.selection.minPosition.z, app.selection.maxPosition.z do
                local pos = Position(x,y,z)
                if app.selection.minPosition and x >= app.selection.minPosition.x and x <= app.selection.maxPosition.x and y >= app.selection.minPosition.y and y <= app.selection.maxPosition.y and z >= app.selection.minPosition.z and z <= app.selection.maxPosition.z then
                    local t = Tile(pos)
                    if t then
                        if t:getGround() and t:getGround():getID() == args.itemId then count = count + 1 end
                        for _, it in ipairs(t:getItems()) do
                            if it:getID() == args.itemId then count = count + 1 end
                        end
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
    app.selection:clear()
    local minX = math.min(args.x1, args.x2)
    local maxX = math.max(args.x1, args.x2)
    local minY = math.min(args.y1, args.y2)
    local maxY = math.max(args.y1, args.y2)
    local minZ = math.min(args.z1, args.z2)
    local maxZ = math.max(args.z1, args.z2)

    for x = minX, maxX do
        for y = minY, maxY do
            for z = minZ, maxZ do
                app.selection:add(Position(x,y,z))
            end
        end
    end
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    local removed = false
    app.transaction("Remove Item", function()
        for _, it in ipairs(t:getItems()) do
            if it:getID() == args.itemId then
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
        algo.drawBrush(Position(args.x, args.y, args.z))
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
        algo.drawBrush(Position(args.x, args.y, args.z))
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
    app.transaction("Draw Wall", function()
        algo.drawLine(Position(args.x1, args.y1, args.z1), Position(args.x2, args.y2, args.z2), function(pos)
            local t = Tile(pos)
            if t then t:addItem(Item(args.wallId)) end
        end)
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
    app.transaction("Create House", function()
        for x = app.selection.minPosition.x, app.selection.maxPosition.x do
            for y = app.selection.minPosition.y, app.selection.maxPosition.y do
                for z = app.selection.minPosition.z, app.selection.maxPosition.z do
                    local pos = Position(x,y,z)
                    if app.selection.minPosition and pos.x >= app.selection.minPosition.x and pos.x <= app.selection.maxPosition.x and pos.y >= app.selection.minPosition.y and pos.y <= app.selection.maxPosition.y and pos.z >= app.selection.minPosition.z and pos.z <= app.selection.maxPosition.z then
                        local t = Tile(pos)
                        if t then t:setHouse(args.houseId) end
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    local hId = t:getHouse()
    return hId > 0 and tostring(hId) or "No house at this tile."
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
    -- app.saveMap() API exists? Assuming it does or user triggers it manually,
    -- but usually we can trigger an event or just alert
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    local sp = t:getSpawn()
    if not sp then return "No spawn found." end
    local creatures = sp:getCreatures()
    local res = "Spawn found with " .. tostring(#creatures) .. " creatures."
    return res
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    app.transaction("Add Spawn", function()
        t:setSpawn(args.radius)
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
    local t = Tile(Position(args.x, args.y, args.z))
    if not t then return "Tile not found." end
    app.transaction("Add Creature", function()
        t:addCreature(args.name, args.spawnTime)
    end)
    return "Creature added."
end)

mcp.registerTool("get_item_info", "Returns basic properties of an item by its ID.", {
    type = "object",
    properties = { itemId = { type = "number" } },
    required = { "itemId" }
}, function(args)
    local it = Item(args.itemId)
    if not it then return "Item not found." end
    return json.encode({
        id = it:getID(),
        name = it:getName(),
        isGround = it:isGround(),
        isBlocking = it:isBlocking(),
        hasElevation = it:hasElevation(),
        isMovable = it:isMoveable(),
        isPickupable = it:isPickupable()
    })
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
