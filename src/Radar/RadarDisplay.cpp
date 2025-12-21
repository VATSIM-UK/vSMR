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
            // Get radar target position and heading
            EuroScopePlugIn::CPosition radarPos =
                radarTarget.GetPosition().GetPosition();
            double heading = radarTarget.GetTrackHeading();

            // Get aircraft type
            std::string aircraftType =
                flightPlan.GetFlightPlanData().GetAircraftFPType();

            // Get aircraft dimensions
            const AircraftData * data =
                aircraftRenderer->GetAircraftData(aircraftType);

            double wingspan  = 34.0; // Default Light Aircraft
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
                // Fallback to WTC category
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

            // Generate aircraft outline in lat/lon
            auto outlinePositions = GenerateAircraftOutline(
                radarPos, heading, wingspan, length, gearWidth);

            // Convert to screen coordinates
            std::vector<POINT> screenPoints;
            for (const auto & pos : outlinePositions)
            {
                POINT screenPos = ConvertCoordFromPositionToPixel(pos);
                screenPoints.push_back(screenPos);
            }

            // Draw aircraft outline
            if (!screenPoints.empty())
            {
                HBRUSH yellowBrush = CreateSolidBrush(RGB(255, 255, 0));
                HPEN yellowPen     = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));

                HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, yellowBrush);
                HPEN oldPen     = (HPEN)SelectObject(hDC, yellowPen);

                Polygon(hDC, screenPoints.data(),
                        static_cast<int>(screenPoints.size()));

                SelectObject(hDC, oldBrush);
                SelectObject(hDC, oldPen);

                DeleteObject(yellowBrush);
                DeleteObject(yellowPen);
            }

            // Draw center correlation symbol
            POINT centerPx = ConvertCoordFromPositionToPixel(radarPos);
            DrawCorrelationSymbol(hDC, centerPx, radarTarget);
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

std::vector<EuroScopePlugIn::CPosition> RadarDisplay::GenerateAircraftOutline(
    const EuroScopePlugIn::CPosition & aircraftPos,
    double heading,
    double wingspan,
    double length,
    double gearWidth) const
{
    std::vector<EuroScopePlugIn::CPosition> outline;

    // Calculate heading vectors
    double trackHead   = fmod(heading, 360.0);
    double inverseHead = fmod(trackHead + 180.0, 360.0);
    double leftHead    = fmod(trackHead - 90.0, 360.0);
    double rightHead   = fmod(trackHead + 90.0, 360.0);

    // Half dimensions
    double halfLength     = length / 2.0;
    double halfCabinWidth = gearWidth / 4.0; // Fuselage narrower than gear
    double halfWingspan   = wingspan / 2.0;

    // Build 12 base points forming aircraft outline
    EuroScopePlugIn::CPosition topMiddle =
        angleUtils::haversine(aircraftPos, trackHead, halfLength);
    EuroScopePlugIn::CPosition topLeft =
        angleUtils::haversine(topMiddle, leftHead, halfCabinWidth);
    EuroScopePlugIn::CPosition topRight =
        angleUtils::haversine(topMiddle, rightHead, halfCabinWidth);

    EuroScopePlugIn::CPosition bottomMiddle =
        angleUtils::haversine(aircraftPos, inverseHead, halfLength);
    EuroScopePlugIn::CPosition bottomLeft =
        angleUtils::haversine(bottomMiddle, leftHead, halfCabinWidth);
    EuroScopePlugIn::CPosition bottomRight =
        angleUtils::haversine(bottomMiddle, rightHead, halfCabinWidth);

    // Wing points
    EuroScopePlugIn::CPosition wingLeftFront = angleUtils::haversine(
        topLeft, fmod(inverseHead + 25.0, 360.0), 0.8 * halfLength);
    EuroScopePlugIn::CPosition wingRightFront = angleUtils::haversine(
        topRight, fmod(inverseHead - 25.0, 360.0), 0.8 * halfLength);
    EuroScopePlugIn::CPosition wingLeftBack = angleUtils::haversine(
        bottomLeft, fmod(trackHead - 15.0, 360.0), 0.8 * halfLength);
    EuroScopePlugIn::CPosition wingRightBack = angleUtils::haversine(
        bottomRight, fmod(trackHead + 15.0, 360.0), 0.8 * halfLength);

    // Wing tips
    EuroScopePlugIn::CPosition leftWingTip =
        angleUtils::haversine(wingLeftBack, leftHead, 0.7 * halfWingspan);
    EuroScopePlugIn::CPosition rightWingTip =
        angleUtils::haversine(wingRightBack, rightHead, 0.7 * halfWingspan);

    // Base outline points in order
    EuroScopePlugIn::CPosition basePoints[12] = {
        topLeft,
        wingLeftFront,
        leftWingTip,
        angleUtils::haversine(leftWingTip, inverseHead, gearWidth / 2.0),
        wingLeftBack,
        bottomLeft,
        bottomRight,
        wingRightBack,
        angleUtils::haversine(rightWingTip, inverseHead, gearWidth / 2.0),
        rightWingTip,
        wingRightFront,
        topRight};

    // Interpolate between base points for smoother outline
    for (int i = 0; i < 12; ++i)
    {
        EuroScopePlugIn::CPosition startPoint = basePoints[i];
        EuroScopePlugIn::CPosition endPoint   = basePoints[(i + 1) % 12];

        outline.push_back(startPoint);

        // Linear interpolation between points
        for (int k = 1; k < 6; ++k)
        {
            double ratio = k / 6.0;
            EuroScopePlugIn::CPosition midPoint;
            midPoint.m_Latitude = startPoint.m_Latitude +
                ratio * (endPoint.m_Latitude - startPoint.m_Latitude);
            midPoint.m_Longitude = startPoint.m_Longitude +
                ratio * (endPoint.m_Longitude - startPoint.m_Longitude);
            outline.push_back(midPoint);
        }
    }

    return outline;
}

void RadarDisplay::DrawCorrelationSymbol(
    HDC hDC,
    const POINT & centerPos,
    const EuroScopePlugIn::CRadarTarget & radarTarget)
{
    HPEN symbolPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
    HPEN oldPen    = (HPEN)SelectObject(hDC, symbolPen);

    if (radarTarget.GetCorrelatedFlightPlan().IsValid())
    {
        // Correlated: Draw diamond
        MoveToEx(hDC, centerPos.x, centerPos.y - 6, nullptr);
        LineTo(hDC, centerPos.x - 6, centerPos.y);
        LineTo(hDC, centerPos.x, centerPos.y + 6);
        LineTo(hDC, centerPos.x + 6, centerPos.y);
        LineTo(hDC, centerPos.x, centerPos.y - 6);
    }
    else
    {
        // No Mode C: Draw asterisk
        MoveToEx(hDC, centerPos.x, centerPos.y, nullptr);
        LineTo(hDC, centerPos.x - 4, centerPos.y - 4);

        MoveToEx(hDC, centerPos.x, centerPos.y, nullptr);
        LineTo(hDC, centerPos.x + 4, centerPos.y - 4);

        MoveToEx(hDC, centerPos.x, centerPos.y, nullptr);
        LineTo(hDC, centerPos.x - 4, centerPos.y + 4);

        MoveToEx(hDC, centerPos.x, centerPos.y, nullptr);
        LineTo(hDC, centerPos.x + 4, centerPos.y + 4);
    }

    SelectObject(hDC, oldPen);
    DeleteObject(symbolPen);
}