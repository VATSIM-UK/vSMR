#include "vSMRPlugIn.hpp"
#include "EuroScopePlugIn.h"
#include "Logger.hpp"
#include "RadarDisplay.hpp"
#include "Version.h"

#include "pathUtils.hpp"
#include "stringUtils.hpp"

#include <cstring>
#include <windows.h>

vSMRPlugIn::vSMRPlugIn()
    : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
                               PLUGIN_NAME,
                               PLUGIN_VERSION,
                               PLUGIN_DEVELOPERS,
                               PLUGIN_COPYRIGHT)
{

    try
    {
        auto logPath = pathUtils::getDllPath() / "vSMR.log";
        Logger::initialise(logPath);
        Logger::getInstance().info("vSMR Plugin Initialised - Version " +
                                   std::string(PLUGIN_VERSION));
    }
    catch (const std::exception & e)
    {
        DisplayMessage("Failed to initialise vSMR logger: " +
                           std::string(e.what()),
                       "Error");
    }
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded",
                   "Initialisation");

    // Register Displays with Euroscope
    RegisterDisplayType(PLUGIN_DISPLAY_NAME, false, true, true, true);

    // Register Tags with Euroscope
}

vSMRPlugIn::~vSMRPlugIn() = default;

void vSMRPlugIn::DisplayMessage(const std::string & message,
                                const std::string & sender)
{
    DisplayUserMessage(PLUGIN_NAME, sender.c_str(), message.c_str(), true,
                       false, false, false, false);
}
EuroScopePlugIn::CRadarScreen *
vSMRPlugIn::OnRadarScreenCreated(const char * displayName,
                                 bool needRadarContent,
                                 bool geoReferenced,
                                 bool canBeSaved,
                                 bool canBeCreated)
{
    // Check to see if we are creating a vSMR display type
    if (strcmp(displayName, PLUGIN_DISPLAY_NAME) == 0)
    {
        Logger::getInstance().info("Created display: " +
                                   std::string(displayName));
        // Create new Radar Screen and add to vector
        auto radarScreen = std::make_unique<RadarDisplay>();
        auto * screenPtr = radarScreen.get();
        radarScreens.push_back(std::move(radarScreen));
        return screenPtr;
    }
    return nullptr;
}

bool vSMRPlugIn::OnCompileCommand(const char * sCommandLine)
{
    if (stringUtils::startsWith(".smr hello", sCommandLine))
    {
        Logger::getInstance().info("Called Hello command");
        DisplayUserMessage("vSMR", "vSMR", "Hello!", true, true, false, true,
                           false);
        return true;
    }

    return false;
}

void vSMRPlugIn::OnTimer(int counter)
{
    // TODO: Implement timer functionality
}

void vSMRPlugIn::OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan flightPlan)
{
    // TODO: Implement flight plan disconnect handling
}

void vSMRPlugIn::OnGetTagItem(EuroScopePlugIn::CFlightPlan flightPlan,
                              EuroScopePlugIn::CRadarTarget radarTarget,
                              int itemCode,
                              int tagData,
                              char itemString[16],
                              int * colourCode,
                              COLORREF * pRGB,
                              double * fontSize)
{
    // TODO: Implement tag item handling
}

void vSMRPlugIn::OnFunctionCall(int functionId,
                                const char * itemString,
                                POINT pt,
                                RECT area)
{
    // TODO: Implement function call handling
}
