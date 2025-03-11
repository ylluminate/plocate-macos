#undef NDEBUG
#include <cassert>

#include "darwin_utils.h"

#include <sys/resource.h>

void darwin_disable_dataless() {
    assert(!setiopolicy_np(IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES, IOPOL_SCOPE_PROCESS, IOPOL_MATERIALIZE_DATALESS_FILES_OFF));
}

void darwin_set_throttling(bool enable_throttling) {
    assert(!setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, enable_throttling ? IOPOL_THROTTLE : IOPOL_IMPORTANT));
}

void darwin_set_atime_updates(bool enable_atime_updates) {
    assert(!setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, enable_atime_updates ? IOPOL_ATIME_UPDATES_DEFAULT : IOPOL_ATIME_UPDATES_OFF));
}
