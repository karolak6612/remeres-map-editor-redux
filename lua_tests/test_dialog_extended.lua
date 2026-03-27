-- @Title: Test Dialog/UI API (Extended)
-- @Description: Comprehensive verification tests for the Dialog/UI API.
local framework = require("framework")

framework.test("Dialog function existence", function()
    -- Dialog may be a global function or accessed differently
    -- Just check that we can create dialogs
    framework.assert(type(Dialog) == "function" or type(Dialog) ~= "nil", "Dialog should exist")
end)

framework.test("Dialog construction", function()
    local dlg = Dialog("Test Dialog")
    framework.assert(type(dlg) ~= "nil", "Dialog should be created")
    framework.assert(type(dlg) == "table" or type(dlg) == "userdata",
        "Dialog should be table or userdata")
end)

framework.test("Dialog with different titles", function()
    local dlg1 = Dialog("")
    framework.assert(type(dlg1) ~= "nil", "empty title should work")

    local dlg2 = Dialog("Dialog with spaces and symbols !@#$%")
    framework.assert(type(dlg2) ~= "nil", "special chars in title should work")

    local longTitle = string.rep("a", 1000)
    local dlg3 = Dialog(longTitle)
    framework.assert(type(dlg3) ~= "nil", "long title should work")
end)

framework.test("Widget methods - basic", function()
    local dlg = Dialog("Widget Test")

    framework.assert(type(dlg.label) == "function", "label should be function")
    framework.assert(type(dlg.input) == "function", "input should be function")
    framework.assert(type(dlg.number) == "function", "number should be function")
    framework.assert(type(dlg.check) == "function", "check should be function")
    framework.assert(type(dlg.button) == "function", "button should be function")
end)

framework.test("Widget methods - advanced", function()
    local dlg = Dialog("Advanced Widgets")

    framework.assert(type(dlg.combobox) == "function", "combobox should be function")
    framework.assert(type(dlg.color) == "function", "color should be function")
    framework.assert(type(dlg.item) == "function", "item should be function")
    framework.assert(type(dlg.image) == "function", "image should be function")
    framework.assert(type(dlg.file) == "function", "file should be function")
    framework.assert(type(dlg.mapCanvas) == "function", "mapCanvas should be function")
end)

framework.test("Layout methods", function()
    local dlg = Dialog("Layout Test")

    framework.assert(type(dlg.box) == "function", "box should be function")
    framework.assert(type(dlg.endbox) == "function", "endbox should be function")
    framework.assert(type(dlg.wrap) == "function", "wrap should be function")
    framework.assert(type(dlg.endwrap) == "function", "endwrap should be function")
    framework.assert(type(dlg.newrow) == "function", "newrow should be function")
    framework.assert(type(dlg.separator) == "function", "separator should be function")
end)

framework.test("Widget methods return dialog", function()
    local dlg = Dialog("Return Test")

    -- All widget methods should return the dialog for chaining
    framework.assert(dlg:label({text = "test"}) == dlg, "label should return dlg")
    framework.assert(dlg:input({id = "i1"}) == dlg, "input should return dlg")
    framework.assert(dlg:number({id = "n1"}) == dlg, "number should return dlg")
    framework.assert(dlg:check({id = "c1"}) == dlg, "check should return dlg")
    framework.assert(dlg:button({text = "b1"}) == dlg, "button should return dlg")
end)

framework.test("Label widget", function()
    local dlg = Dialog("Label Test")

    -- Basic label
    local result1 = dlg:label({text = "Hello World"})
    framework.assert(result1 == dlg, "label should return dialog")

    -- Label with id
    local result2 = dlg:label({id = "lbl1", text = "Test"})
    framework.assert(result2 == dlg, "label with id should return dialog")

    -- Label without text (should handle gracefully)
    local result3 = dlg:label({id = "lbl2"})
    framework.assert(result3 == dlg, "label without text should work")
end)

