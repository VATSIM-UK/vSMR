#include "AircraftRenderer.hpp"
#include "Logger.hpp"
#include "angleUtils.hpp"
#include "stringUtils.hpp"
#include <cmath>
#include <fstream>
#include <sstream>

AircraftRenderer::AircraftRenderer()
{
}

AircraftRenderer::~AircraftRenderer()
{
}

bool AircraftRenderer::LoadAircraftData(
    const std::filesystem::path & csvFilePath)
{

    std::ifstream file(csvFilePath);
    if (!file.is_open())
    {
        Logger::getInstance().error("Failed to open aircraft data file: " +
                                    csvFilePath.string());
        return false;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        AircraftData data           = ParseCsvLine(line);
        aircraftDatabase[data.icao] = data;
        lineCount++;
    }

    file.close();
    Logger::getInstance().info("Loaded " + std::to_string(lineCount) +
                               " aircraft from database");
    return true;
}

AircraftData AircraftRenderer::ParseCsvLine(const std::string & line)
{
    std::stringstream ss(line);
    std::string token;
    AircraftData data;

    // Parse ICAO code
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing ICAO code");
    data.icao = stringUtils::trimString(token);

    // Parse wingspan (in feet, convert to meters)
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing wingspan");
    data.wingspan = std::stod(stringUtils::trimString(token)) * 0.3048;

    // Parse length (in feet, convert to meters)
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing length");
    data.length = std::stod(stringUtils::trimString(token)) * 0.3048;

    // Parse gear width (in feet, convert to meters)
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing gear width");
    data.gearWidth = std::stod(stringUtils::trimString(token)) * 0.3048;

    return data;
}

const AircraftData *
AircraftRenderer::GetAircraftData(const std::string & icaoCode) const
{
    std::string trimmedCode = stringUtils::trimString(icaoCode);
    auto it                 = aircraftDatabase.find(trimmedCode);
    if (it != aircraftDatabase.end()) { return &it->second; }
    return nullptr;
}

void AircraftRenderer::DrawAircraft(HDC hDC,
                                    int x,
                                    int y,
                                    const std::string & icaoCode,
                                    double heading,
                                    double pixelsPerMeter)
{
    const AircraftData * data = GetAircraftData(icaoCode);
    if (!data)
    {
        Logger::getInstance().warning("Aircraft data not found for: " +
                                      icaoCode);
        // Still draw a default blip if aircraft not in database
    }

    DrawRadarBlip(hDC, x, y, heading, 6);
}

void AircraftRenderer::DrawRadarBlip(HDC hDC,
                                     int centerX,
                                     int centerY,
                                     double heading,
                                     int size)
{
    // Convert heading to radians
    double radians  = angleUtils::degToRad(heading);
    double cosAngle = std::cos(radians);
    double sinAngle = std::sin(radians);

    // Define plane-shaped triangle (nose, left wing, right wing)
    // Point forward (nose), with wings extending back
    int corners[3][2] = {
        {0, -size * 2}, // front/nose
        {-size, size},  // left wing
        {size, size}    // right wing
    };

    // Rotate corners and translate to screen position
    POINT rotatedCorners[3];
    for (int i = 0; i < 3; ++i)
    {
        double x = corners[i][0] * cosAngle - corners[i][1] * sinAngle;
        double y = corners[i][0] * sinAngle + corners[i][1] * cosAngle;

        rotatedCorners[i].x = centerX + static_cast<int>(x);
        rotatedCorners[i].y = centerY + static_cast<int>(y);
    }

    // Create yellow pen and brush
    HPEN yellowPen     = CreatePen(PS_SOLID, 2, RGB(255, 255, 0)); // Yellow
    HBRUSH yellowBrush = CreateSolidBrush(RGB(255, 255, 0));       // Yellow

    // Select pen and brush
    HPEN oldPen     = (HPEN)SelectObject(hDC, yellowPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, yellowBrush);

    // Draw the filled plane-shaped triangle
    Polygon(hDC, rotatedCorners, 3);

    // Restore old pen and brush
    SelectObject(hDC, oldPen);
    SelectObject(hDC, oldBrush);

    // Clean up resources
    DeleteObject(yellowPen);
    DeleteObject(yellowBrush);
}

