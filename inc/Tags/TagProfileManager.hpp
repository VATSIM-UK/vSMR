#pragma once

#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include <nlohmann/json.hpp>

/**
 * Tag type enumeration for different aircraft states
 */
enum class TagType
{
    Departure,
    Arrival,
    Airborne,
    Uncorrelated
};

/**
 * Color structure with RGBA values
 */
struct ColorRGBA
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    COLORREF ToCOLORREF() const
    {
        return RGB(r, g, b);
    }
};

/**
 * Tag definition for a specific aircraft type
 */
struct TagDefinition
{
    std::vector<std::vector<std::string>> lines;
    ColorRGBA backgroundColor;
    ColorRGBA backgroundColorOnRunway;
    ColorRGBA textColor;
    bool useDepArrColoring = false;
};

/**
 * Tag formatting settings
 */
struct TagFormattingSettings
{
    int padding;
    int lineSpacing;
    int minWidth;
    int leaderLineLength;
    ColorRGBA leaderLineColor;
    ColorRGBA borderColor;
};

/**
 * Manager class for loading and accessing tag profile configurations
 * Loads tag definitions and formatting from vSMR_Tags.json
 */
class TagProfileManager
{
    public:
    /**
     * Initialize the tag profile manager
     * @param profilePath Path to vSMR_Tags.json file (relative or absolute)
     * @return true if profile loaded successfully, false otherwise
     */
    bool Initialize(const std::string & profilePath);

    /**
     * Get tag definition for a specific tag type
     * @param tagType The type of tag (departure, arrival, etc.)
     * @return Pointer to TagDefinition or nullptr if not found
     */
    const TagDefinition * GetTagDefinition(TagType tagType) const;

    /**
     * Get global tag formatting settings
     * @return Reference to TagFormattingSettings
     */
    const TagFormattingSettings & GetFormattingSettings() const
    {
        return m_formattingSettings;
    }

    /**
     * Get the number of lines in a tag definition
     * @param tagType The type of tag
     * @return Number of lines, or 0 if tag type not found
     */
    int GetLineCount(TagType tagType) const;

    /**
     * Get a specific line definition for a tag
     * @param tagType The type of tag
     * @param lineIndex Zero-based line index
     * @return Pointer to line definition or nullptr
     */
    const std::vector<std::string> * GetLineDefinition(TagType tagType,
                                                        int lineIndex) const;

    private:
    std::map<TagType, TagDefinition> m_tagDefinitions;
    TagFormattingSettings m_formattingSettings = {};

    /**
     * Parse tag definitions from loaded JSON
     */
    bool ParseTagDefinitions(const nlohmann::json & doc);

    /**
     * Parse formatting settings from loaded JSON
     */
    bool ParseFormattingSettings(const nlohmann::json & doc);

    /**
     * Helper to convert string to TagType
     */
    static TagType StringToTagType(const std::string & str);

    /**
     * Helper to parse color from JSON object
     */
    static bool ParseColor(const std::string & jsonStr,
                           ColorRGBA & outColor);
};
