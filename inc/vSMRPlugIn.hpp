#pragma once

#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include <string>

namespace vSMRPlugIn
{
    class vSMRPlugIn : public EuroScopePlugIn::CPlugIn
    {
        public:
            vSMRPlugIn();
            ~vSMRPlugIn();

            void DisplayMessage(
                const std::string &message,
                const std::string &sender = "vSMRPlugIn"
            );

            bool OnCompileCommand(const char * sComandLine);
             
    };
}