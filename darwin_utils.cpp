#include "darwin_utils.h"

#include <cstdio>
#include <sys/resource.h>

void darwin_disable_dataless() {
	if (setiopolicy_np(IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES, IOPOL_SCOPE_PROCESS, IOPOL_MATERIALIZE_DATALESS_FILES_OFF)) {
		fprintf(stderr, "plocate: setiopolicy_np(IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES) failed\n");
	}
}

void darwin_set_throttling(bool enable_throttling) {
	if (setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, enable_throttling ? IOPOL_THROTTLE : IOPOL_IMPORTANT)) {
		fprintf(stderr, "plocate: setiopolicy_np(IOPOL_TYPE_DISK, throttle) failed\n");
	}
}

void darwin_set_atime_updates(bool enable_atime_updates) {
	if (setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, enable_atime_updates ? IOPOL_ATIME_UPDATES_DEFAULT : IOPOL_ATIME_UPDATES_OFF)) {
		fprintf(stderr, "plocate: setiopolicy_np(IOPOL_TYPE_DISK, atime) failed\n");
	}
}
