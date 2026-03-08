#ifndef RME_MAP_LOAD_OPTIONS_H_
#define RME_MAP_LOAD_OPTIONS_H_

#include <string>

struct MapLoadOptions {
	std::string selected_client_id;
	bool force_client_mismatch = false;

	[[nodiscard]] bool hasSelectedClient() const {
		return !selected_client_id.empty();
	}
};

#endif
