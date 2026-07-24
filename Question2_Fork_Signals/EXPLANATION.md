# Question 2 — Fork, Monitoring, Zombie Prevention & Signals

**Task:** Design a C program for a web server that (1) creates children with `fork()`, (2) monitors them, (3) prevents zombie processes, and (4) terminates unresponsive children with signals. Explain how creation, waiting, and signal handling work together.

**Files**
- `process_manager.c` — the solution.
- `output.txt` — captured compile + run output (representative run on Linux).
- `screenshots/` — add your captured screenshots here.

**Build & run**
```
gcc -o process_manager process_manager.c
./process_manager
```

---

## Explanation after every command / key call

**`gcc -o process_manager process_manager.c`**
Compiles the program into an executable. I observed a clean build with no warnings.

**`sigaction(SIGCHLD, &sa, NULL)`**
Installs the `reap_children` handler so the kernel notifies the parent whenever a child exits. I did this so children are reaped the instant they finish, which is what stops zombies from piling up.

**`fork()` (in the loop)**
Creates each child. In the child branch it returns 0; in the parent it returns the child's PID, which I store in `children[]` for later monitoring.

**`waitpid(-1, &status, WNOHANG)`**
Non-blocking reap inside the handler. I observed it collects every finished child without making the parent hang, clearing their exit status from the process table.

**`kill(pid, 0)`**
Probes whether a child is still alive without actually signalling it. I used its return value to decide which children are unresponsive.

**`kill(pid, SIGTERM)` then `kill(pid, SIGKILL)`**
First asks the child to terminate politely; if it ignores `SIGTERM` (child 0 does, on purpose), I escalate to `SIGKILL`, which the kernel enforces and cannot be caught or ignored.

**`ps -el | grep defunct`**
Confirms no `<defunct>` (zombie) processes remain after exit. `grep` returning exit code 1 (no match) verified requirement 3.

---

## How creation, waiting, and signal handling work together

- **Creation (`fork`)** spins up worker children, mirroring how a web server spawns handlers per connection.
- **Waiting (`waitpid`)** removes a finished child's entry from the kernel process table. Without it, the exited child becomes a **zombie** — the exact failure the question describes. `WNOHANG` lets the parent reap opportunistically without blocking its monitoring loop.
- **Signal handling** ties them together: `SIGCHLD` tells the parent *when* to wait, so reaping is event-driven rather than a busy loop. For control in the other direction, `SIGTERM`/`SIGKILL` let the parent shut down a stuck child.
- **The escalation pattern (`SIGTERM` → grace period → `SIGKILL`)** is the standard, safe way to terminate: give the process a chance to clean up, then force it. This keeps the server from being starved by runaway children.

> **Async-signal-safety note:** the handler uses `write()` instead of `printf()` because `printf` is not async-signal-safe and can deadlock if interrupted mid-call.
