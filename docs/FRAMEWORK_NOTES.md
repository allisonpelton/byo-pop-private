# Framework identification notes

What the framework inside Bejeweled Deluxe 1.87 is (`re/bejeweled/bin/BejeweledGame.exe`): its vtable slots, object
layouts, calling conventions and the behaviour the port depends on. Bejeweled 2 has the same vtables in every class
measured. The game's own code is in `games/bejeweled/docs/GAME_NOTES.md` (overrides, ids, random draws in call order,
constants); the class layouts of the port are in `framework/` and `games/bejeweled/src/` (`Board.h`, `WinDMApp.h`).

Conventions: addresses are virtual addresses in that exe; a slot's byte offset in a vtable is its index times 4; field
offsets are absolute from the object's start. Everything was verified in the disassembly (function bodies, `ret`
sizes, dispatch offsets) unless marked. `tools/merge_symbols.py` takes this file's groups (`--notes`).

## 1. Which framework it is

- The virtual layouts of WidgetContainer, Widget, Dialog, ButtonWidget, Checkbox, Slider, EditWidget,
  HyperlinkWidget, every listener interface, Image, Font, the sound and music classes match the SexyAppFramework 1.3
  headers exactly: no virtual added or removed, the signatures are the headers'.
- For SexyAppBase and SexyApp the tree in `E:/retools/refs/pcf/source/SexyAppFramework` matches better than 1.3: its
  `SexyAppBase.h` virtual list equals the binary's 97 slots (1.3 would have 100; there is no ShowFPS), and its
  `SexyApp.h/.cpp` define the 6 further slots and the SexyApp fields (verified offset by offset). Use it for SexyApp,
  for `Common.cpp` (CheckForVista, the app data folder, AllowAllAccess) and for the demos.
- The CRT and STL are Visual Studio 2005's, with `_SECURE_SCL` checked iterators and a 64-bit `time_t`. The framework
  was built with /O2 and LTCG; the game region (0x4c9a94-0x4d87bc) with /O1, so it uses the `__EH_prolog3` helpers,
  does not pad its functions and calls `abs`, `atan2`, `sqrt`, `sin` and `cos` as real functions.

Not in the 1.3 reference:

- `Image` has no `mNumTotal`, so sizeof(Image) is 0x38, not 0x3c.
- A SexyApp layer (registration and update check) sits between SexyAppBase and the game's app; `UpdateCheckDialog`
  (a Dialog subclass, 0x482770-0x482d00) and the Vista userdata migration helper (0x474480-0x474760).
- The Dialog constructor makes no `GetString("ID_YES", ...)` call.
- `SetAppDataFolder` ignores calls on systems before Vista; `SexyApp::WriteToRegistry` lacks pcf's popcinfo.dat and
  TimesPlayed code.

## 2. Reading the binary

**Overloaded virtuals** sit next to each other in reverse declaration order, at the place of the first declaration:
MarkDirty(WC*) before MarkDirty(); SetColors[][4] before [][3]; GetColor(idx, default) before GetColor(idx);
Resize(Rect) before Resize(x,y,w,h); DrawOverlay(g, prio) before DrawOverlay(g); MouseDown 4-arg before 3-arg;
MouseUp 4, 3, 2-arg; WriteCenteredLine 6-arg before 3-arg; ButtonPress(id, cc) before ButtonPress(id);
`int LoadSound(name)` before `bool LoadSound(id, name)`; PlaySample(id, pan) before PlaySample(id). Listener
interfaces have no virtual destructor, so their slot 0 is the first method. Overrides of a secondary base's virtuals
(DialogButtonPress/Depress) get no slot in the primary vtable.

**Register calling conventions (LTCG).** Every virtual, and every constructor the game calls (`DDImage(DDInterface*)`,
`MemoryImage(SexyAppBase*)`, both ImageFont loading constructors, `BassMusicInterface(HWND)`), is ordinary
`__thiscall`. Internal helpers are not, and Ghidra's parameter lists are wrong for them:

| Where `this` or the argument is | Functions |
|---|---|
| `this` in EAX | Color() 0x4042b0; Color(const int*) 0x404330 (source in ECX); Insets() 0x4836f0; Image::Image() 0x427870; Font::Font 0x425c80 and Font(const Font&) (source in ECX); SysFont constructors 0x436fd0 and 0x437030; MemoryImage::Init 0x42d7e0; DSoundInstance::RehupVolume / RehupPan; SexyAppBase::Is3DAccelerated 0x44ff30 and Remove3DData 0x44ff10; WidgetManager::RehupMouse 0x481710; Graphics::GetColor 0x4261c0, GetDrawMode 0x4261e0, GetColorizeImages 0x426200, DrawRect(int x4) 0x4262a0, SetFastStretch 0x426210 and SetLinearBlend 0x426220 (value in CL); Buffer::GetDataLen |
| `this` in ESI | WidgetContainer() 0x47f400; DDImage::Init, DeleteDDSurface, GetSurface; SysFont(const SysFont&) (source in EDI); Buffer::ReadByte; HTTPTransfer::Get (URL in EDI) |
| `this` in EDI | Graphics::PopState 0x4260b0; BetaSupport::Validate 0x46e5d0; TRect<int>::Intersection 0x438090 (ECX = &theRect, EAX = &result) |
| `this` in EBX, EDX | DSoundManager::FindFreeChannel (EBX); RegistryWriteBoolean (EDX) |
| `this` on the stack | Image(const Image&) 0x4278c0 (source in EDI); MemoryImage() (ret 4); MemoryImage(const MemoryImage&) (source in EBX); DSoundManager(HWND, bool) (ret 0xC); DSoundInstance(mgr, buf) (buf in ECX); ImageFont(const ImageFont&) (source in ECX); Graphics::PushState 0x426020, Create 0x426110, DrawLine 0x426520, ClipRect(const Rect&) 0x426a20 (rect in EAX); RegistryWrite (value name in EBX) |
| other | SexyAppBase::SetCursor 0x44f200: cursor number in EAX, app in ECX (0 pointer, 1 hand, 3 text). SRand and MTRand::SRand: seed in EAX; MTRand::NextNoAssert: `this` folded to gMTRand. GraphicsState::CopyStateFrom 0x425e10: ECX = this, EAX = source. InsertWidgetHelper: iterator pointer in EAX. InternetManager::GetUpdateURL: this in EAX, return buffer in ESI. DoExit 0x446570 uses EDI. FontData::Load: file name in EDI. GetCurDir 0x46a140 returns through ESI; RemoveTrailingSlash 0x46b430 takes EDI and returns through EAX. Map find helpers: key in EBX. WStringToString: ECX in, ESI out; WStringToSexyString: ECX in, EAX result. SetAppDataFolder: ECX. The list<GraphicsState> destructor helper 0x488180: list in ESI |

