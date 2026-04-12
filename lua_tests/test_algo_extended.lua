-- @Title: Test Algorithms API (Extended)
-- @Description: Comprehensive verification tests for the Algorithms API.
local framework = require("framework")

framework.test("Algo table existence", function()
    framework.assert(type(algo) ~= "nil", "algo table should exist")
end)

framework.test("Algo cellular automata functions", function()
    framework.assert(type(algo.cellularAutomata) == "function", "algo.cellularAutomata should be function")
    framework.assert(type(algo.generateCave) == "function", "algo.generateCave should be function")
end)

framework.test("Algo erosion functions", function()
    framework.assert(type(algo.erode) == "function", "algo.erode should be function")
    framework.assert(type(algo.thermalErode) == "function", "algo.thermalErode should be function")
end)

framework.test("Algo smoothing", function()
    framework.assert(type(algo.smooth) == "function", "algo.smooth should be function")
end)

framework.test("Algo voronoi functions", function()
    framework.assert(type(algo.voronoi) == "function", "algo.voronoi should be function")
    framework.assert(type(algo.generateRandomPoints) == "function", "algo.generateRandomPoints should be function")
end)

framework.test("Algo maze generation", function()
    framework.assert(type(algo.generateMaze) == "function", "algo.generateMaze should be function")
end)

framework.test("Algo dungeon generation", function()
    framework.assert(type(algo.generateDungeon) == "function", "algo.generateDungeon should be function")
end)

framework.test("Algo cellularAutomata basic", function()
    local grid = {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1}
    }

    local result = algo.cellularAutomata(grid)
    framework.assert(type(result) == "table", "cellularAutomata should return table")
    framework.assert(type(result[1]) == "table", "result rows should be tables")
    framework.assert(type(result[1][1]) == "number", "result values should be numbers")
end)

