#include "RadarDisplay.hpp"
#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)
#include "Identifiers.hpp"
#include "Logger.hpp"
#include "Tag.hpp"
#include "pathUtils.hpp"

#include <filesystem>

RadarDisplay::RadarDisplay()
    : aircraftRenderer(std::make_unique<AircraftRenderer>()),
      menuBar(std::make_unique<MenuBar>()),
      tagProfileManager(std::make_unique<TagProfileManager>())
{
    // Load aircraft data
    std::filesystem::path dataPath =
        pathUtils::getDllPath() / "aircraft-data.csv";
    if (!aircraftRenderer->LoadAircraftData(dataPath))
    {
        Logger::getInstance().warning("Failed to load aircraft data from: " +
                                      dataPath.string());
    }

    // Load tag profiles
    std::filesystem::path tagProfilePath =
        pathUtils::getDllPath() / "vSMR_Tags.json";
    if (!tagProfileManager->Initialize(tagProfilePath.string()))
    {
        Logger::getInstance().warning("Failed to load tag profiles from: " +
                                      tagProfilePath.string());
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
    // Draw menu bar in the correct phase (on top of everything)
    if (phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_LISTS)
    {
        RECT displayArea = GetRadarArea();
        menuBar->Draw(hDC, displayArea);
        return;
    }

    // Only draw aircraft in the correct phase (before tags/lists)
    if (phase != EuroScopePlugIn::REFRESH_PHASE_BEFORE_TAGS)
    {
        Logger::getInstance().debug("OnRefresh: Skipping non-tag phase");
        return;
    }

    Logger::getInstance().debug("OnRefresh: REFRESH_PHASE_BEFORE_TAGS - Drawing aircraft and tags");

    // Draw all aircraft with afterglow - iterate through radar targets, not flight plans!
    // This ensures we get both correlated AND uncorrelated targets
    EuroScopePlugIn::CRadarTarget radarTarget =
        GetPlugIn()->RadarTargetSelectFirst();
    while (radarTarget.IsValid())
    {
        if (radarTarget.IsValid())
        {
            EuroScopePlugIn::CFlightPlan flightPlan =
                GetPlugIn()->FlightPlanSelect(radarTarget.GetCallsign());
            std::string callsign = radarTarget.GetCallsign();

            // Get radar target position and heading
            EuroScopePlugIn::CPosition radarPos =
                radarTarget.GetPosition().GetPosition();
            double groundSpeed = radarTarget.GetPosition().GetReportedGS();

            // Draw afterglow first (older positions)
            aircraftRenderer->DrawAircraftAfterGlow(
                hDC, callsign, groundSpeed,
                [this](const EuroScopePlugIn::CPosition & pos)
                { return ConvertCoordFromPositionToPixel(pos); });
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

            POINT aircraftScreenPos = ConvertCoordFromPositionToPixel(radarPos);

            aircraftRenderer->DrawAircraftShape(
                hDC, radarPos, heading, wingspan, length, gearWidth,
                radarTarget, [this](const EuroScopePlugIn::CPosition & pos)
                { return ConvertCoordFromPositionToPixel(pos); });

            // Get tag offset (use stored offset or default)
            POINT tagOffset;
            auto offsetIt = tagOffsets.find(callsign);
            if (offsetIt != tagOffsets.end()) { tagOffset = offsetIt->second; }
            else
            {
                tagOffset.x = DEFAULT_TAG_OFFSET_X;
                tagOffset.y = DEFAULT_TAG_OFFSET_Y;
            }

            // Prepare tag data with aircraft information
            TagData tagData;
            tagData.items[TagItemType::Callsign] = callsign;
            tagData.items[TagItemType::AcType] =
                flightPlan.GetFlightPlanData().GetAircraftFPType();

            // Add ground speed
            char gsStr[16];
            sprintf_s(gsStr, sizeof(gsStr), "%.0f", groundSpeed);
            tagData.items[TagItemType::GroundSpeed] = gsStr;

            // Add flight level from radar target
            int altitude = radarTarget.GetPosition().GetPressureAltitude();
            char flStr[16];
            sprintf_s(flStr, sizeof(flStr), "FL%03d", altitude / 100);
            tagData.items[TagItemType::FlightLevel] = flStr;

            // Add squawk code
            const char * squawkCode = radarTarget.GetPosition().GetSquawk();
            tagData.items[TagItemType::SSR] = squawkCode;

            // Add SID/STAR if available
            if (flightPlan.IsValid())
            {
                std::string sidInfo = flightPlan.GetFlightPlanData().GetSidName();
                if (!sidInfo.empty())
                {
                    tagData.items[TagItemType::SID] = sidInfo;
                }

                std::string wakeStr = "";
                wakeStr += flightPlan.GetFlightPlanData().GetAircraftWtc();
                tagData.items[TagItemType::Wake] = wakeStr;
            }

            // Determine aircraft state for tag type
            bool isCorrelated = flightPlan.IsValid();
            
            Logger::getInstance().debug("Aircraft " + callsign + ": correlated=" +
                                       (isCorrelated ? "true" : "false") +
                                       " gs=" + std::to_string(static_cast<int>(groundSpeed)));
            
            // Check if on departure or arrival based on flight plan state
            // Simulated = just filed, Notified/Coordinated = being handled
            bool isDeparture = false;
            bool isArrival = false;
            
            if (isCorrelated)
            {
                // You'll need to determine this from your flight plan tracking
                // For now, assume low-level aircraft with climb tendency = departure
                // and descending aircraft = arrival
                // This can be refined based on flight plan actual state
                isDeparture = (groundSpeed < 80.0); // Simple heuristic
                isArrival = false; // Could be determined from flight plan
            }

            // Draw tag based on aircraft state
            RECT tagRect = Tag::DrawTagForAircraft(
                hDC, aircraftScreenPos, tagData, *tagProfileManager,
                isCorrelated, isDeparture, isArrival, groundSpeed,
                tagOffset.x, tagOffset.y);

            Logger::getInstance().debug("Aircraft " + callsign + ": tag drawn at (" +
                                       std::to_string(tagRect.left) + "," +
                                       std::to_string(tagRect.top) + ")");

            // Register tag as a screen object for dragging (only if tag was drawn)
            if (tagRect.right > 0 && tagRect.bottom > 0)
            {
                AddScreenObject(SCREEN_OBJECT_TYPE_TAG, callsign.c_str(),
                                tagRect, true, "");
            }
        }

        radarTarget = GetPlugIn()->RadarTargetSelectNext(radarTarget);
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

void RadarDisplay::OnClickScreenObject(int objectType,
                                       const char * objectId,
                                       POINT pt,
                                       RECT area,
                                       int button)
{
    // Check if click is in menu bar
    if (menuBar->IsPointInMenuBar(pt, area))
    {
        int menuIndex = menuBar->OnClick(pt, area);
        if (menuIndex >= 0) { HandleMenuBarClick(menuIndex); }
    }
}

void RadarDisplay::OnMoveScreenObject(int objectType,
                                      const char * objectId,
                                      POINT pt,
                                      RECT area,
                                      bool released)
{
    // Handle tag dragging
    if (objectType == SCREEN_OBJECT_TYPE_TAG)
    {
        std::string callsign = objectId;

        // Get the radar target to find its screen position
        EuroScopePlugIn::CRadarTarget rt =
            GetPlugIn()->RadarTargetSelect(callsign.c_str());

        if (rt.IsValid())
        {
            POINT aircraftScreenPos =
                ConvertCoordFromPositionToPixel(rt.GetPosition().GetPosition());

            // Calculate center of tag area
            POINT tagCenter;
            tagCenter.x = (area.left + area.right) / 2;
            tagCenter.y = (area.top + area.bottom) / 2;

            // Calculate and store the offset from aircraft to tag center
            POINT offset;
            offset.x = tagCenter.x - aircraftScreenPos.x;
            offset.y = tagCenter.y - aircraftScreenPos.y;

            tagOffsets[callsign] = offset;

            // Request refresh to redraw with new position
            RequestRefresh();
        }
    }
}

void RadarDisplay::HandleMenuBarClick(int menuIndex)
{
    // Menu items: 0=Display, 1=Maps, 2=Windows, 3=Colours, 4=Target, 5=Radar,
    // 6=RIMCAS, 7=AFDAS
    const char * menuNames[] = {"Display", "Maps",  "Windows", "Colours",
                                "Target",  "Radar", "RIMCAS",  "AFDAS"};

    if (menuIndex >= 0 && menuIndex < 8)
    {
        // Log the click for now - later this will open the appropriate
        // menu/dialog
        Logger::getInstance().info("Menu clicked: " +
                                   std::string(menuNames[menuIndex]));
    }
}