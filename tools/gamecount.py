#!/usr/bin/env python3
"""Count the game processes running on this machine, across every session (CLAUDE.md, "At most 10 game processes on
the machine across every session": the limit counts every session's runs, so count before starting one).

A game process is a port, a frozen baseline or a scratch build run from inside this repository or one of its worktrees,
an original's copy (from a byo-pop folder or origrun's --stage-root, e.g. E:\\byo-og), a web build under Node (the
verifier or a page driver), and a running Windows Sandbox (one, whatever runs inside). Tools are not: rundesk, origrun
itself, cmake, ninja, the compilers, git, Python, bash, Claude.

    python tools/gamecount.py            prints "game processes: N" and one line per process
    python tools/gamecount.py --limit    exits 1 when N is at the machine's limit, 10, or more (start nothing new
                                         then), 0 otherwise
    python tools/gamecount.py --limit 4  the same against a lower limit of your own (leaving room for others, or
                                         waiting for a quiet machine before a timing run)

--limit 6 is read as the machine's limit too: it was the limit until 2026-11-16 (the user: "Raise to 10"), and the
scripts written before then pass it meaning "the machine's limit", so they follow the raise without an edit.

Reservations (so two sessions cannot both pass the check before either has started its processes): with --limit,
the check is "running + live reservations + --need > limit" (--need N, default 1, is the number of processes you are
about to start; N=1 is the plain rule above). When the check passes, it records a reservation of N slots that expires
after --hold S seconds (default 30), long enough for the processes to start and show in the count. Every count adds
the live reservations to its total and prints each as "reserved: N by pid P, Ts left". Start your processes within
the hold. A pair run (an original and its port) says --need 2.

    python tools/gamecount.py --limit --need 2            check for two slots and reserve them for 30 s
    python tools/gamecount.py --limit --no-reserve        check only, as before reservations existed
    python tools/gamecount.py                             count only (never reserves)
    python tools/gamecount.py --limit 99                  a count for a log: a limit above 10 never reserves

The reservations are files under %LOCALAPPDATA%/byo-pop/gamecount/ (machine-wide: every worktree sees them). The
check and the reservation are one step under a lock directory taken by mkdir (as tools/ghidra_import.sh takes its
slots); a lock older than about 10 s is broken, and expired reservation files are deleted as they are met.
"""
import argparse, json, os, subprocess, sys, time, uuid

MACHINE_LIMIT = 10  # game processes on the machine across every session (the user, 2026-11-16)
OLD_MACHINE_LIMIT = 6  # the limit from 2026-11-11 to 2026-11-16; an older script's --limit 6 means MACHINE_LIMIT

TOOLS = {"rundesk.exe", "origrun.exe", "cmake.exe", "ninja.exe", "clang.exe", "clang++.exe", "ld.lld.exe", "lld.exe",
         "python.exe", "python3.exe", "bash.exe", "sh.exe", "git.exe", "claude.exe", "conhost.exe", "node.exe",
         "llvm-objdump.exe", "llvm-readobj.exe", "java.exe", "javaw.exe", "cygpath.exe", "timeout.exe", "sleep.exe",
         "grep.exe", "sed.exe", "awk.exe", "cat.exe", "cp.exe", "tee.exe", "make.exe", "emcc.exe", "wasm-ld.exe"}
NODE_GAME = ("wasmrun", "onegame", "play.js", "tour.js", "bejeweled_verify", "driver")

PS = ("Get-CimInstance Win32_Process | Select-Object ProcessId,Name,ExecutablePath,CommandLine,ThreadCount | "
      "ConvertTo-Json -Compress")


def state_dir():
    base = os.environ.get("LOCALAPPDATA") or os.path.join(os.path.expanduser("~"), "AppData", "Local")
    d = os.path.join(base, "byo-pop", "gamecount")
    os.makedirs(d, exist_ok=True)
    return d


