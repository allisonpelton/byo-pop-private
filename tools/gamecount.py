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
the live reservations to its total and prints each as "reserved: N by pid P, Ts left". A slot stops counting once its
game has started (a game process started after the reservation, under the same running shell as the reserving
gamecount), so a launch counts once. Start your processes within the hold, from the shell or script that reserved
them. A pair run (an original and its port) says --need 2.

    python tools/gamecount.py --limit --need 2            check for two slots and reserve them for 30 s
    python tools/gamecount.py --limit --no-reserve        check only, as before reservations existed
    python tools/gamecount.py                             count only (never reserves)
    python tools/gamecount.py --limit 99                  a count for a log: a limit above 10 never reserves
    python tools/gamecount.py --selftest                  the reservation rules on a made-up process list

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
# processes a reservation's chain passes through on the way up to the session (see run)
SHELLS = {"bash.exe", "sh.exe", "dash.exe", "python.exe", "python3.exe", "py.exe", "rundesk.exe", "cmd.exe",
          "powershell.exe", "pwsh.exe", "timeout.exe", "env.exe", "xargs.exe", "make.exe"}
NODE_GAME = ("wasmrun", "onegame", "play.js", "tour.js", "bejeweled_verify", "driver")

PS = ("Get-CimInstance Win32_Process | Select-Object ProcessId,ParentProcessId,Name,ExecutablePath,CommandLine,"
      "ThreadCount,@{n='Created';e={[long](($_.CreationDate.ToUniversalTime() - [datetime]'1970-01-01').TotalSeconds)}} | "
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
    """[(slots, pid, seconds left, made at, chain)] of the reservations not yet expired; expired files are deleted.

    A reservation file holds "slots pid expires", the form every gamecount reads (an older one deletes a file of any
    other form as unreadable). It was made at its file's own time. Its chain (the reserving process's shell ancestors,
    nearest first) is in a .chain file beside it; without one the chain is the pid alone."""
    out = []
    now = time.time()
    names = set(os.listdir(d))
    for name in names:
        path = os.path.join(d, name)
        if name.endswith(".chain"):
            # a chain outlives its reservation only by accident (it is written first): drop it once it is a minute old
            try:
                if name[:-6] + ".res" not in names and now - os.path.getmtime(path) > 60:
                    os.remove(path)
            except OSError:
                pass
            continue
        if not name.endswith(".res"):
            continue
        try:
            with open(path) as f:
                slots, pid, expires = f.read().split()
            made = os.path.getmtime(path)
            left = float(expires) - now
            if left > 0:
                chain = [int(pid)]
                try:
                    with open(path[:-4] + ".chain") as f:
                        chain = [int(x) for x in f.read().split()] or chain
                except (OSError, ValueError):
                    pass
                out.append((int(slots), int(pid), left, made, chain))
                continue
        except (OSError, ValueError):
            pass
        for gone in (path, path[:-4] + ".chain"):
            try:
                os.remove(gone)
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
    # A reservation stops counting a slot once its game has started. Before this, each launch counted twice for up to
    # 30 s, and a script starting short runs one after another held a slot for nothing all the time (the user,
    # 2026-11-22, saw the processor mostly idle while sessions waited for slots: 7 games and 3 reservations made the
    # 10). Its game is a counted process started after the reservation that descends from the nearest of the reserving
    # process's shell ancestors still running (the pid itself is usually Git Bash's stub for the command, gone at once).
    # Each started process stands for one slot of one reservation; a reservation whose shells have all exited counts
    # in full until it expires, as before.
    alive = {p.get("ProcessId"): p for p in procs if p.get("ThreadCount") != 0}
    parent = {p.get("ProcessId"): p.get("ParentProcessId") for p in procs}

    def descends(pid, ancestor):
        for _ in range(64):
            pid = parent.get(pid)
            if not pid:
                return False
            if pid == ancestor:
                return True
        return False

    res = sorted(live_reservations(d), key=lambda r: r[3])
    taken = set()
    held, notes = 0, []
    for slots, pid, left, made, chain in res:
        anchor = next((a for a in chain if a in alive and (alive[a].get("Created") or 0) <= made + 1), None)
        started = 0
        if anchor is not None:
            for p in sorted(counted, key=lambda p: p.get("Created") or 0):
                if started == slots:
                    break
                q = p.get("ProcessId")
                if q not in taken and (p.get("Created") or 0) >= made - 1 and descends(q, anchor):
                    taken.add(q)
                    started += 1
        held += slots - started
        notes.append((slots, pid, left, started))
    print(f"game processes: {len(counted) + held}")
    for p in counted:
        print(f"  {p['ProcessId']:>6}  {p['Name']}  {(p.get('ExecutablePath') or '')[:110]}")
    for slots, pid, left, started in notes:
        print(f"  reserved: {slots} by pid {pid}, {left:.0f}s left" + (f", {started} started (counted above)" if started else ""))
    if args.limit is not None and len(counted) + held + args.need > args.limit:
        sys.exit(1)
    if reserving:
        # the chain: this process's shell ancestors, nearest first, up to the first that is not a shell (the session)
        chain, q = [], parent.get(os.getpid())
        while q and q in alive and (alive[q].get("Name") or "").lower() in SHELLS and len(chain) < 16:
            chain.append(q)
            q = parent.get(q)
        stem = os.path.join(d, uuid.uuid4().hex)
        with open(stem + ".chain", "w") as f:
            f.write(" ".join(str(c) for c in chain))
        with open(stem + ".res", "w") as f:
            f.write(f"{args.need} {os.getppid() or os.getpid()} {time.time() + args.hold}")


def selftest():
    """The reservation rules on a made-up process list in a temporary state folder (tools/check.sh unit)."""
    import contextlib
    import io
    import tempfile
    t0 = time.time()

    def proc(pid, ppid, name, created, path=None):
        return {"ProcessId": pid, "ParentProcessId": ppid, "Name": name, "ThreadCount": 1, "Created": created,
                "ExecutablePath": path or ("C:/byo-pop/build/native/" + name), "CommandLine": name}

    def count(procs, d):
        fake = type("R", (), {"stdout": json.dumps(procs)})
        real = subprocess.run
        subprocess.run = lambda *a, **k: fake
        buf = io.StringIO()
        try:
            with contextlib.redirect_stdout(buf):
                run(argparse.Namespace(limit=None, need=1, hold=30), d, False)
        finally:
            subprocess.run = real
        return int(buf.getvalue().splitlines()[0].split(": ")[1])

    def reserve(d, name, slots, pid, chain, made):
        with open(os.path.join(d, name + ".res"), "w") as f:
            f.write(f"{slots} {pid} {made + 30}")
        os.utime(os.path.join(d, name + ".res"), (made, made))
        if chain is not None:
            with open(os.path.join(d, name + ".chain"), "w") as f:
                f.write(" ".join(map(str, chain)))

    session = proc(10, 1, "claude.exe", t0 - 999, "C:/x/claude.exe")
    script = proc(20, 10, "bash.exe", t0 - 100)
    failures = []
    with tempfile.TemporaryDirectory() as d:
        # a reservation whose stub (30) is gone, its script (20) running: its game (40, started after) takes its slot
        reserve(d, "a", 1, 30, [30, 20], t0 - 5)
        procs = [session, script, proc(35, 20, "rundesk.exe", t0 - 4), proc(40, 35, "alchemy.exe", t0 - 4)]
        if count(procs, d) != 1:
            failures.append("a started game counts once, not once more for its reservation")
        # before the game starts, the reservation holds its slot
        if count(procs[:2], d) != 1:
            failures.append("a reservation whose game has not started holds its slot")
        # a game of the same script started before the reservation is not its game
        procs_old = [session, script, proc(41, 20, "atomica.exe", t0 - 50)]
        if count(procs_old, d) != 2:
            failures.append("a game older than the reservation does not take its slot")
    with tempfile.TemporaryDirectory() as d:
        # an older gamecount's reservation (no chain) by a stub that is gone counts in full, as before
        reserve(d, "b", 1, 30, None, t0 - 5)
        procs = [session, script, proc(40, 20, "alchemy.exe", t0 - 4)]
        if count(procs, d) != 2:
            failures.append("a reservation with no chain and its pid gone counts in full")
        # two reservations by one script, one game started: one slot left held
        reserve(d, "c", 1, 30, [31, 20], t0 - 6)
        reserve(d, "e", 1, 30, [32, 20], t0 - 5.5)
        if count(procs, d) != 3:
            failures.append("one game takes one slot of one reservation (of three: two held, one game)")
        # a pair: two slots, both games started
        for f in os.listdir(d):
            os.remove(os.path.join(d, f))
        reserve(d, "f", 2, 33, [33, 20], t0 - 5)
        procs = [session, script, proc(40, 20, "alchemy.exe", t0 - 4), proc(42, 20, "alchemy_orig.exe", t0 - 4)]
        if count(procs, d) != 2:
            failures.append("a pair's two games take both its slots")
        # a recycled pid: a process with the chain's pid that started after the reservation is not its shell
        for f in os.listdir(d):
            os.remove(os.path.join(d, f))
        reserve(d, "g", 1, 34, [34], t0 - 5)
        procs = [session, proc(34, 10, "bash.exe", t0 - 2), proc(43, 34, "alchemy.exe", t0 - 1)]
        if count(procs, d) != 2:
            failures.append("a shell started after the reservation (a recycled pid) is not its anchor")
    for f in failures:
        print("FAIL: " + f)
    print("gamecount selftest: %d failed" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        sys.exit(selftest())
    main()
