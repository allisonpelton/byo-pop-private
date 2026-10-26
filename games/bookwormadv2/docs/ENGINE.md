# Bookworm Adventures Deluxe: the engine

> Addresses in this file are Bookworm Adventures Deluxe's exe (Volume 1); Volume 2's `BookwormAdventuresVol2.exe` (linked 2009) has the same parts at other addresses, for example `luaV_execute` at 0x67b640 (Volume 1: 0x717540). Where Volume 2 differs, the section says so; the rest is carried over unchecked until the port needs it.

What the exe (`BookwormAdventures.exe`, linked 2007-06-20 with VS2005) is made of, where each part is, and how the
port takes it over. Addresses are the exe's; `tools/fnfind.py` finds functions by string, namespace or range,
`tools/decread.py` prints Ghidra's decompile, `tools/asm.py` disassembles, `tools/regtables.py` lists the
`luaL_reg` tables.

## Code regions

| Region | What | Port |
|---|---|---|
| 0x401000-0x4c8000 | the game's C++ classes (GameApp, Board, BattleEngine, BookManager, ConversationPanel, MainMenu, Credits, TreasureScreen, TooltipWidget, HallOfFame, StatsBook, dialogs, screens) and XWidget::XLayout at 0x4c0000 | `src/` (to come) |
| 0x4c8000-0x5a0000 | SexyAppFramework ~1.3 (DDImage, MemoryImage, fonts, widgets, SexyAppBase, ResourceManager, sound and music interfaces) | the shared `framework/` |
| 0x5a0000-0x650000 | libraries (image codecs, zlib, BASS/FMOD glue, Flash COM, STL) | the shared platform, or not needed |
| 0x650000-0x6a0000 | LuaApp: LuaApp, LuaWidget and `LuaWidgetTpl<>`, LuaImage, LuaFont, LuaResourceManager, the DProps editor, and the `MethodWrapper<>` instantiations | to come |
| 0x6a0000-0x6c9a00 | JeffLib: StateMachine/State, Animator, SoundPlayer, SexyBassMusicInterface, DataDrivenClass, developer dialogs | to come |
| 0x6c9a00-0x6ee000 | LuaLib: LuaLinkedClass (0x6c9a70-0x6caa60), LuaSupport (the state, the C++ class registry, calls, serialisation, save points, the debugger console) | to come |
| 0x6ee000-0x71f000 | PopLua itself: lapi, lauxlib, the libraries, ldo, the compiler, lvm, ltable, llex, lundump, lcode | `poplua/` (docs/POPLUA.md) |

## Classes

`GameApp : LuaApp : SexyApp : SexyAppBase`, with `LuaLinkedClass` as a further base of LuaApp at offset 0x730
(vtables 0x7adc44, 0x7addf0, 0x7addfc). The Lua state is wrapped by `LuaLib::LuaSupport` (vtable 0x7c758c, 31
slots), derived as `Sexy::PopLua` (0x7c1f24) and then the game's `Sexy::LuaInterface` (0x7b5584). The global at
0x825958 is the LuaInterface; 0x825d00 is used as the LuaSupport in the binding code.

## C++ objects in Lua

A C++ object reaches Lua as a full userdata ("cobj") of 0x14 bytes: the object, the userdata itself, an owned
flag, the class name and the class info. PushFullObject (0x6d2830) keeps a C++ multimap from object pointers to
these blocks, so an object pushed again as the same class gets the same userdata (and `==` and table keys work);
nil for NULL. The userdata's metatable is its class table: CreateClass (0x6d1720) makes `CClasses.<name>`, also
registered with luaL_newmetatable, with `__index` (itself), `__gc` (0x6cf460: forgets the block and deletes an
owned object), `__eq` (`sexy.__cobj_compare`) and `__cobj_compare`. SetBaseClass (0x6d1cd0) gives the derived
class table the metatable `{__index = <base class table>}`, so methods are inherited the plain Lua way, and
records the base and the pointer adjustment in the class info. A C++ object deleted while Lua still has its
userdata leaves them in place: using one raises "Deleted object of type '%s' can no longer be accessed".

