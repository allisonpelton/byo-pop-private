# Zuma's Revenge!: working notes

What the analysis has found about the exe, for reading the export. The plan is in `PORT_PLAN.md`.

## Reading the export

- `games/zumasrevenge/tools/zr.py` prints a function of `re/zumasrevenge/gh2` compacted and named (`dec`), its callers
  and callees (`calls`), its disassembly (`asm`), its calls and the strings and names it loads in order (`flow`, for the
  arguments that link-time code generation passes in registers), a table of pointers (`table`), the classes placed by
  their own vtable slots (`classes`), a class's vtable (`vt`), the strings matching a pattern with the functions that
  use them (`str`), and the names matching a pattern (`name`).
- The stack cookie is `DAT_009e5010`; `std::string::assign(const char*, n)` is 0x4071d0, the wide one 0x43f9c0;
  `operator new` 0x8ce667, `free` 0x8ce459.
- The app object is a global reached through `DAT_009fc740` (the original's `gApp`).

## The resources

- The manager is this generation's ResourceManager (0x620e80-0x628a00), with `CustomResManager` (vtable at the
  constructor 0x47a860, 0x154 bytes) over it. It reads ResourceMan2's manifest (`properties\resources.xml`, no version
  attribute, no art-size or composite groups) with the kinds Image, Sound, Font, PopAnim and PIEffect. The functions
  that both have match Bejeweled 3's in size where neither has art sizes (the resources' DeleteResource and ApplyConfig,
  LoadAlphaImage, LoadAlphaGridImage), so `engines/prime/PrimeResourceManager` is the same class one generation later;
  the parsers differ by the art-size groups.
- `CustomResManager::DoLoadImage` (0x465150): a path that starts `images\600\` or `images\1200\` is read from
  `images\<art>\` with the rest of the path kept, where art is 1200 with Hi-Res on (app+0x970) and 600 otherwise; a path
  that starts `images\!` makes an empty DDImage and loads nothing; any other goes to the base DoLoadImage. The manager's
  base art size (+0x14c) is 1200 and its art size (+0x150) 600 or 1200 by the same flag (the constructor, 0x47a860).
  `DoLoadFont` (0x465490) and `DoLoadPopAnim` (0x465540) are the base's.
- The generated code is `src/Res.h` and `src/Res.cpp`, written by `tools/resgen.py` from the exe's id table (2,100 ids,
  GetStringIdById's jump table at 0x520cd0), the manifest's kinds and the exe's extraction functions (2,093 getter
  calls, each with its id pushed before it). Each extraction function first gives every global to the manager (0x51d7e0)
  and then takes its group's resources through five typed getters (0x50b200 images, 0x50b3b0 sounds, 0x50b2e0 fonts,
  0x50b480 animations, 0x50b550 particle effects; the manager's slots 0x54 to 0x64), each recording the resource's id in
  a map under the lock 0x9fcf48. `ExtractResourcesByName` (0x50ae20) compares the name with each group's in the
  manifest's order. The Main group lists nothing: its function (0x519110) makes the seven 3D effects from sources kept
  in the exe (`effects\grayscale.fx`, `hole.fx`, `lava.fx`, `screen_distort.fx`, `topdown_water.fx`,
  `under_dialog.fx`, `wave.fx`), the globals EFFECT_GRAYSCALE and the rest.
- Who calls them: Init calls LoadScreen, Init and Main; StartLevel and Boss::Init call Boss6Common, CommonBoss and
  GrottoSounds; the challenge mode's thread CommonGauntlet; the main menu's thread MenuRelated; the shadow canopy
  ShadowCanopyJungle1; the squid boss Underwater; the board's console MainSounds; and the loading thread, Boss::Init and
  Effect::LoadResources the rest by name.

## Boot

- GameApp: vtable 0x966204 (112 slots), constructor 0x47a860 (called from 0x4ea900), Init 0x47f910 (slot 49), InitHook
  0x481ab0 (slot 19), LoadingThreadProc 0x481fd0 (slot 22), LoadingThreadCompleted 0x483ae0 (slot 16),
  HandleCmdLineParam 0x48a950 (slot 45), UpdateFrames 0x486820 (slot 8). Its second and third vtables are the dialog
  listener (0x9663c8: DialogButtonPress, DialogButtonDepress) and JeffLib's NewUserDialogListener (0x9663d4).
- The constructor: the screen is 800x600 unless Hi-Res is on, then 1600x1200 (app+0xc8, +0xcc). Hi-Res is on when the
  default art size (from the object at +0x71c, through its slot 1 and 0x6ea5f0; probably compat.cfg's rules, still to
  read) or a registry boolean (0x6808f0) asks for it, and it is supported, 3D is supported and on, and the last shutdown
  went well ("Forcing hi-res off. Hi-Res supported: %d, 3D supported: %d, Is3D: %d, Last shutdown OK: %d"); a default art
  size other than 600 or 1200 shows "Default art resolution must be either 1200 or 600". It also logs the machine's memory and
  display adapters (the vendor and device ids), makes the CustomResManager (+0x6f8), the UnderDialogWidget (+0x8e8) and
  three more objects (0x8911f0 at +0x924, 0x504870 at +0x92c, 0x46deb0 into the global 0x9f1fd8).
- Init, in order: SteamAPI_Init (the Steam user's id into +0x9c0; the port has no Steam), the base Init (0x68b450), the
  "users" folder in the app data folder, the LoadScreen group loaded (manager slot 0x50) and extracted, then Init, then
  Main, each failure calling 0x68b500 (the resource error); four cursor images; the music `music/zuma.mo3` (the music
  interface's slot 0x54); then either the loading thread's start (0x481f10) or, when the registry's
  "NeedsConfirmation" is set, the 3D confirmation; then 0x535170 and the UnderDialogWidget added to the widget manager.
- InitHook: the registry's "Z2Cursor" (+0x989, default on) and "Colorblind" (+0x988); an object of 0x60 bytes at +0x87c
  (0x501e10); the properties' "MarketingMode" (when it is "Level", +0x880 is set and two messages go to a window of that
  object); "Marketing mode is: %s"; the crash file's check (left out with the crash reporter).
