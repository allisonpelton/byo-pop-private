# Reverse-engineering notes: Bejeweled Deluxe 1.87 (Steam)

## Binary layout

`WinBej.exe` (2,972,712 bytes, PE32 x86, linked 2007-08-10) is **not the game**. It is PopCap's DRM
wrapper: a SexyAppFramework + Lua ("PopLua") application that drives the trial/purchase screens
(`drm/`, `drm.swf`, `Flash.ocx`) and the Steam2 API (`SteamStartup`, `SteamIsAppSubscribed`).

The real game is appended to the wrapper as a PE overlay:

| Offset in WinBej.exe | Content |
|---|---|
| `0x000000`-`0x17b000` | DRM wrapper PE image (`.text/.rdata/.data/.rsrc`) |
| `0x17b000` | ASCII marker `!popcapdrmprotect!`, zero padded |
| `0x17b1a2`-`0x2d31a2` | **Embedded game PE** (1,409,024 bytes, linked 2007-04-13, unencrypted) |
| `0x2d46f0` | Authenticode certificate of the wrapper |

`tools/extract_game_exe.py` carves the embedded image to `re/bejeweled/bin/BejeweledGame.exe`. Its entry point
(`0x4dbf7b`) is the stock MSVC `__security_init_cookie` / `__tmainCRTStartup` pair, its imports are the
normal Win32 set (DirectDraw is loaded dynamically), and it has full MSVC RTTI, so no unpacking or
decryption is needed.

## Classes (from RTTI)

Game code (source files named in `properties/default.xml` comments):

- `Sexy::WinDMApp` (`WinDMApp.cpp`) - application class (DM = "Diamond Mine", Bejeweled's original name)
- `Sexy::Board` (`Board.cpp`) - the playfield, game rules, scoring, animation
- `Sexy::TitleScreen` (`TitleScreen.cpp`)
- `Sexy::HighScoreDialog`, `Sexy::HighScoreWidget`, `Sexy::OptionsDialog`, `Sexy::RecordScoreDialog`
- `Sexy::RegisterDialog`, `Sexy::UpdateCheckDialog`, `Sexy::BetaSupport` - registration/update/beta
  plumbing that the Steam build never shows; not ported.

Everything else is SexyAppFramework (circa 2005-2007): `SexyAppBase`, `SexyApp`, `Widget`,
`WidgetContainer`, `WidgetManager`, `ButtonWidget`, `Checkbox`, `Slider`, `Dialog`, `DialogButton`,
`EditWidget`, `HyperlinkWidget`, `Graphics`, `Image`, `MemoryImage`, `DDImage`, `DDInterface`,
`D3DInterface`, `ImageFont`, `SysFont`, `ResourceManager`, `PropertiesParser`, `XMLParser`,
`DescParser`, `Buffer`, `BassMusicInterface`, `FModMusicInterface`, `DSoundManager`, `PakInterface`, ...

## Data files

- `images/` - SexyApp alpha-mask convention: `_name.gif` (or `.png`) holds the alpha channel for
  `name.jpg/.png/.gif`. `gemsheet6.png` is 780x728 = 15x14 cells of 52x52.
- `data/afont1.txt`, `data/mainfont1.txt` - SexyApp `ImageFont` descriptor scripts
  (`Define`, `CreateLayer`, `LayerSetImage`, `LayerSetCharWidths`, `LayerSetImageMap`, `SetCharMap`).
- `data/alphadigits4.txt`, `data/scorefont1.txt`, `data/overfont2.txt` - older simple font format:
  first line `<a> <lineHeight>`, then `<char> <width>` pairs; glyphs are packed left to right in the
  image strip (the widths sum exactly to the image width).
- `data/diff.txt` - difficulty constants (see Board notes).
- `music/*.s3m` - ScreamTracker 3 modules (played through BASS in the original).
- `sounds/*.ogg` (+ `.mp3` duplicates).
- `properties/default.xml` - localizable strings, grouped by source file.
