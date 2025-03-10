#include "firmlinks_darwin.h"

#include <CoreFoundation/CoreFoundation.h>
#include <cstddef>
#include <MacTypes.h>
#include <sys/sysctl.h>
#include <uuid/uuid.h>

static constexpr size_t UUID_STR_BUF_LEN_W_NUL = 37;

extern "C"
OSStatus APFSContainerVolumeGroupGetFirmlinks(
    const char *disk, uuid_t volume_group_id,
    CFMutableArrayRef *firmlinks);

std::vector<FirmlinkPair> get_firmlinks(void) {
    char uuid_str[UUID_STR_BUF_LEN_W_NUL] = {0};
    size_t uuid_str_sz = sizeof(uuid_str);
    assert(!sysctlbyname("kern.bootuuid", uuid_str, &uuid_str_sz, nullptr, 0));
    uuid_t vol_group_id;
    assert(!uuid_parse(uuid_str, vol_group_id));
    CFMutableArrayRef firmlinks = NULL;
    const OSStatus getfl_res =
        APFSContainerVolumeGroupGetFirmlinks("drive3" /* FIXME */, vol_group_id, &firmlinks);
    return {};
}
