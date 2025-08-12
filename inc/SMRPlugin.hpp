#pragma once

#include <cstddef>
#include <windows.h>

#include "EuroScopePlugIn.h"


namespace vsmr {
    inline constexpr const char* PLUGIN_NAME = "vSMR Vatsim UK";
    inline constexpr const char* PLUGIN_VERSION = "dev";
    inline constexpr const char* PLUGIN_DEVELOPERS = "VATSIM UK";
    inline constexpr const char* PLUGIN_COPYRIGHT = "GPL v3";
    inline constexpr const char* PLUGIN_VIEW_AVISO = "SMR radar display";
}

class SMRPlugin : public EuroScopePlugIn::CPlugIn{
    public:
        SMRPlugin();
        ~SMRPlugin() override;
};