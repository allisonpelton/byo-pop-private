# Banana Bugs: working notes (analysis state, kept so a compaction loses nothing)

Banana Bugs (PopCap, 2010). The clear image is `re/bananabugs/bin/BananaBugsGame.exe` (the 2010 DRM launcher and its `.dat`
are not used; `docs/POPCAP_GAMES_SURVEY.md`, 5.15). VS2005, linked with link-time code generation: many functions take
their arguments in registers (`in_EAX`, `unaff_ESI`, `unaff_EDI` in the decompile), x87 values pass on the FPU stack, and
`FUN_005bb020` (`_ftol2`) shows with its operand lost. So the decompile is read beside `tools/disasm.py` for nearly every
function that calls, draws or does float maths. There is no Mac build: every name is our own, from RTTI, strings and use.

## Tools (games/bananabugs/tools)

- `stage_bananabugs_assets.py`: unpacks `Main.pak` (XOR 0xF7; 338 pictures and one font) lower-cased into
  `assets/bananabugs`, with the loose `sounds/*.ogg` (the music too) and `properties/resources.xml` (sounds only) and
  `partner.xml`: 460 files, 72 MB.
- `decread.py`, `disasm.py`, `pe.py`: Peggle's, pointed at `re/bananabugs/game` (a copy of the main checkout's export:
  `all.c`, `functions.jsonl`, `strings.jsonl`, `vtables.jsonl`) and `docs/symbols.txt`.
- `vtable.py [--base Class] Class...`: a vtable with our names, `*` on the slots that differ from the base's.

## The code

- Game code 0x401000-0x476000 (1,528 functions, 466 KB, std templates included). The framework follows (DDInterface
  0x475f60 on), then the C runtime from about 0x59e000. The game calls only about 69 framework functions directly; most
  small framework calls are inlined or wrapped in the game's own helpers.
- Shared stubs (folded by the linker): 0x42c290, 0x4d4050, 0x5656b0, 0x4d6be0 `ret`; 0x4e2910, 0x5656c0 `return 0`.
- Library: 0x402100 `std::string::_Tidy`, 0x402430 `std::string::assign(const char*, len)`, 0x59f27f `operator new`,
  0x59f61d `free`, 0x5bb020 `_ftol2`.
- Framework calls seen: 0x479080 `Color(r,g,b,a)` (this in EAX), 0x49a870 `Graphics::SetColor` (color in EAX),
  0x49a900 `Graphics::FillRect(x,y,w,h)` (g in EAX), 0x4b35e0 `SexyTransform2D` identity (in EAX), 0x4790d0 a getter.

### Classes (RTTI, constructor, allocation size)

The screens and the scene objects all derive from `ObjBase` (14 slots, size 0x27c, ctor 0x4509f0); only `GameWidget`
(0x42f230, 0xc0) is a Widget. ObjBase slots: 0 dtor, 1 Init (0x450dd0), 2 (obj, bool) 0x450e40, 3 0x450ec0 (unregisters),
4 forwards to another object's slot 4 (0x450f10), 5 Update (0x450f30, returns false when the object is done),
6 AddToDrawList (0x4511a0: appends to the global list 0x63f410, count 0x670150), 7 Draw(Graphics*) (0x4511c0),
8 and 9 return 0, 10 to 13 empty.

Constructors (and the second address, the destructor) by class, from `= Sexy::X::vftable` stores; sizes from
`operator new` before the call: Board 0x405770 (0x7a0), MetaGame 0x448e10 (0x918), MainMenu 0x43fc90 (0x38c),
LoadGame 0x43f430 (0x298), Transition 0x470a60 (0x304), PopCap 0x45e8d0 (0x288), GameApp 0x42c2b0, BBFont 0x42b8f0
(0x30d0); the full list is in `build/bb_vtables.txt` and the ctor scan (`$TMP/ctors.py`, to be made a tool).

### GameApp (SexyApp, 106 slots)

- Overrides: 8 UpdateFrames 0x42d940, 16 LoadingThreadCompleted 0x42d330, 17 dtor 0x42c660, 22 LoadingThreadProc
  0x42d1f0 (the resource group "SoundFX"), 49 Init 0x42d030, 68 SwitchScreenMode(windowed, is3d, force) 0x42da50,
  82 GotFocus 0x42d9a0 and 83 LostFocus 0x42d970 (one slot later than Bejeweled's 97-slot table: Banana Bugs' SexyAppBase
  has 99 slots, SexyApp 106).
- The game works in a 1920x1200 space: windowed it is 800x600 (scale 2.4 by 2.0, 0x794/0x798), full screen the
  desktop's size (0x600564, 0x600568). 0x7ac: the screen is wider than 4:3 (w*100/h > 133). 0x343: 3D.
