//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "lua_api_color.h"
#include <algorithm>

namespace LuaAPI {

	struct ColorStruct {
		int r, g, b, a;

		ColorStruct(int r = 0, int g = 0, int b = 0, int a = 255) :
			r(r), g(g), b(b), a(a) { }
	};

	void registerColor(sol::state& lua) {
		sol::table Color = lua.create_named_table("Color");

		// Register the struct as a usertype, but bind it to the table's metatable or constructor
		// Actually, sol2 makes this cleaner. We can register the type, then attach static methods.

		sol::usertype<ColorStruct> colorType = lua.new_usertype<ColorStruct>(
			"ColorInstance", // Internal name, we'll expose it as 'Color' via the table manually
			sol::constructors<ColorStruct(int, int, int), ColorStruct(int, int, int, int)>(),
			"r", &ColorStruct::r,
			"g", &ColorStruct::g,
			"b", &ColorStruct::b,
			"a", &ColorStruct::a
		);

		// Make the table callable as a constructor
		Color[sol::meta_function::call] = [](sol::table self, int r, int g, int b, sol::optional<int> a) {
			return ColorStruct(r, g, b, a.value_or(255));
		};

		// Helper to create a color struct
		auto mkColor = [](int r, int g, int b) {
			return ColorStruct(r, g, b);
		};

		// Static methods (restored)
		Color["rgb"] = mkColor;

		Color["hex"] = [](const std::string& hex) {
			unsigned long value = 0;
			std::string h = (!hex.empty() && hex[0] == '#') ? hex.substr(1) : hex;
			if (h.length() == 3) {
				h = { h[0], h[0], h[1], h[1], h[2], h[2] };
			}
			try {
				value = std::stoul(h, nullptr, 16);
			} catch (...) {
				value = 0;
			}
			return ColorStruct((int)((value >> 16) & 0xFF), (int)((value >> 8) & 0xFF), (int)(value & 0xFF));
		};

		Color["lighten"] = [](const ColorStruct& c, int percent) {
			wxColour wx(c.r, c.g, c.b);
			wxColour result = wx.ChangeLightness(100 + percent);
			return ColorStruct(result.Red(), result.Green(), result.Blue(), c.a);
		};

		Color["darken"] = [](const ColorStruct& c, int percent) {
			wxColour wx(c.r, c.g, c.b);
			wxColour result = wx.ChangeLightness(100 - percent);
			return ColorStruct(result.Red(), result.Green(), result.Blue(), c.a);
		};

		// Predefined colors
		Color["white"] = mkColor(255, 255, 255);
		Color["black"] = mkColor(0, 0, 0);
		Color["blue"] = mkColor(49, 130, 206);
		Color["red"] = mkColor(220, 53, 69);
		Color["green"] = mkColor(40, 167, 69);
		Color["yellow"] = mkColor(255, 193, 7);
		Color["orange"] = mkColor(253, 126, 20);
		Color["gray"] = mkColor(128, 128, 128);
		Color["lightGray"] = mkColor(245, 247, 250);
		Color["darkGray"] = mkColor(45, 55, 72);
	}

} // namespace LuaAPI
