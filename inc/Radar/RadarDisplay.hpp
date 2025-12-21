#pragma once

#include "AircraftRenderer.hpp"
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)
#include <memory>
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

    /**
     * Generate aircraft outline polygon based on dimensions and heading
     * @param aircraftPos Aircraft position
     * @param heading Aircraft heading in degrees
     * @param wingspan Wingspan in meters
     * @param length Fuselage length in meters
     * @param gearWidth Landing gear width in meters
     * @return Vector of positions forming the aircraft outline
     */
    std::vector<EuroScopePlugIn::CPosition>
    GenerateAircraftOutline(const EuroScopePlugIn::CPosition & aircraftPos,
                            double heading,
                            double wingspan,
                            double length,
                            double gearWidth) const;

    /**
     * Draw correlation symbol at aircraft center
     * @param hDC Device context
     * @param centerPos Screen position of aircraft center
     * @param radarTarget Radar target for transponder info
     */
    void
    DrawCorrelationSymbol(HDC hDC,
                          const POINT & centerPos,
                          const EuroScopePlugIn::CRadarTarget & radarTarget);
};