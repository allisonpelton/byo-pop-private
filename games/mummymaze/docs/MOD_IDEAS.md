# Mummy Maze Deluxe: mods worth making

None is made: the order of work (CLAUDE.md) gives no further game mods until every PopCap game works in the browser.
These are the ideas, each with the place in the code it would touch. The game is turn based: the explorer takes one
step, then the scorpion and the mummies answer, so a rule change never touches the frame loop.

**Hint key and a danger view.** The Ankh already asks a solver whether the maze can still be won (`Board::CheckSolvable`,
0x411640, and its search `SolveRec`, 0x411a90), and Abandon Hope plays the shortest way out (`Board::Solve`, 0x4050b0).
Both search the graph of chase states depth first. A key that shows only the next step of that solution (the solver
leaves it in `mSolution`) is a small step, and so is a view of where each mummy will stand after the player's next move:
`Board::PlanMummies` (0x410090) is deterministic, so the cells can be drawn before the player decides. The autoplay bot
(`src/testhooks/Autoplay.cpp`) already turns a solution into key presses.

**Unlimited undo and a rewind.** The undo list (`Board::PushUndo`, 0x404a50, and `Undo`, 0x404be0) keeps every step of a
level; the side panel's button only takes one back. A rewind key that takes back to a chosen step, or a setting that
lets Undo cost nothing in the classic score, touch only the panel's button handler and `Board::Undo`.

**More mazes.** The mazes are plain data: `data/mazes/B-<n>.dat`, 101 files of 100 mazes each (the file format is in
`docs/GAME_NOTES.md`), picked at random for the classic game except the pairs in the table of excluded ones
(`kExcluded`, 0x48fb60) and fixed per chamber in the adventure (`kAdventureMaze` and `kAdventureLevel`, 15 x 15 each).
A mod can add files, and a small maze editor or a generator that keeps a maze solvable (the solver is there to check it)
would give the game endless chambers. The table of the adventure could be shuffled by a date for a daily adventure, and
the classic game's picks seeded by the date for a daily maze (the picks use `rand()` only).

**Tuning the rules.** The numbers are in plain code: two mummies that move twice per turn, one scorpion that moves once,
one gate and key, two skull traps, the 15 x 15 pyramid layout, the bonus formula of the classic score (base
`(pyramid + 2) * 50 + chamber * 5`, bonus by the time against `pyramid * 20 + 50` seconds). A settings dialog could change
how many mummies, how fast, and whether the white mummies prefer the horizontal or the vertical step (the red ones do
the opposite). Par times for each chamber, shown as a ghost clock, fit the bonus formula.

**Look and sound.** Every picture is a file in `images`, so a re-skin (mummy colours for players who cannot tell the white
mummy from the red one, a new explorer) needs no code. The mix of music orders (`MummyApp::SetMusicOrder`, 0x41e9e0)
lets a mod swap the module for another.

**Controls.** The board takes the arrow keys and Space, plus clicks on the neighbouring cell
(`Board::ProcessClick`, 0x4065b0). A drag along a path (several steps queued) or WASD are small additions to
`Board::UpdateExplorer` (0x406f10).
