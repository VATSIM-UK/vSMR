#pragma once

#include <EuroScopePlugIn.h>
#include "AircraftRenderer.hpp"
#include <memory>

class RadarDisplay : public EuroScopePlugIn::CRadarScreen
{
    public:
    RadarDisplay();
    virtual ~RadarDisplay();

    virtual void OnAsrContentToBeClosed();
    virtual void OnRefresh(HDC hDC, int phase);

    private:
    std::unique_ptr<AircraftRenderer> aircraftRenderer;
};