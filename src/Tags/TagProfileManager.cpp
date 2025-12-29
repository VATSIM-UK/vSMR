#include "TagProfileManager.hpp"
#include <fstream>
#include <cctype>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
/**
 * Read entire file content
 */
std::string ReadFile(const std::string & filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) { return ""; }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

/**
 * Helper to parse color from CSV string format (r,g,b,a)
 */
ColorRGBA ParseColorString(const std::string & colorStr)
{
    ColorRGBA color = {255, 255, 255, 255};

    size_t pos = 0;
    int components[4] = {255, 255, 255, 255};
    int componentIdx = 0;

    std::string token;
    for (size_t i = 0; i < colorStr.size() && componentIdx < 4; ++i)
    {
        if (colorStr[i] == ',')
        {
            if (!token.empty())
            {
                components[componentIdx] = std::stoi(token);
                componentIdx++;
                token.clear();
            }
        }
        else if (!std::isspace(colorStr[i]))
        {
            token += colorStr[i];
        }
    }

    if (!token.empty() && componentIdx < 4)
    {
        components[componentIdx] = std::stoi(token);
    }

    color.r = static_cast<uint8_t>(components[0]);
    color.g = static_cast<uint8_t>(components[1]);
    color.b = static_cast<uint8_t>(components[2]);
    color.a = static_cast<uint8_t>(components[3]);

    return color;
}

/**
 * Helper to parse color from JSON (supports both string and object formats)
 */
ColorRGBA ParseColorObject(const json & colorObj)
{
    if (colorObj.is_string())
    {
        return ParseColorString(colorObj.get<std::string>());
    }

    ColorRGBA color = {255, 255, 255, 255};

    if (colorObj.contains("r") && colorObj["r"].is_number_integer())
    {
        color.r = static_cast<uint8_t>(colorObj["r"].get<int>());
    }
    if (colorObj.contains("g") && colorObj["g"].is_number_integer())
    {
        color.g = static_cast<uint8_t>(colorObj["g"].get<int>());
    }
    if (colorObj.contains("b") && colorObj["b"].is_number_integer())
    {
        color.b = static_cast<uint8_t>(colorObj["b"].get<int>());
    }
    if (colorObj.contains("a") && colorObj["a"].is_number_integer())
    {
        color.a = static_cast<uint8_t>(colorObj["a"].get<int>());
    }

    return color;
}

} // namespace

TagType TagProfileManager::StringToTagType(const std::string & str)
{
    if (str == "departure") return TagType::Departure;
    if (str == "arrival") return TagType::Arrival;
    if (str == "airborne") return TagType::Airborne;
    if (str == "uncorrelated") return TagType::Uncorrelated;

    return TagType::Uncorrelated; // Default
}

bool TagProfileManager::Initialize(const std::string & profilePath)
{
    std::string jsonContent = ReadFile(profilePath);
    if (jsonContent.empty()) { return false; }

    try
    {
        json doc = json::parse(jsonContent);

        if (!ParseTagDefinitions(doc)) { return false; }

        if (!ParseFormattingSettings(doc)) { return false; }

        return true;
    }
    catch (const json::exception &)
    {
        return false;
    }
}

bool TagProfileManager::ParseTagDefinitions(const json & doc)
{
    if (!doc.contains("tag_definitions")) { return false; }

    const auto & definitions = doc["tag_definitions"];
    if (!definitions.is_object()) { return false; }

    for (auto it = definitions.begin(); it != definitions.end(); ++it)
    {
        const std::string & tagName = it.key();
        TagType tagType = StringToTagType(tagName);
        const auto & tagObj = it.value();

        if (!tagObj.is_object() || !tagObj.contains("lines")) { continue; }

        TagDefinition tagDef = {};

        if (tagObj["lines"].is_array())
        {
            for (const auto & lineArr : tagObj["lines"])
            {
                std::vector<std::string> line;
                if (lineArr.is_array())
                {
                    for (const auto & item : lineArr)
                    {
                        if (item.is_string())
                        {
                            line.push_back(item.get<std::string>());
                        }
                    }
                }
                tagDef.lines.push_back(line);
            }
        }

        if (tagObj.contains("background_color") &&
            tagObj["background_color"].is_object())
        {
            tagDef.backgroundColor = ParseColorObject(tagObj["background_color"]);
        }

        if (tagObj.contains("background_color_on_runway") &&
            tagObj["background_color_on_runway"].is_object())
        {
            tagDef.backgroundColorOnRunway =
                ParseColorObject(tagObj["background_color_on_runway"]);
        }

        if (tagObj.contains("text_color") && tagObj["text_color"].is_object())
        {
            tagDef.textColor = ParseColorObject(tagObj["text_color"]);
        }

        if (tagObj.contains("use_departure_arrival_coloring") &&
            tagObj["use_departure_arrival_coloring"].is_boolean())
        {
            tagDef.useDepArrColoring =
                tagObj["use_departure_arrival_coloring"].get<bool>();
        }

        m_tagDefinitions[tagType] = tagDef;
    }

    return true;
}

bool TagProfileManager::ParseFormattingSettings(const json & doc)
{
    if (!doc.contains("tag_formatting")) { return false; }

    const auto & formatting = doc["tag_formatting"];
    if (!formatting.is_object()) { return false; }

    if (formatting.contains("padding") && formatting["padding"].is_number_integer())
    {
        m_formattingSettings.padding = formatting["padding"].get<int>();
    }

    if (formatting.contains("line_spacing") &&
        formatting["line_spacing"].is_number_integer())
    {
        m_formattingSettings.lineSpacing =
            formatting["line_spacing"].get<int>();
    }

    if (formatting.contains("min_width") &&
        formatting["min_width"].is_number_integer())
    {
        m_formattingSettings.minWidth = formatting["min_width"].get<int>();
    }
    return true;
}

const TagDefinition * TagProfileManager::GetTagDefinition(
    TagType tagType) const
{
    auto it = m_tagDefinitions.find(tagType);
    if (it == m_tagDefinitions.end()) { return nullptr; }

    return &it->second;
}

int TagProfileManager::GetLineCount(TagType tagType) const
{
    const auto * tagDef = GetTagDefinition(tagType);
    if (!tagDef) { return 0; }

    return static_cast<int>(tagDef->lines.size());
}

const std::vector<std::string> * TagProfileManager::GetLineDefinition(
    TagType tagType,
    int lineIndex) const
{
    const auto * tagDef = GetTagDefinition(tagType);
    if (!tagDef || lineIndex < 0 ||
        lineIndex >= static_cast<int>(tagDef->lines.size()))
    {
        return nullptr;
    }

    return &tagDef->lines[lineIndex];
}
