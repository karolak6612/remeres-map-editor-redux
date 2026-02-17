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

/*
 * @file iomap.h
 * @brief Abstract interface for map file input/output.
 *
 * Defines the IOMap class which serves as the base for all map loading
 * and saving operations, handling versioning and error reporting.
 */

#ifndef RME_MAP_IO_H_
#define RME_MAP_IO_H_

#include "app/client_version.h"

/*
 * @brief Strategy for importing map data.
 */
enum ImportType {
	IMPORT_DONT, /* Do not import. */
	IMPORT_MERGE, /* Merge imported map into current map. */
	IMPORT_SMART_MERGE, /* Merge with conflict resolution. */
	IMPORT_INSERT, /* Insert imported map at specific coordinates. */
};

class Map;

/*
 * @brief Base class for map serialization (Load/Save).
 *
 * IOMap provides a uniform interface for reading and writing maps in various
 * formats (OTBM, OTMM, etc.). It manages warnings, errors, and user queries
 * during the I/O process.
 */
class IOMap {
protected:
	std::vector<std::string> warnings;
	wxString errorstr;

	bool queryUser(const wxString& title, const wxString& format);
	void warning(const wxString format, ...);
	void error(const wxString format, ...);

public:
	IOMap() {
		version.otbm = MAP_OTBM_1;
		version.client = CLIENT_VERSION_NONE;
	}
	virtual ~IOMap() = default;

	MapVersion version;

	/*
	 * @brief Gets the list of warnings generated during I/O.
	 * @return Vector of warning strings.
	 */
	std::vector<std::string>& getWarnings() {
		return warnings;
	}

	/*
	 * @brief Gets the last error message.
	 * @return Error string.
	 */
	wxString& getError() {
		return errorstr;
	}

	/*
	 * @brief Loads a map from a file.
	 * @param map The target Map object to populate.
	 * @param identifier The file path or identifier.
	 * @return true if successful.
	 */
	virtual bool loadMap(Map& map, const FileName& identifier) = 0;

	/*
	 * @brief Saves a map to a file.
	 * @param map The source Map object.
	 * @param identifier The target file path.
	 * @return true if successful.
	 */
	virtual bool saveMap(Map& map, const FileName& identifier) = 0;
};

/*
 * @brief Null implementation of IOMap for testing or default initialization.
 *
 * Always fails to load or save.
 */
class VirtualIOMap : public IOMap {
public:
	VirtualIOMap(MapVersion v) {
		version = v;
	}

	bool loadMap(Map& map, const FileName& identifier) override {
		return false;
	}
	bool saveMap(Map& map, const FileName& identifier) override {
		return false;
	}
};

#endif
