#!/usr/bin/env bash
#
# Linux File Backup and Recovery Manager
# Creates, restores, and manages timestamped .tar.gz backups with an
# interactive menu and an activity log. Backups live under ~/backup_manager.

set -u

BACKUP_ROOT="${HOME}/backup_manager"
BACKUP_DIR="${BACKUP_ROOT}/backups"
LOG_FILE="${BACKUP_ROOT}/activity.log"

log_action() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "${LOG_FILE}"
}

pause() {
    echo ""
    read -r -p "Press Enter to continue..." _
}

print_header() {
    echo ""
    echo "=================================================="
    echo "  $1"
    echo "=================================================="
}

