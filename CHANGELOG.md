# Changelog

All notable changes to plocate-macos relative to upstream plocate and jevinskie/plocate-xnu.

Upstream plocate changes are tracked in `NEWS`. This file covers macOS-specific additions and changes only.

---

## [Unreleased] — based on plocate 1.1.24-dev

### Added

- **macOS default configuration** (`updatedb.conf.darwin`) — sane `PRUNEFS`, `PRUNEPATHS`, and `PRUNENAMES` defaults for macOS. Excludes `/Volumes`, CoreSimulator, APFS recovery/preboot volumes, virtual memory volumes, and common noise directories (`.git`, `node_modules`, `__pycache__`, `.venv`, `.Trash`).
- **launchd plist** (`com.plocate-macos.updatedb.plist`) — scheduled updatedb at 2:30am daily under root, with `LowPriorityIO` and `Nice 10`. Does not run at load to avoid indexing on install.
- **`PRUNE_FIRMLINKS` config option** — when set to `yes`, updatedb skips APFS firmlink targets to prevent double-indexing `/System/Volumes/Data` paths that also appear under `/usr`, `/etc`, etc.
- **Homebrew formula** — available via the `ylluminate/utilities` tap (`brew tap ylluminate/utilities && brew install plocate`).
- **GitHub Actions CI** — builds on macOS (arm64 and x86_64) against current Homebrew dependencies.

### Fixed

- **SIGPIPE when piping output** — `plocate httpd | head` no longer prints "FATAL: Child died unexpectedly." The child process now detects SIGPIPE on the parent and exits cleanly instead of treating it as an unexpected death.
- **Compiled-in database path** — the Homebrew formula now passes an absolute path for `dbpath` so meson does not resolve it relative to the Cellar prefix. Without this, the compiled-in default pointed to a path inside the Cellar that would break on upgrades.

### Changed

- **Error handling in macOS compat layer** — replaced `assert()` calls that would crash updatedb on certain I/O policy failures with `fprintf(stderr, ...)` and continued execution. Silent failures on `setiopolicy_np` no longer abort the entire index run.
- **Default branch** renamed from `jev/xnu` to `main`.
- **Group name** defaults to `_plocate` on macOS (underscore prefix follows macOS convention for system-created groups).
- **DBFILE path** defaults to `/opt/homebrew/var/lib/plocate/plocate.db` on macOS Homebrew installs (arm64 prefix).
- **`locate` symlink** — the Homebrew formula installs a `locate` → `plocate` symlink in `bin/` so plocate works as a drop-in replacement. Does not conflict with findutils, which installs `glocate` in `bin/` and only provides `locate` inside `libexec/gnubin/`.

### Inherited from jevinskie/plocate-xnu

These changes were originally written by Jevin Sweval and are carried forward in this fork:

- **APFS firmlink handling** — `firmlinks_darwin.cpp` queries `APFS.framework` (private framework, TBD stub provided) to enumerate firmlinks. `is_firmlink_target()` checks whether a path is a firmlink destination so updatedb can skip it.
- **`mntent` compatibility shim** (`mntent_darwin_compat.cpp`, `mntent_darwin_compat.h`) — implements `setmntent` / `getmntent` / `endmntent` on top of `getmntinfo` for macOS, where `mntent.h` does not exist.
- **`endian.h` compatibility header** (`endian_darwin_compat.h`) — provides `htole32` / `le32toh` and friends on macOS, which uses `<machine/endian.h>` with different names.
- **macOS I/O policy controls** (`darwin_utils.cpp`) — `darwin_disable_dataless()` calls `setiopolicy_np(IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES, ...)` to prevent iCloud Drive stubs from being materialized during indexing. `darwin_set_throttling()` and `darwin_set_atime_updates()` reduce indexing impact on disk and SSD wear.
- **CoreFoundation + `APFS.framework` integration** — meson build detects the Darwin target and links the additional frameworks. The APFS TBD stub is vendored under `tbd/` to avoid a hard Xcode SDK dependency.
- **Meson option for group name** — `locategroup` option defaults to `_plocate` on Darwin, `plocate` elsewhere.

---

## Upstream plocate

See `NEWS` for upstream release history (plocate 1.0.0 through 1.1.23).
