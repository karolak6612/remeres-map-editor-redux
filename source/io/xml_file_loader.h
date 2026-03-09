#ifndef RME_XML_FILE_LOADER_H_
#define RME_XML_FILE_LOADER_H_

#include "app/main.h"

#include <functional>

namespace XmlFileLoader {
	using XmlChildVisitor = std::function<bool(const FileName&, pugi::xml_node, wxString&, std::vector<std::string>&)>;

	[[nodiscard]] FileName resolveRelative(const FileName& source_file, const wxString& relative_path);
	[[nodiscard]] bool visitElements(const FileName& entry_file, const char* root_name, const XmlChildVisitor& visitor, wxString& error, std::vector<std::string>& warnings);
}

#endif
