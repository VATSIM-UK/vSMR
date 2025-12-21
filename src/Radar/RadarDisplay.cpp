#include "RadarDisplay.hpp"
#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)
#include "Logger.hpp"
#include "angleUtils.hpp"
#include "pathUtils.hpp"

#include <cmath>
#include <filesystem>

RadarDisplay::RadarDisplay()
    : aircraftRenderer(std::make_unique<AircraftRenderer>())
{
    // Load aircraft data
    std::filesystem::path dataPath =
        pathUtils::getDllPath() / "aircraft-data.csv";
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
    // Only draw aircraft in the correct phase (before tags/lists)
    if (phase != EuroScopePlugIn::REFRESH_PHASE_BEFORE_TAGS) { return; }

    // Draw all aircraft with afterglow
    EuroScopePlugIn::CFlightPlan flightPlan =
        GetPlugIn()->FlightPlanSelectFirst();
    while (flightPlan.IsValid())
    {
        EuroScopePlugIn::CRadarTarget radarTarget =
            GetPlugIn()->RadarTargetSelect(flightPlan.GetCallsign());

        if (radarTarget.IsValid())
        {
            std::string callsign = radarTarget.GetCallsign();

            // Draw afterglow first (older positions)
            aircraftRenderer->DrawAircraftAfterGlow(
                hDC, callsign, [this](const EuroScopePlugIn::CPosition & pos)
                { return ConvertCoordFromPositionToPixel(pos); });

            // Get radar target position and heading
            EuroScopePlugIn::CPosition radarPos =
                radarTarget.GetPosition().GetPosition();
            double heading =
                radarTarget.GetPosition().GetReportedHeadingTrueNorth();

            // Get aircraft type
            std::string aircraftType =
                flightPlan.GetFlightPlanData().GetAircraftFPType();

            // Get aircraft dimensions
            const AircraftData * data =
                aircraftRenderer->GetAircraftData(aircraftType);

            double wingspan  = 34.0; // Default Med Aircraft
            double length    = 38.0;
            double gearWidth = 12.0;

            if (data)
            {
                wingspan  = data->wingspan;
                length    = data->length;
                gearWidth = data->gearWidth;
            }
            else
            {
                char wtc = flightPlan.GetFlightPlanData().GetAircraftWtc();
                if (wtc == 'H')
                {
                    wingspan  = 61.0;
                    length    = 64.0;
                    gearWidth = 14.0;
                }
                else if (wtc == 'J')
                {
                    wingspan  = 80.0;
                    length    = 73.0;
                    gearWidth = 14.0;
                }
            }

            aircraftRenderer->DrawAircraftShape(
                hDC, radarPos, heading, wingspan, length, gearWidth,
                radarTarget, [this](const EuroScopePlugIn::CPosition & pos)
                { return ConvertCoordFromPositionToPixel(pos); });
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

    std::string callsign = radarTarget.GetCallsign();
    EuroScopePlugIn::CPosition position =
        radarTarget.GetPosition().GetPosition();
    double heading = radarTarget.GetPosition().GetReportedHeadingTrueNorth();

    // Get flight plan for aircraft type
    EuroScopePlugIn::CFlightPlan fp =
        GetPlugIn()->FlightPlanSelect(callsign.c_str());

    std::string aircraftType = "";
    char wtc                 = 'M';

    if (fp.IsValid())
    {
        aircraftType = fp.GetFlightPlanData().GetAircraftFPType();
        wtc          = fp.GetFlightPlanData().GetAircraftWtc();
    }

    // Update aircraft shape in renderer with plain data
    aircraftRenderer->UpdateAircraftShape(callsign, position, heading,
                                          aircraftType, wtc);
}