def take_lock(d):
    """Take the lock directory (mkdir is atomic); a lock older than about 10 s is broken. Gives up after 15 s."""
    lock = os.path.join(d, "lock")
    end = time.time() + 15
    while True:
        try:
            os.mkdir(lock)
            return lock
        except FileExistsError:
            try:
                if time.time() - os.path.getmtime(lock) > 10:
                    os.rmdir(lock)
                    continue
            except OSError:
                pass
        if time.time() > end:
            return None
        time.sleep(0.05)


def live_reservations(d):
    """[(slots, pid, seconds left)] of the reservations not yet expired; expired files are deleted."""
    out = []
    now = time.time()
    for name in os.listdir(d):
        if not name.endswith(".res"):
            continue
        path = os.path.join(d, name)
        try:
            with open(path) as f:
                slots, pid, expires = f.read().split()
            left = float(expires) - now
            if left > 0:
                out.append((int(slots), int(pid), left))
                continue
        except (OSError, ValueError):
            pass
        try:
            os.remove(path)
        except OSError:
            pass
    return out


def main():
    ap = argparse.ArgumentParser(description="Count game processes across every session.")
    ap.add_argument("--limit", type=int, nargs="?", const=MACHINE_LIMIT,
                    help=f"exit 1 when count + reservations + --need exceeds this (alone: the machine's {MACHINE_LIMIT})")
    ap.add_argument("--need", type=int, default=1, help="slots about to be started (default 1)")
    ap.add_argument("--hold", type=int, default=30, help="seconds a reservation lasts (default 30)")
    ap.add_argument("--no-reserve", action="store_true", help="check only: record no reservation")
    args = ap.parse_args()
    if args.limit == OLD_MACHINE_LIMIT:
        args.limit = MACHINE_LIMIT
    # a limit above the machine's is a count for a log (e.g. --limit 99), never a start guard: no reservation
    reserving = args.limit is not None and args.limit <= MACHINE_LIMIT and not args.no_reserve
    d = state_dir()
    lock = take_lock(d) if reserving else None
    if reserving and lock is None:
        print("gamecount: could not take the reservation lock; checking without reserving", file=sys.stderr)
        reserving = False
    try:
        run(args, d, reserving)
    finally:
        if lock:
            try:
                os.rmdir(lock)
            except OSError:
                pass


def run(args, d, reserving):
    out = subprocess.run(["powershell", "-NoProfile", "-Command", PS], capture_output=True, text=True).stdout
    procs = json.loads(out or "[]")
    games = []
    for p in procs:
        # a process that has exited stays listed, with no threads, while another process holds a handle to it
        # (Noah's Ark, 2026-11-09: an orphaned run counted after it was stopped); it holds no slot
        if p.get("ThreadCount") == 0:
            continue
        name = (p.get("Name") or "").lower()
        path = (p.get("ExecutablePath") or "").lower().replace("/", "\\")
        cmd = (p.get("CommandLine") or "").lower()
        if name in ("windowssandbox.exe", "windowssandboxclient.exe"):
            games.append(p)
        elif name == "node.exe":
            if any(k in cmd for k in NODE_GAME):
                games.append(p)
        elif name.endswith(".exe") and name not in TOOLS and ("byo-pop" in path or "\\byo-og" in path):
            games.append(p)
    # one sandbox counts once, however many of its processes show
    seen_sandbox = False
    counted = []
    for p in games:
        if (p.get("Name") or "").lower().startswith("windowssandbox"):
            if seen_sandbox:
                continue
            seen_sandbox = True
        counted.append(p)
    res = live_reservations(d)
    held = sum(r[0] for r in res)
    print(f"game processes: {len(counted) + held}")
    for p in counted:
        print(f"  {p['ProcessId']:>6}  {p['Name']}  {(p.get('ExecutablePath') or '')[:110]}")
    for slots, pid, left in res:
        print(f"  reserved: {slots} by pid {pid}, {left:.0f}s left")
    if args.limit is not None and len(counted) + held + args.need > args.limit:
        sys.exit(1)
    if reserving:
        path = os.path.join(d, uuid.uuid4().hex + ".res")
        with open(path, "w") as f:
            f.write(f"{args.need} {os.getppid() or os.getpid()} {time.time() + args.hold}")


if __name__ == "__main__":
    main()