framework.test("Input widget", function()
    local dlg = Dialog("Input Test")

    -- Basic input
    local result1 = dlg:input({id = "inp1"})
    framework.assert(result1 == dlg, "input should return dialog")

    -- Input with default value
    local result2 = dlg:input({id = "inp2", value = "default"})
    framework.assert(result2 == dlg, "input with default should return dialog")

    -- Input with placeholder
    local result3 = dlg:input({id = "inp3", placeholder = "Enter text..."})
    framework.assert(result3 == dlg, "input with placeholder should return dialog")
end)

framework.test("Number widget", function()
    local dlg = Dialog("Number Test")

    -- Basic number
    local result1 = dlg:number({id = "num1"})
    framework.assert(result1 == dlg, "number should return dialog")

    -- Number with min/max
    local result2 = dlg:number({id = "num2", min = 0, max = 100})
    framework.assert(result2 == dlg, "number with range should return dialog")

    -- Number with default
    local result3 = dlg:number({id = "num3", value = 50})
    framework.assert(result3 == dlg, "number with default should return dialog")
end)

framework.test("Checkbox widget", function()
    local dlg = Dialog("Checkbox Test")

    -- Basic checkbox
    local result1 = dlg:check({id = "chk1"})
    framework.assert(result1 == dlg, "check should return dialog")

    -- Checkbox with default
    local result2 = dlg:check({id = "chk2", value = true})
    framework.assert(result2 == dlg, "check with default should return dialog")

    -- Checkbox with label
    local result3 = dlg:check({id = "chk3", text = "Enable feature"})
    framework.assert(result3 == dlg, "check with label should return dialog")
end)

framework.test("Button widget", function()
    local dlg = Dialog("Button Test")

    -- Basic button
    local result1 = dlg:button({text = "Click me"})
    framework.assert(result1 == dlg, "button should return dialog")

    -- Button with id
    local result2 = dlg:button({id = "btn1", text = "OK"})
    framework.assert(result2 == dlg, "button with id should return dialog")

    -- Button with callback (should handle gracefully)
    local result3 = dlg:button({text = "Test", callback = function() end})
    framework.assert(result3 == dlg, "button with callback should return dialog")
end)

framework.test("Combobox widget", function()
    local dlg = Dialog("Combobox Test")

    -- Basic combobox
    local result1 = dlg:combobox({id = "cb1", options = {"A", "B", "C"}})
    framework.assert(result1 == dlg, "combobox should return dialog")

    -- Combobox with default selection
    local result2 = dlg:combobox({id = "cb2", options = {"X", "Y", "Z"}, value = "Y"})
    framework.assert(result2 == dlg, "combobox with default should return dialog")

    -- Combobox with empty options
    local result3 = dlg:combobox({id = "cb3", options = {}})
    framework.assert(result3 == dlg, "combobox with empty options should work")
end)

framework.test("Color widget", function()
    local dlg = Dialog("Color Test")

    -- Basic color picker
    local result1 = dlg:color({id = "col1"})
    framework.assert(result1 == dlg, "color should return dialog")

    -- Color with default value
    local result2 = dlg:color({id = "col2", value = {r = 255, g = 0, b = 0}})
    framework.assert(result2 == dlg, "color with default should return dialog")
end)

framework.test("Item widget", function()
    local dlg = Dialog("Item Test")

    -- Basic item
    local result1 = dlg:item({id = "itm1"})
    framework.assert(result1 == dlg, "item should return dialog")

    -- Item with default
    local result2 = dlg:item({id = "itm2", value = 2160})
    framework.assert(result2 == dlg, "item with default should return dialog")
end)

framework.test("Image widget", function()
    local dlg = Dialog("Image Test")

    -- Image with itemid
    local result1 = dlg:image({id = "img1", itemid = 2160})
    framework.assert(result1 == dlg, "image with itemid should return dialog")

    -- Image without itemid (should handle gracefully)
    local result2 = dlg:image({id = "img2"})
    framework.assert(result2 == dlg, "image without itemid should work")
end)

