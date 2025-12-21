#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

namespace angleUtils
{

constexpr double PI = 3.14159265359;

double degToRad(double degrees);
double radToDeg(double radians);
EuroScopePlugIn::CPosition haversine(const EuroScopePlugIn::CPosition & origin,
                                     double heading,
                                     double distance);

} // namespace angleUtils