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

show_disk_space() {
    echo "Available disk space (${BACKUP_ROOT}):"
    df -h "${BACKUP_ROOT}"
    echo ""
}

# Fill BACKUP_LIST with archive paths and print them numbered.
# Returns 1 if there are no backups.
BACKUP_LIST=()
list_backups() {
    BACKUP_LIST=()
    for file in "${BACKUP_DIR}"/*.tar.gz; do
        [ -e "${file}" ] && BACKUP_LIST+=("${file}")
    done

    if [ "${#BACKUP_LIST[@]}" -eq 0 ]; then
        echo "No backups found."
        return 1
    fi

    local i=1
    for file in "${BACKUP_LIST[@]}"; do
        printf "  %2d) %-45s (%s)\n" "${i}" "$(basename "${file}")" "$(du -h "${file}" | cut -f1)"
        i=$((i + 1))
    done
}

# Read a menu number in 1..max.
# Echoes the number (0), echoes "INVALID" (1), or returns 2 on EOF.
read_menu_choice() {
    local choice
    read -r -p "$1" choice || return 2
    if [[ "${choice}" =~ ^[0-9]+$ ]] && [ "${choice}" -ge 1 ] && [ "${choice}" -le "$2" ]; then
        echo "${choice}"
    else
        echo "INVALID"
        return 1
    fi
}