0x4744c0, which Ghidra shows as cdecl, is `__thiscall` (it keeps ECX and returns with ret 4).

**Folded code (ICF).** One body serves every identical function, so name a call through these by its slot:

| Address | Body and uses |
|---|---|
| 0x4bfb70 | `ret`: empty virtuals without arguments (WinDMApp::LoadingThreadCompleted is another empty `ret`, 0x4d4591) |
| 0x43e840 | `ret 4`: ButtonListener slots 3-5, Dialog::ButtonDownTick, Widget::DrawOverlay(g) |
| 0x482740 | `ret 8`: Widget::MouseMove and MouseUp(x,y) (slots 52, 57); labelled "DialogListener::vfunction1" |
| 0x492f60, 0x492f50 | `ret 0xC`, `ret 0x10`: ButtonMouseMove; other empty virtuals |
| 0x427f40, 0x427f60, 0x410c30 | `ret 0x18`, `ret 0x1C`, `ret 0x24` |
| 0x44e3d0, 0x492f30 | `xor al,al; ret 4`, `ret 8`: return false |
| 0x483050 | returns true, no arguments |
| 0x427eb0, 0x427f50, 0x427f30, 0x492f40 | Image's empty DrawLine and DrawLineAA (`ret 0x28`), BltRotated (`ret 0x2C`), PolyFill3D (returns false, `ret 0x20`); MusicInterface::FadeIn (`ret 0x14`) |
| 0x4d1221, 0x4d1224, 0x4d2723 | game-region copies: SliderVal (`ret 0xC`), CheckboxChecked and the EditWidgetText default (`ret 8`), Allow* (return true) |
| 0x483700 | a 4-int constructor: Insets(l,t,r,b), Color(r,g,b,a), TRect<int>(x,y,w,h); Ghidra's "CRect::CRect" |
| 0x485d60; 0x4860a0 | shared deleting destructors: Widget, Slider, Checkbox; ButtonWidget, DialogButton, HyperlinkWidget |
| 0x482760, 0x482750 | header-inline forwarders MouseDown(x,y,cc) and MouseUp(x,y,cc): slot 54 of the Dialog and ButtonWidget families, Checkbox and EditWidget; slot 56 of the same but Checkbox |

Widget::Update 0x4821e0 is a bare `jmp WidgetContainer::Update` (which only does ++mUpdateCnt), and Dialog::Update
0x487b30 a `jmp Widget::Update`; Ghidra labels both "WidgetContainer::vfunction23". 0x43e850,
"ButtonListener::vfunction1", is ButtonPress(id, cc), which calls slot 1. The one-instruction getters 0x426180,
0x427a80 (Image::GetWidth) and 0x427a90 (GetHeight) sit at their source-order places, so their names hold.

**Ghidra.** The field names of a decompiled vtable struct come from the function in each slot, not from the slot's
index (vfunction18 is slot 17 MarkDirty(), vfunction41 slot 40 Resize), names of shared stubs cover several slots, and
the vfunction names inside bodies are sometimes wrong: trust the raw offsets. The `*_data` struct offsets are relative
to that class's data block, not to the object: WidgetContainer_data +4, Widget_data +0x54, Board_data +0x88,
SexyAppBase_data +8, SexyApp_data +0x630, WinDMApp_data +0x724. Ghidra drops the ST0 operand of `_ftol2_sse`, so a call
such as `FUN_004d99c0(...)` hides the real expression: read the disassembly. 0x4d89a9 is `_Getcvt`, not `_Getcoll` as
its FLIRT label says, and the STL code at 0x4d6000-0x4d87bc is the `num_get<char>` instantiation; there is no collate.

## 3. Strings, the STL and value types

- **SexyString** is narrow `std::string`, VS2005 layout, 0x1c bytes: +0 allocator pad, +4 a 16-byte inline buffer or
  the heap pointer, +0x14 `_Mysize`, +0x18 `_Myres` (15 when empty; on the heap from 16). `std::wstring` is the same
  with an inline capacity of 8. Helpers: ~basic_string 0x437f70, `_Tidy` 0x4395b0 (the destructor as game code inlines
  it), erase 0x437fa0, c_str 0x438060, assign(ptr, n) 0x43a740, append(ptr, n) 0x43a650, assign(n, ch) 0x439480,
  `_Chassign` 0x43a810, `_Copy` 0x43c170, `_Xlen` 0x4d87c1, `_Xran` 0x4d8800, memcpy_s 0x4d977b, free 0x4d96dc,
  operator new 0x4d9a8f.
- **Containers:** list is 12 bytes {pad, head, size}, its node {next +0, prev +4, value +8}; vector is 16 bytes {pad,
  first, last, end}; a map is 12 bytes with the head at +4 and the size at +8, its node {left, parent, right, value,
  color, isnil} (isnil at +0x45 for DefinesMap); iterators are 8 bytes {container, pointer}. A failed check calls
  `_invalid_parameter_noinfo` (0x4da274; 11 game sites), and `operator[]` allows an offset equal to the size.
- **iostreams** are VS2005's: basic_streambuf has its vtable at +0, a `_Mutex` at +4, `_IGnext` at +0x20 and
  `_IGcount` at +0x30; its vtable is 0 dtor, 1 overflow, 2 pbackfail, 3 showmanyc, 4 underflow, 5 uflow.
  0x43df00-0x43e730 are locale and ios_base internals (`_Locinfo` is 0x74 bytes). A num_get facet is vptr +0, `_Refs`
  +4, `_Cvt` +8/+0xc. A port maps all of it to the host's standard library.
