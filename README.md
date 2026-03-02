# plocate-macos

Fast file search for macOS using posting lists and trigram indexing. A native macOS port of [plocate](https://plocate.sesse.net/), the locate replacement that is significantly faster than GNU locate and mlocate.

If you have ever typed `locate somefile` on macOS and gotten nothing back, or been frustrated that Spotlight (`mdfind`) is not scriptable, this fills that gap. `plocate` builds a local index of your filesystem, keeps it updated via launchd, and answers queries in milliseconds regardless of filesystem size.

## Features

- **Fast queries** — trigram index with posting lists means most searches touch only a small fraction of the database
- **Small index** — Zstd-compressed filename storage, shared dictionary; typically 2-3x smaller than mlocate's database
- **APFS-aware** — skips firmlinks to avoid indexing `/System/Volumes/Data` twice
- **macOS I/O policy** — throttles disk I/O and disables atime writes during indexing so updatedb does not compete with your work
- **launchd scheduling** — runs at 2:30am daily at low priority; no systemd required
- **Sane defaults** — `/Volumes`, CoreSimulator, node_modules, `.git`, and other high-noise paths excluded out of the box
- **mlocate-compatible flags** — `-b`, `-e`, `-i`, `-l`, `-r`, `-0`, `--existing`, `--regex`; drop-in for scripts that call `locate`
- **Opt-in volume indexing** — external drives excluded by default; add them individually if you want

## Install

```bash
brew tap ylluminate/utilities
brew install plocate
```

After install, build the initial database (requires sudo):

```bash
sudo updatedb
```

The launchd agent keeps the database current after that. To load it immediately:

```bash
sudo launchctl load /Library/LaunchDaemons/com.plocate-macos.updatedb.plist
```

## Quick Start

```bash
# Find any file with "httpd" in the name
plocate httpd

# Case-insensitive search
plocate -i readme

# Basename only (skip path components)
plocate -b '*.conf'

# Regex search across your home directory
plocate -r '/Users/you/.*\.log$'

# Print results separated by null bytes (for xargs -0)
plocate -0 libssl | xargs -0 ls -la
```

Queries that would take seconds with `find` return in under 100ms on typical setups.

## Configuration

The configuration file lives at:

```
/opt/homebrew/etc/updatedb.conf
```

The key options:

```bash
# Filesystem types to skip (network mounts, virtual filesystems)
PRUNEFS="devfs autofs nullfs smbfs nfs nfs4 afpfs ..."

# Paths to skip entirely
PRUNEPATHS="/Volumes /System/Volumes/VM /cores /Library/Developer/CoreSimulator ..."

# Directory names to skip at any depth
PRUNENAMES=".git node_modules __pycache__ .venv .Trash ..."

# Skip APFS firmlinks (prevents /System/Volumes/Data double-indexing)
PRUNE_FIRMLINKS="yes"
```

To index an external volume, remove `/Volumes` from `PRUNEPATHS` and add the specific paths you want to exclude instead. Or build a separate database:

```bash
sudo updatedb -U /Volumes/MyDrive -o /opt/homebrew/var/lib/plocate/myvolume.db
plocate -d /opt/homebrew/var/lib/plocate/myvolume.db something
```

The `LOCATE_PATH` environment variable accepts colon-separated database paths if you want plocate to search multiple databases by default.

## How It Works

plocate indexes every filename on your filesystem into a trigram posting list. A trigram is any sequence of three consecutive characters — "foo" → `foo`, "oof`. Each trigram maps to a sorted list of document IDs that contain it. At query time, plocate intersects the posting lists for all trigrams in your search term, giving it a small candidate set to check.

Filenames are stored Zstd-compressed with a shared dictionary trained on a sample of your filesystem. This cuts database size significantly compared to mlocate's plain gzip approach.

APFS firmlinks (the mechanism that makes `/usr` and `/etc` appear to exist while actually living under `/System/Volumes/Data`) are detected via `APFS.framework` so that updatedb does not count the same inode twice.

During indexing, darwin I/O policies (`setiopolicy_np`) disable atime updates and set disk throttling so the scan runs below normal priority.

## Comparison

| Feature | plocate-macos | GNU locate | Spotlight (`mdfind`) |
|---|---|---|---|
| Query speed | Very fast (trigrams) | Slow (linear scan) | Fast |
| Index size | Small (Zstd + trigrams) | Large | Large (proprietary) |
| Privacy | Local, you control it | Local | Apple-managed |
| Scriptable | Yes | Yes | Mostly |
| Regex support | Yes | Limited | No |
| APFS firmlinks | Handled | Not aware | Handled |
| Offline volumes | Indexed when mounted | Indexed when mounted | Not searchable |
| Update mechanism | launchd (daily) | cron/systemd | Continuous (kernel events) |
| macOS native | Yes | No | Yes |

Spotlight is faster for GUI-style "find something now" use cases because it indexes in real time. plocate is better for scripting, regex searches, and situations where you want predictable, controllable behavior and do not want Apple's indexer involved.

## Building from Source

Requires a C++17 compiler, Zstd, and meson.

```bash
brew install meson ninja pkg-config zstd
git clone https://github.com/ylluminate/plocate-macos.git
cd plocate-macos
meson setup build
cd build
ninja
sudo ninja install
```

The build links against `APFS.framework` for firmlink detection. The TBD stub is included in the repository under `tbd/` so you do not need the full Xcode SDK.

## Credits

- [Steinar H. Gunderson](https://sesse.net/) — original plocate design and implementation
- [Jevin Sweval](https://github.com/jevinskie) — initial macOS/XNU port (jevinskie/plocate-xnu)
- [ylluminate](https://github.com/ylluminate) — plocate-macos: Homebrew formula, launchd integration, macOS defaults, error handling fixes

## License

GPLv2 or later. See `COPYING`.
