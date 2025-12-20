#include "Version.h"
#include "vSMRPlugIn.hpp"
#include "Identifiers.hpp"
#include "Logger.hpp"

#include "stringUtils.hpp"
#include "pathUtils.hpp"

#include <windows.h>

vSMRPlugIn::vSMRPlugIn() : EuroScopePlugIn::CPlugIn(
                               EuroScopePlugIn::COMPATIBILITY_CODE,
                               PLUGIN_NAME,
                               PLUGIN_VERSION,
                               PLUGIN_DEVELOPERS,
                               PLUGIN_COPYRIGHT)
{

    try
    {
        auto logPath = getDllPath() / "vSMR.log";
        Logger::initialise(logPath);
        Logger::getInstance().info("vSMR Plugin Initialised - Version " + std::string(PLUGIN_VERSION));
    }
    catch (const std::exception &e)
    {
        DisplayMessage("Failed to initialise vSMR logger: " + std::string(e.what()), "Error");
    }

    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    RegisterDisplayType(PLUGIN_VIEW_AVISO, false, true, true, true);

    RegisterTagItemType("Datalink clearance", TAG_ITEM_DATALINK_STATUS);
    RegisterTagItemFunction("Datalink menu", TAG_FUNCTION_DATALINK_MENU);
}

vSMRPlugIn::~vSMRPlugIn()
{
}

void vSMRPlugIn::DisplayMessage(const std::string &message, const std::string &sender)
{
    DisplayUserMessage(PLUGIN_NAME, sender.c_str(), message.c_str(), true, false, false, false, false);
}

bool vSMRPlugIn::OnCompileCommand(const char *sCommandLine)
{
    if (startsWith(".smr hello", sCommandLine))
    {
        Logger::getInstance().info("Called Hello command");
        DisplayUserMessage("vSMR", "vSMR", "Hello!", true, true, false, true, false);
        return true;
    }

    return false;
}
