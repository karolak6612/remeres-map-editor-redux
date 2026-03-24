-- @Title: Test Image API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("Image constructors", function()
    -- We might not have a file, but we can try an item sprite
    local img = Image.fromItemSprite(2160) -- Crystal coin
    framework.assert(type(img) ~= "nil", "fromItemSprite should return image")
    framework.assert(img.valid == true, "image should be valid")
    framework.assert(img.width > 0, "width should be > 0")
    framework.assert(img.height > 0, "height should be > 0")
    framework.assert(img.isFromSprite == true, "isFromSprite should be true")
    
    local img2 = Image({itemid = 2160})
    framework.assert(img2.valid == true, "Image({itemid=...}) should be valid")
end)

framework.test("Image methods", function()
    local img = Image.fromItemSprite(2160)
    if not img or not img.valid then return end
    
    local resized = img:resize(64, 64)
    framework.assert(resized.width == 64, "resized width should be 64")
    framework.assert(resized.height == 64, "resized height should be 64")
    
    local scaled = img:scale(2.0)
    framework.assert(scaled.width == img.width * 2, "scaled width should be double")
end)

framework.summary()
