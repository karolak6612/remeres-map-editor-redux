//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "lua_api_json.h"
#include "util/json.h"

namespace LuaAPI {

	// Forward declarations
	sol::object jsonToLua(const nlohmann::json& j, sol::state_view& lua);
	nlohmann::json luaToJson(const sol::object& obj);

	// Convert nlohmann::json to Lua Object
	sol::object jsonToLua(const nlohmann::json& j, sol::state_view& lua) {
		switch (j.type()) {
			case nlohmann::json::value_t::null:
				return sol::nil;
			case nlohmann::json::value_t::boolean:
				return sol::make_object(lua, j.get<bool>());
			case nlohmann::json::value_t::number_integer:
			case nlohmann::json::value_t::number_unsigned:
				return sol::make_object(lua, j.get<int64_t>());
			case nlohmann::json::value_t::number_float:
				return sol::make_object(lua, j.get<double>());
			case nlohmann::json::value_t::string:
				return sol::make_object(lua, j.get<std::string>());
			case nlohmann::json::value_t::array: {
				sol::table t = lua.create_table();
				for (size_t i = 0; i < j.size(); ++i) {
					t[i + 1] = jsonToLua(j[i], lua); // Lua 1-based indexing
				}
				return t;
			}
			case nlohmann::json::value_t::object: {
				sol::table t = lua.create_table();
				for (auto& element : j.items()) {
					t[element.key()] = jsonToLua(element.value(), lua);
				}
				return t;
			}
			default:
				return sol::nil;
		}
	}

	// Convert Lua Object to nlohmann::json
	nlohmann::json luaToJson(const sol::object& obj) {
		switch (obj.get_type()) {
			case sol::type::nil:
				return nlohmann::json(nullptr);
			case sol::type::boolean:
				return nlohmann::json(obj.as<bool>());
			case sol::type::number: {
				double d = obj.as<double>();
				if (d == (int64_t)d) {
					return nlohmann::json((int64_t)d);
				}
				return nlohmann::json(d);
			}
			case sol::type::string:
				return nlohmann::json(obj.as<std::string>());
			case sol::type::table: {
				sol::table t = obj.as<sol::table>();

				// Heuristic to detect array vs object
				bool isArray = true;
				size_t maxKey = 0;
				size_t count = 0;

				for (auto& pair : t) {
					count++;
					if (pair.first.get_type() == sol::type::number) {
						double k = pair.first.as<double>();
						if (k >= 1 && k == (size_t)k) {
							size_t idx = (size_t)k;
							if (idx > maxKey) {
								maxKey = idx;
							}
						} else {
							isArray = false;
							break;
						}
					} else {
						isArray = false;
						break;
					}
				}

				if (isArray && count > 0) {
					if (maxKey != count) {
						// Sparse array, treat as object or just accept sparse?
						// nlohmann::json array resizing fills with nulls?
						// Safer to treat as object if sparse to avoid huge null-filled arrays
						// But for simple cases, let's treat strict 1..N as array.
						isArray = false;
					}
				}

				if (count == 0) {
					isArray = false; // Empty table -> empty object {}
				}

				if (isArray) {
					nlohmann::json j = nlohmann::json::array();
					for (size_t i = 1; i <= count; ++i) {
						if (t[i].valid()) {
							j.push_back(luaToJson(t[i]));
						} else {
							j.push_back(nullptr);
						}
					}
					return j;
				} else {
					nlohmann::json j = nlohmann::json::object();
					for (auto& pair : t) {
						std::string key;
						if (pair.first.get_type() == sol::type::string) {
							key = pair.first.as<std::string>();
						} else if (pair.first.get_type() == sol::type::number) {
							key = std::to_string(pair.first.as<int64_t>());
						} else {
							continue;
						}
						j[key] = luaToJson(pair.second);
					}
					return j;
				}
			}
			default:
				return nlohmann::json(nullptr);
		}
	}

	void registerJson(sol::state& lua) {
		// Create "json" table
		sol::table jsonTable = lua.create_table();

		jsonTable.set_function("encode", [](sol::object obj, sol::this_state s) -> std::string {
			nlohmann::json j = luaToJson(obj);
			return j.dump(); // minified
		});

		jsonTable.set_function("encode_pretty", [](sol::object obj, sol::this_state s) -> std::string {
			nlohmann::json j = luaToJson(obj);
			return j.dump(4); // pretty-printed with 4-space indent
		});

		jsonTable.set_function("decode", [](std::string jsonStr, sol::this_state s) -> sol::object {
			try {
				nlohmann::json j = nlohmann::json::parse(jsonStr);
				sol::state_view lua(s);
				return jsonToLua(j, lua);
			} catch (const nlohmann::json::parse_error&) {
				return sol::nil;
			}
		});

		lua["json"] = jsonTable;
	}

} // namespace LuaAPI
