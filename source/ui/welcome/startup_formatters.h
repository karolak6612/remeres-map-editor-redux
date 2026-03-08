#ifndef RME_STARTUP_FORMATTERS_H_
#define RME_STARTUP_FORMATTERS_H_

#include <vector>

#include <wx/colour.h>

#include "io/iomap_otbm.h"
#include "ui/welcome/startup_types.h"

class ClientVersion;

wxColour StartupStatusColour(StartupCompatibilityStatus status);
std::vector<StartupInfoField> BuildStartupMapFields(const OTBMStartupPeekResult* info);
std::vector<StartupInfoField> BuildStartupClientFields(ClientVersion* client);

#endif
