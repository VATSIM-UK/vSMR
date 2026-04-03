#pragma once

#include "AircraftRenderer.hpp"
#include "Display/MenuBar.hpp"
#include "Tags/TagProfileManager.hpp"
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)
#include <map>
#include <memory>
#include <string>
#include <vector>

class RadarDisplay : public EuroScopePlugIn::CRadarScreen
{
    public:
    RadarDisplay();
    virtual ~RadarDisplay();

    virtual void OnAsrContentToBeClosed();
    virtual void OnRefresh(HDC hDC, int phase);
    virtual void
    OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget radarTarget);
    virtual void OnClickScreenObject(int objectType,
                                     const char * objectId,
                                     POINT pt,
                                     RECT area,
                                     int button);
    virtual void OnMoveScreenObject(int objectType,
                                    const char * objectId,
                                    POINT pt,
                                    RECT area,
                                    bool released);

    private:
    std::unique_ptr<AircraftRenderer> aircraftRenderer;
    std::unique_ptr<MenuBar> menuBar;
    std::unique_ptr<TagProfileManager> tagProfileManager;

    // Tag offset storage: callsign -> offset from aircraft position
    std::map<std::string, POINT> tagOffsets;

    // Default tag offset
    static constexpr int DEFAULT_TAG_OFFSET_X = 40;
    static constexpr int DEFAULT_TAG_OFFSET_Y = 25;

    void HandleMenuBarClick(int menuIndex);
};