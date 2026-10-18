# Working on this repo

## Running the game in tests

Every test run of a game exe must be invisible, silent and low priority, because the user is working on
the same machine. A plain run opens a window, raises it over whatever the user is doing and plays sound.

- Start every game exe (`bejeweled.exe`, `fwtest.exe`, `smoke.exe`, frozen exes under `build/baseline*`,
  scratch builds) through `build/native/rundesk.exe`, or run the script that starts them under it:
  `build/native/rundesk.exe bash my-loop.sh`. rundesk puts the command and all its children on a private
  desktop, gives them SDL's silent audio driver and below-normal priority. Build it with
  `cmake --build build/native --target rundesk`.
- `tools/oracle/compare.sh` already re-runs itself under rundesk.
- Run at most 3 game processes at a time, and never add artificial CPU load.
- Do not leave anything running: loops started in the background must end before you finish, and a stopped
  task's child processes must be killed.
- Never run `-script`/`-autoplay` runs on the user's desktop. `ORACLE_VISIBLE=1` exists only for the user.
