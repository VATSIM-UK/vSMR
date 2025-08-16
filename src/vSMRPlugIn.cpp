#include "vSMRPlugIn.hpp"
#include "Version.h"

namespace vSMRPlugIn
{
    vSMRPlugIn::vSMRPlugIn() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_DEVELOPERS, PLUGIN_COPYRIGHT)
    {
        DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Init");
    }
    vSMRPlugIn::~vSMRPlugIn()
    {

    }
    void vSMRPlugIn::DisplayMessage(const std::string &message, const std::string &sender)
    {
        DisplayUserMessage(PLUGIN_NAME, sender.c_str(), message.c_str(), true, false, false, false, false);
    }
}