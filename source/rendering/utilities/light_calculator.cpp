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
#include "rendering/utilities/light_calculator.h"
#include <cmath>
#include <algorithm>

float LightCalculator::calculateIntensity(int map_x, int map_y, int light_map_x, int light_map_y, uint8_t light_intensity) {
	int dx = map_x - light_map_x;
	int dy = map_y - light_map_y;
	float distance = std::sqrt(dx * dx + dy * dy);
	if (distance > MaxLightIntensity) {
		return 0.f;
	}
	float calc_intensity = (-distance + light_intensity) * 0.2f;
	if (calc_intensity < 0.01f) {
		return 0.f;
	}
	return std::min(calc_intensity, 1.f);
}
