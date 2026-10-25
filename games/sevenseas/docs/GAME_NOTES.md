# Seven Seas Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in `re/sevenseas/bin/WinSS.exe` (linked 2003-01-29 with VC++ 6.0, image base
0x400000, no wrapper). The exe has RTTI for the 50 framework and library classes only: the game's own classes
have none (compiled without /GR), so their vtables were found from the constructors (a store of an address in
.rdata into `[this]`). Ghidra did not create about 125 of the game's functions (they are reached only through
vtables); `re/sevenseas/gapfuncs.jsonl` (candidates at 16-byte function starts after a `ret` or padding in the
gaps of the function list) was applied with `ApplySymbols.java`. Game code is 0x401000-0x427880 (about 410
functions); the framework starts at 0x427880 (Widget) and the library after 0x450000. `tools/funcmatch.py`
against Noah's Ark matched 1,269 functions (framework and string helpers, not the game's classes).

## 1. Classes (vtable, constructor, destructor, Draw)

| Class | vtable | ctor | notes |
|---|---|---|---|
| Board | 0x493360 | 0x401000 | 0x95a0 bytes of state; Draw 0x406400 (slot 16), Update 0x4060e0 (17), MouseMove 0x40ad20, MouseDown 0x40a4a0, MouseUp 0x40ae70; ButtonListener at +0x58 (0x493350) |
| BonusLevelDialog-like | 0x49348c | 0x40c7d0 | Dialog with a check box, "do not show this screen again" (BonusNag) |
| help/pause widget | 0x493580 | 0x40d330 | draw 0x40da40, "return to game" button |
| HighScoreDialog | 0x49363c | 0x40f7b0 | Dialog, draw 0x40faf0 |
| HighScoreWidget | 0x49370c | 0x40ff70 | draw 0x411260; file hiscores.dat |
| 0x4132b0 widget | 0x493844 | 0x4132b0 | "Click here to start", draw 0x413850, update 0x414830 |
| Dialog (register-ish) | 0x493910 | 0x416160 | "CANCEL" |
| IntroWidget | 0x4939f0 | 0x416900 | "WELCOME TO", draw 0x416df0 |
| 0x4175d0 widget | 0x493aac | 0x4175d0 | draw 0x4177d0 |
| NextLevel dialog | 0x493b68 | 0x417a30 | draw 0x417ca0 ("Next rank at %d") |
| Options dialog | 0x493c40 | 0x418760 | draw 0x4192e0 |
| 0x4199f0 widget | 0x493d04 | 0x4199f0 | draw 0x419ac0 |
| RecordScore dialog | 0x493e08 | 0x419f40 | draw 0x41a2f0 |
| Register dialog | 0x493ee8 | 0x41a650 | draw 0x41ae30 |
| 0x41b560 widget | 0x493fa8 | 0x41b560 | draw 0x41b5a0 |
| SevenSeasApp | 0x494080 | 0x41bc00 | 55 slots like Noah's; Init 0x41e580 (slot 30), loading 0x41db50 (11) |
| Dialog base | 0x49416c | 0x425540 | generic dialog |
| TitleScreen | 0x49423c | 0x4256f0 | draw 0x425c00, update 0x425fb0 |
| 0x4262c0 dialog | 0x494304 | 0x4262c0 | "Warning" (nag with a check box) |

In this framework the Widget vtable has 43 slots (Noah's 2002 one has 44): slot n here is slot n+1 there from
about slot 3 on.

## 2. Application (0x41bc00 etc.)

Offsets in the app object: images array +0x400 (88 images, table 0x4a5ab8), fonts +0x404..0x420, widgets: title
+0x424, intro +0x428, high-score widget +0x42c, +0x430 trial start screen, Board +0x434, +0x438 next-level, +0x43c help.
+0x494 byte (tutorial done), +0x498 int difficulty 1..3, +0x4f1..0x4f5 nag flags (registry BonusNag, WarnBlueNag,
WarnGhostNag, WarnSerpNag, WarnWhirlNag), +0x4d0 LastScoreName, +0x4e0 LastInternetScoreName, +0x3cc games played.
Registry key "PopCap\SevenSeas". The 88 images are loaded by 0x41d880 and also entered in a name map (+0x4b4);
data\anims.txt (0x41c960) is parsed into animation definitions (+0x4a4 map name -> frame list).