- LoadingThreadCompleted loads the music 0-10 (music1-4, meta_music, mainmenu, statsriff, victorymusic, music_intro,
  statsriff2, music5) and fades in 4 (meta_music? to check), makes the object at 0x78c (size 0x198, ctor 0x46c7e0) and
  the GameWidget (0x790), adds it to the widget manager, and calls 0x428190, 0x435aa0, 0x4346e0, 0x45e7f0.
- Init (0x42d030): SexyApp::Init, a registry/user-data move, the j2k codec, a key string, the resources manifest.

### GameWidget (0x42f230; Update 0x42f510, Draw 0x42fc40)

A state machine of screens at +0x8c: 0 the PopCap logo (+0xb4, ctor 0x45e8d0), 1 LoadGame (+0xb8, 0x43f430), 2 a
Transition (0x470a60) then the MainMenu (+0x98, 0x43fc90), 4 the Board (+0x94, 0x405770) or the MetaGame (+0x9c,
0x448e10) by the profile (global 0x670198, field 0xee*4), 5 the board, 7 the meta game. Mouse and keys: slots 47 KeyChar
0x42fd60, 48 KeyDown 0x42fd70, 52 MouseMove 0x42fcc0, 54 MouseDown 0x42fd00, 57 MouseUp 0x42fd30, 58 MouseDrag 0x42fce0.

## Reaching a level in a run

