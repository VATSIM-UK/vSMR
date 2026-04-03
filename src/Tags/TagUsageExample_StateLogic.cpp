// Tag Display Logic for Aircraft
//
// The tag system now automatically determines which tag to display based on aircraft state.
// This is handled by Tag::DetermineTagType() and Tag::DrawTagForAircraft()

/**
 * TAG DISPLAY RULES:
 * ==================
 * 
 * UNCORRELATED AIRCRAFT (not in flight plan):
 * - GS < 5 knots  → No tag displayed (-1)
 * - GS ≥ 5 knots  → Uncorrelated tag (purple system ID)
 *
 * CORRELATED AIRCRAFT (in flight plan):
 * - GS ≥ 80 knots → Airborne tag (regardless of departure/arrival status)
 * - GS < 80 knots + Departure → Departure tag (blue)
 * - GS < 80 knots + Arrival   → Arrival tag (red)
 * - GS < 80 knots + neither   → Airborne tag (default fallback)
 */

// USAGE EXAMPLE:
// ==============

void DrawAircraftTag(HDC hDC,
                     const Aircraft& aircraft,
                     const TagProfileManager& profileManager)
{
    // Prepare tag data with all aircraft information
    TagData tagData;
    tagData.items[TagItemType::Callsign] = aircraft.callsign;
    tagData.items[TagItemType::AcType] = aircraft.acType;
    tagData.items[TagItemType::FlightLevel] = aircraft.flightLevel;
    tagData.items[TagItemType::SSR] = aircraft.squawk;
    tagData.items[TagItemType::SID] = aircraft.sid;
    tagData.items[TagItemType::Wake] = aircraft.wakeCategory;
    // ... populate other fields as needed

    // Determine aircraft state
    bool isCorrelated = aircraft.flightPlan.IsValid();
    bool isDeparture = aircraft.flightPlan.GetDeparture() != "";
    bool isArrival = aircraft.flightPlan.GetArrival() != "";
    double groundSpeed = aircraft.groundSpeed; // in knots

    // Draw the appropriate tag automatically
    RECT tagRect = Tag::DrawTagForAircraft(
        hDC,
        aircraft.screenPosition,
        tagData,
        profileManager,
        isCorrelated,
        isDeparture,
        isArrival,
        groundSpeed,
        50,   // tagOffsetX (pixels right of aircraft)
        -30   // tagOffsetY (pixels above aircraft)
    );

    // tagRect will be {0,0,0,0} if no tag was drawn (stationary uncorrelated)
    if (tagRect.right > 0 && tagRect.bottom > 0)
    {
        // Register tag rect with screen object system if applicable
        // RegisterScreenObject(tagRect);
    }
}

// ALTERNATIVE: Use low-level DrawProfileTag if you've already determined tag type
// =================================================================================

void DrawWithDeterminedTagType()
{
    TagData tagData;
    // ... populate tagData ...

    int tagType = Tag::DetermineTagType(
        isCorrelated,
        isDeparture,
        isArrival,
        groundSpeed
    );

    if (tagType >= 0)  // -1 means no tag
    {
        RECT tagRect = Tag::DrawProfileTag(
            hDC,
            aircraftPos,
            tagData,
            profileManager,
            tagType,
            offsetX,
            offsetY
        );
    }
}