- **Color** is four ints, mRed +0, mGreen +4, mBlue +8, mAlpha +0xc. `Color()` is (0,0,0,255); `Color(int argb)`
  0x4042d0 unpacks ARGB and turns alpha 0 into 255; `Color(r,g,b)` 0x404310 has alpha 255; ToInt is 0x404370.
  `Color::White` is at 0x53dfb4 and `Color::Black` (0,0,0,255) at 0x53dfc4. FillRect and DrawRect return early when the
  alpha is 0.
- **Insets** is {mLeft, mTop, mRight, mBottom}; **Rect** is {x, y, w, h}. `TRect<int>::Intersection` gives (0,0,0,0)
  for a negative width or height, as 1.3.

## 4. Slot tables

Index: method; the signatures are the 1.3 headers' (`framework/`).

**WidgetContainer** (30 slots): 0 dtor, 1 GetRect, 2 Intersects, 3 AddWidget, 4 RemoveWidget, 5 HasWidget,
6 DisableWidget, 7 RemoveAllWidgets(doDelete, recursive), 8 SetFocus, 9 IsBelow, 10 MarkAllDirty, 11 BringToFront,
12 BringToBack (inserts at begin()), 13 PutBehind, 14 PutInfront, 15 GetAbsPos, 16 MarkDirty(WC*), 17 MarkDirty(),
18 MarkDirtyFull(WC*), 19 MarkDirtyFull(), 20 AddedToManager, 21 RemovedFromManager, 22 Update, 23 UpdateAll,
24 UpdateF, 25 UpdateFAll, 26 Draw, 27 DrawAll, 28 SysColorChangedAll, 29 SysColorChanged.

**Widget** (70): 30 OrderInManagerChanged, 31 SetVisible, 32 SetColors(int[][4], n), 33 SetColors(int[][3], n)
(0x481da0, called directly by the constructors), 34 SetColor, 35 GetColor(idx, default), 36 GetColor(idx),
37 SetDisabled, 38 ShowFinger, 39 Resize(const Rect&), 40 Resize(x,y,w,h), 41 Move, 42 WantsFocus,
43 DrawOverlay(g, priority), 44 DrawOverlay(g), 45 GotFocus, 46 LostFocus, 47 KeyChar (one dword), 48 KeyDown,
49 KeyUp, 50 MouseEnter, 51 MouseLeave, 52 MouseMove, 53 MouseDown(x,y,btn,cc), 54 MouseDown(x,y,cc),
55 MouseUp(x,y,btn,cc), 56 MouseUp(x,y,cc), 57 MouseUp(x,y), 58 MouseDrag, 59 MouseWheel, 60 IsPointVisible,
61 WriteCenteredLine(g, offset, line, Color, Color, const Point& shadow) (ret 0x34: the two Colors are passed by value,
plus a hidden Rect pointer), 62 WriteCenteredLine(g, offset, line), 63 WriteString(g, line, x, y, width, justification,
drawString, offset, length), 64 WriteWordWrapped(g, rect, line, lineSpacing, justification), 65 GetWordWrappedHeight,
66 GetNumDigits, 67 WriteNumberFromStrip, 68 Contains, 69 GetInsetRect.

The WidgetManager dispatches through slots 54 MouseDown(x,y,cc), 56 MouseUp(x,y,cc), 52, 58, 59, 50, 51, 47, 48, 49,
45, 46 and 43 DrawOverlay(g, priority). Dialog, ButtonWidget, Checkbox and EditWidget override the 4-arg mouse
methods; Slider the 3-arg MouseDown and the 2-arg MouseUp.

