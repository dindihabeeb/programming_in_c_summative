# Linux File Backup and Recovery Manager

A small Bash tool I wrote to take the pain out of backing up a folder and getting it back later. You point it at a directory, it makes a compressed, timestamped archive, and keeps a log of everything it does. Restoring is just picking a backup from a list.

## Running it

```bash
chmod +x backup_manager.sh
./backup_manager.sh
```

No arguments needed. Everything happens through the menu.

## Where things live

The first time it runs it sets up a working folder in your home directory:

```
~/backup_manager/
├── backups/         all the .tar.gz archives
└── activity.log     a record of every action
```

I went with `.tar.gz` instead of plain copied folders because it saves space and keeps each backup as one tidy file.

## What the menu can do

1. Create a backup. Type the path of the folder you want saved. Before it runs it shows you the free disk space so you don't fill the drive.
2. Restore a backup. Pick one from the numbered list and choose where to unpack it (defaults to the current directory).
3. View backup history. Lists every archive with its size.
4. Delete a backup. Asks for confirmation first so you can't wipe one by accident.
5. View or clear the activity log.
6. Exit.

## A few things I made sure it handles

- Typing a folder that doesn't exist, or one you can't read, gets caught with a clear message instead of a crash.
- Menu choices are checked, so letters or out-of-range numbers just get rejected and re-asked.
- If a backup file disappears between listing it and restoring it, the script notices.
- If the input stream closes (for example when piping commands in), it exits cleanly rather than looping.

## Naming

Archives are named after the source folder plus a timestamp, like:

```
Documents_backup_20260802_163333.tar.gz
```

so it's obvious what each one is and when it was made.

## Requirements

Standard Linux tools only: `bash`, `tar`, `df`, `du`. Nothing to install.
