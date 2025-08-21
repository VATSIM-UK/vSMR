#include "vSMRPlugIn.hpp"
#include "Version.h"

#include <cstring>

namespace vSMRPlugIn {

static bool startsWith(const char* prefix, const char* str) { // @TODO remove to a utils file/class??
    if (!prefix || !str) return false;
    const size_t n = std::strlen(prefix);
    return std::strncmp(str, prefix, n) == 0;
}

vSMRPlugIn::vSMRPlugIn() : EuroScopePlugIn::CPlugIn(
    EuroScopePlugIn::COMPATIBILITY_CODE,
    PLUGIN_NAME,
    PLUGIN_VERSION,
    PLUGIN_DEVELOPERS,
    PLUGIN_COPYRIGHT)
{
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
}

vSMRPlugIn::~vSMRPlugIn()
{
}

void vSMRPlugIn::DisplayMessage(const std::string &message, const std::string &sender)
{
    DisplayUserMessage(PLUGIN_NAME, sender.c_str(), message.c_str(), true, false, false, false, false);
}

bool vSMRPlugIn::OnCompileCommand(const char * sCommandLine)
{
    if (startsWith(".smr hello", sCommandLine))
    {
        DisplayUserMessage("vSMR", "vSMR", "Hello!", true, true, false, true, false);
        return true;
    }

    return false;
}

} // namespace vSMRPlugIn