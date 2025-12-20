#include "stringUtils.hpp"
#include <cstring>

bool startsWith(const char *prefix, const char *str)
{
    if (!prefix || !str)
        return false;
    const size_t n = std::strlen(prefix);
    return std::strncmp(str, prefix, n) == 0;
}