# Retro64's engine in Water Bugs

Water Bugs runs on the same engine as Cosmic Bugs, and `games/cosmicbugs/docs/ENGINE.md` describes it (start-up and
the main loop, the banks, the fonts and the blitters, the draw list and the dirty rectangles, the XML and the
FLICs, the sound and the music). This port's `engine/` is the same code, translated again from `WaterBugs.exe`.

What differs is where things are, and a few calls:

- The addresses. Every function of the engine sits at a fixed distance from its Cosmic Bugs twin inside a region (the
  game's code first, then the engine's from 0x41f5d0; the blit wrappers are 0x429110+0x4e0 on): `re/waterbugs/match.jsonl`
  pairs them and `re/waterbugs/addrmap.json` carries Cosmic Bugs' addresses over to Water Bugs'. Both are made by
  `games/waterbugs/tools/remap_addresses.py` and `tools/funcmatch.py`, and `games/waterbugs/tools/fdiff.py` shows two
  twins side by side.
- The emitter set-up takes three more numbers (100.0, 10.0 and 400.0 for the smoke of a shot).
- The first palette tints (`ColourShift` calls of the animation loader) differ for two of the tint slots.
- The system layer is the same set of calls; the files are `water_art.bbk` and `water_art8.bbk`, the game file is
  `wanims\water_bugs.eco` (read as `water_bugs.xml`), and the saves are the same two files as Cosmic Bugs'
  (`cp.cfg` and `cb.dat`).

The two engines are so alike that a shared engine in `framework/` or `platform/` is possible: the differences above
are the ones found while porting the functions the two games share (about a hundred of them differ in more than their addresses).
