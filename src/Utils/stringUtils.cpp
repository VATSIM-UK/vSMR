#include "stringUtils.hpp"
#include <cstring>
#include <string>

namespace stringUtils
{

bool startsWith(const char * prefix, const char * str)
{
    if (!prefix || !str) return false;
    const size_t n = std::strlen(prefix);
    return std::strncmp(str, prefix, n) == 0;
}

std::string trimString(const std::string & str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, (end - start + 1));
}

} // namespace stringUtils
