#!/usr/bin/env bash
#
# log_monitor.sh
# Continuously watches a log file, shows new lines in real time,
# extracts ERROR entries into a separate report, and suppresses noise.
#
# Usage: ./log_monitor.sh /path/to/app.log
#
set -u

LOG="${1:-app.log}"
REPORT="error_report.txt"

# Core pipeline:
#   tail -F      -> stream newly appended lines in real time (follows rotation)
#   grep --line-buffered "ERROR"
#                -> keep only ERROR lines; line-buffered so they appear instantly
#   tee -a       -> append matches to the report AND pass them to the terminal
#   2>/dev/null  -> discard tail's "file truncated / cannot open" noise
tail -F "$LOG" 2>/dev/null \
    | grep --line-buffered "ERROR" \
    | tee -a "$REPORT"
