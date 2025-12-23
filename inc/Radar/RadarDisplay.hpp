#pragma once

#include "AircraftRenderer.hpp"
#include "Display/MenuBar.hpp"
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

    private:
    std::unique_ptr<AircraftRenderer> aircraftRenderer;
    std::unique_ptr<MenuBar> menuBar;

    void HandleMenuBarClick(int menuIndex);
};