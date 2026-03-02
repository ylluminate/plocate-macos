// clang-format: off
// clang-format: on

#include "mntent_darwin_compat.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mount.h>

struct bsd_mntent {
	int num_mounts;
	int idx;
	struct mntent *mntents;
};

FILE *setmntent(const char *filename, const char *type)
{
	auto *ctx = static_cast<bsd_mntent *>(calloc(1, sizeof(bsd_mntent)));
	if (!ctx) {
		fprintf(stderr, "plocate: calloc failed in setmntent\n");
		return nullptr;
	}
	struct statfs *mounts = nullptr;
	ctx->num_mounts = getmntinfo_r_np(&mounts, 0);
	if (!mounts || ctx->num_mounts < 0) {
		fprintf(stderr, "plocate: getmntinfo_r_np failed in setmntent\n");
		free(mounts);
		free(ctx);
		return nullptr;
	}
	ctx->mntents = static_cast<mntent *>(calloc(ctx->num_mounts, sizeof(mntent)));
	if (!ctx->mntents) {
		fprintf(stderr, "plocate: calloc failed for mntents in setmntent\n");
		free(mounts);
		free(ctx);
		return nullptr;
	}
	for (int i = 0; i < ctx->num_mounts; ++i) {
		ctx->mntents[i].mnt_dir = strdup(mounts[i].f_mntonname);
		ctx->mntents[i].mnt_type = strdup(mounts[i].f_fstypename);
	}
	free(mounts);
	return reinterpret_cast<FILE *>(ctx);
}

struct mntent *getmntent(FILE *fp)
{
	if (!fp) {
		return nullptr;
	}
	auto *ctx = reinterpret_cast<bsd_mntent *>(fp);
	mntent *res = nullptr;
	if (ctx->idx < ctx->num_mounts) {
		res = &ctx->mntents[ctx->idx++];
	}
	return res;
}

int endmntent(FILE *fp)
{
	if (!fp) {
		return 0;
	}
	auto *ctx = reinterpret_cast<bsd_mntent *>(fp);
	for (int i = 0; i < ctx->num_mounts; ++i) {
		free(ctx->mntents[i].mnt_dir);
		free(ctx->mntents[i].mnt_type);
	}
	free(ctx->mntents);
	free(ctx);
	return 0;
}
