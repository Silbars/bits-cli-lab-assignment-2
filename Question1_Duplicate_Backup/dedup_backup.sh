#!/usr/bin/env zsh
#
# dedup_backup.sh
# Identifies duplicate student submissions, backs up unique files,
# generates a summary report, and stores all errors separately.
#
# Usage: ./dedup_backup.sh <submissions_dir> <backup_dir>
#
set -u

SRC="${1:-submissions}"
BACKUP="${2:-backup}"
REPORT="report.txt"
ERRLOG="errors.log"

# Send every error message (stderr) of this script to a separate file.
: > "$ERRLOG"
exec 2>>"$ERRLOG"

if [ ! -d "$SRC" ]; then
    echo "ERROR: source directory '$SRC' does not exist" >&2
    echo "Source directory missing. See $ERRLOG"
    exit 1
fi

mkdir -p "$BACKUP" || { echo "ERROR: cannot create backup dir" >&2; exit 1; }

total=0
duplicates=0
backed_up=0

# Build a checksum listing: "<hash>  <path>" for every regular file.
# Sorting by hash groups content-identical files together.
HASHES=$(find "$SRC" -type f -exec md5sum {} \; 2>>"$ERRLOG" | sort)

# seen_hashes accumulates hashes we've already backed up (space-delimited).
seen_hashes=" "

while IFS= read -r line; do
    [ -z "$line" ] && continue
    total=$((total + 1))
    sum=$(printf '%s' "$line" | awk '{print $1}')
    file=$(printf '%s' "$line" | cut -d' ' -f3-)

    case "$seen_hashes" in
        *" $sum "*)
            duplicates=$((duplicates + 1))
            echo "DUPLICATE: '$file' (hash $sum already seen)"
            ;;
        *)
            seen_hashes="$seen_hashes$sum "
            if cp -p "$file" "$BACKUP/" 2>>"$ERRLOG"; then
                backed_up=$((backed_up + 1))
            else
                echo "ERROR: failed to back up '$file'" >&2
            fi
            ;;
    esac
done <<EOF
$HASHES
EOF

{
    echo "==== Submission Processing Report ===="
    echo "Files processed : $total"
    echo "Duplicates found: $duplicates"
    echo "Unique backed up: $backed_up"
    echo "======================================"
} > "$REPORT"

cat "$REPORT"
