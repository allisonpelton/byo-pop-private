# Mods worth making for TipTop Deluxe

None of these is made (the user's rule: no new game gets mods until every PopCap game works in the browser). They are
ordered by how much they change the game for how little work the port needs.

**Archive overrides.** Most of the others need it: a mod is a folder of files named as in `TipTop.dat`
(`art\...`, `sounds\...`, `tiptap.cfg`, the instruction pages), and the resource manager (`engine/Res.cpp`, which
already opens the archive through the platform's file layer) reads an entry from the mod when it has one. A mod that
changes only some values of `tiptap.cfg` would be a list of `KEY value` lines merged over the original's, so two mods
can change different keys.

**The hidden timed mode.** TipTop has a fourth mode that the shipped game hides: its button on the new game screen and
its high score rows are placed off the screen by the configuration, but its level list, its ramps, its starting high
scores and its rules (a meter that empties with time, a level won when the meter reaches the goal, the game lost when
it runs out) are all there and translated. A mod that moves the button and the high score rows on screen brings it
back, but it needs pictures too: the archive's button for the mode is a 16x16 placeholder, and the backgrounds of the
main menu and the high scores screen have panels and names for three modes only. The menus' layout leaves room for a
fourth button only if the others move a little.

**Rules from the configuration.** The configuration already holds almost every number of the game: the board's columns
and rows, each mode's levels (speed, rows to clear, goals), the ramps that make the special blocks and the random
floaters more frequent, the bonus rounds, the effects and their timings. Mods of values alone: a gentler speed mode,
a strategy mode with more special blocks, longer bonus rounds, a board of other proportions where the background
pictures allow it.

**A calm mode.** No game over: when a column fills, its oldest blocks fade out instead, and the levels go on. It suits a
relaxed game and is a place to try the other mods.

**Look.** The fruit, the special blocks, the backgrounds and the fonts are images of the archive (PCX and JPEG), so a
mod with archive overrides can replace any of them; a set of higher resolution pictures would also need the blitter
to scale, since the game draws at 640x480.

**Texts.** The texts come from the exe at run time and a `GT_<address>` string property replaces any of them; the
instruction and credit pages are text files of the archive. A language pack is a list of these.

**Music.** The music is one General MIDI file played by the game's own sequencer, so a mod can give the game another
tune (any type 0 or 1 MIDI file) once the port has its synth, or turn the sequencer's instruments into other General
MIDI programs.

**Daily seed.** A fixed seed for each day and a local best score for it: the game's random numbers are one generator
that the port can seed (`-seed=N` does it now).

**New special blocks.** The special blocks (rocket, exploder, hammer, magnet, rainbow, color bomb, column nuke) are
kinds 10-16 of the field, each with its own motion, its tap rows in the configuration and its effect. A new kind (a
row clearer, a block that turns its neighbors into its fruit) would need code in the field's removal and a picture,
and a frequency ramp of its own.
