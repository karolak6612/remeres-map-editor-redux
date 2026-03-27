-- @Title: Test Geometry API (Extended)
-- @Description: Comprehensive verification tests for the Geometry API.
local framework = require("framework")

framework.test("Geo table existence", function()
    framework.assert(type(geo) ~= "nil", "geo table should exist")
end)

framework.test("Geo line functions existence", function()
    framework.assert(type(geo.bresenhamLine) == "function", "geo.bresenhamLine should be function")
    framework.assert(type(geo.bresenhamLine3d) == "function", "geo.bresenhamLine3d should be function")
end)

framework.test("Geo curve functions existence", function()
    framework.assert(type(geo.bezierCurve) == "function", "geo.bezierCurve should be function")
    framework.assert(type(geo.bezierCurve3d) == "function", "geo.bezierCurve3d should be function")
end)

framework.test("Geo flood fill functions existence", function()
    framework.assert(type(geo.floodFill) == "function", "geo.floodFill should be function")
    framework.assert(type(geo.getFloodFillPositions) == "function", "geo.getFloodFillPositions should be function")
end)

framework.test("Geo shape functions existence", function()
    framework.assert(type(geo.circle) == "function", "geo.circle should be function")
    framework.assert(type(geo.ellipse) == "function", "geo.ellipse should be function")
    framework.assert(type(geo.rectangle) == "function", "geo.rectangle should be function")
    framework.assert(type(geo.polygon) == "function", "geo.polygon should be function")
end)

framework.test("Geo distance functions existence", function()
    framework.assert(type(geo.distance) == "function", "geo.distance should be function")
    framework.assert(type(geo.distanceSq) == "function", "geo.distanceSq should be function")
    framework.assert(type(geo.distanceManhattan) == "function", "geo.distanceManhattan should be function")
    framework.assert(type(geo.distanceChebyshev) == "function", "geo.distanceChebyshev should be function")
end)

framework.test("Geo point in shape functions existence", function()
    framework.assert(type(geo.pointInCircle) == "function", "geo.pointInCircle should be function")
    framework.assert(type(geo.pointInRectangle) == "function", "geo.pointInRectangle should be function")
    framework.assert(type(geo.pointInPolygon) == "function", "geo.pointInPolygon should be function")
end)

framework.test("Geo scatter functions existence", function()
    framework.assert(type(geo.randomScatter) == "function", "geo.randomScatter should be function")
    framework.assert(type(geo.poissonDiskSampling) == "function", "geo.poissonDiskSampling should be function")
end)

