# Contributing

## Building

```bash
brew install meson ninja pkg-config zstd
meson setup build
cd build
ninja
```

The build requires macOS (the APFS firmlink code is Darwin-only). Xcode command line tools or a full Xcode install is needed for `CoreFoundation`. The APFS TBD stub is vendored, so you do not need a private SDK.

## Testing

Run the test script from the repository root:

```bash
./tests/test-basic.sh
```

Tests require a built binary in `build/`. They cover basic query correctness, `--existing`, `-b`/`--basename`, `-i`, `-r`/`--regex`, and null-separated output.

If you are changing updatedb or the database builder, rebuild the index with `sudo ./build/updatedb` and verify results look sane before running tests.

## What We Accept

- Bug fixes with a clear description of what was broken and how to reproduce it
- macOS-specific improvements (launchd, APFS, I/O policy, Homebrew formula)
- Compatibility fixes for new macOS versions
- Upstream merges from plocate (Steinar H. Gunderson's repository)

We are not the right place for general plocate feature requests. File those upstream at https://salsa.debian.org/codehelp/plocate.

## Code Style

Follow the existing style. The upstream code uses clang-format (version 18.x was used for the last upstream format pass). Run `clang-format -i` on any files you touch.

macOS-specific files (`*_darwin*`, `darwin_utils*`) should guard against non-Apple builds with `#ifndef __APPLE__` / `#error`.

## License

Contributions must be licensed under GPLv2 or later to match the rest of the codebase. Do not include code from incompatibly-licensed projects.
