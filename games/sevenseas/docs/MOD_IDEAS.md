# Seven Seas Deluxe: mods worth making

None is made: the order of work (CLAUDE.md) gives no further game mods until every PopCap game works in the browser.
These are the ideas, each with the place in the code it would touch. The board is turn based, so a rule change does not
touch the frame loop.

**Danger view and a move helper.** Show the cells the enemy ships will reach on their next turn (their plans are made in
`Board::PlanEnemyMoves`, 0x404bb0, before they sail) and shade the cells the player must not enter. The autoplay bot
(`src/testhooks/Autoplay.cpp`) already scores the neighbouring cells by safety and by targets in the broadside, so a key
that makes the bot's move for the player (or only highlights it) is a small step. It touches `Board::Draw` and the input.

**Unlimited undo.** The undo list (`Board::SaveUndo`, 0x40aec0, and `Undo`, 0x40b0b0) is cleared by a whirlpool and by a
lost life (`ClearUndo` with a reason, the "cannot undo" messages). A mod could keep the list through them or take back a
whole level.

**Fleet and difficulty settings.** The ships of a level come from a short formula of the level's rank in
`Board::NewLevel` (0x401580): `rank = level + (difficulty - 2) * 10`, `rank / 10 + 4` pirates, the last digit of the
rank adding extra ships, red pirates, blue pirates, ghost ships or a sea monster. The number of lives (3), the points per ship, the
bonus per pirate (10) and the broadside's reach (3 cells) are plain numbers in the board code, so a settings dialog could
change them.

**Daily board.** The same levels for everyone on a day: seed `CrtSRand` with the date when a level is generated (the
generator draws from `rand()` only).

**Whirlpools.** The corner whirlpools could be a setting: how many, and whether they open and close
(`Board::ToggleWhirls`).

**The treasure hunt.** The bonus screen (a 9 x 9 dig on every tenth level) buries one treasure and some pirate graves after the
first click (`Board::SetupBonus`, 0x402540); a mod could bury two treasures, give a limited number of digs, or
show the distance as a hint.

**Ranks and texts.** The rank names and thresholds are tables in the high-score screen and the next-level dialog. All of the
game's texts are read by address from the exe (`GameText.h`), so a translation or a different ladder needs a text
override, not code.

**Retro mode.** An option that draws the board without the moving waves (`Board::UpdateWaves`) and the shadows, for slow
browsers.
