#include "angleUtils.hpp"
#include <cmath>

namespace angleUtils
{

double degToRad(double degrees)
{
    return degrees * PI / 180.0;
}

double radToDeg(double radians)
{
    return radians * 180.0 / PI;
}

/**
 * Calculate a new position using the Haversine formula
 * @param origin Starting position
 * @param heading Direction in degrees (0-360)
 * @param distance Distance in meters
 * @return New position
 */
EuroScopePlugIn::CPosition haversine(const EuroScopePlugIn::CPosition & origin,
                                     double heading,
                                     double distance)
{
    EuroScopePlugIn::CPosition newPos;

    // Convert meters to radians (nautical miles conversion)
    double d    = (distance * 0.00053996) / 60.0 * PI / 180.0;
    double trk  = degToRad(heading);
    double lat0 = degToRad(origin.m_Latitude);
    double lon0 = degToRad(origin.m_Longitude);

    double lat = asin(sin(lat0) * cos(d) + cos(lat0) * sin(d) * cos(trk));
    double lon = cos(lat) == 0
        ? lon0
        : fmod(lon0 + asin(sin(trk) * sin(d) / cos(lat)) + PI, 2.0 * PI) - PI;

    newPos.m_Latitude  = radToDeg(lat);
    newPos.m_Longitude = radToDeg(lon);

    return newPos;
}

} // namespace angleUtils