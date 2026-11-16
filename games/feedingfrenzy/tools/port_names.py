#!/usr/bin/env python3
"""The names a finished port gives the original's functions, as a symbol file for tools/funcmatch.py --symbols or
ApplySymbols.java: one line a function, {"addr", "name", "comment"}, read from the address comments above the port's
definitions the way tools/coverage.py reads them (docs/COVERAGE.md).

    python -I games/feedingfrenzy/tools/port_names.py <game> <out.jsonl>

Used to carry Word Harmony's names for Sprout's engine and the engine's Windows layer over to Feeding Frenzy's export
through the two exes' function match (docs/README.md, "The export").
"""
import importlib.util
import json
import os
import subprocess
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    game, out = sys.argv[1], sys.argv[2]
    spec = importlib.util.spec_from_file_location("coverage", os.path.join(REPO, "tools", "coverage.py"))
    coverage = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(coverage)
    cfg = coverage.settings(game)
    # a game with no coverage.txt yet: the shared engine's names are the ones another game takes
    # (only the engine's names are carried to another game: its game classes are that game's own)
    cfg['src'] = ['engines/sprout']
    export = cfg["export"] if os.path.isabs(cfg["export"]) else os.path.join(REPO, cfg["export"])
    if not os.path.isfile(export):   # a worktree: the export is in the main checkout's re/
        common = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=REPO, text=True).strip()
        export = os.path.join(os.path.dirname(os.path.abspath(os.path.join(REPO, common))), cfg["export"])
    cfg["export"] = export
    fns = coverage.load_export(cfg)
    sites = coverage.port_sites(cfg, fns)
    count = 0
    with open(out, "w", encoding="utf-8") as f:
        for addr in sorted(sites):
            defs = [s for s in sites[addr] if s[0] == "def" and s[3]]
            if not defs:
                continue
            # a definition in a .cpp file names the function with its class; prefer it to a header's declaration
            defs.sort(key=lambda s: (("::" not in s[3]), not s[1].endswith(".cpp")))
            kind, path, line, name = defs[0]
            f.write(json.dumps({"addr": "%08x" % addr, "name": name, "comment": "%s:%d" % (path, line)}) + "\n")
            count += 1
    print("%s: %d named functions" % (game, count))


if __name__ == "__main__":
    main()
