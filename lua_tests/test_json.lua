-- @Title: Test JSON API
-- @Description: Comprehensive verification tests for the JSON API.
local framework = require("framework")

framework.test("JSON table existence", function()
    framework.assert(type(json) ~= "nil", "json table should exist")
    framework.assert(type(json.encode) == "function", "json.encode should be function")
    framework.assert(type(json.encode_pretty) == "function", "json.encode_pretty should be function")
    framework.assert(type(json.decode) == "function", "json.decode should be function")
end)

framework.test("json.encode basic types", function()
    -- String
    local str = json.encode("hello")
    framework.assert(type(str) == "string", "encode string should return string")
    framework.assert(str == '"hello"', 'encode string should be quoted')

    -- Number (integer)
    local num = json.encode(42)
    framework.assert(type(num) == "string", "encode number should return string")
    framework.assert(num == "42", "encode number should be unquoted")

    -- Number (float)
    local float = json.encode(3.14)
    framework.assert(type(float) == "string", "encode float should return string")
    framework.assert(tonumber(float) == 3.14, "encode float should preserve value")

    -- Boolean true
    local boolTrue = json.encode(true)
    framework.assert(boolTrue == "true", "encode true should be 'true'")

    -- Boolean false
    local boolFalse = json.encode(false)
    framework.assert(boolFalse == "false", "encode false should be 'false'")

    -- Nil
    local nilVal = json.encode(nil)
    framework.assert(nilVal == "null", "encode nil should be 'null'")
end)

framework.test("json.encode arrays", function()
    -- Simple array
    local arr = {1, 2, 3, 4, 5}
    local encoded = json.encode(arr)
    framework.assert(type(encoded) == "string", "encode array should return string")
    framework.assert(encoded:find("%[") and encoded:find("%]"), "encoded array should have brackets")

    -- Empty array (note: Lua empty table may encode as {} not [])
    local emptyArr = {}
    local encodedEmpty = json.encode(emptyArr)
    -- Empty table can encode as {} or [] depending on implementation
    framework.assert(type(encodedEmpty) == "string", "empty array should encode")

    -- Array with strings
    local strArr = {"a", "b", "c"}
    local encodedStr = json.encode(strArr)
    framework.assert(type(encodedStr) == "string", "string array should encode")

    -- Array with mixed types
    local mixedArr = {1, "two", true, nil, 3.14}
    local encodedMixed = json.encode(mixedArr)
    framework.assert(type(encodedMixed) == "string", "mixed array should encode")
end)

framework.test("json.encode objects (tables with string keys)", function()
    -- Simple object
    local obj = {name = "John", age = 30}
    local encoded = json.encode(obj)
    framework.assert(type(encoded) == "string", "encode object should return string")
    framework.assert(encoded:find("%{") and encoded:find("%}"), "encoded object should have braces")
    framework.assert(encoded:find('"name"'), "encoded object should have quoted keys")

    -- Empty object
    local emptyObj = {}
    -- Note: empty table might encode as [] or {} depending on implementation
    local encodedEmpty = json.encode(emptyObj)
    framework.assert(type(encodedEmpty) == "string", "empty object should encode")

    -- Nested object
    local nested = {
        user = {
            name = "Alice",
            stats = {score = 100, level = 5}
        }
    }
    local encodedNested = json.encode(nested)
    framework.assert(type(encodedNested) == "string", "nested object should encode")
end)

framework.test("json.encode_pretty", function()
    local obj = {name = "Test", values = {1, 2, 3}}

    local pretty = json.encode_pretty(obj)
    framework.assert(type(pretty) == "string", "encode_pretty should return string")

    -- Pretty should have newlines and indentation
    framework.assert(pretty:find("\n"), "pretty output should have newlines")

    -- Pretty should be valid JSON (decode back)
    local decoded = json.decode(pretty)
    framework.assert(type(decoded) == "table", "pretty JSON should be decodable")
end)

framework.test("json.decode basic types", function()
    -- String
    local str = json.decode('"hello"')
    framework.assert(type(str) == "string", "decode string should return string")
    framework.assert(str == "hello", "decoded string should match")

    -- Number (integer)
    local num = json.decode("42")
    framework.assert(type(num) == "number", "decode number should return number")
    framework.assert(num == 42, "decoded number should match")

    -- Number (float)
    local float = json.decode("3.14")
    framework.assert(type(float) == "number", "decode float should return number")
    framework.assert(math.abs(float - 3.14) < 0.0001, "decoded float should match")

    -- Boolean true
    local boolTrue = json.decode("true")
    framework.assert(boolTrue == true, "decode true should return true")

    -- Boolean false
    local boolFalse = json.decode("false")
    framework.assert(boolFalse == false, "decode false should return false")

    -- Null
    local nilVal = json.decode("null")
    framework.assert(nilVal == nil, "decode null should return nil")
end)