framework.test("File widget", function()
    local dlg = Dialog("File Test")

    -- Basic file picker
    local result1 = dlg:file({id = "file1"})
    framework.assert(result1 == dlg, "file should return dialog")

    -- File with filter
    local result2 = dlg:file({id = "file2", filter = "*.lua"})
    framework.assert(result2 == dlg, "file with filter should return dialog")
end)

framework.test("MapCanvas widget", function()
    local dlg = Dialog("MapCanvas Test")

    -- Basic map canvas
    local result1 = dlg:mapCanvas({id = "map1"})
    framework.assert(result1 == dlg, "mapCanvas should return dialog")

    -- Map canvas with options
    local result2 = dlg:mapCanvas({id = "map2", width = 200, height = 200})
    framework.assert(result2 == dlg, "mapCanvas with options should return dialog")
end)

framework.test("Box layout", function()
    local dlg = Dialog("Box Layout Test")

    -- Box without options
    local result1 = dlg:box({})
    framework.assert(result1 == dlg, "box should return dialog")

    -- Box with title
    local result2 = dlg:box({title = "Group"})
    framework.assert(result2 == dlg, "box with title should return dialog")

    -- Endbox
    local result3 = dlg:endbox({})
    framework.assert(result3 == dlg, "endbox should return dialog")
end)

framework.test("Wrap layout", function()
    local dlg = Dialog("Wrap Layout Test")

    -- Wrap
    local result1 = dlg:wrap({})
    framework.assert(result1 == dlg, "wrap should return dialog")

    -- Endwrap
    local result2 = dlg:endwrap({})
    framework.assert(result2 == dlg, "endwrap should return dialog")
end)

framework.test("NewRow layout", function()
    local dlg = Dialog("NewRow Test")

    local result = dlg:newrow()
    framework.assert(result == dlg, "newrow should return dialog")
end)

framework.test("Separator widget", function()
    local dlg = Dialog("Separator Test")

    -- Basic separator
    local result1 = dlg:separator({})
    framework.assert(result1 == dlg, "separator should return dialog")
end)

framework.test("Widget chaining", function()
    local dlg = Dialog("Chaining Test")

    -- Chain multiple widgets
    local result = dlg
        :label({text = "Label"})
        :input({id = "inp"})
        :button({text = "OK"})

    framework.assert(result == dlg, "chaining should return dialog")
end)

framework.test("Multiple dialogs", function()
    local dlg1 = Dialog("First Dialog")
    local dlg2 = Dialog("Second Dialog")
    local dlg3 = Dialog("Third Dialog")

    framework.assert(type(dlg1) ~= "nil", "first dialog should exist")
    framework.assert(type(dlg2) ~= "nil", "second dialog should exist")
    framework.assert(type(dlg3) ~= "nil", "third dialog should exist")

    -- They should be different instances
    framework.assert(dlg1 ~= dlg2, "dialogs should be different instances")
end)

framework.test("Widget methods with nil options", function()
    local dlg = Dialog("Nil Options Test")

    -- Methods should handle nil gracefully or error appropriately
    local success1, _ = pcall(function() dlg:label(nil) end)
    -- May fail, that's ok

    local success2, _ = pcall(function() dlg:input(nil) end)
    -- May fail, that's ok
end)

framework.test("Widget methods with empty table", function()
    local dlg = Dialog("Empty Table Test")

    local result1 = dlg:label({})
    framework.assert(result1 == dlg, "label with empty table should return dialog")

    local result2 = dlg:button({})
    framework.assert(result2 == dlg, "button with empty table should return dialog")
end)

framework.test("Dialog method existence summary", function()
    local dlg = Dialog("Summary Test")

    -- All expected methods should exist
    local methods = {
        "label", "input", "number", "check", "button",
        "combobox", "color", "item", "image", "file", "mapCanvas",
        "box", "endbox", "wrap", "endwrap", "newrow", "separator"
    }

    for _, method in ipairs(methods) do
        framework.assert(type(dlg[method]) == "function",
            method .. " should be function")
    end
end)

framework.summary()
