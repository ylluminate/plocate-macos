#!/usr/bin/env bash
set -euo pipefail

# Integration tests for plocate-macos

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

# Resolve symlinks (macOS: /tmp → /private/tmp) so paths match what updatedb indexes
TMPDIR_BASE="$(mktemp -d /tmp/plocate-test.XXXXXX)"
TMPDIR_BASE="$(cd "$TMPDIR_BASE" && pwd -P)"
trap 'rm -rf "$TMPDIR_BASE"' EXIT

ok() {
    echo -e "${GREEN}PASS${NC} $1"
    PASS=$((PASS + 1))
}

fail() {
    echo -e "${RED}FAIL${NC} $1"
    FAIL=$((FAIL + 1))
}

# ── Build ────────────────────────────────────────────────────────────────────

echo -e "\n${YELLOW}==> Build${NC}"

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "  build dir missing, running meson setup..."
    (cd "$PROJECT_DIR" && meson setup build >/dev/null 2>&1)
fi

if ninja -C "$BUILD_DIR" >/dev/null 2>&1; then
    ok "meson setup + ninja"
else
    fail "meson setup + ninja"
fi

PLOCATE="$BUILD_DIR/plocate"
UPDATEDB="$BUILD_DIR/updatedb"
PLOCATE_BUILD="$BUILD_DIR/plocate-build"

# Prevent interference from system LOCATE_PATH (may point at a non-plocate db)
unset LOCATE_PATH

for bin in "$PLOCATE" "$UPDATEDB" "$PLOCATE_BUILD"; do
    name="$(basename "$bin")"
    if [ -x "$bin" ]; then
        ok "binary exists: $name"
    else
        fail "binary exists: $name"
    fi
done

# ── Database creation ────────────────────────────────────────────────────────

echo -e "\n${YELLOW}==> Database creation${NC}"

TESTROOT="$TMPDIR_BASE/testroot"
mkdir -p "$TESTROOT/alpha/beta"
mkdir -p "$TESTROOT/alpha/gamma"
touch "$TESTROOT/alpha/hello.txt"
touch "$TESTROOT/alpha/world.txt"
touch "$TESTROOT/alpha/beta/deep.log"
touch "$TESTROOT/alpha/gamma/UPPER.TXT"

DBFILE="$TMPDIR_BASE/test.db"

# No --conf flag: updatedb reads from compiled-in path (/etc/updatedb.conf),
# which doesn't exist in test, so it silently proceeds with empty prune lists.
if "$UPDATEDB" \
    --require-visibility no \
    --output "$DBFILE" \
    -U "$TESTROOT" \
    >/dev/null 2>&1; then
    ok "updatedb creates database"
else
    fail "updatedb creates database"
fi

if [ -f "$DBFILE" ]; then
    ok "database file exists"
else
    fail "database file exists"
fi

# ── Search ───────────────────────────────────────────────────────────────────

echo -e "\n${YELLOW}==> Search${NC}"

search() {
    "$PLOCATE" -d "$DBFILE" "$@" 2>/dev/null || true
}

if search "hello.txt" | grep -q "hello.txt"; then
    ok "search by exact name"
else
    fail "search by exact name"
fi

if search "deep" | grep -q "deep.log"; then
    ok "search by partial name"
else
    fail "search by partial name"
fi

if search -i "upper" | grep -q "UPPER.TXT"; then
    ok "case-insensitive search (-i)"
else
    fail "case-insensitive search (-i)"
fi

if ! search "xyzzy_nonexistent_12345" | grep -q .; then
    ok "no results for nonexistent pattern"
else
    fail "no results for nonexistent pattern"
fi

# ── Count mode ───────────────────────────────────────────────────────────────

echo -e "\n${YELLOW}==> Count mode${NC}"

COUNT=$(search -c "alpha" 2>/dev/null || echo 0)
if echo "$COUNT" | grep -Eq '^[0-9]+$' && [ "$COUNT" -gt 0 ]; then
    ok "plocate -c returns numeric count ($COUNT)"
else
    fail "plocate -c returns numeric count (got: '$COUNT')"
fi

COUNT_TXT=$(search -c ".txt" 2>/dev/null || echo 0)
if [ "$COUNT_TXT" -eq 2 ]; then
    ok "count matches expected (2 .txt files, case-sensitive)"
else
    fail "count matches expected (want 2, got $COUNT_TXT)"
fi

# ── Exclusion: PRUNENAMES via CLI ────────────────────────────────────────────

echo -e "\n${YELLOW}==> Exclusion (PRUNENAMES)${NC}"

EXCL_ROOT="$TMPDIR_BASE/exclroot"
mkdir -p "$EXCL_ROOT/project/src"
mkdir -p "$EXCL_ROOT/project/.git/objects"
mkdir -p "$EXCL_ROOT/project/node_modules/lodash"
touch "$EXCL_ROOT/project/src/main.c"
touch "$EXCL_ROOT/project/.git/HEAD"
touch "$EXCL_ROOT/project/node_modules/lodash/index.js"

EXCL_DB="$TMPDIR_BASE/excl.db"
"$UPDATEDB" \
    --require-visibility no \
    --output "$EXCL_DB" \
    --add-prunenames ".git node_modules" \
    -U "$EXCL_ROOT" \
    >/dev/null 2>&1

search_excl() {
    "$PLOCATE" -d "$EXCL_DB" "$@" 2>/dev/null || true
}

if search_excl "main.c" | grep -q "main.c"; then
    ok "non-excluded file is searchable"
else
    fail "non-excluded file is searchable"
fi

if ! search_excl "HEAD" | grep -q ".git"; then
    ok ".git excluded by PRUNENAMES"
else
    fail ".git excluded by PRUNENAMES"
fi

if ! search_excl "index.js" | grep -q "node_modules"; then
    ok "node_modules excluded by PRUNENAMES"
else
    fail "node_modules excluded by PRUNENAMES"
fi

# ── Exclusion: PRUNEPATHS via CLI ────────────────────────────────────────────

echo -e "\n${YELLOW}==> Exclusion (PRUNEPATHS)${NC}"

VOL_ROOT="$TMPDIR_BASE/volroot"
FAKE_VOLUMES="$VOL_ROOT/Volumes"
mkdir -p "$VOL_ROOT/local/data"
mkdir -p "$FAKE_VOLUMES/ExternalDisk/photos"
touch "$VOL_ROOT/local/data/local-file.txt"
touch "$FAKE_VOLUMES/ExternalDisk/photos/vacation.jpg"

VOL_DB="$TMPDIR_BASE/vol.db"
"$UPDATEDB" \
    --require-visibility no \
    --output "$VOL_DB" \
    --add-prunepaths "$FAKE_VOLUMES" \
    -U "$VOL_ROOT" \
    >/dev/null 2>&1

search_vol() {
    "$PLOCATE" -d "$VOL_DB" "$@" 2>/dev/null || true
}

if search_vol "local-file.txt" | grep -q "local-file.txt"; then
    ok "non-excluded path is searchable"
else
    fail "non-excluded path is searchable"
fi

if ! search_vol "vacation.jpg" | grep -q "vacation.jpg"; then
    ok "PRUNEPATHS volume excluded"
else
    fail "PRUNEPATHS volume excluded"
fi

# ── Summary ──────────────────────────────────────────────────────────────────

echo ""
echo "─────────────────────────────────"
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo "─────────────────────────────────"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
