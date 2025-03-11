#include "firmlinks_darwin.h"

#include <CoreFoundation/CoreFoundation.h>
#include <cstddef>
#include <MacTypes.h>
#include <stdint.h>
#include <sys/sysctl.h>
#include <sys/syslimits.h>
#include <uuid/uuid.h>
#include <sys/mount.h>

// FIXME: remember what the way is to determine if firmlinks are being used at all?
// like maybe somehow crazy you're running this off of hfs boot partition?

static constexpr size_t UUID_STR_BUF_LEN_W_NUL = 37;

extern "C"
OSStatus APFSContainerVolumeGroupGetFirmlinks(
	const char *disk, uuid_t volume_group_id,
	CFMutableArrayRef *firmlinks);

std::vector<FirmlinkPair> get_firmlinks(void) {
	char dev_buf[PATH_MAX];
	struct statfs *mounts = nullptr;
	int num_mounts = getmntinfo_r_np(&mounts, 0);
	assert(num_mounts > 0);
	assert(mounts);
	for (int i = 0; i < num_mounts; ++i) {
		if (memcmp(mounts[i].f_mntonname, "/", sizeof("/"))) {
			continue;
		}
		const char  * const dev_path = mounts[i].f_mntfromname;
		assert(memcmp(dev_path, "/dev/disk", sizeof("/dev/disk") - 1));
		const char * const dev_name = dev_path + (sizeof("/dev/") - 1);
		const size_t dev_name_len = strlen(dev_name);
		const char * const dev_trailer = dev_name + (sizeof("disk") - 1);
		const size_t dev_trailer_len = strlen(dev_trailer);
		const char * const dev_trailer_nul = dev_trailer + dev_trailer_len;
		assert(dev_trailer_len > 0);
		size_t num_s = 0;
		const char *first_s = nullptr;
		for (size_t j = 0; j < dev_trailer_len; ++j) {
			const char c = dev_trailer[j];
			if (c == 's') {
				if (!num_s) {
					first_s = &dev_trailer[j];
				}
				++num_s;
			} else {
				assert(c >= '0' && c <= '9');
			}
		}
		assert(num_s <= 2);
		if (num_s == 2) {
			assert(dev_trailer_len >= 4); // sXsY
			assert(first_s[1] != 's');
			assert(dev_trailer[dev_trailer_len - 1] != 's');
		} else if (num_s == 1) {
			assert(dev_trailer_len >= 2); // sX
			assert(first_s < dev_trailer_nul);
		}
		if (num_s >= 1) {
			assert(first_s);
			const size_t dev_len = dev_trailer_len - strlen(first_s);
			assert(dev_len + 1 < sizeof(dev_buf));
			memcpy(dev_buf, dev_name, dev_len);
			dev_buf[dev_len-1] = '\0';
		} else {
			assert(dev_name_len + 1 < sizeof(dev_buf));
			memcpy(dev_buf, dev_name, dev_name_len + 1);
		}
	}
	free(mounts);
	mounts = nullptr;

	char uuid_str[UUID_STR_BUF_LEN_W_NUL] = {0};
	size_t uuid_str_sz = sizeof(uuid_str);
	assert(!sysctlbyname("kern.bootuuid", uuid_str, &uuid_str_sz, nullptr, 0));

	uuid_t vol_group_id;
	assert(!uuid_parse(uuid_str, vol_group_id));

	CFMutableArrayRef firmlinks = NULL;
	const OSStatus getfl_res =
		APFSContainerVolumeGroupGetFirmlinks(dev_buf, vol_group_id, &firmlinks);
	assert(!getfl_res);

	return {};
}