framework.test("json.decode arrays", function()
    -- Simple array
    local arr = json.decode("[1, 2, 3, 4, 5]")
    framework.assert(type(arr) == "table", "decode array should return table")
    framework.assert(#arr == 5, "decoded array should have correct length")
    framework.assert(arr[1] == 1, "first element should match")
    framework.assert(arr[5] == 5, "last element should match")

    -- Empty array
    local emptyArr = json.decode("[]")
    framework.assert(type(emptyArr) == "table", "decode empty array should return table")
    framework.assert(#emptyArr == 0, "decoded empty array should have length 0")

    -- Array with strings
    local strArr = json.decode('["a", "b", "c"]')
    framework.assert(type(strArr) == "table", "decode string array should return table")
    framework.assert(strArr[1] == "a", "string element should match")

    -- Array with mixed types
    local mixedArr = json.decode('[1, "two", true, null, 3.14]')
    framework.assert(type(mixedArr) == "table", "decode mixed array should return table")
    framework.assert(mixedArr[1] == 1, "number element should match")
    framework.assert(mixedArr[2] == "two", "string element should match")
    framework.assert(mixedArr[3] == true, "bool element should match")
    framework.assert(mixedArr[4] == nil, "null element should be nil")
end)

framework.test("json.decode objects", function()
    -- Simple object
    local obj = json.decode('{"name": "John", "age": 30}')
    framework.assert(type(obj) == "table", "decode object should return table")
    framework.assert(obj.name == "John", "string value should match")
    framework.assert(obj.age == 30, "number value should match")

    -- Nested object
    local nested = json.decode('{"user": {"name": "Alice", "stats": {"score": 100}}}')
    framework.assert(type(nested) == "table", "decode nested object should return table")
    framework.assert(type(nested.user) == "table", "nested object should be table")
    framework.assert(nested.user.name == "Alice", "nested value should match")
    framework.assert(nested.user.stats.score == 100, "deeply nested value should match")
end)

framework.test("json roundtrip - basic types", function()
    -- String
    local originalStr = "hello world"
    local encodedStr = json.encode(originalStr)
    local decodedStr = json.decode(encodedStr)
    framework.assert(decodedStr == originalStr, "string roundtrip should match")

    -- Number
    local originalNum = 12345
    local encodedNum = json.encode(originalNum)
    local decodedNum = json.decode(encodedNum)
    framework.assert(decodedNum == originalNum, "number roundtrip should match")

    -- Float
    local originalFloat = 3.14159
    local encodedFloat = json.encode(originalFloat)
    local decodedFloat = json.decode(encodedFloat)
    framework.assert(math.abs(decodedFloat - originalFloat) < 0.0001, "float roundtrip should match")

    -- Boolean
    local originalBool = true
    local encodedBool = json.encode(originalBool)
    local decodedBool = json.decode(encodedBool)
    framework.assert(decodedBool == originalBool, "boolean roundtrip should match")
end)

framework.test("json roundtrip - arrays", function()
    local originalArr = {1, 2, 3, "four", true, nil}
    local encodedArr = json.encode(originalArr)
    local decodedArr = json.decode(encodedArr)

    framework.assert(type(decodedArr) == "table", "decoded array should be table")
    framework.assert(#decodedArr == #originalArr, "decoded array length should match")
    framework.assert(decodedArr[1] == 1, "first element should match")
    framework.assert(decodedArr[4] == "four", "string element should match")
    framework.assert(decodedArr[5] == true, "bool element should match")
end)

framework.test("json roundtrip - objects", function()
    local originalObj = {
        name = "Test",
        count = 42,
        active = true,
        nested = {
            x = 10,
            y = 20
        }
    }

    local encodedObj = json.encode(originalObj)
    local decodedObj = json.decode(encodedObj)

    framework.assert(type(decodedObj) == "table", "decoded object should be table")
    framework.assert(decodedObj.name == originalObj.name, "string field should match")
    framework.assert(decodedObj.count == originalObj.count, "number field should match")
    framework.assert(decodedObj.active == originalObj.active, "bool field should match")
    framework.assert(type(decodedObj.nested) == "table", "nested object should be table")
    framework.assert(decodedObj.nested.x == originalObj.nested.x, "nested field should match")
end)

framework.test("json.decode edge cases", function()
    -- Invalid JSON (should return nil or handle gracefully)
    local invalid1 = json.decode("not valid json")
    framework.assert(invalid1 == nil, "invalid JSON should return nil")

    -- Empty string
    local empty = json.decode("")
    framework.assert(empty == nil, "empty string should return nil")

    -- Malformed JSON
    local malformed = json.decode('{"key": }')
    framework.assert(malformed == nil, "malformed JSON should return nil")

    -- Unclosed bracket
    local unclosed = json.decode('[1, 2, 3')
    framework.assert(unclosed == nil, "unclosed array should return nil")

    -- Unclosed brace
    local unclosedObj = json.decode('{"key": "value"')
    framework.assert(unclosedObj == nil, "unclosed object should return nil")
end)

framework.test("json special characters", function()
    -- String with quotes
    local withQuotes = 'hello "world"'
    local encodedQuotes = json.encode(withQuotes)
    local decodedQuotes = json.decode(encodedQuotes)
    framework.assert(decodedQuotes == withQuotes, "string with quotes should roundtrip")

    -- String with backslash
    local withBackslash = "path\\to\\file"
    local encodedBackslash = json.encode(withBackslash)
    local decodedBackslash = json.decode(encodedBackslash)
    framework.assert(decodedBackslash == withBackslash, "string with backslash should roundtrip")

    -- String with newline
    local withNewline = "line1\nline2"
    local encodedNewline = json.encode(withNewline)
    local decodedNewline = json.decode(encodedNewline)
    framework.assert(decodedNewline == withNewline, "string with newline should roundtrip")

    -- String with tab
    local withTab = "col1\tcol2"
    local encodedTab = json.encode(withTab)
    local decodedTab = json.decode(encodedTab)
    framework.assert(decodedTab == withTab, "string with tab should roundtrip")
end)

framework.test("json unicode characters", function()
    -- Unicode string
    local unicode = "Hello 世界 🌍"
    local encoded = json.encode(unicode)
    local decoded = json.decode(encoded)
    framework.assert(decoded == unicode, "unicode string should roundtrip")
end)

framework.test("json large numbers", function()
    -- Large integer
    local largeInt = 9007199254740991 -- Max safe integer in JS
    local encoded = json.encode(largeInt)
    local decoded = json.decode(encoded)
    framework.assert(type(decoded) == "number", "large number should decode")

    -- Negative number
    local negNum = -12345
    local encodedNeg = json.encode(negNum)
    local decodedNeg = json.decode(encodedNeg)
    framework.assert(decodedNeg == negNum, "negative number should roundtrip")

    -- Very small float
    local smallFloat = 0.0000001
    local encodedSmall = json.encode(smallFloat)
    local decodedSmall = json.decode(encodedSmall)
    framework.assert(type(decodedSmall) == "number", "small float should decode")
end)

framework.test("json complex nested structure", function()
    local complex = {
        users = {
            {id = 1, name = "Alice", roles = {"admin", "user"}},
            {id = 2, name = "Bob", roles = {"user"}}
        },
        settings = {
            theme = "dark",
            notifications = {
                email = true,
                push = false
            }
        },
        metadata = {
            version = "1.0.0",
            tags = {"test", "json", "complex"}
        }
    }

    local encoded = json.encode(complex)
    framework.assert(type(encoded) == "string", "complex structure should encode")

    local decoded = json.decode(encoded)
    framework.assert(type(decoded) == "table", "complex structure should decode")
    framework.assert(type(decoded.users) == "table", "users array should exist")
    framework.assert(#decoded.users == 2, "users array should have 2 elements")
    framework.assert(decoded.users[1].name == "Alice", "nested value should match")
    framework.assert(type(decoded.users[1].roles) == "table", "nested array should exist")
    framework.assert(decoded.settings.theme == "dark", "deeply nested value should match")
end)

framework.test("json pretty print structure", function()
    local obj = {a = 1, b = {c = 2, d = 3}}

    local pretty = json.encode_pretty(obj)

    -- Should have indentation (4 spaces as per C++ code)
    framework.assert(pretty:find("    "), "pretty should use 4-space indentation")

    -- Should be parseable
    local decoded = json.decode(pretty)
    framework.assert(decoded.a == 1, "pretty decoded value should match")
    framework.assert(decoded.b.c == 2, "pretty decoded nested value should match")
end)

framework.summary()
