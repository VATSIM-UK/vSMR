#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include "rapidjson/document.h"

class DisplayProfileManager
{
public:
    DisplayProfileManager(std::filesystem::path profilesPath);
    virtual ~DisplayProfileManager();

    void loadProfiles();
    void setActiveProfile(const std::string &profileName);
    rapidjson::SizeType getActiveProfile() const;

private:
    std::filesystem::path profiles_path;
    std::string active_profile_name; // acts as key for profiles
    std::unordered_map<std::string, rapidjson::SizeType> profiles;
};