Each bound method (AddMethod 0x6d2400) is `CClasses.<class>[name]` = a C closure of the trampoline 0x6cf370 with
the class, the method entry and its wrapper as upvalues. The trampoline checks self (0x6d2d30 compares class
names and walks the base classes, adding the adjustments; a light userdata is accepted too), removes it from the
stack and calls the wrapper's vf1, which reads the arguments from index 1, calls the member function and pushes
its result. The wrappers are `LuaLib::MethodWrapper<Class, member pointer type>` (358 types, named by RTTI) and
`LuaLib::RawMethodWrapper<Class>` (the method gets the lua_State). Arguments are checked from the first on: ints
take ints, numbers (truncated) and int strings, else "int expected"; booleans only booleans ("boolean
expected"); strings strings or numbers; objects nil (NULL) or an object of the class or a derived one, else
`cobj:<expected> expected, got cobj:<actual>` (luaL_typerror 0x6f1b40 names the actual class from the registry
entry of its metatable); a missing argument is "no value". Results: ints as ints, floats and doubles as
numbers, bools, strings, objects (PushFullObject, not owned), and `LuaLib::LuaMultiRet` for methods that push
their own. The port has the same in `lualib/` (LuaSupport.h, LuaBind.h).

Classes register their methods in their constructors (or a registration function) with a chain of calls, one
per method, to small functions that make the wrapper and call AddMethod. `tools/bindings.py` follows those
chains and lists the 820 bound methods of the 43 classes with their signatures and the functions they call
(output for reading only, kept under `re/`). The raw functions of `sexy.Graphics` and `sexy.WidgetContainer`
are listed without their addresses. The free functions are registered in the `sexy` table (0x4c5a50 and
others): the scripts use NextRand, NumToCommaStr, PlaySample, GetFilesInDirectory, DoPointerCursor,
DoHandCursor, PrintWarning, StrLess, DebugMsgBox, GetOffsetRectFromImage and the PopAnim functions
(LoadPAMFile, ClonePAM, PlayPopAnim, DrawPopAnim, UpdatePopAnim, ResetPopAnim, DeletePopAnim,
ChangePopAnimCallback, GetPopAnimFile, GetPopAnimLabelFrame, SetPopAnimColor).

C++ objects with a script half are `LuaLinkedClass`es: the C++ side calls the Lua functions of
`scripts/common/classlink.lua` (RegisterLinkedClass, CreateLinkedObject, SetLinkedObject, DeleteLinkedObject,
CallLinkedMethod, GetLinkedLuaObject; tables gLinkedObjectTable and friends).

C++ calls Lua through LuaSupport's vf08 (0x6d77e0): `Call(path, nargs, keep results)` with the arguments
already pushed; the path is dotted from the globals, and `a.b:Method` calls a method of a.b. The paths the exe
calls: `main.*` (LoadRequiredScripts, LoadResources, LoadInitialResources, SetBetaBuild, SetTimedMode,
GetNumMiniGames, GetMiniGameClass, NeedsIntroCutscene, StartMinigameFromMainMenu and others), `profile.*` (Get,
Set, Save, KeyExists, Delete, GetName, GetTableSize), `convpanel.*` (the conversation panel's setup),
`gBattleEngine:*`, `gBookManager:*`, `gMiniGame:*`, `gTileEngine:*`, `gTreasureScreen:*`, `PAMAnimator.*`,
`perf.*`.

LuaSupport also saves and restores Lua values with per-class serialisers (SaveValue 0x6d39d0, PushValue), keeps
save points (luatemp.dat), and has a debugger console and breakpoints (developer features).

## The app's virtual functions

The 1.34 framework headers (`E:\retools\refs\sexy13`) give SexyAppBase's 98 slots exactly
(`python tools/saf_vtable_layout.py <its SexyAppFramework folder> --dump SexyAppBase`); SexyApp adds 6 and LuaApp
2 (slot 105 at +0x1a4 registers the classes; LuaApp::Init calls it first). LuaApp overrides UpdateFrames,
~SexyAppBase, Shutdown, HandleCmdLineParam, Init, KillDialog, DebugKeyDown, DebugKeyDownAsync and
CloseRequestAsync. GameApp also overrides ButtonPress, ButtonDepress, LoadingThreadCompleted,
CreateMusicInterface, InitHook, LoadingThreadProc, NewDialog, PreDisplayHook, GetGameSEHInfo, both PlaySample,
SwitchScreenMode, GetDialog, LostFocus, IsAltKeyUsed, CheckSignature and DoMainLoop. GameApp::Init (0x445a50)
creates the LuaInterface (0x4c4fb0), registers the classes, runs SexyApp::Init, registers the `sexy` functions
(0x4c5a50) and GameApp's 73 methods, makes the CustomResManager and reads `properties\config.xml`.

## Start-up

0x4185d0 (a GameApp virtual): LuaInterface DoFile `scripts\main.lua` (0x4c5050), then Call (vtable slot 8,
0x6d77e0) `main.LoadRequiredScripts` and `main.LoadResources`. 0x44a0e0 registers the linked objects
gBookManager and gTreasureScreen and calls LoadBookScripts. 0x414660 runs `scripts/BattleEngine.lua`, 0x499c80
`scripts/TileEngine.lua`. A script error ends the game with a dialog (0x4c5910).
