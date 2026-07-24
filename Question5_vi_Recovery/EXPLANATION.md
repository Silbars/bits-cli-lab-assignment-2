# Question 5 — vi Crash Recovery: Evaluation & Recommendation

**Task:** A developer edits a critical config file in vi; the system crashes before saving. Evaluate vi's recovery mechanisms (swap files, undo history, registers, backup files, auto-recovery), then propose and justify the most reliable strategy.

**Files**
- `output.txt` — demonstration of the recovery commands.
- `screenshots/` — add your captured screenshots here.

---

## Explanation after every command (from `output.txt`)

**`vim server.conf` (initial edit)**
Opens the file for editing. As soon as the buffer is modified, vim writes a hidden swap file (`.server.conf.swp`) recording changes — this is what survives the crash.

**`ls -a`**
Reveals the `.swp` swap file sitting next to the original, confirming a recovery source exists even though the file was never saved.

**`vim server.conf` (re-open after crash) → `R`**
vim detects the leftover swap file and shows the `E325` prompt. Choosing **R**ecover reconstructs the unsaved buffer from the swap file.

**`:w`**
Saves the recovered content back to disk, making the recovery permanent; vim then removes the stale swap file.

**`vim -r server.conf`**
Recovers non-interactively straight from the swap file — useful in scripts or when you want to skip the prompt.

**`:set undofile` / `.un~`**
Persists undo history to disk so the full undo tree survives closing and reopening the file, not just swap contents.

---

## Evaluation of each mechanism

| Mechanism | What it protects | Reliability after a crash |
|-----------|------------------|---------------------------|
| **Swap file (`.swp`)** | All unsaved buffer changes, written incrementally during editing. | **Highest.** Exists on disk independent of the editor process; designed exactly for crash recovery. |
| **Auto-recovery (`-r` / E325 prompt)** | The mechanism that *reads* the swap file back. | High — but only as good as the swap file it depends on. |
| **Undo history / `undofile`** | The sequence of edits (undo tree). | Medium. In-memory undo dies with the crash; only persists if `undofile` was enabled beforehand. |
| **Backup files (`~`, `backup`/`writebackup`)** | The *previous saved* version. | Low for unsaved work — a backup is made at write time, so it holds the last saved state, not the crashed-but-unsaved edits. |
| **Registers** | Only explicitly yanked/deleted text, in-memory. | Lowest. Volatile; lost on crash unless `viminfo`/`shada` persisted them, and they never hold the whole file. |

## Recommendation

**The swap file, applied through auto-recovery, is the most reliable strategy.**

Justification:
1. **It is written continuously during editing and lives on disk**, so it captures unsaved work right up to (near) the moment of the crash — the exact scenario in the question.
2. **It is independent of the editor process**, so a crash that kills vim does not take the recovery data with it (unlike in-memory registers and undo state).
3. **Recovery is built-in and low-effort**: reopening the file triggers the `E325` prompt, or `vim -r` recovers directly.

**Defense-in-depth (best practice):** enable persistent extras in `~/.vimrc` so multiple layers exist:
```vim
set swapfile        " on by default; keep it
set undofile        " persist undo history across sessions
set backup          " keep a backup of the last saved version
```
Backup files and persistent undo are useful *complements* — backups recover the last saved version and persistent undo lets you walk back through edits — but neither replaces the swap file for capturing unsaved changes after an unexpected crash.
