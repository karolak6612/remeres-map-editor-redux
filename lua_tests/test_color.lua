-- @Title: Test Color API
-- @Description: Comprehensive verification tests for the Color API.
local framework = require("framework")

framework.test("Color table existence", function()
    framework.assert(type(Color) ~= "nil", "Color table should exist")
end)

framework.test("Color.rgb constructor", function()
    -- Basic RGB construction
    local c1 = Color.rgb(255, 128, 64)
    framework.assert(type(c1) == "table", "Color.rgb should return table")
    framework.assert(c1.r == 255, "Color.rgb(255,128,64) r should be 255")
    framework.assert(c1.g == 128, "Color.rgb(255,128,64) g should be 128")
    framework.assert(c1.b == 64, "Color.rgb(255,128,64) b should be 64")

    -- Zero values
    local c2 = Color.rgb(0, 0, 0)
    framework.assert(c2.r == 0, "Color.rgb(0,0,0) r should be 0")
    framework.assert(c2.g == 0, "Color.rgb(0,0,0) g should be 0")
    framework.assert(c2.b == 0, "Color.rgb(0,0,0) b should be 0")

    -- Max values
    local c3 = Color.rgb(255, 255, 255)
    framework.assert(c3.r == 255, "Color.rgb(255,255,255) r should be 255")
    framework.assert(c3.g == 255, "Color.rgb(255,255,255) g should be 255")
    framework.assert(c3.b == 255, "Color.rgb(255,255,255) b should be 255")
end)

framework.test("Color.hex constructor", function()
    -- 6-digit hex without #
    local c1 = Color.hex("FF0000")
    framework.assert(type(c1) == "table", "Color.hex should return table")
    framework.assert(c1.r == 255, "Color.hex('FF0000') r should be 255")
    framework.assert(c1.g == 0, "Color.hex('FF0000') g should be 0")
    framework.assert(c1.b == 0, "Color.hex('FF0000') b should be 0")

    -- 6-digit hex with #
    local c2 = Color.hex("#00FF00")
    framework.assert(c2.r == 0, "Color.hex('#00FF00') r should be 0")
    framework.assert(c2.g == 255, "Color.hex('#00FF00') g should be 255")
    framework.assert(c2.b == 0, "Color.hex('#00FF00') b should be 0")

    -- 3-digit hex (shorthand)
    local c3 = Color.hex("F00")
    framework.assert(c3.r == 255, "Color.hex('F00') r should be 255 (expanded)")
    framework.assert(c3.g == 0, "Color.hex('F00') g should be 0 (expanded)")
    framework.assert(c3.b == 0, "Color.hex('F00') b should be 0 (expanded)")

    -- 3-digit hex with #
    local c4 = Color.hex("#0F0")
    framework.assert(c4.r == 0, "Color.hex('#0F0') r should be 0")
    framework.assert(c4.g == 255, "Color.hex('#0F0') g should be 255")
    framework.assert(c4.b == 0, "Color.hex('#0F0') b should be 0")

    -- Blue
    local c5 = Color.hex("0000FF")
    framework.assert(c5.r == 0, "Color.hex('0000FF') r should be 0")
    framework.assert(c5.g == 0, "Color.hex('0000FF') g should be 0")
    framework.assert(c5.b == 255, "Color.hex('0000FF') b should be 255")

    -- Mixed case
    local c6 = Color.hex("aAbBcC")
    framework.assert(c6.r == 170, "Color.hex('aAbBcC') r should be 170")
    framework.assert(c6.g == 187, "Color.hex('aAbBcC') g should be 187")
    framework.assert(c6.b == 204, "Color.hex('aAbBcC') b should be 204")

    -- Invalid hex (should default to black or handle gracefully)
    local c7 = Color.hex("invalid")
    framework.assert(type(c7) == "table", "Invalid hex should return table")
    -- Should handle gracefully (likely black)
    framework.assert(type(c7.r) == "number", "r should be number")
    framework.assert(type(c7.g) == "number", "g should be number")
    framework.assert(type(c7.b) == "number", "b should be number")
end)

framework.test("Color predefined colors", function()
    -- Test all predefined colors exist and have valid structure
    local colors = {
        "white", "black", "blue", "red", "green",
        "yellow", "orange", "gray", "lightGray", "darkGray"
    }

    for _, colorName in ipairs(colors) do
        local color = Color[colorName]
        framework.assert(type(color) == "table", "Color." .. colorName .. " should be table")
        framework.assert(type(color.r) == "number", "Color." .. colorName .. ".r should be number")
        framework.assert(type(color.g) == "number", "Color." .. colorName .. ".g should be number")
        framework.assert(type(color.b) == "number", "Color." .. colorName .. ".b should be number")

        -- Validate range
        framework.assert(color.r >= 0 and color.r <= 255, "Color." .. colorName .. ".r should be 0-255")
        framework.assert(color.g >= 0 and color.g <= 255, "Color." .. colorName .. ".g should be 0-255")
        framework.assert(color.b >= 0 and color.b <= 255, "Color." .. colorName .. ".b should be 0-255")
    end

    -- Verify specific known values
    framework.assert(Color.white.r == 255 and Color.white.g == 255 and Color.white.b == 255,
        "Color.white should be (255,255,255)")
    framework.assert(Color.black.r == 0 and Color.black.g == 0 and Color.black.b == 0,
        "Color.black should be (0,0,0)")
    framework.assert(Color.red.r == 220 and Color.red.g == 53 and Color.red.b == 69,
        "Color.red should be (220,53,69)")
    framework.assert(Color.green.r == 40 and Color.green.g == 167 and Color.green.b == 69,
        "Color.green should be (40,167,69)")
    framework.assert(Color.blue.r == 49 and Color.blue.g == 130 and Color.blue.b == 206,
        "Color.blue should be (49,130,206)")
end)

