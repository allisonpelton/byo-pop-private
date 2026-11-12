# Venice Deluxe: game notes

What the port does differently from the original, what it leaves out, and what is odd. The analysis state is in
`WORKING_NOTES.md`; the code is the translation of `VeniceGame.exe` (Retro64 and PopCap, 2007), written to
`docs/CODE_STANDARDS.md` from its first draft.

## Test commands

Reachable only from a test (`-script`, `-commands`, the web page's test call; `docs/TEST_COMMANDS.md`). Replies go to stdout:
`STATE {json}`, `OK <command>`, `ERROR <command>: <reason>`.

| command | what it does |
| --- | --- |
| `state` | `{screen, scenes, mode, modename, level, score, lives, profile, over}` (`over`: a summary screen is up, or the level in play is won or lost) and, while a level is in play, `timer` (updates), `moves` (shots fired) and `board` (`plates`, `slots`, `filled`, `middle`, `shooter`, `hits`) |
| `goto <screen>` | `menu`, `options`, `help`, `highscores`, `credits`, a mode (`journey`, `flood`, `survival`, `trickshot`) or the name of any screen the game has; the scenes there are killed |
| `level <n>` | starts level n of the current mode (a trick shot's number in Trick Shot) over the scenes there |
| `unlock all` | the flags that unlock Flood, Trick Shot and Survival and every Journey and Flood level |
| `set score\|mode\|level\|lives <v>` | the profile's score, mode, level or tries |
| `win`, `lose` | the level in play goes to its victory stage, or is lost as a shot that fell is |
| `seed <n>` | seeds the game's random numbers |
| `move` | `MOVE {"kind","input"}` for the bot's next shot (`Bot.cpp`), a `wait` while the level is not ready for one, or `MOVE null` |
| `memreport` | the framework's, `MEMREPORT {...}` |

The commands wait for a player: the name screen is up until a name is typed (`key T` ... then `keydown RETURN` and `keyup RETURN`
a few updates apart, since a button's alt key needs both). The bot reads the game's state and draws no random number. The shooter always points straight up and follows the pointer's x, so a shot is an x: the bot aims at the lowest unfilled slot on the screen that the ready jewel fills (any for a wild one), moves the pointer there, waits for the shooter to catch up (it closes 1/1.3 of the distance an update), presses, waits two updates and releases; it also clicks away the cupid's messages. It does not allow for plates that move or turn, nor for bumpers, and it never swaps jewels or uses a power-up, so it fills about half of what it shoots at (`tools/playbymove` on Journey level 5 made 26 shots, 11 hits and lost the level). The press is held two updates because the game reads the button's state once an update: a press and release that both come between two updates (the burst a posted click is) is not seen, as in the original.

## Deviations (each without a visible effect)

- Loading is done in slices of a few files a frame with the window drawing meanwhile, pictures at first use and sounds at
  their first play (decoded then); the original reads and decodes everything on a thread while the loading bar draws.
- The layer merge (0x40de30) keeps the original's structure: positions reset from the stage start values, the content
  height measured, the filler pictures of the mirrored backdrop added on a display that is not accelerated. It does not bake
  the pictures into 800 x 1024 textures (the same pixels are drawn from the pictures that stay alive).
- No pool of 4000 objects (the original's limit); objects are heap objects.
- `Emitter` guards a division by zero.
- The name box does not write the typed text back into the translator's static buffer.
- `DrawTextOnObject` keeps its text object in a heap static; `PlaySoundByName` and the checkbox's hot sound do nothing for a
  sound that is not there; `FormatScore` has static buffers; the tooltip buffer is as long as its text.
- `Slot::Load` frees the siblings it removes when they are not selectable; `Warp` and the quotes and tips use
  `std::vector`; a quote number out of range shows none.
- The volume setters ignore the original's flag at 0x5b51cc (it is never set in play).
- `UserDataDisplay` draws nothing for a kind of display the game never makes; an unknown slider kind starts at the left.
- Switching to a window does not refuse a desktop that is neither 16 nor 32 bits.
- `MissTrickShot` does nothing to the music when the level has none.
- `Straighten` and `UpdateJewelColours` are simplified to what the data files use.
- `MergeChildren` guards a missing backdrop picture and a missing lowest child.
- A firework's last event rebuilds its stages (0x404580 from the custom event), which frees the stage that is firing its events;
  the original goes on reading and writing that freed stage (0x42a980, 0x429f10), the port stops the stage's update there
  (`SceneStage::CheckEvents` returns false) and gives the result the original's read of the stale progress gave. Found by the
  AddressSanitizer run of the win sequence (level 72).

## Fix candidates (for the fixes mod)

None found yet.

## Oddities (not chased)

- The first update of a `Firework` draws random numbers, as does its constructor, whatever the effect shows.
- `BonusBasket`'s search for a jewel continues from a jewel that is in the basket.
- A `Powerup` that closes in the Journey mode turns Rewind into Wings.
- The tutorial's first lesson shows its tip again on every update until a target slot is found.
- `LevelScroller` runs its setup twice.
- `CutTextLine` with no space within the width gives an empty line and skips a letter.
- The layer parallax is unstable in the original too, when the parent layer is taller than the tiles layer and the tiles are at
  rest (the content heights measured at the merge decide it).
- A scripted key press needs its release some updates apart.

## Not translated: what nothing in play can reach

The editor (class 0x42dc60, no caller of its constructor) and everything only it uses: 0x42dc60-0x42e560, 0x42eb20-0x434b70.
Functions by address that nothing calls: 0x41f290, 0x414bd0, 0x410020, 0x410320, 0x410370, 0x410550, 0x410570, 0x410630,
0x414be0, 0x414d90, 0x418bc0, 0x41bda0, 0x41ba70, the plate and slot editors' save functions, the level's editor slots, and the
tutorial's 0x421130 and no-ops 0x421100, 0x421110, 0x421120. The library's own list and allocator helpers and the object pool
functions are replaced by the standard library.
