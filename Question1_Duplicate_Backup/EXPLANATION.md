# Question 1 — Duplicate Detection, Backup & Reporting

**Task:** On a Linux server holding hundreds of student submissions, (1) identify duplicates, (2) back up unique submissions, (3) generate a report of files processed / duplicated / backed up, and (4) store all error messages separately.

**Files in this folder**
- `dedup_backup.sh` — the zsh solution script.
- `output.txt` — captured commands and outputs (representative run on Linux).
- `report.txt`, `errors.log`, `backup/` — artifacts produced by a run.
- `submissions/` — sample input used to demonstrate the script.
- `screenshots/` — add your captured screenshots here.

---

## Explanation after every command

**`exec 2>>"$ERRLOG"`**
Redirects the script's entire standard error stream to `errors.log`. I did this so every error message from any later command lands in one separate file, satisfying requirement 4.

**`find "$SRC" -type f -exec md5sum {} \;`**
Walks the whole submissions tree and computes an MD5 checksum for every regular file. I observed that identical file *content* yields the same hash even when filenames differ, which is exactly how duplicate submissions are detected.

**`| sort`**
Sorts the `hash  path` lines so files with the same checksum sit next to each other. This makes grouping duplicates deterministic and cheap.

**`case "$seen_hashes" in *" $sum "*) ... ;;`**
Checks whether a checksum was already recorded. When observed, I count it as a duplicate; otherwise I record the hash and treat the file as unique.

**`cp -p "$file" "$BACKUP/"`**
Copies each unique file into the backup directory, preserving mode and timestamps (`-p`). I confirmed only one copy per unique content reaches `backup/`.

**Report block (`{ ... } > "$REPORT"`)**
Groups several `echo` statements and redirects them once into `report.txt`, producing the processed/duplicated/backed-up summary required by point 3.

---

## Justification of commands, redirection & file handling

- **`md5sum`** — content hashing is the correct way to catch duplicates that were renamed; comparing filenames alone would miss `carol_copy.txt`.
- **`find -type f`** — recursively reaches submissions in nested student folders, not just the top level.
- **`sort`** — groups equal hashes so the duplicate test is reliable.
- **`>` vs `>>`** — `>` writes a fresh report each run; `>>` appends errors so nothing is lost.
- **`2>`/`exec 2>>`** — separates stderr from stdout so the report stays clean and errors are auditable in one place (requirement 4).
- **`cp -p`** — preserves metadata so the backup is a faithful copy.

> **Note:** MD5 is used for *content-identity* detection here, not security. For tamper-evidence use `sha256sum` instead.