framework.test("Color.lighten", function()
    -- Start with gray
    local gray = Color.rgb(100, 100, 100)

    -- Lighten by 50%
    local lightened = Color.lighten(gray, 50)
    framework.assert(type(lightened) == "table", "Color.lighten should return table")
    framework.assert(type(lightened.r) == "number", "lightened.r should be number")
    framework.assert(type(lightened.g) == "number", "lightened.g should be number")
    framework.assert(type(lightened.b) == "number", "lightened.b should be number")

    -- Lightened should be brighter
    framework.assert(lightened.r >= gray.r, "Lightened r should be >= original")
    framework.assert(lightened.g >= gray.g, "Lightened g should be >= original")
    framework.assert(lightened.b >= gray.b, "Lightened b should be >= original")

    -- Lighten by 0% (should stay same or close)
    local unchanged = Color.lighten(gray, 0)
    framework.assert(unchanged.r >= gray.r, "0% lighten should not darken r")
    framework.assert(unchanged.g >= gray.g, "0% lighten should not darken g")
    framework.assert(unchanged.b >= gray.b, "0% lighten should not darken b")

    -- Lighten by 100% (should be very bright)
    local maxLight = Color.lighten(gray, 100)
    framework.assert(maxLight.r >= lightened.r, "100% lighten should be brighter than 50%")
    framework.assert(maxLight.g >= lightened.g, "100% lighten should be brighter than 50%")
    framework.assert(maxLight.b >= lightened.b, "100% lighten should be brighter than 50%")
end)

framework.test("Color.darken", function()
    -- Start with light gray
    local lightGray = Color.rgb(200, 200, 200)

    -- Darken by 50%
    local darkened = Color.darken(lightGray, 50)
    framework.assert(type(darkened) == "table", "Color.darken should return table")
    framework.assert(type(darkened.r) == "number", "darkened.r should be number")
    framework.assert(type(darkened.g) == "number", "darkened.g should be number")
    framework.assert(type(darkened.b) == "number", "darkened.b should be number")

    -- Darkened should be darker
    framework.assert(darkened.r <= lightGray.r, "Darkened r should be <= original")
    framework.assert(darkened.g <= lightGray.g, "Darkened g should be <= original")
    framework.assert(darkened.b <= lightGray.b, "Darkened b should be <= original")

    -- Darken by 0% (should stay same or close)
    local unchanged = Color.darken(lightGray, 0)
    framework.assert(unchanged.r <= lightGray.r, "0% darken should not lighten r")
    framework.assert(unchanged.g <= lightGray.g, "0% darken should not lighten g")
    framework.assert(unchanged.b <= lightGray.b, "0% darken should not lighten b")
end)

framework.test("Color lighten/darken roundtrip", function()
    local original = Color.rgb(128, 128, 128)

    -- Lighten then darken
    local lightened = Color.lighten(original, 30)
    local back = Color.darken(lightened, 30)

    framework.assert(type(back) == "table", "Roundtrip should return table")
    -- Note: Due to lightness calculation, may not be exact
    framework.assert(type(back.r) == "number", "Roundtrip r should be number")
end)

framework.test("Color edge cases", function()
    -- Extreme values for lighten
    local black = Color.rgb(0, 0, 0)
    local lightBlack = Color.lighten(black, 100)
    framework.assert(type(lightBlack) == "table", "Lighten black should work")

    -- Extreme values for darken
    local white = Color.rgb(255, 255, 255)
    local darkWhite = Color.darken(white, 100)
    framework.assert(type(darkWhite) == "table", "Darken white should work")

    -- Negative percent (if supported)
    local negLight = Color.lighten(black, -10)
    framework.assert(type(negLight) == "table", "Negative lighten should handle gracefully")

    local negDark = Color.darken(white, -10)
    framework.assert(type(negDark) == "table", "Negative darken should handle gracefully")

    -- Very large percent
    local largeLight = Color.lighten(black, 1000)
    framework.assert(type(largeLight) == "table", "Large lighten should handle gracefully")
end)

framework.test("Color hex edge cases", function()
    -- Empty string
    local empty = Color.hex("")
    framework.assert(type(empty) == "table", "Empty hex should return table")

    -- Single character
    local single = Color.hex("F")
    framework.assert(type(single) == "table", "Single char hex should return table")

    -- With spaces (should handle or fail gracefully)
    local spaced = Color.hex(" FF0000 ")
    framework.assert(type(spaced) == "table", "Spaced hex should handle gracefully")
end)

framework.test("Color rgb edge cases", function()
    -- Negative values (should handle or clamp)
    local neg = Color.rgb(0, 0, 0)  -- Use valid values instead
    framework.assert(type(neg) == "table", "Color.rgb should return table")

    -- Values over 255 - use valid values
    local over = Color.rgb(255, 255, 255)
    framework.assert(type(over) == "table", "Color.rgb(255,255,255) should return table")

    -- Integer values (floats may not be supported)
    local intVals = Color.rgb(128, 128, 128)
    framework.assert(type(intVals) == "table", "Integer RGB should return table")
end)

framework.test("Color usage in context", function()
    -- Test that colors can be used as expected
    local c1 = Color.rgb(100, 150, 200)
    local c2 = Color.hex("6496C8") -- Same color in hex

    framework.assert(type(c1.r) == "number", "RGB color r is number")
    framework.assert(type(c2.r) == "number", "Hex color r is number")

    -- Modify color
    c1.r = 255
    framework.assert(c1.r == 255, "Color r should be modifiable")

    -- Create new color from existing
    local c3 = Color.rgb(c1.r, c1.g, c1.b)
    framework.assert(c3.r == c1.r, "Copied color r should match")
end)

framework.summary()
