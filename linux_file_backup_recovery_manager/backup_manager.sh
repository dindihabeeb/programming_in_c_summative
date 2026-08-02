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

create_backup() {
    print_header "Create a Backup"

    local source_dir
    read -r -p "Directory to back up: " source_dir

    if [ ! -d "${source_dir}" ]; then
        echo "Error: '${source_dir}' is not a directory."
        log_action "CREATE FAILED - missing directory: ${source_dir}"
        pause; return
    fi
    if [ ! -r "${source_dir}" ]; then
        echo "Error: '${source_dir}' is not readable."
        log_action "CREATE FAILED - unreadable: ${source_dir}"
        pause; return
    fi

    echo ""
    show_disk_space

    local archive_name archive_path
    archive_name="$(basename "${source_dir}")_backup_$(date '+%Y%m%d_%H%M%S').tar.gz"
    archive_path="${BACKUP_DIR}/${archive_name}"

    echo "Backing up '${source_dir}'..."
    if tar -czf "${archive_path}" -C "$(dirname "${source_dir}")" "$(basename "${source_dir}")"; then
        local size; size="$(du -h "${archive_path}" | cut -f1)"
        echo "Done (${size})."
        log_action "CREATE SUCCESS - ${source_dir} -> ${archive_name} (${size})"
    else
        echo "Error: backup failed."
        rm -f "${archive_path}"
        log_action "CREATE FAILED - tar error for ${source_dir}"
    fi
    pause
}

restore_backup() {
    print_header "Restore a Backup"
    list_backups || { pause; return; }

    echo ""
    local choice
    choice="$(read_menu_choice "Backup number to restore: " "${#BACKUP_LIST[@]}")"
    [ $? -eq 2 ] && return
    if [ "${choice}" = "INVALID" ]; then
        echo "Error: invalid selection."
        log_action "RESTORE FAILED - invalid selection"
        pause; return
    fi

    local archive_path="${BACKUP_LIST[$((choice - 1))]}"
    if [ ! -f "${archive_path}" ]; then
        echo "Error: backup no longer available."
        log_action "RESTORE FAILED - missing archive: ${archive_path}"
        pause; return
    fi

    local restore_dir
    read -r -p "Restore into [default: current directory]: " restore_dir
    [ -z "${restore_dir}" ] && restore_dir="$(pwd)"

    if [ ! -d "${restore_dir}" ]; then
        read -r -p "'${restore_dir}' doesn't exist. Create it? (y/n): " yn
        if [[ "${yn}" =~ ^[Yy]$ ]]; then
            mkdir -p "${restore_dir}" || { echo "Error: cannot create it."; pause; return; }
        else
            echo "Cancelled."; pause; return
        fi
    fi

    echo "Restoring to '${restore_dir}'..."
    if tar -xzf "${archive_path}" -C "${restore_dir}"; then
        echo "Done."
        log_action "RESTORE SUCCESS - $(basename "${archive_path}") -> ${restore_dir}"
    else
        echo "Error: restore failed."
        log_action "RESTORE FAILED - tar error for $(basename "${archive_path}")"
    fi
    pause
}

view_history() {
    print_header "Backup History"
    list_backups
    pause
}

delete_backup() {
    print_header "Delete a Backup"
    list_backups || { pause; return; }

    echo ""
    local choice
    choice="$(read_menu_choice "Backup number to delete: " "${#BACKUP_LIST[@]}")"
    [ $? -eq 2 ] && return
    if [ "${choice}" = "INVALID" ]; then
        echo "Error: invalid selection."
        log_action "DELETE FAILED - invalid selection"
        pause; return
    fi

    local archive_path="${BACKUP_LIST[$((choice - 1))]}"
    local name; name="$(basename "${archive_path}")"

    read -r -p "Delete '${name}'? (y/n): " yn
    if [[ "${yn}" =~ ^[Yy]$ ]]; then
        rm -f "${archive_path}" && echo "Deleted." && log_action "DELETE SUCCESS - ${name}"
    else
        echo "Cancelled."
        log_action "DELETE CANCELLED - ${name}"
    fi
    pause
}

manage_log() {
    print_header "Activity Log"
    echo "  1) View log"
    echo "  2) Clear log"
    echo "  3) Back"
    echo ""

    local choice
    choice="$(read_menu_choice "Select: " 3)"
    [ $? -eq 2 ] && return
    case "${choice}" in
        1)
            echo ""
            if [ -s "${LOG_FILE}" ]; then cat "${LOG_FILE}"; else echo "Log is empty."; fi
            ;;
        2)
            read -r -p "Clear the log? (y/n): " yn
            if [[ "${yn}" =~ ^[Yy]$ ]]; then
                : > "${LOG_FILE}"
                log_action "LOG CLEARED"
                echo "Cleared."
            else
                echo "Cancelled."
            fi
            ;;
        3) return ;;
        *) echo "Error: invalid selection." ;;
    esac
    pause
}

main_menu() {
    while true; do
        print_header "Linux File Backup and Recovery Manager"
        echo "  1) Create a backup"
        echo "  2) Restore a backup"
        echo "  3) View backup history"
        echo "  4) Delete an existing backup"
        echo "  5) View or clear the activity log"
        echo "  6) Exit"
        echo ""

        local choice
        choice="$(read_menu_choice "Select an option (1-6): " 6)"
        if [ $? -eq 2 ]; then
            echo ""; echo "Input closed. Exiting."
            log_action "PROGRAM EXIT - end of input"
            exit 0
        fi
        if [ "${choice}" = "INVALID" ]; then
            echo "Error: choose a number from 1 to 6."
            pause; continue
        fi

        case "${choice}" in
            1) create_backup ;;
            2) restore_backup ;;
            3) view_history ;;
            4) delete_backup ;;
            5) manage_log ;;
            6) echo ""; echo "Goodbye!"; log_action "PROGRAM EXIT"; exit 0 ;;
        esac
    done
}

mkdir -p "${BACKUP_DIR}"
touch "${LOG_FILE}"
log_action "PROGRAM START"
main_menu
