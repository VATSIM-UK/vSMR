#include "DisplayProfileManager.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include "rapidjson/document.h"

DisplayProfileConfig::DisplayProfileConfig(std::filesystem::path profilesPath)
    : profiles_path(std::move(profilesPath))
{
    loadProfiles();
}

DisplayProfileConfig::~DisplayProfileConfig() = default;

void DisplayProfileConfig::loadProfiles()
{
    profiles.clear();

    std::ifstream ifs(profiles_path, std::ios::in | std::ios::binary);
    if (!ifs)
    {
        return;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    const std::string json = oss.str();
    if (json.empty()) return;

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError() || !doc.IsArray())
    {
        return; // Invalid format
    }

    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
    {
        const rapidjson::Value &profile = doc[i];
        if (!profile.IsObject()) continue;
        if (!profile.HasMember("name") || !profile["name"].IsString()) continue;
        std::string name = profile["name"].GetString();
        profiles.emplace(name, i);
        if (active_profile_name.empty() || name == "Default")
        {
            active_profile_name = name;
        }
    }
}