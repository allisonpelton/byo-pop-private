# Jolly Bear's engine in MahjongEscapeAC.exe

China runs on the same JollyBear engine as Ancient Japan, three months older (`MahjongEscapeAC.exe` was linked on
2006-08-21 by MSVC 6, Japan's on 2006-11-08). `games/mahjongjapan/docs/ENGINE.md` is the map of it: the object tree,
the fonts, the streams, the app, the manager and the device. Its addresses are Japan's. China's engine functions are
laid out in another order, so the same function sits at another address (Japan's 0x424d30 is China's 0x42a500, 0x432610 is
0x42c5c0); `re/mahjongchina/compare/addrmap.tsv` pairs them. `engines/jollybear/` is the one copy of it that both
games build, and its comments give Japan's addresses.

Every engine function the comparison flagged was looked at. What differs is how it was compiled, not what it does.
China's build inlined many short calls (accessors of the lists, the strings and the objects' flags) and
passes the manager to calls that Japan's reach through a global, and the classes' fields sit a word or two lower: the objects have no alpha field of their own (Japan's +0x68
was always 0xff, so `Object::DrawWorker`, 0x41f270, draws the same), and the app's names come from `SetupNames`
(China 0x429610) as Japan's do. Nothing the engine shows or plays changed, with one exception that only a capture
sees: `WinMain` registers China's window class with a black background brush (China 0x4295a0; Japan's has none).
The port has no window class of its own, and `DEVIATIONS.md` ("The window's background") says what the brush did to
the comparison in Windows Sandbox.

`symbols/10_engine.jsonl` names 142 of the engine's functions at China's addresses for the Ghidra pass.