framework.test("Algo cellularAutomata with options", function()
    local grid = {
        {1, 1, 1, 1},
        {1, 0, 0, 1},
        {1, 0, 0, 1},
        {1, 1, 1, 1}
    }

    local result = algo.cellularAutomata(grid, {
        iterations = 2,
        birthLimit = 4,
        deathLimit = 3,
        width = 4,
        height = 4
    })
    framework.assert(type(result) == "table", "cellularAutomata with options should work")
    framework.assert(#result == 4, "result should have correct height")
end)

framework.test("Algo generateCave basic", function()
    local cave = algo.generateCave(20, 20)
    framework.assert(type(cave) == "table", "generateCave should return table")
    framework.assert(type(cave[1]) == "table", "cave rows should be tables")
    framework.assert(#cave == 20, "cave should have correct height")
    framework.assert(#cave[1] == 20, "cave should have correct width")
end)

framework.test("Algo generateCave with options", function()
    local cave = algo.generateCave(30, 30, {
        fillProbability = 0.4,
        iterations = 5,
        birthLimit = 4,
        deathLimit = 3,
        seed = 12345
    })
    framework.assert(type(cave) == "table", "generateCave with options should work")
    framework.assert(#cave == 30, "cave should have correct dimensions")

    -- Check that values are 0 or 1
    for y = 1, #cave do
        for x = 1, #cave[y] do
            framework.assert(cave[y][x] == 0 or cave[y][x] == 1,
                "cave values should be 0 or 1")
        end
    end
end)

framework.test("Algo generateCave edges are walls", function()
    local cave = algo.generateCave(20, 20, {seed = 1234})

    -- Edges should be walls (1)
    for x = 1, 20 do
        framework.assert(cave[1][x] == 1, "top edge should be wall")
        framework.assert(cave[20][x] == 1, "bottom edge should be wall")
    end
    for y = 1, 20 do
        framework.assert(cave[y][1] == 1, "left edge should be wall")
        framework.assert(cave[y][20] == 1, "right edge should be wall")
    end
end)

framework.test("Algo erode basic", function()
    local heightmap = {
        {0.5, 0.5, 0.5},
        {0.5, 1.0, 0.5},
        {0.5, 0.5, 0.5}
    }

    local result = algo.erode(heightmap, {iterations = 10})
    framework.assert(type(result) == "table", "erode should return table")
    framework.assert(type(result[1]) == "table", "result rows should be tables")
    framework.assert(type(result[1][1]) == "number", "result values should be numbers")
end)

framework.test("Algo erode with options", function()
    local heightmap = {
        {0.8, 0.6, 0.4},
        {0.6, 1.0, 0.6},
        {0.4, 0.6, 0.8}
    }

    local result = algo.erode(heightmap, {
        iterations = 100,
        erosionRadius = 2,
        inertia = 0.05,
        sedimentCapacity = 4.0,
        minSlope = 0.01,
        erosionSpeed = 0.3,
        depositSpeed = 0.3,
        evaporateSpeed = 0.01,
        gravity = 4.0,
        seed = 1234,
        maxDropletLifetime = 30
    })
    framework.assert(type(result) == "table", "erode with options should work")
end)

framework.test("Algo thermalErode basic", function()
    local heightmap = {
        {0.5, 0.5, 0.5},
        {0.5, 1.0, 0.5},
        {0.5, 0.5, 0.5}
    }

    local result = algo.thermalErode(heightmap, {iterations = 10})
    framework.assert(type(result) == "table", "thermalErode should return table")
end)

framework.test("Algo thermalErode with options", function()
    local heightmap = {
        {0.3, 0.5, 0.7},
        {0.5, 1.0, 0.5},
        {0.7, 0.5, 0.3}
    }

    local result = algo.thermalErode(heightmap, {
        iterations = 20,
        talusAngle = 0.3,
        erosionAmount = 0.7
    })
    framework.assert(type(result) == "table", "thermalErode with options should work")
end)

framework.test("Algo smooth basic", function()
    local grid = {
        {0, 1, 0},
        {1, 0, 1},
        {0, 1, 0}
    }

    local result = algo.smooth(grid)
    framework.assert(type(result) == "table", "smooth should return table")
    framework.assert(type(result[1][1]) == "number", "smooth values should be numbers")
end)

framework.test("Algo smooth with options", function()
    local grid = {
        {0, 1, 0, 1},
        {1, 0, 1, 0},
        {0, 1, 0, 1},
        {1, 0, 1, 0}
    }

    local result = algo.smooth(grid, {
        iterations = 2,
        kernelSize = 3
    })
    framework.assert(type(result) == "table", "smooth with options should work")
end)

framework.test("Algo generateRandomPoints", function()
    local points = algo.generateRandomPoints(100, 100, 10)
    framework.assert(type(points) == "table", "generateRandomPoints should return table")
    framework.assert(#points == 10, "should return requested number of points")

    -- Check point structure
    for i = 1, #points do
        framework.assert(type(points[i]) == "table", "each point should be table")
        framework.assert(type(points[i].x) == "number", "point.x should be number")
        framework.assert(type(points[i].y) == "number", "point.y should be number")
        framework.assert(points[i].x >= 0 and points[i].x < 100, "x should be in bounds")
        framework.assert(points[i].y >= 0 and points[i].y < 100, "y should be in bounds")
    end
end)

framework.test("Algo generateRandomPoints with seed", function()
    local points1 = algo.generateRandomPoints(100, 100, 5, 12345)
    local points2 = algo.generateRandomPoints(100, 100, 5, 12345)

    framework.assert(#points1 == #points2, "same seed should give same count")

    -- Same seed should give same points
    for i = 1, #points1 do
        framework.assert(points1[i].x == points2[i].x, "same seed should give same x")
        framework.assert(points1[i].y == points2[i].y, "same seed should give same y")
    end
end)

framework.test("Algo voronoi basic", function()
    local points = {
        {x = 25, y = 25},
        {x = 75, y = 25},
        {x = 50, y = 75}
    }

    local result = algo.voronoi(100, 100, points)
    framework.assert(type(result) == "table", "voronoi should return table")
    framework.assert(#result == 100, "result should have correct height")
    framework.assert(#result[1] == 100, "result should have correct width")
end)

framework.test("Algo voronoi region values", function()
    local points = {
        {x = 10, y = 10},
        {x = 90, y = 10},
        {x = 50, y = 50}
    }

    local result = algo.voronoi(100, 100, points)

    -- Values should be 1, 2, or 3 (region indices)
    for y = 1, #result do
        for x = 1, #result[y] do
            local val = result[y][x]
            framework.assert(val >= 1 and val <= 3,
                "voronoi values should be region indices (1-3)")
        end
    end
end)

framework.test("Algo generateMaze basic", function()
    local maze = algo.generateMaze(21, 21)
    framework.assert(type(maze) == "table", "generateMaze should return table")
    framework.assert(#maze == 21, "maze should have correct height")
    framework.assert(#maze[1] == 21, "maze should have correct width")
end)

framework.test("Algo generateMaze with seed", function()
    local maze1 = algo.generateMaze(21, 21, {seed = 12345})
    local maze2 = algo.generateMaze(21, 21, {seed = 12345})

    framework.assert(#maze1 == #maze2, "same seed should give same dimensions")

    -- Same seed should give same maze
    for y = 1, #maze1 do
        for x = 1, #maze1[y] do
            framework.assert(maze1[y][x] == maze2[y][x],
                "same seed should give same maze")
        end
    end
end)

framework.test("Algo generateMaze structure", function()
    local maze = algo.generateMaze(21, 21, {seed = 1234})

    -- Values should be 0 (path) or 1 (wall)
    for y = 1, #maze do
        for x = 1, #maze[y] do
            framework.assert(maze[y][x] == 0 or maze[y][x] == 1,
                "maze values should be 0 or 1")
        end
    end

    -- Start position should be path (0)
    -- Note: Maze carving starts at (1,1) in C++ 0-indexed = [2][2] in Lua 1-indexed
    framework.assert(maze[2][2] == 0, "start should be path")
end)

framework.test("Algo generateDungeon basic", function()
    local result = algo.generateDungeon(50, 50)
    framework.assert(type(result) == "table", "generateDungeon should return table")
    framework.assert(type(result.grid) == "table", "result.grid should be table")
    framework.assert(type(result.rooms) == "table", "result.rooms should be table")
end)

framework.test("Algo generateDungeon with options", function()
    local result = algo.generateDungeon(60, 60, {
        minRoomSize = 5,
        maxRoomSize = 15,
        seed = 12345,
        maxDepth = 4
    })

    framework.assert(type(result) == "table", "dungeon with options should work")
    framework.assert(type(result.grid) == "table", "grid should exist")
    framework.assert(type(result.rooms) == "table", "rooms should exist")
    framework.assert(#result.rooms > 0, "should have at least one room")
end)

framework.test("Algo generateDungeon grid structure", function()
    local result = algo.generateDungeon(50, 50, {seed = 1234})
    local grid = result.grid

    framework.assert(#grid == 50, "grid should have correct height")
    framework.assert(#grid[1] == 50, "grid should have correct width")

    -- Values should be 0 (floor) or 1 (wall)
    for y = 1, #grid do
        for x = 1, #grid[y] do
            framework.assert(grid[y][x] == 0 or grid[y][x] == 1,
                "dungeon grid values should be 0 or 1")
        end
    end
end)

framework.test("Algo generateDungeon rooms structure", function()
    local result = algo.generateDungeon(50, 50, {seed = 1234})
    local rooms = result.rooms

    if #rooms > 0 then
        for i = 1, #rooms do
            local room = rooms[i]
            framework.assert(type(room) == "table", "each room should be table")
            framework.assert(type(room.x) == "number", "room.x should be number")
            framework.assert(type(room.y) == "number", "room.y should be number")
            framework.assert(type(room.width) == "number", "room.width should be number")
            framework.assert(type(room.height) == "number", "room.height should be number")
        end
    end
end)

framework.test("Algo consistency with seed", function()
    -- Test that same seed gives same results
    local cave1 = algo.generateCave(20, 20, {seed = 99999})
    local cave2 = algo.generateCave(20, 20, {seed = 99999})

    framework.assert(#cave1 == #cave2, "same seed caves should have same height")

    for y = 1, #cave1 do
        for x = 1, #cave1[y] do
            framework.assert(cave1[y][x] == cave2[y][x],
                "same seed should give same cave")
        end
    end
end)

framework.test("Algo edge cases", function()
    -- Small grids
    local smallGrid = {{1, 0}, {0, 1}}
    local smallResult = algo.cellularAutomata(smallGrid)
    framework.assert(type(smallResult) == "table", "small grid should work")

    -- Large grids
    local largeCave = algo.generateCave(100, 100, {seed = 1234})
    framework.assert(type(largeCave) == "table", "large cave should work")
    framework.assert(#largeCave == 100, "large cave should have correct size")
end)

framework.test("Algo empty/invalid inputs", function()
    -- Empty table
    local emptyResult = algo.cellularAutomata({})
    framework.assert(type(emptyResult) == "table", "empty grid should handle gracefully")

    -- Nil options (should use defaults)
    local cave = algo.generateCave(20, 20, nil)
    framework.assert(type(cave) == "table", "nil options should use defaults")
end)

framework.summary()
