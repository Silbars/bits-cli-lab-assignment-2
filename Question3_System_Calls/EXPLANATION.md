# Question 3 — Secure File Processing with System Calls

**Task:** Design a utility using Linux **system calls** (not stdio) that (1) creates a file, (2) writes employee records, (3) updates specific records without rewriting the whole file, and (4) retrieves records from any location efficiently. Explain the role of `open()`, `read()`, `write()`, `lseek()`, `close()`.

**Files**
- `employee_records.c` — the solution.
- `output.txt` — captured compile + run output (representative run on Linux).
- `employees.dat` — the binary record file produced by a run.
- `screenshots/` — add your captured screenshots here.

**Build & run**
```
gcc -o employee_records employee_records.c
./employee_records
```

---

## Explanation after every key call

**`open(DB, O_RDWR | O_CREAT | O_TRUNC, 0600)`**
Creates (or truncates) the data file and returns a file descriptor for reading and writing. I set mode `0600` so only the owner can read/write, which is the "secure" part of the requirement.

**`lseek(fd, index * sizeof(record), SEEK_SET)`**
Moves the file offset directly to the start of record N. I observed this is what enables random access — no need to scan preceding records.

**`write(fd, e, sizeof(*e))`**
Writes exactly one fixed-length record at the current offset. Because record size is constant, writing at slot 1 overwrites only that slot and leaves the rest of the file untouched (requirement 3).

**`read(fd, e, sizeof(*e))`**
Reads exactly one record from the current offset into a struct. I used it to fetch record 2 directly and to re-read record 1 to confirm the update persisted.

**`close(fd)`**
Releases the file descriptor and flushes the kernel's reference. I confirmed a clean close (return 0), which prevents descriptor leaks.

**`ls -l employees.dat`**
Shows the file stayed at 3 records (144 bytes) after the update, proving the update was in place rather than a full rewrite.

---

## Role of each system call

| Call | Contribution |
|------|-------------|
| `open()` | Creates the file with secure `0600` permissions and returns the fd all other calls use. |
| `lseek()` | The key to efficiency: computes `index * record_size` and jumps straight to any record for both update and retrieval — O(1) positioning instead of O(n) scanning. |
| `write()` | Writes one fixed-size record at the current offset, enabling in-place updates. |
| `read()` | Reads one fixed-size record at the current offset for random-access retrieval. |
| `close()` | Frees the descriptor and ensures data is handed to the kernel; avoids resource leaks. |

**Why fixed-length records:** predictable byte offsets (`N * sizeof(record)`) are what make `lseek`-based direct access possible. Variable-length records would require an index or a full scan.

**Why raw system calls over stdio:** direct control of the offset and of when bytes hit the kernel, no hidden buffering — appropriate for a utility that must update precise byte ranges.
