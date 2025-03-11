#pragma once

#ifndef __APPLE__
#error trying to build Darwin firmlinks support on non-__APPLE__ platform
#endif

#ifndef HAS_FIRMLINKS_DARWIN_H
#error Why isn't HAS_FIRMLINKS_DARWIN_H defined?
#endif

#include <string>
#include <vector>

struct FirmlinkPair {
    std::string logical_path;
    std::string raw_path;
};

std::vector<FirmlinkPair> get_firmlinks(void);

/* Return firmlink source if PATH is the target of a firmlink, nullptr otherwise. */
extern const std::string *is_firmlink_target(const std::string &path);

/* Initialize state for is_firmlink_source(). */
extern void firmlink_init();