| Class | Slots | Its own virtuals |
|---|---|---|
| Dialog | 76 | 70 SetButtonFont, 71 SetHeaderFont, 72 SetLinesFont, 73 GetPreferredHeight(width), 74 IsModal, 75 WaitForResult(autoKill) (loops the app's UpdateAppStep while mWidgetManager is set and mResult == 0x7fffffff; with autoKill it calls KillDialog(mId)). It overrides 20, 21, 22, 26, 30 (PutInfront for its buttons), 34 SetColor, 40, 53, 55, 58 |
| ButtonWidget, DialogButton, HyperlinkWidget | 73 | 70 DrawButtonImage(g, image, rect, x, y), 71 SetFont, 72 IsButtonDown |
| Slider | 72 | 70 SetValue(double), 71 HasTransparencies |
| Checkbox | 72 | 70 SetChecked(checked, tellListener), 71 IsChecked |
| EditWidget | 77 | 70 ProcessKey(KeyCode, SexyChar), 71 HiliteWord, 72 SetFont(font, widthCheckFont), 73 SetText(text, leftPosToZero), 74 IsPartOfWord (it tests a byte >= 0xC0), 75 GetCharAt(x, y), 76 FocusCursor(bigJump) |
| UpdateCheckDialog | 76 | overrides 22 Update, 26 Draw, 73 GetPreferredHeight (base + 0x20) |
| ButtonListener | 7 | 0 ButtonPress(id, clickCount) (the default calls slot 1), 1 ButtonPress(id), 2 ButtonDepress, 3 ButtonDownTick, 4 ButtonMouseEnter, 5 ButtonMouseLeave, 6 ButtonMouseMove(id, x, y) |
| DialogListener | 2 | 0 DialogButtonPress(dialogId, buttonId) 0x4414c0, 1 DialogButtonDepress 0x441510. SexyAppBase's map ID_YES (1000) to ButtonPress/ButtonDepress(2000 + dialogId) and ID_NO (1001) to 3000 + dialogId |
| CheckboxListener, SliderListener | 1 | CheckboxChecked(id, checked); SliderVal(id, double) |
| EditListener | 4 | 0 EditWidgetText(id, text), 1 AllowKey(id, KeyCode) (EditWidget::KeyDown calls +4), 2 AllowChar(id, SexyChar), 3 AllowText(id, text); 1 to 3 return true by default |

**SexyAppBase** (0x50de9c, 97 slots; SexyApp 0x51e52c and WinDMApp 0x507ccc have 103): 0-6 ButtonListener,
7 Process(allowSleep), 8 UpdateFrames, 9 DoUpdateFrames, 10 DoUpdateFramesF(float), 11 MakeWindow, 12 EnforceCursor,
13 ReInitImages, 14 DeleteNativeImageData, 15 DeleteExtraImageData, 16 LoadingThreadCompleted, 17 dtor,
18 CreateMusicInterface(HWND) (MusicInterface if mNoSoundNeeded, FMod if mWantFMod, otherwise Bass), 19 InitHook,
20 ShutdownHook, 21 PreTerminate, 22 LoadingThreadProc, 23 WriteToRegistry, 24 ReadFromRegistry,
25 NewDialog(id, modal, header, lines, footer, buttonMode), 26 PreDisplayHook, 27 BeginPopup, 28 EndPopup,
29 MsgBox(wstring, wstring, int), 30 MsgBox(string, string, int), 31 Popup(wstring), 32 Popup(string),
33 LogScreenSaverError, 34 SafeDeleteWidget, 35 URLOpenFailed, 36 URLOpenSucceeded, 37 OpenURL(url, shutdownOnOpen),
38 GetProductVersion, 39 SEHOccured, 40 GetGameSEHInfo, 41 GetSEHWebParams, 42 Shutdown, 43 DoParseCmdLine,
44 ParseCmdLine, 45 HandleCmdLineParam, 46 HandleNotifyGameMessage, 47 HandleGameAlreadyRunning, 48 Start, 49 Init,
50 PreDDInterfaceInitHook, 51 PostDDInterfaceInitHook, 52 ChangeDirHook, 53 PlaySample(id, pan), 54 PlaySample(id),
55 GetMasterVolume, 56 GetMusicVolume, 57 GetSfxVolume, 58 IsMuted, 59 SetMasterVolume, 60 SetMusicVolume,
61 SetSfxVolume, 62 Mute, 63 Unmute, 64 GetLoadingThreadProgress, 65 GetImage(file, commitBits) (returns a DDImage),
66 GetSharedImage(file, variant, isNew), 67 SwitchScreenMode(bool, bool, bool), 68 SwitchScreenMode(bool),
69 SwitchScreenMode(), 70 SetAlphaDisabled, 71 DoDialog(id, modal, header, lines, footer, mode), 72 GetDialog,
73 AddDialog(Dialog*), 74 AddDialog(id, Dialog*), 75 KillDialog(Dialog*), 76 KillDialog(id),
77 KillDialog(id, bool, bool), 78 GetDialogCount, 79 ModalOpen, 80 ModalClose, 81 GotFocus, 82 LostFocus,
83 IsAltKeyUsed, 84 DebugKeyDown, 85 DebugKeyDownAsync, 86 CloseRequestAsync, 87 Done3dTesting, 88 NotifyCrashHook,
89 CheckSignature, 90 DrawDirtyStuff, 91 Redraw(Rect*), 92 InitPropertiesHook, 93 DoMainLoop, 94 UpdateAppStep(bool*),
95 UpdateApp, 96 AppCanRestore. **SexyApp** adds, in pcf's order: 97 ShouldCheckForUpdate, 98 UpdateCheckQueried,
99 OpenRegisterPage(), 100 OpenRegisterPage(DefinesMap by value) (0x48aab0; it builds the ProdName, Version, Variation,
ReferId, DownloadId, TimesPlayed and TimesExecuted pairs), 101 OpenHTMLTemplate(template, defines), 102 OpenUpdateURL.
Ghidra mislabels 101 and 102 "WinDMApp::scalar_deleting_destructor" and 0x4662a0 "~WinDMApp". The DialogListener
vptr is at object+4.

**Image** (0x50cb8c, 17 slots; MemoryImage 0x50cbd4 and DDImage 0x50cabc share them): 0 dtor, 1 PolyFill3D,
2 FillRect(rect, color, mode), 3 DrawRect, 4 ClearRect, 5 DrawLine(4 doubles, color, mode), 6 DrawLineAA,
7 FillScanLines, 8 FillScanLinesWithCoverage, 9 Blt(image, x, y, src, color, mode), 10 BltF, 11 BltRotated,
12 StretchBlt(image, dest, src, clip, color, mode, fast), 13 BltMatrix, 14 BltTrianglesTex, 15 BltMirror,
16 StretchBltMirror. **MemoryImage** (35) adds 17 GetNativeAlphaData(NativeDisplay*), 18 GetRLAlphaData,
19 GetRLAdditiveData(NativeDisplay*), 20 PurgeBits, 21 DeleteSWBuffers, 22 Delete3DBuffers, 23 DeleteExtraBuffers,
24 ReInit, 25 BitsChanged, 26 CommitBits, 27 DeleteNativeData, 28 Clear, 29 SetBits(bits, w, h, commit),
30 Create(w, h), 31 GetBits, 32 SetImageMode(hasTrans, hasAlpha), 33 SetVolatile, 34 Palletize. **DDImage** (50) adds
35 SetVideoMemory, 36 RehupFirstPixelTrans, 37 NormalFillRect, 38 AdditiveFillRect, 39 NormalBlt, 40 AdditiveBlt,
41 NormalDrawLine, 42 AdditiveDrawLine, 43 NormalDrawLineAA, 44 AdditiveDrawLineAA (empty), 45 NormalBltMirror,
46 AdditiveBltMirror, 47 LockSurface, 48 UnlockSurface, 49 SetSurface. `DDImage::Check3D(Image*)` is 0x40b5e0, its
argument in EAX.

**Font** (0x50cccc), **SysFont** (0x50cc84) and **ImageFont** (0x50cd44), 12 slots: 0 dtor, 1 GetAscent,
2 GetAscentPadding, 3 GetDescent (= mHeight - mAscent), 4 GetHeight, 5 GetLineSpacingOffset, 6 GetLineSpacing
(= mHeight + mLineSpacingOffset), 7 StringWidth, 8 CharWidth (it builds a 1-char string), 9 CharWidthKern(c, prev),
10 DrawString(g, x, y, text, color, clip), 11 Duplicate (pure in Font). **ImageFont** (23) adds
12 GenerateActiveFontLayers, 13 DrawStringEx(g, x, y, text, color, clip*, drawnAreas*, width*), 14 SetPointSize,
15 GetPointSize, 16 SetScale(double), 17 GetDefaultPointSize, 18 AddTag, 19 RemoveTag, 20 HasTag, 21 GetDefine,
22 Prepare; its slot 10 (0x42d080) forwards to slot 13 with (&clip, NULL, NULL).

**MusicInterface** (0x51f2fc) and **BassMusicInterface** (0x51f354), 20 slots: 0 dtor, 1 LoadMusic(id, file),
2 PlayMusic(id, offset = 0, noLoop = false), 3 StopMusic, 4 PauseMusic, 5 ResumeMusic, 6 StopAllMusic, 7 UnloadMusic,
8 UnloadAllMusic, 9 PauseAllMusic, 10 ResumeAllMusic, 11 FadeIn(id, offset = -1, speed = 0.002, noLoop = false),
12 FadeOut(id, stop = true, speed = 0.004), 13 FadeOutAll, 14 SetSongVolume, 15 SetSongMaxVolume, 16 IsPlaying,
17 SetVolume, 18 SetMusicAmplify, 19 Update. FModMusicInterface's 22 slots were not analysed.

**SoundManager** (0x51f21c) and **DSoundManager** (0x51f29c), 18 slots: 0 dtor, 1 Initialized, 2 `int LoadSound(name)`
(it calls slot 3), 3 `bool LoadSound(id, name)`, 4 ReleaseSound, 5 SetVolume, 6 SetBaseVolume(id, double),
7 SetBasePan(id, int), 8 GetSoundInstance, 9 ReleaseSounds, 10 ReleaseChannels, 11 GetMasterVolume, 12 SetMasterVolume,
13 Flush (empty), 14 SetCooperativeWindow(HWND, bool), 15 StopAllSounds, 16 GetFreeSoundId, 17 GetNumSounds.
**SoundInstance** (0x51f1e8) and **DSoundInstance** (0x51f268), 12 slots: 0 dtor, 1 Release, 2 SetBaseVolume,
3 SetBasePan, 4 AdjustPitch, 5 SetVolume, 6 SetPan, 7 Play(looping, autoRelease), 8 Stop, 9 IsPlaying, 10 IsReleased,
11 GetVolume. PlaySample(id, pan) calls GetSoundInstance, SetPan and Play(false, true).

**Graphics** (0x50cc7c) has one slot, its deleting destructor 0x425f00.

## 5. Layouts

| Class (size) | Fields |
|---|---|
| WidgetContainer (0x54) | vtbl 0; mWidgets (list) 0x04, head 0x08, size 0x0c; mWidgetManager 0x10; mParent 0x14; mUpdateIteratorModified 0x18; mUpdateIterator 0x1c/0x20; mLastWMUpdateCount 0x24; mUpdateCnt 0x28; mDirty 0x2c; mX 0x30; mY 0x34; mWidth 0x38; mHeight 0x3c; mHasAlpha 0x40; mClip 0x41 (default 1); mWidgetFlagsMod {mAddFlags 0x44, mRemoveFlags 0x48}; mPriority 0x4c; mZOrder 0x50 |
| Widget (0x88) | mVisible 0x54; mMouseVisible 0x55; mDisabled 0x56; mHasFocus 0x57; mIsDown 0x58; mIsOver 0x59; mHasTransparencies 0x5a; mColors (vector) 0x5c, first 0x60, last 0x64, end 0x68; mMouseInsets 0x6c; mDoFinger 0x7c; mWantsFocus 0x7d; mTabPrev 0x80; mTabNext 0x84 |
| ButtonWidget (0x120) | mId 0x88; mLabel 0x8c; mLabelJustify 0xa8; mFont 0xac; mButtonImage 0xb0; mOverImage 0xb4; mDownImage 0xb8; mDisabledImage 0xbc; mNormalRect 0xc0; mOverRect 0xd0; mDownRect 0xe0; mDisabledRect 0xf0; mInverted 0x100; mBtnNoDraw 0x101; mFrameNoDraw 0x102; mButtonListener 0x104; mOverAlpha 0x108; mOverAlphaSpeed 0x110; mOverAlphaFadeInSpeed 0x118 (doubles) |
| DialogButton (0x138) | mComponentImage 0x120; mTranslateX 0x124; mTranslateY 0x128; mTextOffsetX 0x12c; mTextOffsetY 0x130 |
| HyperlinkWidget (0x148) | mColor 0x120; mOverColor 0x130; mUnderlineSize 0x140; mUnderlineOffset 0x144 |
| Dialog (0x150) | ButtonListener vptr 0x88; mDialogListener 0x8c (gSexyAppBase+4, null-checked); mComponentImage 0x90; mYesButton 0x94; mNoButton 0x98; mNumButtons 0x9c; mDialogHeader 0xa0; mDialogFooter 0xbc; mDialogLines 0xd8; mButtonMode 0xf4 (1 YES_NO, 2 OK_CANCEL, 3 FOOTER); mHeaderFont 0xf8; mLinesFont 0xfc; mTextAlign 0x100; mLineSpacingOffset 0x104; mButtonHeight 0x108; mBackgroundInsets 0x10c; mContentInsets 0x11c; mSpaceAfterHeader 0x12c; mDragging 0x130; mDragMouseX 0x134; mDragMouseY 0x138; mId 0x13c; mIsModal 0x140; mResult 0x144; mButtonHorzSpacing 0x148; mButtonSidePadding 0x14c |
| UpdateCheckDialog (0x160) | finished (bool) 0x150; the progress bar's tiled image 0x154; scroll offset 0x158 ((x+1)&31 when mUpdateCnt%3 == 0 and not finished); 0x15c = 0x42 (the bar's y is mHeight - 0x42 - mContentInsets.mBottom). When the transfer completes it calls gSexyApp->DoDialog with id mId+20000 (up to date, a footer OK button) or mId+10000 (a new version, yes/no) |
| Checkbox (0xec) | mListener 0x88; mId 0x8c; mChecked 0x90; mUncheckedImage 0x94; mCheckedImage 0x98; mCheckedRect 0x9c; mUncheckedRect 0xac; mOutlineColor 0xbc; mBkgColor 0xcc; mCheckColor 0xdc |
| Slider (0xb8) | mListener 0x88; mVal (double) 0x90; mId 0x98; mTrackImage 0x9c; mThumbImage 0xa0; mDragging 0xa4; mRelX 0xa8; mRelY 0xac; mHorizontal 0xb0 |
| EditWidget (0x124) | mId 0x88; mString 0x8c; mPasswordDisplayString 0xa8; mFont 0xc4; mWidthCheckList 0xc8; mEditListener 0xd4; mShowingCursor 0xd8; mDrawSelOverride 0xd9; mHadDoubleClick 0xda; mCursorPos 0xdc; mHilitePos 0xe0; mBlinkAcc 0xe4; mBlinkDelay 0xe8 (40); mLeftPos 0xec; mMaxChars 0xf0; mMaxPixels 0xf4; mPasswordChar 0xf8 (one byte); mUndoString 0xfc; mUndoCursor 0x118; mUndoHilitePos 0x11c; mLastModifyIdx 0x120 |
| WidgetManager | mApp 0x5c; mPopupCommandWidget 0x6c; mLastDownWidget 0x8c; mOverWidget 0x90; mPreModalInfoList 0xb0 (head 0xb4; node data mBaseModalWidget +8, mPrevBaseModalWidget +0xc, mPrevFocusWidget +0x10); mMouseIn 0xdc; mLastMouseX 0xe0; mLastMouseY 0xe4; mKeyDown[] 0xf4 (SHIFT at 0x104) |
| Graphics (0x68) | vfptr 0, then the GraphicsState base: mDestImage 0x04; mTransX 0x08; mTransY 0x0c; mScaleX 0x10; mScaleY 0x14; mScaleOrigX 0x18; mScaleOrigY 0x1c (floats); mClipRect 0x20; mColor 0x30; mFont 0x40; mDrawMode 0x44 (0 normal, 1 additive); mColorizeImages 0x48; mFastStretch 0x49; mWriteColoredString 0x4a (ctor sets 1); mLinearBlend 0x4b; mIs3D 0x4c; mPFActiveEdgeList 0x50; mPFNumActiveEdges 0x54; mPFNumVertices 0x58; mStateStack (list) 0x5c |
| Image (0x38) | vtbl 0; mDrawn 0x04; mFilePath 0x08; mWidth 0x24; mHeight 0x28; mNumRows 0x2c (1); mNumCols 0x30 (1); mAnimInfo 0x34 (AnimInfo is 0x30 bytes, deep-copied) |
| MemoryImage (0x6c) | mBits 0x38 (w*h+1 with the sentinel 0x4beefade); mBitsChangedCount 0x3c; mD3DData 0x40; mD3DFlags 0x44; mColorTable 0x48; mColorIndices 0x4c; mForcedMode 0x50; mHasTrans 0x51; mHasAlpha 0x52; mIsVolatile 0x53; mPurgeBits 0x54; mWantPal 0x55; mNativeAlphaData 0x58; mRLAlphaData 0x5c; mRLAdditiveData 0x60; mBitsChanged 0x64; mApp 0x68 |
| DDImage (0xec) | mDDInterface 0x6c; mSurface 0x70; mSurfaceSet 0x74; mNoLock 0x75; mVideoMemory 0x76; mFirstPixelTrans 0x77; mWantDDSurface 0x78; mDrawToBits 0x79; mLockCount 0x7c; DDSURFACEDESC 0x80 (0x6c bytes). DDInterface: mApp 0x2c, mCritSect 0x3c |
| Font (0x14) | mAscent 0x04; mAscentPadding 0x08; mHeight 0x0c; mLineSpacingOffset 0x10 |
| SysFont (0x20) | mHFont 0x14; mApp 0x18; mDrawShadow 0x1c; mSimulateBold 0x1d |
| ImageFont (0x50) | mFontData 0x14; mPointSize 0x18; mTagVector 0x1c; mActiveListValid 0x2c; mActiveLayerList 0x30; mScale (double) 0x40; mForceScaledImagesWhite 0x48. FontData is 0x1b0 bytes, mDefaultPointSize at +0x5c |
| DSoundManager (0x30a0) | mSourceSounds[256] 0x04; mSourceFileNames[256] 0x404; mPrimaryBuffer 0x2004; mSourceDataSizes 0x2008; mBaseVolumes (double[256]) 0x2408; mBasePans 0x2c08; mPlayingSounds[32] 0x3008; mMasterVolume 0x3088; mLastReleaseTick 0x3090; mDirectSound 0x3094; mHaveFMod 0x3098 |
| DSoundInstance (0x38) | mSoundManagerP 0x04; mSourceSoundBuffer 0x08; mSoundBuffer 0x0c; mAutoRelease 0x10; mHasPlayed 0x11; mReleased 0x12; mBasePan 0x14; mBaseVolume 0x18; mPan 0x20; mVolume 0x28; mDefaultFrequency 0x30 |
| MusicInterface (4), BassMusicInterface (0x18), FModMusicInterface (0x20) | BassMusicInterface: mMusicMap 0x04; mMaxMusicVolume = 40; MOD load flags 0x204 for BASS 2 (LOOP, RAMP) or 4 |
| Buffer | vptr; vector<uchar> 0x04; mDataBitSize 0x14; mReadBitPos 0x18; mWriteBitPos 0x1c; GetDataLen = (bits+7)/8 |

