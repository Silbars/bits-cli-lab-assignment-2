# Question 4 — Real-Time Log Monitoring Pipeline

**Task:** Build a monitoring tool that (1) displays newly added log entries in real time, (2) extracts ERROR messages, (3) maintains a separate report file, and (4) suppresses unnecessary output. Explain how pipes, `grep`, `tail`, redirection, and `/dev/null` improve efficiency.

**Files**
- `log_monitor.sh` — the pipeline.
- `app.log` — sample log used to demonstrate it.
- `output.txt` — captured commands and outputs (representative run on Linux).
- `error_report.txt` — the accumulated ERROR report produced by a run.
- `screenshots/` — add your captured screenshots here.

**Run**
```
bash log_monitor.sh app.log
# in another terminal, append lines:  echo "... ERROR ..." >> app.log
```

The one-line pipeline:
```
tail -F "$LOG" 2>/dev/null | grep --line-buffered "ERROR" | tee -a error_report.txt
```

---

## Explanation after every component

**`tail -F "$LOG"`**
Streams the log and blocks waiting for new lines, printing them as they are appended — this is the real-time display (requirement 1). `-F` (vs `-f`) also re-opens the file after log rotation, so monitoring survives a rotate.

**`2>/dev/null`**
Redirects `tail`'s error stream to the null device. I did this so warnings like "file truncated" during rotation don't clutter the useful output (requirement 4).

**`| grep --line-buffered "ERROR"`**
The pipe feeds each new line into `grep`, which keeps only ERROR entries (requirement 2). `--line-buffered` flushes each match immediately so alerts aren't held back by block buffering — important for real-time monitoring.

**`| tee -a error_report.txt`**
`tee` appends matched ERROR lines to the report file *and* echoes them to the terminal at the same time, satisfying requirement 3 without a second pass over the data.

**`cat error_report.txt`**
Confirms the report contains only accumulated ERROR lines.

---

## How pipes, grep, tail, redirection & /dev/null improve efficiency

- **Pipes (`|`)** connect the stages into a stream: each line flows `tail → grep → tee` in memory, with no temporary files and no waiting for the whole log. This is O(new lines), not O(whole file).
- **`tail -F`** avoids re-reading the file repeatedly; the kernel wakes it only when new data arrives, so idle cost is near zero.
- **`grep`** filters at the source so downstream stages and the report only ever see relevant lines — less I/O, smaller report.
- **`tee -a`** writes to the report and the screen in a single pass instead of running the filter twice.
- **`/dev/null`** discards diagnostic noise cheaply (the kernel drops the bytes), keeping the signal clean.
- **`--line-buffered`** trades a tiny amount of buffering efficiency for immediacy, which is the right call for a live monitor.

> **Tip:** To also capture `Fatal`/`Critical`, widen the filter with `grep -E "ERROR|FATAL|CRITICAL"`.