std::vector<EuroScopePlugIn::CPosition>
AircraftRenderer::GenerateAircraftOutline(
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

void AircraftRenderer::DrawAircraftShape(
    HDC hDC,
    const EuroScopePlugIn::CPosition & aircraftPos,
    double heading,
    double wingspan,
    double length,
    double gearWidth,
    const EuroScopePlugIn::CRadarTarget & radarTarget,
    std::function<POINT(const EuroScopePlugIn::CPosition &)> coordConverter)
{
    // Generate aircraft outline in lat/lon
    auto outlinePositions = GenerateAircraftOutline(
        aircraftPos, heading, wingspan, length, gearWidth);

    // Convert to screen coordinates
    std::vector<POINT> screenPoints;
    for (const auto & pos : outlinePositions)
    {
        POINT screenPos = coordConverter(pos);
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
}

void AircraftRenderer::DrawCorrelationSymbol(
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

void AircraftRenderer::DrawAircraftAfterGlow(
    HDC hDC,
    const std::string & callsign,
    std::function<POINT(const EuroScopePlugIn::CPosition &)> coordConverter)
{
    // Check if we have shape data for this aircraft
    auto it = aircraftShapes.find(callsign);
    if (it == aircraftShapes.end()) { return; }

    const AircraftShapeData & shapeData = it->second;

    // Define afterglow colors (from brightest to dimmest)
    COLORREF colors[3] = {
        RGB(0, 255, 255), // Cyan (most recent history)
        RGB(0, 219, 219), // Slightly dimmer
        RGB(0, 183, 183)  // Dimmest
    };

    // Draw three levels of history
    const std::map<int, Point2D> * histories[3] = {
        &shapeData.historyOne, &shapeData.historyTwo, &shapeData.historyThree};

    for (int historyLevel = 0; historyLevel < 3; historyLevel++)
    {
        const std::map<int, Point2D> & historyPoints = *histories[historyLevel];

        if (historyPoints.empty()) { continue; }

        // Convert history points to screen coordinates
        std::vector<POINT> screenPoints;
        for (const auto & [index, point] : historyPoints)
        {
            EuroScopePlugIn::CPosition pos;
            pos.m_Longitude = point.x;
            pos.m_Latitude  = point.y;
            POINT screenPos = coordConverter(pos);
            screenPoints.push_back(screenPos);
        }

        // Draw the afterglow shape
        if (!screenPoints.empty())
        {
            HBRUSH brush = CreateSolidBrush(colors[historyLevel]);
            HPEN pen     = CreatePen(PS_SOLID, 1, colors[historyLevel]);

            HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, brush);
            HPEN oldPen     = (HPEN)SelectObject(hDC, pen);

            Polygon(hDC, screenPoints.data(),
                    static_cast<int>(screenPoints.size()));

            SelectObject(hDC, oldBrush);
            SelectObject(hDC, oldPen);

            DeleteObject(brush);
            DeleteObject(pen);
        }
    }
}

void AircraftRenderer::UpdateAircraftShape(
    const std::string & callsign,
    const EuroScopePlugIn::CPosition & position,
    double heading,
    const std::string & aircraftType,
    char wtc)
{
    // Update history - shift previous positions back
    aircraftShapes[callsign].historyThree = aircraftShapes[callsign].historyTwo;
    aircraftShapes[callsign].historyTwo   = aircraftShapes[callsign].historyOne;
    aircraftShapes[callsign].historyOne   = aircraftShapes[callsign].points;

    // Clear current points
    aircraftShapes[callsign].points.clear();

    // Default dimensions (in meters)
    double width      = 34.0;
    double cabinWidth = 4.0;
    double length     = 38.0;

    // Get aircraft dimensions from database
    const AircraftData * data = GetAircraftData(aircraftType);

    if (data)
    {
        width      = data->wingspan;
        length     = data->length;
        cabinWidth = data->gearWidth;
    }
    else
    {
        // Fallback to WTC category
        if (wtc == 'H')
        {
            width      = 61.0;
            length     = 64.0;
            cabinWidth = 14.0;
        }
        else if (wtc == 'J')
        {
            width      = 80.0;
            length     = 73.0;
            cabinWidth = 14.0;
        }
    }

    // Add small random variation
    width += static_cast<double>((rand() % 5) - 2);
    cabinWidth += static_cast<double>((rand() % 3) - 1);
    length += static_cast<double>((rand() % 5) - 2);

    // Calculate heading vectors
    double trackHead        = heading;
    double inverseTrackHead = fmod(trackHead + 180.0, 360.0);
    double leftTrackHead    = fmod(trackHead - 90.0, 360.0);
    double rightTrackHead   = fmod(trackHead + 90.0, 360.0);

    double halfLength    = length / 2.0;
    double halfCabWidth  = cabinWidth / 2.0;
    double halfSpanWidth = width / 2.0;

    // Build aircraft shape using haversine calculations

    EuroScopePlugIn::CPosition topMiddle =
        angleUtils::haversine(position, trackHead, halfLength);
    EuroScopePlugIn::CPosition topLeft =
        angleUtils::haversine(topMiddle, leftTrackHead, halfCabWidth);
    EuroScopePlugIn::CPosition topRight =
        angleUtils::haversine(topMiddle, rightTrackHead, halfCabWidth);

    EuroScopePlugIn::CPosition bottomMiddle =
        angleUtils::haversine(position, inverseTrackHead, halfLength);
    EuroScopePlugIn::CPosition bottomLeft =
        angleUtils::haversine(bottomMiddle, leftTrackHead, halfCabWidth);
    EuroScopePlugIn::CPosition bottomRight =
        angleUtils::haversine(bottomMiddle, rightTrackHead, halfCabWidth);

    EuroScopePlugIn::CPosition middleTopLeft = angleUtils::haversine(
        topLeft, fmod(inverseTrackHead + 25.0, 360.0), 0.8 * halfLength);
    EuroScopePlugIn::CPosition middleTopRight = angleUtils::haversine(
        topRight, fmod(inverseTrackHead - 25.0, 360.0), 0.8 * halfLength);
    EuroScopePlugIn::CPosition middleBottomLeft = angleUtils::haversine(
        bottomLeft, fmod(trackHead - 15.0, 360.0), 0.8 * halfLength);
    EuroScopePlugIn::CPosition middleBottomRight = angleUtils::haversine(
        bottomRight, fmod(trackHead + 15.0, 360.0), 0.8 * halfLength);

    EuroScopePlugIn::CPosition rightTop = angleUtils::haversine(
        middleBottomRight, rightTrackHead, 0.7 * halfSpanWidth);
    EuroScopePlugIn::CPosition rightBottom =
        angleUtils::haversine(rightTop, inverseTrackHead, cabinWidth);

    EuroScopePlugIn::CPosition leftTop = angleUtils::haversine(
        middleBottomLeft, leftTrackHead, 0.7 * halfSpanWidth);
    EuroScopePlugIn::CPosition leftBottom =
        angleUtils::haversine(leftTop, inverseTrackHead, cabinWidth);

    // Store the 12 base points
    EuroScopePlugIn::CPosition basePoints[12] = {
        topLeft,          middleTopLeft, leftTop,        leftBottom,
        middleBottomLeft, bottomLeft,    bottomRight,    middleBottomRight,
        rightBottom,      rightTop,      middleTopRight, topRight};

    // Generate interpolated points between base points
    for (int i = 0; i < 12; i++)
    {
        EuroScopePlugIn::CPosition startPoint = basePoints[i];
        EuroScopePlugIn::CPosition endPoint   = basePoints[(i + 1) % 12];

        // Store start point
        Point2D point;
        point.x                                    = startPoint.m_Longitude;
        point.y                                    = startPoint.m_Latitude;
        int baseIndex                              = i * 6;
        aircraftShapes[callsign].points[baseIndex] = point;

        // Interpolate intermediate points
        for (int k = 1; k < 6; k++)
        {
            double ratio = static_cast<double>(k) / 6.0;
            Point2D midPoint;
            midPoint.y = startPoint.m_Latitude +
                ratio * (endPoint.m_Latitude - startPoint.m_Latitude);
            midPoint.x = startPoint.m_Longitude +
                ratio * (endPoint.m_Longitude - startPoint.m_Longitude);
            aircraftShapes[callsign].points[baseIndex + k] = midPoint;
        }
    }
}