**SexyAppBase** (vptr 0, DialogListener vptr 4; sizeof effectively 0x638):

| Group | Fields |
|---|---|
| start | mRandSeed 0x08; mCompanyName 0x0c ("PopCap"); mFullCompanyName 0x28 ("PopCap Games"; HKLM PartnerName can override it); mProdName 0x44; mTitle 0x60; mRegKey 0x7c; mChangeDirTo 0x98; mWidth 0xc0; mHeight 0xc4; mMusicVolume 0xd0; mSfxVolume 0xd8; mNoSoundNeeded 0xf0; mWantFMod 0xf1; mBetaValidate 0x11c |
| window and widgets | mWidgetManager 0x320; mDialogMap 0x324 (size 0x32c); mShutdown 0x341; mIsWindowed 0x343; mHWnd 0x350; mInvisHWnd 0x354; mDDInterface 0x36c; mMusicInterface 0x374; mRegisterLink 0x37c; mProductVersion 0x398; mCursorImages[13] 0x3b4 |
| state | mIsOpeningURL 0x3ec; mShutdownOnURLOpen 0x3ed; mOpeningURL 0x3f0; mOpeningURLTime 0x40c; mMuteCount 0x428; mAutoMuteCount 0x42c; mUpdateCount 0x484; mCursorNum 0x4b0; mSoundManager 0x4b4; mMinimized 0x4cf; mIsDisrupted 0x4d1 |
| loading | mLoadingThreadStarted 0x4f9; mLoadingThreadCompleted 0x4fa; mLoaded 0x4fb; mYieldMainThread 0x4fc; mLoadingFailed 0x4fd; mCustomCursorsEnabled 0x500; mLastShutdownWasGraceful 0x502; mNumLoadingThreadTasks 0x504; mCompletedLoadingThreadTasks 0x508 |
| demo | mRecordingDemoBuffer 0x50c; mPlayingDemoBuffer 0x50d; mManualShutdown 0x50e; mDemoFileName 0x52c; mDemoBuffer 0x548. The demo command enum is 1.3's (REGISTRY_READ 11, REGISTRY_WRITE 12, FILE_EXISTS 14, FILE_READ 15) |
| properties | mStringProperties 0x5f0 (a map of string to wstring: node key +0xc, value +0x28); mBoolProperties 0x5fc; mIntProperties 0x608; then the double map 0x614 and the string-vector map 0x620 by pcf's order (not verified); 0x62c is probably mResourceManager and 0x630 pcf's mOldWndProc |

