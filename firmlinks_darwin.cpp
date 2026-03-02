#include "firmlinks_darwin.h"
#include "conf.h"

#include <CoreFoundation/CoreFoundation.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <MacTypes.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/syslimits.h>
#include <uuid/uuid.h>
#include <vector>

// FIXME: remember what the way is to determine if firmlinks are being used at all?
// like maybe somehow crazy you're running this off of hfs boot partition?

namespace {
static constexpr size_t UUID_STR_BUF_LEN_W_NUL = 37;
static std::vector<FirmlinkPair> firmlink_list;
}

extern "C"
OSStatus APFSContainerVolumeGroupGetFirmlinks(
	const char *disk, uuid_t volume_group_id,
	CFMutableArrayRef *firmlinks);

std::vector<FirmlinkPair> get_firmlinks(void) {
	char dev_buf[PATH_MAX];
	dev_buf[0] = '\0';
	struct statfs *mounts = nullptr;
	int num_mounts = getmntinfo_r_np(&mounts, 0);
	if (num_mounts <= 0 || !mounts) {
		fprintf(stderr, "plocate: getmntinfo_r_np failed\n");
		free(mounts);
		return {};
	}
	for (int i = 0; i < num_mounts; ++i) {
		if (memcmp(mounts[i].f_mntonname, "/", sizeof("/"))) {
			continue;
		}
		const char  * const dev_path = mounts[i].f_mntfromname;
		if (memcmp(dev_path, "/dev/disk", sizeof("/dev/disk") - 1)) {
			fprintf(stderr, "plocate: root device path does not start with /dev/disk: %s\n", dev_path);
			free(mounts);
			return {};
		}
		const char * const dev_name = dev_path + (sizeof("/dev/") - 1);
		const size_t dev_name_len = strlen(dev_name);
		const char * const dev_trailer = dev_name + (sizeof("disk") - 1);
		const size_t dev_trailer_len = strlen(dev_trailer);
		const char * const dev_trailer_nul = dev_trailer + dev_trailer_len;
		if (dev_trailer_len == 0) {
			fprintf(stderr, "plocate: unexpected root device name format: %s\n", dev_path);
			free(mounts);
			return {};
		}
		size_t num_s = 0;
		const char *first_s = nullptr;
		bool bad_char = false;
		for (size_t j = 0; j < dev_trailer_len; ++j) {
			const char c = dev_trailer[j];
			if (c == 's') {
				if (!num_s) {
					first_s = &dev_trailer[j];
				}
				++num_s;
			} else if (c < '0' || c > '9') {
				bad_char = true;
				break;
			}
		}
		if (bad_char || num_s > 2) {
			fprintf(stderr, "plocate: unexpected root device name format: %s\n", dev_path);
			free(mounts);
			return {};
		}
		if (num_s == 2) {
			if (dev_trailer_len < 4 || first_s[1] == 's' || dev_trailer[dev_trailer_len - 1] == 's') {
				fprintf(stderr, "plocate: unexpected root device name format: %s\n", dev_path);
				free(mounts);
				return {};
			}
		} else if (num_s == 1) {
			if (dev_trailer_len < 2 || first_s >= dev_trailer_nul) {
				fprintf(stderr, "plocate: unexpected root device name format: %s\n", dev_path);
				free(mounts);
				return {};
			}
		}
		if (num_s >= 1) {
			if (!first_s) {
				fprintf(stderr, "plocate: unexpected root device name format: %s\n", dev_path);
				free(mounts);
				return {};
			}
			const size_t dev_len = (sizeof("disk") - 1) + (dev_trailer_len - strlen(first_s));
			if (dev_len + 1 >= sizeof(dev_buf)) {
				fprintf(stderr, "plocate: root device name too long: %s\n", dev_path);
				free(mounts);
				return {};
			}
			memcpy(dev_buf, dev_name, dev_len);
			dev_buf[dev_len] = '\0';
		} else {
			if (dev_name_len + 1 >= sizeof(dev_buf)) {
				fprintf(stderr, "plocate: root device name too long: %s\n", dev_path);
				free(mounts);
				return {};
			}
			memcpy(dev_buf, dev_name, dev_name_len + 1);
		}
		break;
	}
	free(mounts);

	if (dev_buf[0] == '\0') {
		fprintf(stderr, "plocate: could not determine root device\n");
		return {};
	}

	char uuid_str[UUID_STR_BUF_LEN_W_NUL] = {0};
	size_t uuid_str_sz = sizeof(uuid_str);
	if (sysctlbyname("kern.bootuuid", uuid_str, &uuid_str_sz, nullptr, 0)) {
		fprintf(stderr, "plocate: sysctlbyname(kern.bootuuid) failed\n");
		return {};
	}

	uuid_t vol_group_id;
	if (uuid_parse(uuid_str, vol_group_id)) {
		fprintf(stderr, "plocate: uuid_parse failed for boot uuid\n");
		return {};
	}

	CFMutableArrayRef firmlinks = nullptr;
	const OSStatus getfl_res =
		APFSContainerVolumeGroupGetFirmlinks(dev_buf, vol_group_id, &firmlinks);
	if (getfl_res) {
		fprintf(stderr, "plocate: APFSContainerVolumeGroupGetFirmlinks failed: %d\n", (int)getfl_res);
		return {};
	}
	if (!firmlinks) {
		if (conf_debug_pruning) {
			fprintf(stderr, "Didn't find any firmlinks.\n");
		}
		return {};
	}

	const CFIndex numfl = CFArrayGetCount(firmlinks);
	if (numfl % 2 != 0) {
		fprintf(stderr, "plocate: firmlink array has odd count\n");
		CFRelease(firmlinks);
		return {};
	}
	if (numfl == 0) {
		CFRelease(firmlinks);
		return {};
	}
	FirmlinkPair tmp;
	std::vector<FirmlinkPair> flplist(numfl/2 + 1);
	for (CFIndex i = 0; i < numfl; ++i) {
		CFStringRef val = reinterpret_cast<CFStringRef>(
			CFArrayGetValueAtIndex(firmlinks, i));
		if (!val) {
			fprintf(stderr, "plocate: null entry in firmlink array\n");
			CFRelease(firmlinks);
			return {};
		}
		char path[1024];
		if (!CFStringGetCString(val, path, sizeof(path), kCFStringEncodingUTF8)) {
			fprintf(stderr, "plocate: CFStringGetCString failed for firmlink entry\n");
			CFRelease(firmlinks);
			return {};
		}
		if (i % 2 == 0) {
			tmp.logical_path = path;
		} else {
			tmp.raw_path = path;
			flplist[i/2] = tmp;
		}
	}
	CFRelease(firmlinks);

	flplist.push_back({"/", "/System/Volumes/Data"});

	if (conf_debug_pruning) {
		const size_t nflp = flplist.size();
		fprintf(stderr, "Number of firmlinks: %zu\n", nflp);
		for (size_t i = 0; i < nflp; ++i) {
			fprintf(stderr, "firmlink[%zu]: logical: %s raw: %s\n", i,
				flplist[i].logical_path.c_str(), flplist[i].raw_path.c_str());
		}
	}

	return flplist;
}

const std::string *is_firmlink_target(const std::string &path) {
	for (const auto &fl : firmlink_list) {
		if (fl.raw_path == path) {
			return &fl.logical_path;
		}
	}
	return nullptr;
}

void firmlink_init() {
	firmlink_list = get_firmlinks();
	if (firmlink_list.empty()) {
		fprintf(stderr, "plocate: warning: no firmlinks found\n");
	}
}
