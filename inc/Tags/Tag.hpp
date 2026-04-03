#pragma once

#include <map>
#include <string>
#include <vector>
#include <windows.h>

#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "TagProfileManager.hpp"

/**
 * Tag item types - these correspond to the items in vSMR_Profiles.json
 */
enum class TagItemType
{
    Callsign,     // callsign: Callsign with freq state
    AcType,       // actype: Aircraft type
    ScType,       // sctype: Aircraft type that changes for squawk error
    SqError,      // sqerror: Squawk error if there is one
    DepRwy,       // deprwy: Departure runway
    SepRwy,       // seprwy: Departure runway that changes to speed if speed >
                  // 25kts
    ArvRwy,       // arvrwy: Arrival runway
    SrvRwy,       // srvrwy: Speed that changes to arrival runway if speed <
                  // 25kts
    Gate,         // gate: Gate, from scratchpad
    Sate,         // sate: Gate that changes to speed if speed > 25kts
    FlightLevel,  // flightlevel: FL/altitude
    GroundSpeed,  // gs: Ground speed
    Tendency,     // tendency: Climbing/descending symbol
    Wake,         // wake: Wake turbulence category
    GroundStatus, // groundstatus: Current status
    SSR,          // ssr/tssr: Squawk code
    SID,          // asid: Assigned SID
    ShortSID,     // ssid: Short version of SID
    Origin,       // origin/dep: Origin aerodrome
    Dest,         // dest: Destination aerodrome
    SystemId,     // systemid: System ID for uncorrelated targets
    UkStand       // uk_stand: UK specific stand
};

/**
 * Tag data structure containing all possible tag item values
 */
struct TagData
{
    std::map<TagItemType, std::string> items;
};

/**
 * Tag line definition - a line consists of multiple tag items
 */
using TagLine = std::vector<std::string>;

/**
 * Represents a radar tag with multi-line display and leader line
 */
class Tag
{
    public:
    /**
     * Draw tag based on aircraft state
     * Automatically determines tag type and draws if applicable
     * @param hDC Device context for drawing
     * @param aircraftScreenPos Screen position of aircraft center
     * @param tagData Tag data containing all tag items
     * @param profileManager TagProfileManager for getting colors/formatting
     * @param isCorrelated Whether aircraft is correlated to flight plan
     * @param isDeparture Whether aircraft is on a departure
     * @param isArrival Whether aircraft is on an arrival
     * @param groundSpeed Ground speed in knots
     * @param tagOffsetX X offset from aircraft center (pixels)
     * @param tagOffsetY Y offset from aircraft center (pixels)
     * @return Rectangle containing the drawn tag, or empty rect if no tag
     */
    static RECT DrawTagForAircraft(HDC hDC,
                                   POINT aircraftScreenPos,
                                   const TagData & tagData,
                                   const TagProfileManager & profileManager,
                                   bool isCorrelated,
                                   bool isDeparture,
                                   bool isArrival,
                                   double groundSpeed,
                                   int tagOffsetX,
                                   int tagOffsetY);

    /**
     * Draw a multi-line tag with leader line to aircraft using profile config
     * @param hDC Device context for drawing
     * @param aircraftScreenPos Screen position of aircraft center
     * @param tagData Tag data containing all tag items
     * @param profileManager TagProfileManager for getting colors/formatting
     * @param tagType Type of tag (departure, arrival, etc.)
     * @param tagOffsetX X offset from aircraft center (pixels)
     * @param tagOffsetY Y offset from aircraft center (pixels)
     * @return Rectangle containing the drawn tag (for screen object
     * registration)
     */
    static RECT DrawProfileTag(HDC hDC,
                               POINT aircraftScreenPos,
                               const TagData & tagData,
                               const TagProfileManager & profileManager,
                               int tagType,
                               int tagOffsetX,
                               int tagOffsetY);

    /**
     * Draw a multi-line tag with leader line to aircraft
     * @param hDC Device context for drawing
     * @param aircraftScreenPos Screen position of aircraft center
     * @param tagData Tag data containing all tag items
     * @param tagLines Tag line definitions (from JSON config)
     * @param tagOffsetX X offset from aircraft center (pixels)
     * @param tagOffsetY Y offset from aircraft center (pixels)
     * @param backgroundColor Background color for tag
     * @param textColor Text color for tag
     * @param borderColor Border color for tag (if drawing border)
     * @return Rectangle containing the drawn tag (for screen object
     * registration)
     */
    static RECT DrawMultiLineTag(HDC hDC,
                                 POINT aircraftScreenPos,
                                 const TagData & tagData,
                                 const std::vector<TagLine> & tagLines,
                                 int tagOffsetX,
                                 int tagOffsetY,
                                 COLORREF backgroundColor,
                                 COLORREF textColor,
                                 COLORREF borderColor = RGB(255, 255, 255));

    /**
     * Draw a simple single-line tag (for basic testing/fallback)
     * @param hDC Device context for drawing
     * @param aircraftScreenPos Screen position of aircraft center
     * @param callsign Aircraft callsign to display
     * @param tagOffsetX X offset from aircraft center (pixels)
     * @param tagOffsetY Y offset from aircraft center (pixels)
     * @return Rectangle containing the drawn tag (for screen object
     * registration)
     */
    static RECT DrawTag(HDC hDC,
                        POINT aircraftScreenPos,
                        const std::string & callsign,
                        int tagOffsetX,
                        int tagOffsetY);

    /**
     * Determine the appropriate tag type based on aircraft state
     * @param isCorrelated Whether the aircraft is correlated to a flight plan
     * @param isDeparture Whether the aircraft is on a departure
     * @param isArrival Whether the aircraft is on an arrival
     * @param groundSpeed Current ground speed in knots
     * @return The appropriate TagType for display
     *
     * Logic:
     * - Uncorrelated + GS < 5kts: Return -1 (no tag)
     * - Uncorrelated + GS >= 5kts: Return Uncorrelated tag
     * - Correlated + GS >= 80kts: Always return Airborne tag
     * - Correlated + GS < 80kts + Departure: Return Departure tag
     * - Correlated + GS < 80kts + Arrival: Return Arrival tag
     * - Otherwise: Return Airborne tag
     */
    static int DetermineTagType(bool isCorrelated,
                                bool isDeparture,
                                bool isArrival,
                                double groundSpeed);

    /**
     * Convert tag item name string to TagItemType enum
     * @param itemName Item name from JSON (e.g., "callsign", "actype")
     * @return Corresponding TagItemType
     */
    static TagItemType ParseTagItemType(const std::string & itemName);

    /**
     * Calculate bounding box for tag text
     * @param hDC Device context
     * @param text Text to measure
     * @param outRect Output rectangle
     * @return true if measurement was successful
     */
    static bool GetTextRect(HDC hDC, const std::string & text, RECT & outRect);
};