**SexyApp** (pcf's order): mInternetManager 0x638; mBetaSupport 0x63c; mReferId 0x640; mVariation 0x65c; mDownloadId
0x678; mRegSource 0x67c ("ingame"); mLastVerCheckQueryTime 0x698; mSkipAd 0x69c; mDontUpdate 0x69d; mBuildNum 0x6a0;
mBuildDate 0x6a4; mUserName 0x6c0 (the name entered; not mRegUserName); mRegUserName 0x6dc; mRegCode 0x6f8;
mIsRegistered 0x714; mBuildUnlocked 0x715; mTimesPlayed 0x718; mTimesExecuted 0x71c; mTimedOut 0x720. A successful
registration sets mUserName, mRegCode and mIsRegistered, and WriteToRegistry then copies mUserName into mRegUserName.

**WinDMApp** (data from 0x724, sizeof 0x878; the names are in `games/bejeweled/src/WinDMApp.h`): the `diff.txt` values
0x724-0x774; mBoard 0x788; mTitleScreen 0x78c; mImages (Image*[36]) 0x790; the gem image table 0x794; the
HighScoreWidget 0x798; mLastScoreName 0x79c; mButtons (list) 0x7b8; fonts alphadigits4 0x7c4, scorefont1 0x7c8, afont1
0x7cc, mainfont1 0x7d0, overfont2 0x7d4; the mode buttons 0x808/0x80c; colorized images 0x810; mTimedMode 0x874;
mNoPopLink 0x876; mIsDiamondMine 0x877. Its classes: Board is a Widget only (its data starts at +0x88); TitleScreen and
HighScoreWidget are Widget + ButtonListener (listener vptr at +0x88); HighScoreDialog is a Dialog; OptionsDialog is
Dialog + CheckboxListener (vtable 0x506fa0) + SliderListener (0x506fa8); RecordScoreDialog and RegisterDialog are
Dialog + EditListener; WinDMApp's primary vtable starts with the ButtonListener slots, and its DialogListener vtable
is 0x507cc0.

**Globals:** the app object 0x53b570 (WinMain calls Init, Start and Shutdown on it non-virtually); gSexyAppBase 0x53dc8c
(set in the SexyAppBase constructor 0x43eaa0); gSexyApp 0x53dd2c (set in SexyApp::SexyApp 0x488aa0, used for GetString
0x44d330 and DoDialog); gMTRand 0x571ff8 (mti at 0x5729b8); gBass 0x53dd38 and the FMod function table 0x53dd40; the
app data folder string 0x53a8a8 (GetAppDataFolder 0x4693e0 returns a copy); the image-name table 0x536250;
GraphicsState::mStaticImage 0x53df7c; the CheckForVista cache 0x572f81 with its init flag 0x53afcd;
DIALOG_YES/NO/OK/CANCEL_STRING 0x53a8c4, 0x53a8e0, 0x53a8fc, 0x53a918, which WinDMApp::Init overwrites with
GetString("DIALOG_BUTTON_YES" and so on).

**Default colour tables** (int[n][3]): gDialogColors 0x539aa0 = {255,255,255}, {255,255,0}, {255,255,255} x3,
{80,80,80}, {255,255,255}, of which entries 3 and 4 are zeroed by the Dialog constructor when there is no button image;
gButtonWidgetColors 0x539a58 = {0,0,0} x3, {255,255,255}, {132,132,132}, {212,212,212}; gDialogButtonColors 0x539af8 =
{255,255,255} x2, {0,0,0}, {255,255,255}, {132,132,132}, {212,212,212}; gEditWidgetColors 0x5399d4 = {255,255,255},
{0,0,0} x3, {255,255,255}.

## 6. Behaviour the port depends on

**Random numbers.**
- `Sexy::Rand()` (0x469260) is textbook MT19937 on the one global generator (mag01 {0, 0x9908b0df}, standard
  tempering) and returns the tempered value & 0x7FFFFFFF, so a draw is 31 bits and never negative; callers do their own
  `% n`, and `Rand() % 2^k` equals `Rand() & (2^k - 1)` (the `and eax,0x80000007; jns; dec; or; inc` sequence is only
  the signed-% fix-up).
- `SRand(seed)` maps 0 to 4357 and initialises with `mt[i] = 1812433253 * (mt[i-1] ^ (mt[i-1] >> 30)) + i`
  (0x6c078965). The static initializer seeds 4357; `SexyAppBase::Init` (0x44e4d0) does `mRandSeed = GetTickCount();
  SRand(mRandSeed); srand(GetTickCount())`; demo playback (0x440853) checks the magic 0x42BEEF78 and seeds from the
  file. Nothing in the game region seeds it.
- During play the game's 11 draw sites are the generator's only consumers. The framework's own draws,
  `SexyApp::PreTerminate` (0x48bdb0, slot 21) and its callee 0x466540, run only at shutdown.
- CRT `rand` (0x4dd837) is the MSVC LCG, `x = x*214013 + 2531011; return (x>>16) & 0x7fff`, used only for the
  framework's temp-file names. `MTRand::SetRandAllowed` exists (counter 0x53dd1c), but its asserts are compiled out.

**Floating point.**
- Every `(int)double` in game code (66 sites) and every Graphics coordinate goes through `_ftol2_sse` (0x4d99c0), which
  truncates toward zero. Its SSE2 path (`cvttsd2si`) returns 32 bits and gives 0x80000000 out of range; only the x87
  fallback `_ftol2` (0x4d99f6) produces EDX:EAX.
- `sin` and `cos` use the SSE2 kernels (`_sin_pentium4` 0x4e42d8, `_cos_pentium4` 0x4e4648) when `__use_sse2_mathfcns`
  (0x574180, set at startup from CPUID) is set and both MXCSR and the x87 control word mask all exceptions; the x87
  fallback uses fsin/fcos with the control word forced to 0x27f. `atan2` and `sqrt` are x87 only, with the same
  forcing. `pow` is not used. The framework uses the intrinsic `_CIsin` and `_CIcos` in its rotated blitters.
- The game-region wrappers 0x4c9a94 (atan2f), 0x4c9ab7 (sqrtf) and 0x4c9ad1 (atan2(float, float)) round their result to
  float before it is stored in a double field.
- Precision control: CRT startup forces 53 bits. DirectDraw's SetCooperativeLevel is called with DDSCL_NORMAL (0x40205e)
  or EXCLUSIVE|FULLSCREEN (0x402322), never with DDSCL_FPUPRESERVE, so if the D3D7 device is created (the 3D mode) D3D
  may switch the x87 to 24 bits and game double arithmetic would then round like float. Not verified at run time; in
  the default DirectDraw mode the precision stays 53 bits. The framework's blitters set and restore the control word
  locally for `fistp`.

**Graphics.**
- Every Draw*, FillRect and ClipRect computes float(int argument) + mTransX and truncates. A scaled DrawImage(x, y, src)
  uses ceil (0x4dc570) for the size and floor (0x4f6310) for the position, with float intermediates.
- DrawRect draws the outline of (w+1) x (h+1), through Image slot 3 when unclipped, otherwise as 4 FillRects.
- The NULL-image constructor uses `mStaticImage`, so its clip rect is (0,0,0,0).
- `MemoryImage::Create` sets mHasTrans = mHasAlpha = 1; `DDImage::Create` does not touch them (as 1.3), so the game
  calls SetImageMode(0,0) before Create on its DDImages.
- The linker dropped unreferenced members such as GetFastStretch, GetLinearBlend, ClearRect and DrawImageMirror.

**Properties and strings.** `mStringProperties` holds wide strings. `GetString(id)` returns WStringToString(value)
(wcstombs into a 16 KB buffer; "" on failure or when the id is missing); `GetString(id, default)`, `GetInteger` and
`GetBoolean` return the default when missing. LoadProperties() loads `properties\default.xml` with required = true and
checkSig = false. MsgBox and Popup keep string and wstring overloads.

**HSLToRGB(h, s, l)** is exactly the 1.3 code: `v = (l < 128) ? l*(255+s)/255 : l + s - l*s/255` in int math stored as
a double; `y = (int)(2l - v)`; x and z are computed in x87 double and truncated, x clamped to <= 255 and z to >= 0; the
result is 0xFF000000 | r<<16 | g<<8 | b.

**Registry.** The root is HKCU\SOFTWARE\PopCap\Bejeweled; reads use KEY_READ, writes open with KEY_WRITE or create with
KEY_ALL_ACCESS. SexyApp reads ReferId, RegisterLink (default
`http://www.popcap.com/register.php?theGame=...&referid=...`), Variation, PartnerName, DontUpdate and DownloadId from
HKLM\SOFTWARE\PopCap\Bejeweled, and LastVerCheckQueryTime from HKCU first, then HKLM; properties can override ReferId,
RegisterLink, DontUpdate and PartnerName. The base SexyAppBase values (MusicVolume, SfxVolume, Muted, ScreenMode,
CustomCursors, InProgress...) were taken from 1.3 and not re-verified.

**User data.** `GetAppDataFolder()` returns "" (the current directory) unless CheckForVista() is true. On Vista and
later `SexyAppBase::Init` asks SHGetFolderPathA for CSIDL_COMMON_APPDATA (through shell32.dll, falling back to
shfolder.dll) and the folder is that path + `\` + mFullCompanyName + `\` + mProdName + `\`, that is
`%ProgramData%\PopCap Games\Bejeweled\`; mDemoFileName is prefixed with it, and WinDMApp::Init runs MkDir,
AllowAllAccess and the userdata migration there. ReadBufferFromFile goes through `p_fopen`, so it reads from a pak too;
EraseFile is DeleteFileA, the only Win32 call the game region makes directly.

**Online code.** InternetManager (0x54c bytes) does the update check, GET
`http://www.popcap.com/win32updatecheck.php?prod=<mProdName>&ver=<mProductVersion>&referid=<ReferId>`, and the ad
popups (adlist.txt). The port leaves all of it out.

**Other.** `SetMasterVolume` writes mSfxVolume and does not null-check mSoundManager (as 1.3 and pcf).
