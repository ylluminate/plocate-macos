#pragma once

#ifndef __APPLE__
#error trying to build Darwin firmlinks support on non-__APPLE__ platform
#endif

#include <string>
#include <vector>

struct FirmlinkPair {
    std::string raw_path;
    std::string logical_path;
};

std::vector<FirmlinkPair> get_firmlinks(void);
