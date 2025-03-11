#pragma once

#ifndef __APPLE__
#error trying to build Darwin utils on non-__APPLE__ platform
#endif

#ifndef HAS_DARWIN_UTILS_H
#error Why isn't HAS_DARWIN_UTILS_H defined?
#endif

extern void darwin_disable_dataless();
extern void darwin_set_throttling(bool enable_throttling);
extern void darwin_set_atime_updates(bool enable_atime_updates);
