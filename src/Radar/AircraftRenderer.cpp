#include "AircraftRenderer.hpp"
#include "Logger.hpp"
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

    // Parse wingspan
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing wingspan");
    data.wingspan = std::stod(stringUtils::trimString(token));

    // Parse length
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing length");
    data.length = std::stod(stringUtils::trimString(token));

    // Parse height
    if (!std::getline(ss, token, ','))
        throw std::runtime_error("Missing gear width");
    data.gearWidth = std::stod(stringUtils::trimString(token));

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
    double radians  = (heading * 3.14159265359) / 180.0;
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