framework.test("Geo bresenhamLine basic", function()
    local points = geo.bresenhamLine(0, 0, 5, 5)
    framework.assert(type(points) == "table", "bresenhamLine should return table")
    framework.assert(#points > 0, "line should have points")

    -- First and last points
    framework.assert(points[1].x == 0 and points[1].y == 0, "first point should be (0,0)")
    framework.assert(points[#points].x == 5 and points[#points].y == 5, "last point should be (5,5)")
end)

framework.test("Geo bresenhamLine horizontal", function()
    local points = geo.bresenhamLine(0, 0, 10, 0)
    framework.assert(#points == 11, "horizontal line should have 11 points")

    for i = 1, #points do
        framework.assert(points[i].y == 0, "all y should be 0")
        framework.assert(points[i].x == i - 1, "x should increment")
    end
end)

framework.test("Geo bresenhamLine vertical", function()
    local points = geo.bresenhamLine(0, 0, 0, 10)
    framework.assert(#points == 11, "vertical line should have 11 points")

    for i = 1, #points do
        framework.assert(points[i].x == 0, "all x should be 0")
        framework.assert(points[i].y == i - 1, "y should increment")
    end
end)

framework.test("Geo bresenhamLine3d", function()
    local points = geo.bresenhamLine3d(0, 0, 0, 5, 5, 5)
    framework.assert(type(points) == "table", "bresenhamLine3d should return table")
    framework.assert(#points > 0, "3D line should have points")

    -- Check first and last
    framework.assert(points[1].x == 0 and points[1].y == 0 and points[1].z == 0,
        "first point should be (0,0,0)")
    framework.assert(points[#points].x == 5 and points[#points].y == 5 and points[#points].z == 5,
        "last point should be (5,5,5)")
end)

framework.test("Geo bezierCurve basic", function()
    local controlPoints = {{x = 0, y = 0}, {x = 50, y = 100}, {x = 100, y = 0}}
    local curve = geo.bezierCurve(controlPoints, 10)

    framework.assert(type(curve) == "table", "bezierCurve should return table")
    framework.assert(#curve == 11, "10 steps should give 11 points")

    -- Check first and last points
    framework.assert(curve[1].x == 0 and curve[1].y == 0, "first point should match first control point")
    framework.assert(curve[#curve].x == 100 and curve[#curve].y == 0,
        "last point should match last control point")
end)

framework.test("Geo bezierCurve with different steps", function()
    local controlPoints = {{x = 0, y = 0}, {x = 50, y = 50}, {x = 100, y = 0}}

    local curve5 = geo.bezierCurve(controlPoints, 5)
    framework.assert(#curve5 == 6, "5 steps should give 6 points")

    local curve20 = geo.bezierCurve(controlPoints, 20)
    framework.assert(#curve20 == 21, "20 steps should give 21 points")
end)

framework.test("Geo bezierCurve3d", function()
    local controlPoints = {
        {x = 0, y = 0, z = 0},
        {x = 50, y = 50, z = 50},
        {x = 100, y = 0, z = 0}
    }

    local curve = geo.bezierCurve3d(controlPoints, 10)
    framework.assert(type(curve) == "table", "bezierCurve3d should return table")
    framework.assert(#curve == 11, "should have 11 points")

    -- Check z coordinates
    framework.assert(curve[1].z == 0, "first z should be 0")
    framework.assert(curve[#curve].z == 0, "last z should be 0")
end)

framework.test("Geo floodFill basic", function()
    local grid = {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1}
    }

    local result = geo.floodFill(grid, 2, 2, 2)
    framework.assert(type(result) == "table", "floodFill should return table")
    framework.assert(result[2][2] == 2, "center should be filled")
end)

framework.test("Geo floodFill 8-connected", function()
    local grid = {
        {1, 0, 1},
        {0, 0, 0},
        {1, 0, 1}
    }

    -- 4-connected should not fill diagonals
    local result4 = geo.floodFill(grid, 2, 2, 2, {eightConnected = false})
    -- 8-connected should fill diagonals
    local result8 = geo.floodFill(grid, 2, 2, 2, {eightConnected = true})

    framework.assert(type(result4) == "table", "4-connected should work")
    framework.assert(type(result8) == "table", "8-connected should work")
end)

framework.test("Geo getFloodFillPositions", function()
    local grid = {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1}
    }

    local positions = geo.getFloodFillPositions(grid, 2, 2)
    framework.assert(type(positions) == "table", "getFloodFillPositions should return table")
    framework.assert(#positions > 0, "should return positions")

    -- Check position structure
    for i = 1, #positions do
        framework.assert(type(positions[i]) == "table", "each position should be table")
        framework.assert(type(positions[i].x) == "number", "position.x should be number")
        framework.assert(type(positions[i].y) == "number", "position.y should be number")
    end
end)

framework.test("Geo circle outline", function()
    local circle = geo.circle(0, 0, 5, {filled = false})
    framework.assert(type(circle) == "table", "circle should return table")
    framework.assert(#circle > 0, "circle should have points")

    -- All points should be approximately at radius distance
    for i = 1, #circle do
        local dx = circle[i].x - 0
        local dy = circle[i].y - 0
        local dist = math.sqrt(dx * dx + dy * dy)
        framework.assert(dist >= 4 and dist <= 6, "point should be near radius distance")
    end
end)

framework.test("Geo circle filled", function()
    local circle = geo.circle(0, 0, 5, {filled = true})
    framework.assert(type(circle) == "table", "filled circle should return table")

    -- All points should be within radius
    for i = 1, #circle do
        local dx = circle[i].x - 0
        local dy = circle[i].y - 0
        local distSq = dx * dx + dy * dy
        framework.assert(distSq <= 25, "point should be within radius")
    end
end)

framework.test("Geo ellipse", function()
    local ellipse = geo.ellipse(0, 0, 10, 5, {filled = false})
    framework.assert(type(ellipse) == "table", "ellipse should return table")
    framework.assert(#ellipse > 0, "ellipse should have points")
end)

framework.test("Geo ellipse filled", function()
    local ellipse = geo.ellipse(0, 0, 10, 5, {filled = true})
    framework.assert(type(ellipse) == "table", "filled ellipse should return table")
end)

framework.test("Geo rectangle outline", function()
    local rect = geo.rectangle(0, 0, 10, 5, {filled = false})
    framework.assert(type(rect) == "table", "rectangle should return table")

    -- Check that points are on the border
    for i = 1, #rect do
        local p = rect[i]
        local onBorder = (p.x == 0 or p.x == 10 or p.y == 0 or p.y == 5)
        framework.assert(onBorder, "point should be on rectangle border")
    end
end)

framework.test("Geo rectangle filled", function()
    local rect = geo.rectangle(0, 0, 10, 5, {filled = true})
    framework.assert(type(rect) == "table", "filled rectangle should return table")

    -- Check that all points are within bounds
    for i = 1, #rect do
        local p = rect[i]
        framework.assert(p.x >= 0 and p.x <= 10, "x should be in bounds")
        framework.assert(p.y >= 0 and p.y <= 5, "y should be in bounds")
    end
end)

framework.test("Geo polygon", function()
    local triangle = {
        {x = 0, y = 0},
        {x = 10, y = 0},
        {x = 5, y = 10}
    }

    local polygon = geo.polygon(triangle)
    framework.assert(type(polygon) == "table", "polygon should return table")
    framework.assert(#polygon > 0, "polygon should have points")
end)

framework.test("Geo distance", function()
    local d = geo.distance(0, 0, 3, 4)
    framework.assert(math.abs(d - 5) < 0.0001, "distance(0,0,3,4) should be 5")

    local d2 = geo.distance(1, 1, 4, 5)
    framework.assert(math.abs(d2 - 5) < 0.0001, "distance(1,1,4,5) should be 5")
end)

framework.test("Geo distanceSq", function()
    local d = geo.distanceSq(0, 0, 3, 4)
    framework.assert(d == 25, "distanceSq(0,0,3,4) should be 25")
end)

framework.test("Geo distanceManhattan", function()
    local d = geo.distanceManhattan(0, 0, 3, 4)
    framework.assert(d == 7, "manhattan(0,0,3,4) should be 7")
end)

framework.test("Geo distanceChebyshev", function()
    local d = geo.distanceChebyshev(0, 0, 3, 4)
    framework.assert(d == 4, "chebyshev(0,0,3,4) should be 4")
end)

framework.test("Geo pointInCircle", function()
    framework.assert(geo.pointInCircle(0, 0, 0, 0, 5) == true, "center should be in circle")
    framework.assert(geo.pointInCircle(3, 4, 0, 0, 5) == true, "point on edge should be in circle")
    framework.assert(geo.pointInCircle(4, 4, 0, 0, 5) == false, "point outside should not be in circle")
end)

framework.test("Geo pointInRectangle", function()
    framework.assert(geo.pointInRectangle(5, 5, 0, 0, 10, 10) == true,
        "point inside should be in rectangle")
    framework.assert(geo.pointInRectangle(0, 0, 0, 0, 10, 10) == true,
        "point on corner should be in rectangle")
    framework.assert(geo.pointInRectangle(15, 15, 0, 0, 10, 10) == false,
        "point outside should not be in rectangle")
end)

framework.test("Geo pointInPolygon", function()
    local triangle = {
        {x = 0, y = 0},
        {x = 10, y = 0},
        {x = 5, y = 10}
    }

    framework.assert(geo.pointInPolygon(5, 5, triangle) == true,
        "point inside triangle should return true")
    framework.assert(geo.pointInPolygon(0, 0, triangle) == true,
        "point on vertex should return true")
    framework.assert(geo.pointInPolygon(20, 20, triangle) == false,
        "point outside should return false")
end)

framework.test("Geo randomScatter", function()
    local points = geo.randomScatter(0, 0, 100, 100, 10)
    framework.assert(type(points) == "table", "randomScatter should return table")
    framework.assert(#points <= 10, "should return at most requested points")

    -- Check points are in bounds
    for i = 1, #points do
        framework.assert(points[i].x >= 0 and points[i].x <= 100, "x should be in bounds")
        framework.assert(points[i].y >= 0 and points[i].y <= 100, "y should be in bounds")
    end
end)

framework.test("Geo randomScatter with minDistance", function()
    local points = geo.randomScatter(0, 0, 100, 100, 5, {minDistance = 20})

    -- Check minimum distance between points
    for i = 1, #points do
        for j = i + 1, #points do
            local dx = points[i].x - points[j].x
            local dy = points[i].y - points[j].y
            local dist = math.sqrt(dx * dx + dy * dy)
            framework.assert(dist >= 20 or i == j,
                "points should be at least minDistance apart")
        end
    end
end)

framework.test("Geo randomScatter with seed", function()
    local points1 = geo.randomScatter(0, 0, 100, 100, 5, {seed = 12345})
    local points2 = geo.randomScatter(0, 0, 100, 100, 5, {seed = 12345})

    framework.assert(#points1 == #points2, "same seed should give same count")

    for i = 1, #points1 do
        framework.assert(points1[i].x == points2[i].x, "same seed should give same x")
        framework.assert(points1[i].y == points2[i].y, "same seed should give same y")
    end
end)

framework.test("Geo poissonDiskSampling", function()
    local points = geo.poissonDiskSampling(0, 0, 100, 100, 10)
    framework.assert(type(points) == "table", "poissonDiskSampling should return table")

    -- Check points are in bounds
    for i = 1, #points do
        framework.assert(points[i].x >= 0 and points[i].x <= 100, "x should be in bounds")
        framework.assert(points[i].y >= 0 and points[i].y <= 100, "y should be in bounds")
    end
end)

framework.test("Geo poissonDiskSampling with seed", function()
    local points1 = geo.poissonDiskSampling(0, 0, 100, 100, 10, {seed = 12345})
    local points2 = geo.poissonDiskSampling(0, 0, 100, 100, 10, {seed = 12345})

    framework.assert(#points1 == #points2, "same seed should give same count")
end)

framework.test("Geo functions return types", function()
    -- Verify all functions return expected types
    local line = geo.bresenhamLine(0, 0, 5, 5)
    framework.assert(type(line) == "table", "bresenhamLine returns table")

    local curve = geo.bezierCurve({{x = 0, y = 0}, {x = 10, y = 10}}, 5)
    framework.assert(type(curve) == "table", "bezierCurve returns table")

    local circle = geo.circle(0, 0, 5)
    framework.assert(type(circle) == "table", "circle returns table")

    local dist = geo.distance(0, 0, 3, 4)
    framework.assert(type(dist) == "number", "distance returns number")

    local inCircle = geo.pointInCircle(0, 0, 0, 0, 5)
    framework.assert(type(inCircle) == "boolean", "pointInCircle returns boolean")
end)

framework.test("Geo edge cases", function()
    -- Zero length line
    local zeroLine = geo.bresenhamLine(5, 5, 5, 5)
    framework.assert(type(zeroLine) == "table", "zero length line should work")
    framework.assert(#zeroLine == 1, "zero length line should have 1 point")

    -- Very small circle
    local tinyCircle = geo.circle(0, 0, 1)
    framework.assert(type(tinyCircle) == "table", "tiny circle should work")

    -- Single point polygon (should handle gracefully)
    local singlePoint = geo.polygon({{x = 0, y = 0}})
    framework.assert(type(singlePoint) == "table", "single point polygon should handle gracefully")
end)

framework.summary()
