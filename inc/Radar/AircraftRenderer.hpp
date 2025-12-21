#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <windows.h>

struct AircraftData
{
    std::string icao;
    double wingspan;  // in meters
    double length;    // in meters
    double gearWidth; // in meters
};

class AircraftRenderer
{
    public:
    AircraftRenderer();
    ~AircraftRenderer();

    /**
     * Load aircraft data from CSV file
     * @param csvFilePath Path to aircraft-data.csv
     * @return true if loading was successful
     */
    bool LoadAircraftData(const std::filesystem::path & csvFilePath);

    /**
     * Get aircraft data by ICAO code
     * @param icaoCode The ICAO aircraft code (e.g., "B738")
     * @return Pointer to AircraftData if found, nullptr otherwise
     */
    const AircraftData * GetAircraftData(const std::string & icaoCode) const;

    /**
     * Draw an aircraft on the radar screen
     * @param hDC Device context for drawing
     * @param x X position on screen
     * @param y Y position on screen
     * @param icaoCode ICAO aircraft code
     * @param heading Aircraft heading in degrees
     * @param pixelsPerMeter Scale factor for drawing
     */
    void DrawAircraft(HDC hDC,
                      int x,
                      int y,
                      const std::string & icaoCode,
                      double heading,
                      double pixelsPerMeter);

    private:
    std::map<std::string, AircraftData> aircraftDatabase;

    /**
     * Parse a single line from the CSV file
     * @param line CSV line to parse
     * @return AircraftData if successful
     */
    AircraftData ParseCsvLine(const std::string & line);

    /**
     * Draw a diamond/radar blip representing the aircraft
     * @param hDC Device context
     * @param centerX Center X position
     * @param centerY Center Y position
     * @param heading Rotation angle in degrees
     * @param size Size of diamond (pixels from center)
     */
    void DrawRadarBlip(HDC hDC,
                       int centerX,
                       int centerY,
                       double heading,
                       int size = 6);
};
