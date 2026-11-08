#!/usr/bin/env python3
"""Count the game processes running on this machine, across every session (CLAUDE.md, "Run at most 3 game processes
at a time": the limit counts every session's runs, so count before starting one).

A game process is a port, a frozen baseline or a scratch build run from inside this repository or one of its worktrees,
an original's copy (from a byo-pop folder or origrun's --stage-root, e.g. C:\\byo-og), a web build under Node (the
verifier or a page driver), and a running Windows Sandbox (one, whatever runs inside). Tools are not: rundesk, origrun
itself, cmake, ninja, the compilers, git, Python, bash, Claude.

    python tools/gamecount.py            prints "game processes: N" and one line per process
    python tools/gamecount.py --limit 8  exits 1 when N is 8 or more (start nothing new then), 0 otherwise
"""
import argparse, json, subprocess, sys

TOOLS = {"rundesk.exe", "origrun.exe", "cmake.exe", "ninja.exe", "clang.exe", "clang++.exe", "ld.lld.exe", "lld.exe",
         "python.exe", "python3.exe", "bash.exe", "sh.exe", "git.exe", "claude.exe", "conhost.exe", "node.exe",
         "llvm-objdump.exe", "llvm-readobj.exe", "java.exe", "javaw.exe", "cygpath.exe", "timeout.exe", "sleep.exe",
         "grep.exe", "sed.exe", "awk.exe", "cat.exe", "cp.exe", "tee.exe", "make.exe", "emcc.exe", "wasm-ld.exe"}
NODE_GAME = ("wasmrun", "onegame", "play.js", "tour.js", "bejeweled_verify", "driver")

PS = ("Get-CimInstance Win32_Process | Select-Object ProcessId,Name,ExecutablePath,CommandLine | "
      "ConvertTo-Json -Compress")


def main():
    ap = argparse.ArgumentParser(description="Count game processes across every session.")
    ap.add_argument("--limit", type=int, help="exit 1 when the count is at or over this")
    args = ap.parse_args()
    out = subprocess.run(["powershell", "-NoProfile", "-Command", PS], capture_output=True, text=True).stdout
    procs = json.loads(out or "[]")
    games = []
    for p in procs:
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
    print(f"game processes: {len(counted)}")
    for p in counted:
        print(f"  {p['ProcessId']:>6}  {p['Name']}  {(p.get('ExecutablePath') or '')[:110]}")
    if args.limit is not None and len(counted) >= args.limit:
        sys.exit(1)


if __name__ == "__main__":
    main()
