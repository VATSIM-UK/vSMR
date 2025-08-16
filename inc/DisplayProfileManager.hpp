#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include "rapidjson/document.h"

class DisplayProfileConfig
{
    public:
        DisplayProfileConfig(std::filesystem::path profilesPath);
        virtual ~DisplayProfileConfig();

        void loadProfiles();

    private:
        std::filesystem::path profiles_path;
        std::string active_profile_name; // acts as key for profiles
        std::unordered_map<std::string, rapidjson::SizeType> profiles; 

};