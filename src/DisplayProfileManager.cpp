#include "DisplayProfileManager.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include "rapidjson/document.h"

DisplayProfileManager::DisplayProfileManager(std::filesystem::path profilesPath)
    : profiles_path(std::move(profilesPath))
{
    loadProfiles();
}

DisplayProfileManager::~DisplayProfileManager() = default;

void DisplayProfileManager::loadProfiles()
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
        return; // Invalid format @TODO handle error / alert user
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

void DisplayProfileManager::setActiveProfile(const std::string& profileName)
{
    auto it = profiles.find(profileName);
    if (it == profiles.end()){
        throw std::invalid_argument("Profile does not exist: " + profileName);
    }
    active_profile_name = profileName;
}

rapidjson::SizeType DisplayProfileManager::getActiveProfile() const {
    auto it = profiles.find(active_profile_name);
    if (it != profiles.end())
    {
        return it->second;
    }
    throw std::runtime_error("Active profile not found");
}

