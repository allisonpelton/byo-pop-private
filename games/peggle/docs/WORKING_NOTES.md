# Peggle Deluxe: working notes (analysis state, kept so a compaction loses nothing)

Peggle Deluxe (PopCap, 2007). The exe's internal name is "Thunderball" (`Sexy::ThunderballApp`, the window title "Peggle Deluxe"). It is
a VS2005 build (MSVC RTTI, 207 classes) of SexyAppFramework about 1.3, the generation of Bejeweled 2 and Venice (SexyApp has 104 vtable
slots). The game image is `re/peggle/deluxe/bin/PeggleGame.exe` (the DRM wrapper `Peggle.exe` is not used); the Ghidra export is in
`re/peggle/deluxe/gh` (`all.c`, `functions.jsonl`), the Mac build in `re/peggle/deluxe/mac`. Everything under `re/` and `assets/` is git-ignored.

## Tools (games/peggle/tools)

- `stage_peggle_assets.py`: unpacks `main.pak` (XOR 0xF7) lower-cased into `assets/peggle` (911 files), plus the loose music and
  `properties/partner.xml`. A Steam copy that has a theme installed keeps the stock pak as `main.pak.original`; that one is staged.
- `unscramble_mac.py`: the Mac executable's code section is scrambled; this writes `re/peggle/deluxe/mac/Peggle.dec` readable, which
  `tools/macmatch.py` needs. With it 1710 functions pair with the Mac build's names (`re/peggle/deluxe/macmatch/peggle.match.jsonl`;
  `peggle.names.jsonl`, `rtti.classes.json`, `classindex.txt` (one line a class), `runs.txt` (class runs by address)).
  The vtable pass (score 0.9) is dependable; call-pass names (0.5 to 0.9) are read against the code before they are used.
- `pe.py`: the exe's sections and bytes. `decread.py`: prints functions compactly, with Mac names, resource globals
  (`res_map.txt`) and `docs/symbols.txt` names in place of FUN_/DAT_ ones, floats and strings noted in place:
  `decread.py <addr>...`, `<lo>-<hi>`, `--list <lo>-<hi>` (address, size, name, strings), `--raw`.
- `resgen.py`: writes `src/Res.h`, `src/Res.cpp` (421 resources in 15 groups, the exe's ResGen output) and `res_map.txt`.

## The code

- Game code: about 0x401000-0x4c1800 (750 KB, about 2,066 functions). The framework follows (0x4c2000+ and the CRT); the port needs none of it.
- The translation units are interleaved by address (each class's functions sit in several runs; `runs.txt`): the compiler's
  link order is not the class order.
- Classes (Mac names): ThunderballApp (the app, about 150 methods, fields up to offset 0x880), Board (the game board, 83+ matched),
  LogicMgr (about 99, the physics and the peg logic), PhysObj / Ball / Line / Poly / Brick / Hole / Gun / Mover (the level pieces),
  PegInfo, CollisionMgr, Effect / EffectMgr, FloatingText(Mgr), InterfaceMgr, CharacterMgr, AIMgr / AIBall (the bot the game uses for
  the replays and the opponent), StageMgr / StageInfo, TrophyMgr, ProfileMgr / PlayerInfo, StatsMgr / GameStats, HighScoreMgr, SoundMgr,
  ImageMgr, DebugMgr, ConfigParser / StringParser (text .cfg files), DataSync / DataWriter / DataReader (saves, levels, replays),
  and the screens and dialogs (MainMenu, LevelScreen, AdventureScreen, StoryScreen, HelpScreen, TrophyScreen, LoadingScreen,
  UpsellScreen, EndLevelDialog, CharacterDialog, OptionsDialog, UserDialog, NewUserDialog, SlotMachineDialog, ReplayDialog,
  ReplayFileDialog, ContinueDialog, StageDialog, ThunderButton / ThunderCheckbox / ThunderSlider / ThunderDialog / ThunderFrame).
- Start-up: `WinMain` makes the global `ThunderballApp` (ctor 0x426f10), then `Init` (0x42ca30): SexyApp::Init, the user-data folder move,
  resource manifest, groups "Init" then "Load" (`ExtractInitResources`, `ExtractLoadResources`), the cursors, CharacterMgr, HighScoreMgr,
  ProfileMgr, StageMgr, StatsMgr (and `LevelEditor/stats_cheat.dat`), TrophyMgr loaded; the current user from the registry value `CurUser`;
  the window buttons; `ShowLoadingScreen`. The LoadingThreadProc (0x421a70) loads the other groups (LuckySpin, Upsell, MainMenu,
  LoadingThread, LevelSelect, Interface, Dialog, Character, ...).
- The marketing mode (`InitHook` 0x41bff0): the property "MarketingMode" (60Normal, Stage3, Stage4lvl1, Stage1lvl4, Stage2lvl1, Stage4)
  picks a demo set-up used for the distributor's screenshots: not reachable in the port and left out.

## Data

- `main.pak`: images (.jp2 with `_` alpha masks, .gif, .jpg), `levels/*.dat` (binary level files), `levels/stages.cfg`,
  `characters/characters.cfg`, `trophy.cfg`, LevelEditor config, sounds (.ogg), `properties/resources.xml`. The music is MO3
  (`music/PeggleBeats.mo3`, libopenmpt) and two loose .ogg files.
- Registry values the game keeps (`ReadFromRegistry` 0x41c400, `WriteToRegistry` 0x427380): MaxStage, MaxLevel, CurUser, FeverVolume,
  ShouldShowUpsellButton, ...

## Left out of the port (registration and outbound buttons are removed, not disabled)

IsRegistered, IsExpired, IsTrialOver, IsLevelDemoLocked / IsLevelLockedTrial, TryExpire, CheckCanExpire, UpsellScreen (and the
`ShowUpsellScreen`, `EndUpsellScreen`, `TryPlayUpsell` calls), the More Games and Buy buttons, the marketing mode, the partner logo.
The registered path stays as the only one. Each function left out is listed by address in `GAME_NOTES.md` once the port has them.

## State

The app skeleton builds (`TARGET=peggle bash tools/build.sh native`) and starts: it loads the "Init" and "Load" resource groups in
0.3 s (the JPEG 2000 images through OpenJPEG) and shows an empty window. Next: the data managers, then the loading screen and the main menu.
