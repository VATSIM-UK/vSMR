#include "pathUtils.hpp"

#include <Windows.h>

namespace pathUtils
{

std::filesystem::path getDllPath()
{
    HMODULE hModule = nullptr;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&getDllPath), &hModule))
    {
        char path[MAX_PATH];
        if (GetModuleFileName(hModule, path, MAX_PATH))
        {
            std::filesystem::path dllPath{path};
            return dllPath.parent_path();
        }
    }
    auto currentPath = std::filesystem::current_path();
    if (std::filesystem::exists(currentPath / "UK/Data/Plugin/vSMR/vSMR.dll"))
    {
        return currentPath / "UK/Data/Plugin/vSMR/vSMR.dll";
    }

    return currentPath;
}

} // namespace pathUtils
