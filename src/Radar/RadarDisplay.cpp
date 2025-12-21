#include "RadarDisplay.hpp"
#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)
#include "Logger.hpp"
#include "pathUtils.hpp"

#include <filesystem>

RadarDisplay::RadarDisplay()
    : aircraftRenderer(std::make_unique<AircraftRenderer>())
{
    // Load aircraft data
    std::filesystem::path dataPath =
        pathUtils::getDllPath() / "data" / "aircraft-data.csv";
    if (!aircraftRenderer->LoadAircraftData(dataPath))
    {
        Logger::getInstance().warning("Failed to load aircraft data from: " +
                                      dataPath.string());
    }
}

RadarDisplay::~RadarDisplay()
{
}

void RadarDisplay::OnAsrContentToBeClosed()
{
    // TODO: Implement ASR content closure handling
}

void RadarDisplay::OnRefresh(HDC hDC, int phase)
{
    // Draw all aircraft
    EuroScopePlugIn::CFlightPlan flightPlan =
        GetPlugIn()->FlightPlanSelectFirst();
    while (flightPlan.IsValid())
    {
        EuroScopePlugIn::CRadarTarget radarTarget =
            GetPlugIn()->RadarTargetSelect(flightPlan.GetCallsign());

        if (radarTarget.IsValid())
        {
            // Get radar target position
            EuroScopePlugIn::CPosition pos =
                radarTarget.GetPosition().GetPosition();
            POINT screenPos = ConvertCoordFromPositionToPixel(pos);

            // Get aircraft type from flight plan
            std::string aircraftType =
                flightPlan.GetFlightPlanData().GetAircraftFPType();

            // Draw the aircraft
            aircraftRenderer->DrawAircraft(hDC, screenPos.x, screenPos.y,
                                           aircraftType,
                                           radarTarget.GetTrackHeading(), 0.5);
        }

        flightPlan = GetPlugIn()->FlightPlanSelectNext(flightPlan);
    }
}

void RadarDisplay::OnRadarTargetPositionUpdate(
    EuroScopePlugIn::CRadarTarget radarTarget)
{
    if (!radarTarget.IsValid() || !radarTarget.GetPosition().IsValid())
    {
        return;
    }
}