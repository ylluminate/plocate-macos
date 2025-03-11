#pragma once

#ifndef __APPLE__
#error trying to build Darwin firmlinks support on non-__APPLE__ platform
#endif

#ifndef HAS_MNTENT_DARWIN_COMPAT_H
#error Why isn't HAS_MNTENT_DARWIN_COMPAT_H defined?
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

struct mntent {
	char *mnt_dir;
	char *mnt_type;
};

FILE *setmntent(const char *filename, const char *type);
struct mntent *getmntent(FILE *fp);
int endmntent(FILE *fp);

#ifdef __cplusplus
}  // extern "C"
#endif
