# The screen coverage pass's scripts (2026-11-21)

These generate the scenarios behind `docs/SCREENS.md`: saved games made one word from a level's end, one script for the
port (`<ms> <command>`) and one for the original under origrun, from one step list (`gen.py`). Copy this folder into
`build/scr/` (`cp -r games/bonnies/tools/screens/. build/scr/`) and run from the repository root; every output (saves,
shots, logs) stays in `build/` and is never committed.

- `mkend.py`, `mkbatch*.py`, `mkgx.py`, `mklk.py`, `lv.py`: the scenarios (book ends, mid-book chapters, action wins,
  game-over variants, the locked chooser, the level thumbnails and starts).
- `run_port.sh <script> <out>` runs the port under rundesk with `-fast` (`SAVES=<dir>` copies saves into its user folder);
  `orig.sh <script> <run>` runs the original in Windows Sandbox (`TMO`, `SAVES`).
- `assemble.py` pairs the shots by name into `build/shots/bonnies-screens/{orig,port}`; shotdiff then compares them.
- `screens_md.py` writes `docs/SCREENS.md`.
