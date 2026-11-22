# Translating a function of Peggle Deluxe

How the translation of the exe's code is done here, so that a function reads the same whoever (or whichever session) writes it.

1. **Read the code, not only the decompile.** `python games/peggle/tools/decread.py <addr>...` prints a function with names. Ghidra
   loses x87 maths (`fld`, `fmul`, ...) and the arguments of calls that take floats or many pushes, so the real order and
   values are read from `python games/peggle/tools/disasm.py <addr> [<from> <to>] [--math]`. Stack pushes read right to left; a
   later push is an earlier argument. A function that draws or moves things is checked with the disassembly every time.
2. **ModVal calls are named constants.** `ModValInt(0, "File.cppLINE,COL", default)` returns the default (the tweaking is not in
   the exe): `python games/peggle/tools/modvals.py` lists them. A constant gets a name that says what it is and a comment with
   the source line (`// line 518`). A ModVal that is 0 and gates a block is dead code: left out and listed in `GAME_NOTES.md`.
3. **Floats.** `ftol` (0x59c5a0) truncates. The x87 keeps extended precision between instructions and rounds when it stores, so
   a store to a `float` local is a `(float)` cast in the translation; `sin` is 0x59c070, `cos` 0x59bdd0, `floor` 0x59bec0.
4. **A secondary base** (`ButtonListener`, `EditListener`, `DialogListener`) has its `this` at the object plus the base's offset
   (`MainMenu`'s ButtonListener is at +0x88), so a method of it reads its object's fields at offsets less 0x88.
5. **Names.** No unknown name: a field is named by what it does, found from its users; `mUnknownNN` is a stop on the way and is
   listed in the notes until renamed. Loops keep their shape (a Sonnet session translates loops as they are). A comment that says
   what the original does is checked against the code first.
6. **The Mac build** gives names, types and class boundaries only (`re/peggle/deluxe/macmatch/peggle.match.jsonl`); a name from the
   call pass (score under 0.9) can be wrong (`LogicMgr::KeyDown` at 0x437350 is a stub, `GetMusicOrder` returns the row,
   `UpsellScreen::SetState` is a string assign). The PC exe is the authority for behaviour.
7. **Check by running.** A screen or dialog is run under `build/native/rundesk.exe` with a `-script` (`profile`, `click`, `move`,
   `screenshot`) and looked at; a function that is not reached yet is `NOT_TRANSLATED("...")`, which stops the game with its name.
