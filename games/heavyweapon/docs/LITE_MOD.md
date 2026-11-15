# Heavy Weapon Deluxe: the lite mod (2026-11-15)

Heavy Weapon has no lite mod, because nothing in its game logic is worth a toggle. This file says what was looked at
and why nothing was taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The performance numbers are in `GAME_NOTES.md`,
"Performance".

## What was looked at

The game's logic is a set of lists on the board, updated 100 times a second: the enemies, the bullets, the
projectiles, the explosions and the effects. Each moves itself. A few passes test them against each other: the tank's
bullets against the enemies, the enemies' bullets against the tank and its shield, and the lasers and orbs against
both. The game has no solver, search or fill. The bot behind the `bot` test command is test code and is not part of a
player's run.

The port's cycle split (`GAME_NOTES.md`, "Performance": 3000 updates of level 4 in play, at the game's own size, 2D)
puts the whole update at 0.142 M cycles an update. That is every list, every collision pass and the music's tick, 5.3%
of the process's 2.67 M (less its audio thread). The rest is the picture, its present and the GPU driver's thread. A
lite change keeps the look, the frame rate and the pace, so it may not touch those.

| Computation | What it does | A faster method | Result |
|---|---|---|---|
| The board's bullet pass (`Board::Update`, 0x41b690) | each of the tank's bullets is tested against each live enemy, and each enemy bullet against the tank, in list order; the first enemy hit takes the bullet | a grid of the enemies' rectangles, rebuilt each update, walked in list order | exact where it keeps the order, so it belongs in the port, not here; it would change which enemy a bullet hits only if the order changed |
| `Sprite::Collides` (0x42d750) | a rectangle test, then the two cels' alpha masks compared pixel by pixel over the overlap, stopping at the first pixel both cover | a coarser mask, or rectangles alone | another result wherever two cels' rectangles overlap and their pixels do not (the tank's shield, the thin beams), so it would be lite. The pixel loop runs only after the rectangle test passes |
| `Board::HitProjectiles` (0x417390) | the first destructible projectile in reach takes the damage ("One projectile a hit" under the oddities) | none needed: a short list | |
| `UpdateExplosions`, `UpdateSparks`, `UpdateShellCasings`, `UpdateFloatingTexts`, the particles | each item's own small update, and the floating texts' overlap test against each other | none needed | |
| The x87 sines (`GAME_NOTES.md`, "The x87's sines") | the original's `fsin`, `fcos`, `fpatan` and `fyl2x` kept to their 64-bit results, so that the tank's X agrees in every bit | the C library's functions | another result in the last bit of about a quarter of products, so it would be lite; the sines are a small part of the 0.142 M |

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs.
The whole update of the stretch is 5.3% of the process, and no single computation in the table reaches a fifth of it.
The five-round medians resolve a few percent of the process at best. On a machine busy with other sessions' games, the
port's own runs spread from 2.3 to 5.3 M cycles an update, mostly through the GPU driver's present. A coarser collision
test or the C library's sines would therefore save less than the measurement can see. Both would also put the game's
identical play at risk: one changes what a shot hits, the other the tank's place in its last bits. The exact change,
the enemies' grid in list order, gives the same result, so it is not lite either, and it does not earn a place in the
port's performance pass, where the port is already below the original. The cost that is left is the picture and its
present.

The lite-mod step is therefore done with no mod.