The test commands so far (src/TestCommands.cpp) are `state` and `set progress|tutorial|walkthru|score|lives <n>`. A
script reaches level n + 1 with bugs this way: at update 1000 type a name (`char` T, E, S, T; earlier, the New Player
window is not open yet), `1100 key RETURN`, then `set progress n`, `set tutorial 1`, `set walkthru 1`; Play at
(418, 207); the game asks to continue the last game: YES at (310, 320); the map's continue at (600, 555). The level is
playing by about update 2600. Level 1 with the tutorial not done has no bugs of its own (the intro brings them); the
first four levels use only the skeeter and the beetle. After them each kind comes in at its first level (the spawn
table, LevelObj::InitSpawnTable): the dragonfly at 9, the roly-poly at 17, the caterpillar at 21, the roach at 25, the
fly at 37, the ant at 58, the centipede at 73, the moth at 101 and the bee at 121 (levels counted from 1). `state`
lists the kinds of the bugs in play. `win` sets the percent filled to the target, so the level ends through its own
check and shows its tally. The tally (and the level's other clicks) read whether the button is down during an update
(ObjBase::sMouseHeld), as the original does, so a script's `click`, which presses and releases within one update, is
not seen: use `down` and `up` some updates apart, as real input is. The level's MENU button is at the screen's top
left, (25, 15) in an 800 by 600 window (`state` gives its centre in design space); its menu's Resume, Options and
Main Menu are at (400, 236), (400, 300) and (400, 364), taking clicks after 50 updates.

## Test commands and the bot

The commands live in src/TestCommands.cpp (the header comment lists them) and the bot in src/Bot.cpp. `state` prints
one JSON line: screen (menu, map, level, stats, victory, slots, intro, and the loading and transition ones), sub (the
screen's own state), over, update, pending, player, level, levelNo, score, lives, bananas, percent, target, mode, shots,
monkey (pos, cell, state, shotPhase, cooldown, aim, vertical, from, to, locked, holding, held), rope (state, vertical,
from, to, cell), origin, field, bugs, alive, kinds and menuButton; `state bugs` adds every bug and the menu's buttons.
`player [name]` adds or selects a profile (default BOT), `level <n>` goes to level n through the game's own steps
(progress, unblocked bonuses, tutorial done for n above 1) and answers at once with `pending` 1 until the level plays,
`goto menu|map|level`, `win`, `lose` (one life left, then the game's own LoseLife) and `seed <n>` (CrtSRand and the
level generator GameRandomSeed). The game seeds GameRandom itself in LevelObj::InitCells (level + 0x58d2764), so a
`seed` before a level starts is overwritten: seed after the level is playing. The slots seed both generators from the
tick count; SexyApp seeds CrtRand at start (`-seed=N` overrides). From nothing: `player BOT`, then `level n`; or
`move` alone, which types a name and clicks through the menu.

`move` prints `MOVE {...}` (input steps in 800x600 window coordinates, updates, what) or `MOVE null`. Facts the steps
depend on: a click counts only if the button is down during an update; hover is computed in DrawAll, which under
`-fast` runs every 10th update, so the bot waits 12 updates after the pointer arrives before pressing; the monkey
follows the mean of the last two mouse positions and a repeated move to the same point sends no event, so a shot starts
with a move from (x-1, y). The bot never changes state and draws from no generator. Strategy: enumerate the stretches
between consecutive active pipes in every row and column, copy LevelObj::ShowRopePreview's flood fill to get the area
each would cut (the smaller side; both if each side is at most 32 places or no bug lives), reject lines with a bug
within 36 px + 0.16 px per px of length (the rope flies shotInterval * length + 10 frames), take the biggest cut, else
the shortest safe line, else wait. Shots are blocked while the monkey is not PLAYING, shooting, on cooldown, a bug is
within the startle reach (64 px), or the rope is FLYING (CUTTING and SNAPPED stay until the next shot, so they do not
block). A held bonus is placed at a bug. Weaknesses: it never turns the rope (below), so only auto-oriented lines are
used; it is cautious and slow (a 300-move run from the menu reached 62 of 85 percent on level 1 after 23 shots, 7690
updates, no life lost); no stats, map, shop, victory, game-over, `lose`, `goto` and held-bonus path was exercised
beyond the intro and the menu clicks.

Game bug seen: a right click shoots instead of turning the rope. GameWidget::MouseDown passes theBtnNum (right is 1)
but BoardClick tests for a value below 0. Left untouched (original behaviour); to see it, right-click on a level.

## Translating a class (the register conventions, checked against the code)

Read each function three ways: `decread.py <addr>` for the shape, `callargs.py <addr>` for the registers and pushes at
every call, and `disasm.py <addr>` wherever callargs is ambiguous (a join after a branch, which key a register holds).
`tools/classmap.py` gives each class's address range; the decompile of a whole range is `decread.py <lo>-<hi>`.
The finished translations to copy the style from: `src/PopCap.cpp`, `src/Transition.cpp`, `src/Cursor.cpp`.

- Objects: `new ObjBase()` is 0x59f27f(0x27c) then 0x4509f0. A subclass's own fields start at +0x27c.
  0x451580 SetParent(ESI=obj, EAX=parent, EBX=owner); parent 0 is the engine's root. 0x4514a0 SetEventTarget
  (EAX=obj, ECX=target). 0x451830 SetState(ECX=obj, EAX=channel, float pushed); it leaves EAX as the channel, so an
  Anim::Get right after it may take that as its mode. 0x451870 SetStateInt (same, int pushed). 0x451990 ComputeDrawn
  (EBX=obj). 0x451880 AddTrack(EDX=obj, ESI=track, push next) returns the index. 0x451940 StartTrack(EDI=obj,
  EAX=index). Virtual calls: +0x8 Activate(obj, hidden) (`this` and the first argument are usually the same object),
  +0xc Deactivate(obj), +0x10 Notify, +0x14 Update, +0x18 Delete, +0x1c Draw; slot 0 with 1 is `delete`.
  0x473440 is `FromCorner(x, size)` (x - size/2: a place from the design space's corner, from its centre).
- Anims: 0x401c80 Anim::Get(name, mode, noScale, celW, celH, cels, uniform); 0x401210 AddRef (EAX=anim, returns it),
  0x401220 Release; 0x401950/0x401960 the source cel's width/height (EAX=anim). `obj+0x28` is mAnim.
- Keys: 0x59f27f(0x13c) then 0x453290 is `new AnimKey()`. +0x10 mEventId, +0x24 mEasing (0x453570, EAX=key,
  ECX=easing), +0x134 mAnim (with an AddRef), 0x453580 SetRemoveAtEnd. 0x453590 SetDuration(EAX=key, push frames).
  0x4533c0 AddModifier(EDI=key, ESI=channel, AL=keepCurrent; push value, kind, rate, amount, base, table): kind 0
  MOD_HOLD, 1 MOD_TOWARD_NEXT, 2 MOD_SINE...; with AL=0 and no rate/amount/base/table it is `AddValue(channel, value,
  kind)`. ESI often stays set from the call before: callargs shows a register only when it was just written, so read
  back to the last write. 0x4533b0 makes a sound key (EAX=key, ECX=KEY_SOUND (7), EDX=sound id, push the pitch range).
- Tracks: 0x59f27f(0x20) then 0x453930 is `new AnimTrack()` (ECX=track after). 0x4539f0 AddKey(ECX=track, EAX=key,
  push next); a last key whose next is itself holds. Check the order of the keys in the disassembly: they come from
  saved registers and stack slots. 0x453a90 SetDelay(EDX=track, ECX=frames, AL=skipFirstFrame).
- Sound ids are `SoundId` in `src/Res.h` (resources.xml order); `gApp->mSoundQueue->Request(id, pitchRange, loop)`.
- Strings shown to the player stay as the original has them (e.g. "CONTINUE LAST"); docs name them by id only.

## Fix candidates and guesses found so far (for GAME_NOTES)

- UserSelect: Enter while renaming checks the name against every profile, the player's own among them (0x472604,
  EBX -1), so an unchanged name is "already used"; OK skips the player's own and also turns down names under two
  characters, which Enter takes.
- Options skips BBWindow's update (0x4541a0 calls ObjBase's), so its OK and Cancel do not swell.
- Player (the agent's translation, kept as the original has them): Notify case 2 starts both decays from the cosine;
  UpdateLook compares |vx - vy| with 0.01; Update's "copy previous cell" branch never runs; UpdateAim's right and
  bottom scans keep the starting cell when no wall is found; both Shoot branches pass the same rope arguments;
  SortProfiles sets the current index to the old slot; Update reads the level's rope unchecked (the port checks).
- Player guesses to check when the level is in: mBugKinds (+0x2c8), mRopeMode, the bonus-blocked array, the upgrade
  field names, and which object three SetState calls act on (UpdateLook FRAME, Notify case 10 X/Y, spazz-done FRAME).
- FireBall (0x42ae80): a ball that lands once the level has reached its target, or with no bugs alive, is never put
  away; it stays where it landed, still trailing.
- BananaBonus (0x404b00): the last key of a bunch sets the alpha twice (0, then 0.1) and an orbit radius nothing reads.
- LevelObj's shuffle (0x4733f0) takes the C library's rand (CrtRand); the original seeds it (GetTickCount in
  SexyApp's Init, to check), so the test hooks need `-seed=` reseeding it like the other ports.
- PauseWin::PauseForFocusLoss (0x455920) calls LevelObj::CloseMenuForPause (0x43a4f0) at 0x455981: wired (PauseWin.cpp).
- InGameMenu's second button opens Options (0x453e30) without a parent; the quit button's AskWin is a child of the menu.
  PauseWin's picture (0x4550f0) has three tracks nothing starts (left from a button). Victory (0x473460) stores its
  coronas at +0x288 (the decompile shows them as the trophy's fields: a misread of `this`).
- LevelObj's morph table sums each pixel's channels on the x87 (port: doubles); a last-bit difference is possible.
- BackDrop's table names backgrounds up to 18, but the game ships background01..14 (ground_01..14): check which map
  levels exist before a late level runs.
- The dragonfly's body shares Notify 0x4237d0 with the skeeter's and the bee's, which starts the track whose number it
  reads at body +0x2a4; on the dragonfly's body that field holds an Anim pointer, so the original starts a track by a
  wild number. The port's BugImgDragonfly::Notify does nothing instead (a deviation to list: check against the original
  whether the wild start ever shows, StartTrack may simply ignore an index past its tracks).
- PathTrace::Load (0x454ab0) loops for ever on a path picture with a pixel that has no neighbour and has no bound on its
  points; the port stops at both (neither happens with the shipped pictures, to be checked).
- ScoreBoard::ShowScore (0x4684a0) blanks every leading '0' of the formatted score, so a score of 0 shows an empty
  field (seen in the port at a level's start); to check against the original's screen. ShowBananaTotal (0x4685a0)
  formats the banana total and never shows it (kept).
- Float constants: the decompile prints short rounded literals; Stats uses the exe's exact values (12.566371f,
  1.5707964f and so on). BananaBonus.cpp's 12.5664f, 6.37045f and 1.5708f are probably the same rounding: check them
  against the hex in the disassembly in the logic pass.
- BonusZuma's Update (0x412f20..) starts its end fade every frame the frog's cell stays cut, restarting the track each
  frame, so the bonus never ends while that lasts. The port's mEnding makes End run once. That changes what the
  original does: check in the original whether the frog ever goes on a cut cell and whether anything is blocked (a
  next bonus, the level's end) before keeping it as a fix of a soft lock; otherwise it belongs to the fixes mod.
- BonusParrot reads the link of a null next pipe at a wall's end or on a cell with no wall (a crash in the port); the
  port ends the bonus there. To check what the original does at that place (it reads whatever lies at that address).

## The bot's record (playbymove from level 5, 400 moves, -fast)

- Margin 36 + 0.16 px a px of line: levels 5 and 6 won, three lives lost, all on lines of 465 to 713 px.
- Margin 36 + 0.3: the same two levels, two lives lost, both to skeeters 160 px away.
- With the fliers' boxes grown by 80 px: level 5 won and level 6 at 61%, one life lost. Slower, steadier; the bot pass
  should weigh cut against risk rather than only reject.
