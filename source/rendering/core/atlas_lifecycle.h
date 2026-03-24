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

#ifndef RME_RENDERING_CORE_ATLAS_LIFECYCLE_H_
#define RME_RENDERING_CORE_ATLAS_LIFECYCLE_H_

#include <memory>

class AtlasManager;

class AtlasLifecycle {
public:
	AtlasManager* get() { return atlas_manager_.get(); }
	bool has() const;
	bool ensure();
	void clear();

private:
	std::unique_ptr<AtlasManager> atlas_manager_;
};

#endif
