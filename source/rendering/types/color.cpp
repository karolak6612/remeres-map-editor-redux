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

#include "color.h"
#include <wx/colour.h>

namespace rme {
	namespace render {

		Color::Color(const wxColour& wx) : r(wx.Red()), g(wx.Green()), b(wx.Blue()), a(wx.Alpha()) {
		}

		wxColour Color::toWx() const {
			return wxColour(r, g, b, a);
		}

	} // namespace render
} // namespace rme
