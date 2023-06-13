#include "util.hh"
#include <unistd.h>
#include <string.h>
#include <stdexcept>

void checked_rename(const std::string src, const std::string dst)
{
    if(std::rename(src.c_str(), dst.c_str()))
        throw std::runtime_error("renaming '" + src + "' to '" + dst + "': "
            + strerror(errno));
}
