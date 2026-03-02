#!/bin/sh
# plocate-macos-setup.sh
# Post-install setup. Must be run as root: sudo ./plocate-macos-setup.sh

set -e

PLOCATE_GROUP="_plocate"
DB_DIR="/opt/homebrew/var/lib/plocate"
PLIST_NAME="com.plocate-macos.updatedb.plist"
PLIST_SRC="$(cd "$(dirname "$0")/.." && pwd)/${PLIST_NAME}"
PLIST_DEST="/Library/LaunchDaemons/${PLIST_NAME}"
UPDATEDB="/opt/homebrew/sbin/updatedb"
DB_PATH="${DB_DIR}/plocate.db"

# --- root check ---

if [ "$(id -u)" -ne 0 ]; then
    echo "error: this script must be run as root." >&2
    echo "       try: sudo $0" >&2
    exit 1
fi

echo "==> plocate-macos setup"

# --- create _plocate group ---

if dseditgroup -o read "${PLOCATE_GROUP}" > /dev/null 2>&1; then
    echo "    group '${PLOCATE_GROUP}' already exists, skipping"
else
    echo "    creating group '${PLOCATE_GROUP}'"
    dseditgroup -o create -r "plocate database readers" "${PLOCATE_GROUP}"
fi

# --- database directory ---

if [ ! -d "${DB_DIR}" ]; then
    echo "    creating ${DB_DIR}"
    mkdir -p "${DB_DIR}"
fi

echo "    setting permissions on ${DB_DIR}"
chown root:"${PLOCATE_GROUP}" "${DB_DIR}"
chmod 750 "${DB_DIR}"

# --- install launchd plist ---

if [ ! -f "${PLIST_DEST}" ]; then
    if [ ! -f "${PLIST_SRC}" ]; then
        echo "error: plist not found at ${PLIST_SRC}" >&2
        exit 1
    fi
    echo "    installing ${PLIST_DEST}"
    cp "${PLIST_SRC}" "${PLIST_DEST}"
    chown root:wheel "${PLIST_DEST}"
    chmod 644 "${PLIST_DEST}"
else
    echo "    ${PLIST_DEST} already installed, skipping"
fi

# --- load launchd job ---

if launchctl print "system/com.plocate-macos.updatedb" > /dev/null 2>&1; then
    echo "    launchd job already loaded, skipping"
else
    echo "    loading launchd job"
    launchctl bootstrap system "${PLIST_DEST}"
fi

# --- initial database build ---

echo "    running initial updatedb (this may take a few minutes)"
"${UPDATEDB}" --output "${DB_PATH}"

if [ -f "${DB_PATH}" ]; then
    chown root:"${PLOCATE_GROUP}" "${DB_PATH}"
    chmod 640 "${DB_PATH}"
fi

# --- summary ---

echo ""
echo "==> setup complete"
echo "    group:    ${PLOCATE_GROUP}"
echo "    database: ${DB_PATH}"
echo "    schedule: daily at 02:30 (launchd: com.plocate-macos.updatedb)"
echo "    log:      /var/log/plocate-updatedb.log"
echo ""
echo "    Add users to the '${PLOCATE_GROUP}' group to let them run plocate:"
echo "      sudo dseditgroup -o edit -a USERNAME -t user ${PLOCATE_GROUP}"
