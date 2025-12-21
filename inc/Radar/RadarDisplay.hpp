#pragma once

#include "AircraftRenderer.hpp"
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

    private:
    std::unique_ptr<AircraftRenderer> aircraftRenderer;
};