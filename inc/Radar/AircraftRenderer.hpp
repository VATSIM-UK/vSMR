#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

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
     * Draw a complete aircraft shape with outline and correlation symbol
     * @param hDC Device context
     * @param aircraftPos Aircraft position in lat/lon
     * @param heading Aircraft heading in degrees
     * @param wingspan Wingspan in meters
     * @param length Length in meters
     * @param gearWidth Gear width in meters
     * @param radarTarget Radar target for correlation info
     * @param coordConverter Function to convert CPosition to screen POINT
     */
    void
    DrawAircraftShape(HDC hDC,
                      const EuroScopePlugIn::CPosition & aircraftPos,
                      double heading,
                      double wingspan,
                      double length,
                      double gearWidth,
                      const EuroScopePlugIn::CRadarTarget & radarTarget,
                      std::function<POINT(const EuroScopePlugIn::CPosition &)>
                          coordConverter);

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

    /**
     * Draw afterglow effect for an aircraft using history points
     * @param hDC Device context
     * @param callsign Aircraft callsign to draw afterglow for
     * @param groundSpeed Aircraft ground speed in knots
     * @param coordConverter Function to convert CPosition to screen POINT
     */
    void DrawAircraftAfterGlow(
        HDC hDC,
        const std::string & callsign,
        double groundSpeed,
        std::function<POINT(const EuroScopePlugIn::CPosition &)>
            coordConverter);

    /**
     * Update aircraft shape data with position and dimensions
     * @param callsign Aircraft callsign
     * @param position Aircraft position
     * @param heading Aircraft heading in degrees
     * @param aircraftType ICAO aircraft type code
     * @param wtc Wake turbulence category
     */
    void UpdateAircraftShape(const std::string & callsign,
                             const EuroScopePlugIn::CPosition & position,
                             double heading,
                             const std::string & aircraftType,
                             char wtc);

    private:
    std::map<std::string, AircraftData> aircraftDatabase;

    // Structure to hold 2D points for aircraft shape
    struct Point2D
    {
        double x;
        double y;
    };

    // Structure to hold aircraft outline points with history
    struct AircraftShapeData
    {
        std::map<int, Point2D> points;
        std::map<int, Point2D> historyOne;
        std::map<int, Point2D> historyTwo;
        std::map<int, Point2D> historyThree;
    };

    // Map of callsigns to aircraft shape data
    std::map<std::string, AircraftShapeData> aircraftShapes;

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
