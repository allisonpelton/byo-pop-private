# Working notes (Word Harmony port)

The state of the work, for picking it up again (after a compaction or in a later session).

## Done

<!-- private -->
The game exe is carved out of `WordHarmony.exe` (PopCap's DRM wrapper) with `python tools/extract_game_exe.py
<install>/WordHarmony.exe -o re/wordharmony/bin/WordHarmonyGame.exe` (671,744 bytes).
<!-- /private -->

The game exe `re/wordharmony/bin/WordHarmonyGame.exe` is analysed in the Ghidra project `wordharmony_game` (`E:/retools/projects`) with `CLASS_RECOVERY=0`, and exported to
`re/wordharmony/gh` (`all.c`, `functions.jsonl`, `strings.jsonl`). `tools/vtables.py` lists the vtables
(`re/wordharmony/vtables.txt`) and `tools/ghidra_pass.sh` created the virtual functions, gave the indirect calls their
argument counts and exported again. The archive is extracted for reading to `re/wordharmony/saf` (git-ignored).
`tools/stage_wordharmony_assets.py` stages `assets/wordharmony/` (WHArchive.saf and res/). ENGINE.md has what is
known of the engine.

The coordinator was told (2026-10-26) that the engine is Sprout's first generation and shared with Pizza Frenzy and
Iggle Pop; the plan is to keep it in `engine/`, free of game code, so it can move to shared code later.

## Plan

The engine runs one frame per call (no modal loops), so the port hosts it from a framework widget like Ning Po's
(`games/ningpo/src/NingPoApp.*`), without ASYNCIFY: the widget's Update runs the frame (0x4542e0), its Draw blits the
800x600 screen. `engine/` is the engine translated (namespace `engine`), mapped onto the framework and `platform/`
through a host interface; `src/` is the game. STL code in the binary (map, list, deque, vector, string internals)
is not translated: the port uses the standard containers.

Order: Object and Ref; the render context over the framework's MemoryImage (scaled, rotated, alpha and colour
blits); nodes, containers, windows; images and fonts; the XML parser (0x43b1f0, translate it: the data has quirks such
as a repeated attribute); the screen layout and its widgets; the archive and the resource manager; particles; sound
and the music layers; the application; then the game from 0x401000 up.

## Port progress

Built and checked (`wordharmony_enginetest` parses all 52 XML files of the archive, 7366 elements): `engine/Object.h`
(Interface, Object, Ref), `Exception.h`, `Geometry.h`, `Graphics.h/.cpp` (render states; the image draw is pure, the
host's subclass implements it), `Font.h/.cpp`, `Xml.h/.cpp` (the parser 0x43b1f0 translated state by state, the
attribute getters), `Stream.h/.cpp` (memory streams: the host reads whole files), `ResourceManager.h/.cpp` (the
archive loader without the MD5 checks, the resource manager), `Host.h` (what the Windows layer got from Windows),
`Node.h/.cpp` (Node, Container, Window: every slot of the three vtables) and `App.h/.cpp` (the caches, the node map,
the image loader's mask rule, the frame 0x4542e0). The engine test: `TARGET=wordharmony_enginetest bash tools/build.sh
native`, then `build/native/rundesk.exe bash -c "build/native/wordharmony_enginetest.exe assets/wordharmony"`.

Next: the game's controller (created by 0x407370, 0x71c bytes, constructor 0x406980; its slots: 4 configure the
AppConfig at app+0x88, 8 init, 0xc shutdown, 0x10 update), then the screen layout (0x42c4f0-0x431000) and the
widgets it makes, the image node (0x435d50), TextItem, buttons, particles, sound and music, the host app in src/.
The run loop 0x4552b0: frame period 1000 / fps (fps = config +0x90); the root window is the Windows window class
(vtable 0x48a528, a Window subclass that owns the HWND). The App's slots: 0x0c is 0x427000, 0x64 a stopwatch in
seconds (+0xb8), 0x6c and 0x70 not yet read. The image loader (0x457510) reads a PNG or JPEG and, when
`<dir>_<name>.png` exists, takes its alpha from that mask (0x45ea10; which channel is not yet checked).

Screens, ported and checked (the engine test reads all 27 layouts of the archive into ScreenLayouts, 469 named nodes,
every font of the layouts loaded):
`ImageNode`, `FadeContainer` (0x429b70: groups and screens), `Selector` (0x426f10: one child shown, cross-fades),
`Button` and `ImageButton` (0x432600, 0x429c70: hover scaling, flashing, enter and press sounds, listeners told under
the button's action; a press is told on mouse down), `ScreenLayout` (0x428640: named nodes, passes a pressed action to
its ScreenListener, the controller's +0xc), `Layout` (the reader 0x42c7f0 with image, imageButton, group and include),
`Sound.h` (the two SoundSystem slots the buttons call; App::getSoundSystem is the global 0x49bfc0), `TextItem`
(0x427680: lines split at a literal backslash-n, aligned, underlined or struck through; setNumber puts commas in,
setTime gives h:mm:ss) and the text element 0x42ef20, `CheckBox` (0x483dc8: four images, a press flips it) and
`TextButton` (0x42c2c0: a text node whose font follows the mouse) with their elements 0x42d6f0 and 0x42e2b0, and
textSub 0x4300c0, and the particles (`Particles`, `Random`; the particleSystem element 0x42f760). The dumps are in
`re/wordharmony/work/psystem.c` (0x42b4c0-0x42c1e0), `particles.c` (0x430d20-0x434ce0) and `particles2.c`
(0x435300-0x435d50). Next: sound (SimpleSoundDX 0x4425f0-0x444970) and music (0x424160-0x426000), then the
controller and the game code. 0x430d20/0x430ee0 are not particles: they belong to another file reader (vtable
0x484288, slot 0x20 makes a 200-byte object 0x430c20 with lifeTime and name), to be read with the code that uses it.
The sounds file's handler (0x42bdf0, element "sound": name, file, vol, pitch, count, category) feeds 0x426760 of the
object at its +0x14 (the sound part).

The particles as ported. A system's children are streams; a stream's children are the particles it emitted, and its
pool (+0x178, counted) holds them all: a particle whose reference count is 1 is free. A stream's pool is made when its
end tag is read: (int)(the longest life x the emit rate x 1.5 + 1) particles, one per image <path>0, <path>1, ...
(.png, else .jpg) in rounds; the original loops forever when <path>0 is missing (smoke.xml, which no code names,
would hang it), the port stops. The emit credit starts at 1, so a started stream emits at its first update. emit
tries 25 random pool indexes (0x42bce0 never reaches its upper end, so the last particle is never picked) and draws the
particle's values in a fixed order (acceleration, position, start and end scale, life, velocity, rotation, opacity,
jitter rates, then the jitter amounts y before x); keep that order, the random sequence depends on it. A particle
lives in screen coordinates: applyTransform translates by its position less the graphics' current translation (no
rotation, no colour offset) and sets the stream's filter as the graphics' snap. Jitter (0x431930) adds amount x
cos(rate x time) to the position each frame. The random numbers are the C library's (Random.h); the game seeds them
with srand(GetTickCount()) in the controller 0x406b30, not ported yet. Slot 0x64/0x68 (start/stop, which containers forward)
are not the systems' start and stop, so the port names those startEmitting and stopEmitting.

Node vtable (0x484b50, 27 slots), by offset: 0 dtor, 4 getX, 8 getY, 0xc getPosition, 0x10 getAbsPosition, 0x14
setPosition(Point), 0x18 setPosition(x, y), 0x1c setScale, 0x20 setRotation, 0x24 getBounds, 0x28 setOpacity, 0x2c
getOpacity, 0x30 setFont, 0x34 setName, 0x38 getName, 0x3c applyTransform(Graphics), 0x40 contains(Point), 0x44
hitTest(Point), 0x48 hitTest(flags, Point), 0x4c toLocal, 0x50 parent's toLocal, 0x54 setVisible, 0x58 isVisible,
0x5c setEnabled, 0x60 isEnabled, 0x64 and 0x68 empty (start and stop: containers forward them to the children).
Container (0x481ca8) and Window (0x484a08) add: 0x6c draw, 0x70 update(UpdateInfo: origin Point, then the seconds
elapsed at +8, clamped to 0.33), 0x74 layout, 0x78 dump, 0x7c/0x80 mouse down (public, protected), 0x84/0x88 up,
0x8c/0x90 right down, 0x94/0x98 right up, 0x9c mouse move, 0xa0/0xa4 enter, 0xa8/0xac exit, 0xb0 setParent, 0xb4
clearParent, 0xb8 getParent, 0xbc setFlags (propagates up), 0xc0 clearFlags, 0xc4 notify listeners, 0xc8 removeChild,
0xcc setClipRect, 0xd0 addChild, 0xd4 removeAllChildren. Flags: 1 drawable, 2 takes the mouse, 8 needs layout, 0x10
remove me, 0x14 needs update. The root window's mouse events (0x4387f0...) hit-test with flag 2, update the hover
node (0x4386c0: exit on the old, enter on the new) and call the hit node's handler; key events go to the handlers
in +0xb0, last first, until one returns true.

Sound as read (for the port's SoundSystem and Sound over platform/Audio.h; the DirectSound buffer code itself is
not translated). The game's sounds are loose WAV files under res/ (sounds and music; names differ in case from the
files, e.g. shaku1.wav for Shaku1.wav). SimpleSoundDX (vtable 0x485190, 0x134 bytes): +0x118 the voices it made,
+0x124 a map name -> loaded data (+0x28 of the node), +0x130 the master volume. Slots: 0 0x443a30 load(name, file),
a whole file (decoder 0x445810); 4 0x443b60 load a streamed file (decoder 0x445ae0; the music clips); 8 0x4427d0 get
+0x12c; 0xc 0x443120 unload the data no voice uses; 0x10 0x443190 unload(name); 0x14 0x4435e0 createSound(name, a,
b); 0x18 0x4425a0 setVolume (0..1); 0x1c 0x443a20 getVolume; 0x20 0x453e50 and 0x24 0x453ea0 (base class; the game
calls 0x20 with (1, 0) from 0x406520); 0x28 0x442810 each voice's slot 8; 0x2c 0x442840(ms) each voice's update
(slot 0x5c; the window layer calls it with 100); 0x30, 0x34 debug dumps. A voice (vtable 0x4851e0; 0x4850f8 for a
short sound): 0 play (from where it is; with a fade-in when +0x48 > 0; nothing when playing), 4 0x443dd0 halt (no
rewind), 8 0x443df0 stop (a fade-out when +0x4c > 0, else halt, rewind (0xc) and tell the owner +4 its slot 4), 0xc
rewind, 0x10 0x4447b0 close, 0x14 setPitch (+0x28; the frequency pitch x rate, 100..100000), 0x18 getPitch, 0x1c
setPan (+0x30, -1..1), 0x20 getPan, 0x24 setVolume (+0x2c, 0..1), 0x28 getVolume, 0x2c setLoop (+0x34), 0x30
isLoop, 0x34 isPlaying (+0x35), 0x38 setFadeIn (+0x48), 0x3c setFadeOut (+0x4c), 0x40-0x4c base class, 0x5c
update(ms) 0x444240 (fades 0x444380, streaming 0x444270, volume 0x444730). The music (0x424160-0x426000): a layer
(0x424810, 0x84 bytes: +0x10 its clips, +0x1c sequence (0 random, 1 random-not-last, 2 in order, 3 chosen), +0x20 the
delay range, +0x28 the initial delay range, +0x30 fade-in, +0x34 fade-out, +0x38 volume, +0x3c the time left, +0x40
the first clip, +0x44 the ending clip, +0x48/+0x4c clips, +0x50 the index, +0x54 how it stops (0 cut, 1 fade out over
+0x34, 2 the ending clip), +0x58 running, +0x5c the chosen index) picks clips 0x4245f0, plays 0x424740, starts
0x424910, stops 0x424970, updates 0x424a10, adds a clip 0x424b80 (stream-loaded, no loop, volume +0x38); the music
system 0x4252f0 runs its layers (0x424be0 update, 0x424c10 start, 0x424c40 stop, 0x424c70 choose). Its file reader
0x425250 (vtable 0x483178), the sound bank 0x426760 (the sounds file 0x425cf0) at controller +0x710 (0x426ac0).

More sound, read and checked in the disassembly. The voice's base (0x453dd0): +4 an owner told at a stop (slot 4
of it; nothing sets one that was seen), +8 its category; slots 0x40 setOwner, 0x44 clearOwner, 0x48 setCategory,
0x4c getCategory, 0x50 getName (+0x24). The system's base (0x453e30): 64 category volumes at +4, all 1; slot 0x20
0x453e50 setCategoryVolume(category 0..63, 0..1), 0x24 0x453ea0 getCategoryVolume (1 out of range). The volume a voice
plays at (0x444730): category volume x master volume x the faded volume (DirectSound's dB from ln(gain), so linear).
Fades (0x444380, slot 0x64, ms x 0.001 seconds; +0x54 the state, +0x50 the time left): 0 fading in (left -= dt; below
0: left 0, state 1; volume x (in - left) / in); 1 playing (a fade-out time and no loop: once the position reaches
slot 0x74's, the length less the fade-out, state 2 with left = fade-out); 2 and 3 fading out (left -= dt, not below
0; volume x (1 - (out - left) / out)); then the volume is applied, and at 0 or below in state 2 the voice halts,
tells its owner and rewinds. The window layer calls the system's slot 0x2c with 100 on a 100 ms timer (WM_TIMER 1).
The sound bank (0x426ac0, 0x14 bytes: +4 a map name -> group, +0xc volume, +0x10 pitch): add 0x426760(name, file,
count, vol, pitch, category) returns at once when the name is there, else loads the file under the name (system slot
0), makes a group (0x435a10: count voices of createSound(name, 1, 0), each given the category by slot 0x48) and maps
it; then the bank's +0xc and +0x10 become this sound's vol and pitch (the last sound's, bank-wide; the files give
neither, so 1). play 0x425f10(name, vol, pitch): the group's slot 4 (0x4358e0) with vol x +0xc and pitch x +0x10:
the first voice not playing gets setVolume, setPitch and play; none free, nothing. 0x435940 counts the playing
voices, 0x435af0(make) a free voice or (make) a new one. The sounds file (0x425cf0, reader 0x42bdf0/0x42beb0, element
"sound"): name (""), file (""), vol (1), pitch (1), count (1), category (0). The music file (reader 0x425250, vtable
0x483178, 0x4259a0): musicSystem (nothing), musicLayer (0x425400: sequence "random-not-last" (random 0,
random-not-last 1, sequential 2, loop 3), initialDelay (range, 0) +0x28, volume (1) +0x38, fadeInTime (0) +0x30,
fadeOutTime (0) +0x34, stop "immediate" (immediate 0, fadeOut 1, playFinal 2) +0x54, delay (range, 0) +0x20, name
("Layer") +0x60), clip (inside a layer: path ("INVALID_CLIP_PATH"), special (""), name (the path); special empty:
added (0x424b80); "startOnly" registered (0x424160) as +0x40 only, "start" added and +0x40; "finalOnly"/"final" the
same for +0x44; "bridge" registered as +0x48; special is searched with find, so it can hold several). A clip is
registered with the system's slot 4 (streamed) and createSound(name, 0, 1); an added clip gets no loop, no fades and
the layer's volume. The end of musicLayer adds the layer to the system (0x4251e0); the system's load (0x424ca0)
deletes its old layers first (0x425230 -> 0x424e90). A layer's random pick uses 0x401050 (randomIndex, Random.h);
random-not-last takes size - 1 when it draws the last index again; sequential wraps to 0; loop keeps the index
unless one was chosen (0x4247c0 select(index, now)), and its clips loop.

The music as ported (engine/Music.h/.cpp): the layer, the system and the reader as read above; a clip that fails to
register is left out of the list, as in the original (0x424b80 returns null). The engine
test reads every XML file as a sounds file and as a music file over a sound system that plays nothing: 17 sounds, 4
music files (drums, game-over-music, menuMusic, music: 1, 1, 1 and 4 layers), every file found ignoring case, and
each music file runs two minutes, stops and runs ten seconds more.

The ball system as ported (game/WordBallSystem.h/.cpp, compiled, not yet run): the constructor and destructor, the balls and their looks, the rope, the mouse, the water and chain drawing, spawning, the row and column groups, the keys and startLevel (0x417b20). The rules are ported too: the letter queue and the hints (0x418f30-0x41a050), the scoring of a found word (0x41a120), the word finders for solitaire and freematch (0x41a8b0, any word of the shortest length or more, wildcards filled from the word list) and for the puzzles (0x41b2d0, the level's own words), submit (0x41b6f0), update (0x41b760) and the right button (0x41c2d0). Still to port: the saved game 0x4142c0 and 0x4199d0 (with the writer and reader 0x41e760-0x41f7e0). Kept as in the exe: a multiplier ball adds no letter to a run, yet the wildcards' balls are then found by the letters' positions; 0x41a8b0 counts an empty word as scored when a start passes the end before any word was kept; +0xd8 (the last physics step's time) and +0x140 (the last word's time) start unset in the original, at 0 in the port. Left out: the level editor's own functions (0x413af0, 0x413c40, 0x416920, 0x416ba0, the editor HUD's actions 0x417680), since this build never sets +0x1f8 (the controller loads EditorHUD.xml, 0x408190, but never shows it); the editor branches inside live functions are ported. game/Game.h, Hud.h, Level.h and engine/TextEffects.h only declare what the ball system calls, with the addresses still to port. startLevel's puzzle pool loses everything from a placed letter on (the original's erase(at), not erase(at, 1)); solitaire's run test compares with '\0' while the pieces leave '0's (both as in the exe).

The level code as ported (game/Level.h/.cpp): the level's constructors (0x41c760, the rules carried on from the level before; 0x41c910, a deep copy), its destructor, the bubbles (0x41e380, 0x41e420, the special flags 0x41e280 and 0x41e460, the element 0x41e5b0 the saved game writes) and the level file's reader (0x41ce30 with its handler 0x41cf70). The reader's defaults: gravity 0,500, shortestWord 3, spawnDelay 5, the rest 0; the time and the longest piece are never read, so they stay at 60 and 2 or carry on. A "breakable" obstacle becomes a bubble of one letter with the flag 0x100. The ball flags take the names the special attribute gives them: 1 bomb, 0x20 block, 0x40 heavy (rock.png), 0x80 stone, 0x100 obstacle. Left out: the level file's writer 0x41c450 (the editor's, from 0x4053b0) and the editor's one-letter bubble 0x41e3a0.

The word lists as ported (game/Dictionary.h/.cpp): config/hintList.txt goes into the anagram tree (the controller's +0x10, words of 4 letters or more, also kept in the list at +0x1c) and config/matchList.txt into the word tree (+0x14, 3 letters or more), both read line by line by 0x420a80. The anagram tree's 0x420100 finds whether the letters and pieces can be used up entirely by words (the puzzle hints), 0x420390 every word of 4 to 6 letters they make (solitaire and freematch). The word tree's lookup 0x420b70 returns a status: 0 only for the word in its own case (1 or 2 when the case differs, 3 not found, 4 past or inside a word), and the ball system takes a run of balls as a word only on 0; 0x420ec0 finds the words a pattern with '?' matches, spelled as in the list.

The high score tables as read (0x421200-0x422030, ported in game/HighScores.cpp; the controller fills an empty table with five names, 0x403760): a table holds a vector of entries at +4 (each 0x14 bytes: the name, 16 chars by strncpy, and the score at +0x10) and the registry key at +0x10. 0x421750 loads it: the key is Software/GameHouse/WordHarmony/ (with backslashes) plus the table's name, and for i 0-9 it reads the string scoreName<i> and the DWORD scoreValue<i>, adding the entry (0x421600) only when the value is not 0. 0x421600 inserts before the first entry whose score is lower or equal, else at the end, then drops entries past 10 without freeing them. 0x421200 writes every entry back by its index; 0x4213b0 returns entry i's name. 0x421c20-0x422030 are a registry key helper (open 0x421c50 with HKEY_CURRENT_USER, close 0x421c30, write string 0x421e60, write DWORD 0x421f60, read DWORD 0x421d80, read string 0x422030); the port goes through engine::Host's registry functions instead, the table's name before the value's.

The HUD (0x422220-0x424040) is ported in game/Hud.cpp, all but its part of the saved game (0x422400). It carries out its buttons' actions itself (0x422850): the menu's (retry, quit, pause, help: the controller's, declared but not read yet), the ball system's (submit, nextLevel, wildcard, hint, spawnBall) and the recycle buttons "submit1" to "submit14", which put a found word's letters back. The controller's +0x3c is the HUD's balls remaining (the level's ball criteria less the letters used), now mLettersLeft; Game::playSound takes the name only (ret 0x1c). Its three texts go through game/GameText.h (GT(address)); the loader, which reads them from the player's WordHarmony.exe, comes with the host (the texts are there as they are in the game image).

The floating texts are ported in engine/TextEffects.cpp: the factory ("SplatFactory", 0x428780-0x4298b0), its template reader (0x430a70-0x431930, the particle file's elements less position, particleLife and blackHole, plus resource type text and destination) and the text ("Splat", 0x42a400-0x42aa50). create takes a fifth argument, passed to the callback: the found word's letter count, which Game::scoreShown (0x401290) takes off the letters left. A text's own transform multiplies its opacity in but leaves out the rotation and the colour offset. The screen at 0x42a0d0/0x42a240 (vtable 0x483950) sits between them and belongs with the screens.

Remaining, in order (2026-10-26): the screens 0x409c20-0x40df90 (90 functions, about 17 KB of code; the editor's WordEntry 0x40dcd0-0x40df90 left out, the saved game's reader 0x40cf90 kept for the save code), then the controller 0x401000-0x409c00 (0x403760 fills an empty score table with five names: game text, by GT), then the save code, then src/: main.cpp with the engine::Host (registry included), the GameText loader (game/GameText.h; table generated from the carved exe, texts read from the player's WordHarmony.exe), the sound system and the web shell (port 18778).

The settings (0x409d50-0x40aec0) are ported in game/Settings.cpp, all but the three saved games kept as XML strings in the registry (PuzzleSavedGame, SolitaireSavedGame, ActionSavedGame: parsed by 0x40c8f0's reader through a string stream 0x421a70, written by 0x41e820), which come with the save code. Full screen goes through new engine::Host calls (App slots 0x04 and 0x08 are the window's). CongratsScreen (0x409c20) is a ScreenLayout named "CongratsScreen" with vtable 0x480e88; 0x40aec0 fills the game over screen (score, hints used, the five longest words of the controller's +0x5c, "extraScores" shown outside the puzzles).

Screens ported so far (game/Screens.cpp): CongratsScreen, GameOverScreen with its results (0x40aec0) and HighScoreEntryLayout (0x40b1d0-0x40b640: a key handler on the root window, printable keys 0x20-0x87 up to 16 letters, backspace, enter tells the controller "highScoreEntryDone"). Next is the score tables class at 0x40b6c0, which 0x40b660 (three fields at +0xd4-+0xdc, enabled, +0xe0 cleared) and 0x40b6a0 (+0xe0 cleared, its slot 0xf4) belong to; dumps in build/wh_sc3.txt (0x40b0d0-0x40c190). 0x40b6c0 fills config/highscore.xml's three tables (puzzle +0xd4, tsunami +0xdc, solitaire presumably +0xd8; HighScores objects) for i from +0xe0 to +0xe0+10: below the table's count (0x421080) the nodes <mode>-scoreRank<n> (text "%d."), -scoreName<n> and -score<n> are set and shown, else hidden. The nodes are numbered from 0: the sprintf calls pass i less +0xe0, and the rank text is i + 1 (checked in the disassembly at 0x40b72a and 0x40b782).

The controller as ported (game/Game.h/.cpp, compiled, not yet run): every function of 0x401000-0x409c00 except the
saved game's own code. The state machine is 0x4055f0 (states in Game.h); several callers inline parts of it (the
splash's end, retry, newGame, the high score entry and update's state 2 and 6), and the port calls setState there,
which differs only in the redundant root window fetch. The fades' callbacks 0x401200 and 0x401220 set +0x208 (removed
at the next update) and +0x190 (the root's only child at the next update). Quirks kept: continueGame takes the level's
points from the saved letters used (+0x24), not from +0x20; the end of a level sets text 4 again every frame once the
timer is under 1.5 seconds (the else of text 3's test); the score tables get their default names when any table has
fewer than 10 entries, but each table only when it has fewer than 5. Left out: the beta's play time upload (+0x10c),
the frame rate display (+0x300), the screen saver's install path from the registry, and the distributor's game link
button (ShowLink and ILink); openLink still checks VisitURL. The host gained createSoundSystem and commandLine (the
screen saver's /s, /c, /p), the controller its slot 0x14 activate (a lost focus pauses the game).

The saved game as read (to port next). SavedGame (0x8c bytes): +0 level, +4 mode, +8 score, +0xc the respawn timer
(float, the ball system's +0x100), +0x10 wildcards, +0x14 hints used, +0x18 the level's hints used, +0x1c hints, +0x20
level points, +0x24 letters used, +0x28 the template, +0x2c time, +0x30 the bonus word (the ball system's +0x148),
+0x4c vector<Bubble*> (one per ball of the array, null for one not in play), +0x5c vector<Chain*> (Chain(null, 0); its
word +0x18 and indices +0xc), +0x6c vector<FoundWord*> (the HUD's +0xdc), +0x7c vector<string> (the queue +0x110).
Adders: 0x41e760 header(level, score, respawn), 0x41f0d0 a ball (new Bubble: origin = position (Node slot 0xc), letters
append(1, ball +0xf0), flags = ball +0xac), 0x41f1c0 a read bubble (letter, special, x, y; '?' adds flag 2), 0x41f2b0 a
null, 0x41f310 a read chain (its word), 0x41f3d0 a read word, 0x41f6c0 a copied word, 0x41f7d0 a queue entry, 0x41f7e0 a
copied chain. Destructor 0x41f4e0. saveState 0x4142c0: header from the controller's +0x308 and +0x2c and +0x100; +0x30 =
+0x148; each chain of +0xe8; each ball (+4 the count): state +0xd8 0 or 4 saved, else a null; the HUD's words (0x422400);
the queue. The writer 0x41e820 (text for the registry): `<level mode level score respawnTimer wc hug hul ha lp lba bonus
ilg time>`, then per chain `	<rope text>` with its balls' bubbles (each written once: the slot is nulled) and
`	</rope>`, the other bubbles, `	<HUD>` with `		<word displayText word isMatched score multiplier chains>` (chains
joined by ','), `	</HUD>`, `	<spawnqueue>` with `		<letters text>`, `</level>`. loadState 0x4199d0: every ball
state 3 (+0xd8) and +0xe0 cleared, the chains deleted; count (+4) and +0x10c = the bubbles' count; each bubble: in play
+8 counted, position set (slot 0x14) and +0xb8 = it, +0xd4 false, +0xe0 null, state 0, its own gravity (a copy of the
level's +0x3c0), flags, letter = toupper(first letter), 0x4113f0, added to the layer (slot 0xd0); each chain: Chain(this,
2) with the word and the indices, +0x14 false, pushed to +0xe8, its balls' +0xe0 = it and 0x4113f0; the HUD's words
(0x423e40); the queue replaced; freematch (+0xd4 == 2): +0x100 = the respawn timer, the HUD's 0x422340, +0x124 = +0x128
= min(1, in play / the level's max balls); "LEVEL_BEGIN"; a mode other than puzzle: the hint thread stopped and 0x419540
(the hints found again); +0x148 = the bonus word. Still to read: the reader 0x40cf90 (with 0x40c8f0) and Settings'
load 0x40a250 and save 0x40aa10 of the three saved games, the HUD's 0x423e40 and 0x422340.

The host (`src/`, 2026-10-26): `WordHarmonyApp` runs the engine from a framework widget (a frame whenever the frame
period has passed, the sound system's update(100) every 100 ms as the window timer did), `Render.*` decodes images
and draws them into the 800x600 MemoryImage, `SoundHost.*` is SimpleSoundDX over the platform mixer, `GameText.*`
reads the exe's texts by address (`tools/make_gametext.py` makes the table). The saved games are ported (SavedGame,
its reader, Settings' load and save, the ball system's saveState and loadState). Played natively under rundesk:
the splash, the menu, all three modes' level choice, help and start, Save/Quit with Save Game, and Continue Game
(the board comes back as saved). Found while checking: the music WAVs are MP3 inside a WAV (format tag 0x55), which
the platform decoder now reads (a `shared:` commit); a mask's alpha is its pixels' last channel after the original's
PNG set-up (0x45ea10, 0x4651c0: alpha for masks with alpha, else the grey level), not its red; the white line round
the play area is in comp01.jpg, the first chain link on the first ball's centre is the original's loop, and the
level title's font has a space width of 1 ("LEVEL1"), all as in the original.

## Next pieces, as read so far

The controller (vtable 0x480a14: 0 dtor 0x4073d0, 4 configure 0x402eb0, 8 init 0x406b30, 0xc shutdown 0x401750, 0x10
update 0x409060, 0x14 0x4070a0; a one-slot listener vtable 0x480a10 at +0xc, 0x407580). init seeds rand with
GetTickCount, makes the sound system (0x443ca0, from the window handle, App slot 0x74 stores it) and a 0x14-byte
object at +0x710 (0x426ac0), sets up a beta play-time reporter (0x42a090; an online feature, left out of the port),
then runs the screensaver (+0x1fc set) or 0x4055f0(this, 0). The screen layout handler (constructor 0x42c7f0, a
group stack at +0x18) dispatches elements in 0x430910: image 0x430280, text 0x42ef20, imageButton 0x42c920,
checkBox 0x42d6f0, textButton 0x42e2b0, group 0x42fa90, particleSystem 0x42f760, textSub 0x4300c0, include
0x42fed0; the end of a group pops (0x42c4f0, "Mis-matched end group"). `re/wordharmony/work/layout.c` and
`controller.c` hold the dumps.

Images: the image interface (the image's +0xc, vtable 0x48aba8) has 0 getWidth, 4 getHeight, 8 nothing, 0xc
setPivot(mode: 1 = the centre, w/2 and h/2 in integers), 0x10 setPivot(x, y), 0x14 getPivot (integers), 0x18
setAlphaMode(mode, threshold byte), 0x1c getAlphaMode. The pivot and the alpha mode belong to the cached image, so
every node of that image shares them. Modes: 0 opaque (JPEGs, PNGs without alpha), 1 alpha test (PNGs with alpha get
1 with threshold 0; the layout's alphaType="alphaTest" sets 1 with alphaThresh, default 5), 2 blend (masked images,
alphaType="alphaBlend"). The draw 0x45e2d0: a colour offset uses 0x45d5a0; mode 2 or an opacity other than 1 the
blend 0x45ce90; mode 1 the alpha test 0x45c780; mode 0 the plain 0x45c070 (the pivot and the rotation are applied
inside the blitters; how mode 1 treats partial alpha is not yet read). The image node (0x435d50, 0x94 bytes, vtable
0x484858): a counted image at +0x88, own flag DRAWABLE, name "Image"; draw 0x435d20 draws the image when visible;
layout 0x435c60: bounds = position - pivot to position - pivot + size, then scaled by |scale| (after the offset).
The layout's attribute getters take the map by value: 0x42abd0 string, 0x42ad20 int (atol), 0x42aeb0 float; the
scratch `attrs.py <start> <end>` lists each read with its name and default. The image element 0x430280: file
("" = none), name ("_Image_"), x, y (0), opacity, scaleX, scaleY (1), alphaType, alphaThresh (5), pivot
("upperleft"; "center" centres the image's pivot); the node goes to the screen's slot 0xe8 and to the top group.

## The game code as read (0x401000-0x424000)

`re/wordharmony/work/game.c` is the whole range from fr.py (16,400 lines; about 60 functions are container
internals). The extracted archive is in `re/wordharmony/saf/` (lower-case names): the level files show which
branches the readers take. Source modules in address order: the controller 0x401000-0x409c00 (a global at
0x4995b8 points at it), CongratsScreen 0x409c20, the options in the registry 0x40a000-0x40aec0, GameOverScreen
0x40b0d0, HighScoreEntryLayout 0x40b510, the score tables 0x40b6c0, HighScoreScreen 0x40c190, LevelsSelectionScreen
0x40c2b0-0x40c480, MainMenuScreen 0x40c5a0-0x40c770, the saved game's reader 0x40cf90, ScreenSaver 0x40db70, the
editor's WordEntry 0x40dcd0-0x40df90, the physics 0x40e100-0x40ede0, the ball 0x40efa0-0x40f200, the obstacles
0x40f310-0x410c90, the bounds 0x410ef0, the ball system 0x4113f0-0x417680 (class 0x416fd0, "WordBallSystem"), the
game rules 0x417b20-0x41c2f0, the level 0x41c2f0-0x41e820, the saved game's writer 0x41e820, the dictionary and the
anagram finder 0x41f...-0x420b40, the high scores and the registry 0x421200-0x422030, the HUD 0x422220-0x423e40, and
a 500-byte screen 0x423fa0.

Physics (game/Physics.h; Verlet, Jakobsen style). The ball array (0x40ec20, vtable 0x481bb0): +4 count, +0x10 gravity (Point), +0x18
friction 0.2, +0x1c the balls (an array of 0xfc-byte balls, new[] with a count; 0x40ecd0 gets ball i). 0x40ed10
accumulate(dt): a ball not fixed gets acceleration = (gravity, or its own at +0xdc, or (g.x, -g.y) when its inverse
mass <= 0) x dt x dt; a fixed one (0, 0). 0x40ede0 integrate(dt, lastDt): pos += (pos - old) x damping x dt/lastDt
+ acceleration, old = pos, damping = 1; a fixed ball's old = pos; state 3 balls are skipped. Constraints (base
vtable 0x481b20, +4 active, slot 0 apply): 0x40e100 (0x2c bytes) pulls a ball towards a target with a lerp, or with
a random jitter when +0x28 is set; 0x40e220 (0x2c) a mouse rope: within +0x18 of the target it does nothing, else
pulls by the excess capped at +0xc times +0x10, and blends the old position by +0x14; 0x40e5d0 (0x3c) a chain over
a list of ball indices; 0x40e6c0 every pair of balls (not state 3 or 4, not fixed) pushed apart within 44 px
(1936 = 44^2, the inverse masses), and a 0x40ea90 stick between two balls whose +0xf1 is set; 0x40e920/0x40e950 a
distance constraint (mode 0 at most, 1 at least, 2 exactly); 0x410ef0 (vtable 0x482198, 0x1c bytes) the screen's
bounds: x in [r, +0x18 - r], y in [r, +0x14 - r], damping -= hits x friction, and the old position clamped too.

The ball (Container, 0xfc bytes, constructor 0x40f200, vtable 0x481bd0; overrides only the destructor and draw
0x40ef30: Container::draw with snap 3, then its letter centred): +0xa8 a Selector (off and on images), +0xac
special flags, +0xb0, +0xb8 old position, +0xc0 acceleration, +0xc8 radius 22, +0xcc inverse mass 1, +0xd0 damping
1, +0xd4 fixed, +0xd8 state (0 free, 1 and 2 picked, 3 gone, 4), +0xdc own gravity (owned), +0xe0 its chain, +0xf0
the letter (upper case, 0x4111b0), +0xf1 linked. 0x4113f0 builds its look from the flags: 2 wildcard (green font,
blank letter, a particle system from the ball system's pool and wildcardGlow.png), 1 (becomes 2), 0x100 breakable
(cube02.png, fixed), 0x20 (cube02.png, inverse mass 0.01), 0x40 stone (rock.png, inverse mass 0.25), else ball.png
and ball_on.png (or the -chain versions with a chain); 4, 8, 0x10 the x2, x3, x4 badges.

Obstacles (game/Obstacles.h; 0xb0 bytes; the port: Obstacle : Node with the new slots 0xc8 collide(balls), 0xcc setActive, 0xd0
isActive, 0xd4 touches(balls, i, margin), 0xd8/0xdc origin (the node's position), 0xe0/0xe4 extents, 0xe8/0xec
letter, 0xf0 the XML line, 0xf4 type, 0xf8 clone; contains 0x40 is a rectangle test, draw the image and a yellow
letter, layout the image's bounds). Box 0x40fb00 (vtable 0x481ea8): +4 extents (22, 22), +0xc box type (0 cube01.png,
1 "star", 3 invisible), +0x10 active, +0x14 image, +0x18 letter; collide 0x40f440 pushes a ball out along the
shallower axis, or from the nearest corner. Ramp 0x4109f0 (vtable 0x482010): +4 extents, +0xc active, +0x10 image
(cube03-<rotation>.png), +0x14 letter, +0x18 rotation (0, 90, 180, 270); collide 0x410040 is the box test with the
diagonal half (x87 compares: read 0x410040 in the disassembly).

The level (0x3e8 bytes, constructor 0x41c760 copying time, shortestWord, ballCriteria, maxBalls, spawnDelay and +0x3b8
from the previous level): 3 intro texts, 14 words, 14 definitions, 14 bonuses (+0x364), balls +0x39c, time +0x3a0
(60), shortestWord +0x3a4 (3), freematch +0x3a8, ballCriteria +0x3ac (10), spawnDelay +0x3b0 (5), maxBalls +0x3b4,
+0x3b8 (2), gravity +0x3c0 (0, 500), obstacles +0x3cc, bubbles +0x3dc (0x28 bytes: origin, flags +8, letter +0xc;
"?" sets flag 2). The reader (0x41cf70): level (gravity, shortestWord 3, balls, maxBalls, ballCriteria, freematch,
spawnDelay 5), intro (text0-2), word (the first empty slot: text, definition, bonus), obstacle (origin, letter of
one character, type: breakable makes a bubble with flag 0x100, box a Box, ramp a Ramp with rotation; extents; named
"obstacle"), bubble (letter, origin, special parsed by 0x41e280: block, bomb, heavy, obstacle, stone, wildcard, and
2x/3x/4x). A tag before the first level: "Level parse error".

The ball system (0x220 bytes, constructor 0x416fd0): the ball array at +0 and a Container at +0x20 (offsets below
are from the object's start). +0x8 a ball count (0x412b30 lowers it), +0xd4 the game mode (0, 1 or 2; 0x411c00 picks
the HUD +0x198 from +0x1a0, +0x19c or +0x1a4 and tells it 0x422740; mode 2 has the rising water), +0xe0 the mouse rope (0x411ac0 grab, 0x411ba0 let
go), +0xe4 the grab target, +0x104 the water level (mode 2, drawn by 0x412310 from four images +0x1a8-+0x1b4), +0x128,
+0x1a8 (no picking while set), +0x1b8 the mouse, +0x1c0/+0x1c4 particle pools, +0x1dc the hint thread (a Win32 thread; the port runs
it without one). 0x411c90 a weighted random 0-7 (thresholds 26, 51, 61, 71, 81, 91, 96).
0x411ac0 grab(ball): a Rope (0x40e220: ball, offset = mouse - ball, 21.99, 0.3, 0.8) at +0xe0 with the mouse as target, shows +0x1ac (slot 0x54 true) and tells the HUD 0x422220(0, 0, 0); 0x411ba0 lets go (the ball's setOn(false), deletes the rope, hides +0x1ac, HUD 0x422220(1, 0, 0)). 0x412120 the mouse press (a Container-part method: its offsets are 0x20 lower; here they are the object's) unless +0x1a8: the nearest free ball, else (in the editor, +0x1f8) the nearest obstacle within 23 px (0x412080 over the obstacle list at the object's +0xf8) gets a Follow (0x40e100: 21.99, 0.3, 0.8, grid 44; snap = controller +0x108) at +0xe4; a ball not fixed is grabbed (0x411ac0) or, in the editor, followed; then setOn(true). 0x412310 draw: in mode 2 four water images (+0x1b0, +0x1ac, +0x1a8, +0x1b4) at the level (1 - +0x104) x 456 - 15 with sines of +0x1b8 (lost floats: read the disassembly), then Container::draw, then each chain's links (image +0x190) midway between its balls. 0x412920 removes a chain (its balls' +0xe0 cleared, out of the list +0xec, deleted). 0x412ca0 puts every free ball in state 1. 0x4137b0 adds the HUD (+0x198) as a child, resets it (0x423db0), in mode 0 gives it the level's 14 words (0x423ba0), then 0x422450(""). 0x411dc0 (the level's end): mode 0 and the last level -> controller 0x404e10(0, 0), else controller 0x4055f0(7); then its +0x1a8 = 1 (which part's offsets is not checked). Modes: 0 the puzzle levels (HUD.xml at +0x1a0), 1 solitaire (HUD-solitaire.xml, +0x19c), 2 freematch, the action game with the rising water (HUD-freematch.xml, +0x1a4); 0x413f00 makes the three HUDs (500-byte screens 0x424040, position (-212, -110), loaded by 0x423330). 0x413860 spawn(x, y, letter, special, quiet): the first gone ball after +0x10c (or a new slot, at most 105 balls), state 4, +0xb0 = 1.0, position and old position (x, y), no acceleration, the letter upper-cased (none: the first of controller 0x4050f0(1), a random letter), its own gravity (-200, 100), special (a '?' adds the wildcard flag), its look (0x4113f0), scale 0.2, added to the balls' layer +0x1b4, a particle system from the pool +0x1c0 at it, and unless quiet the sound BALL_RUSTLE2 (controller 0x402b10); the count +8 goes up. 0x4140d0 the same at (470, -26) with scale 1 and the letter as given. 0x413af0 and 0x413c40 (the editor) add a box or a ramp at (x, y) to the level and the layer. 0x413da0 shows word i's definition in the HUD ("word: definition", 0x422450). 0x4142c0 writes the saved game (0x41e760 and friends). 0x4111d0 the nearest free ball within 23 px; 0x412b30 puts a ball in state 1 (in mode 2, +0x128 = count / the level's maxBalls, at most 1); 0x412bf0 puts
one in state 3 and clears its look.

The ball system's parts (no RTTI, so the vtables sit back to back): +0 the ball array (vtable 0x482514: deleting destructor 0x4174d0, slot 4 the physics step 0x414b90), +0x20 a Container subclass (vtable 0x482438; its base, vtable 0x4821c8 with destructor 0x411190, only adds a pure slot 0xd8), +0xc8 a KeyHandler (0x416400 keyDown, 0x411ab0 keyUp), +0xd0 a ScreenListener (0x417680, the editor HUD's actions), and the virtual Interface base. The Container part overrides 0x6c draw 0x412310, 0x70 update 0x41b760, 0x80 onMouseDown 0x412120, 0x88 onMouseUp 0x411d80, 0x98 the right button's onMouseUp 0x41c2d0, 0x9c mouseMove 0x411260 and 0xd8 0x417b20 (the game rules, 0x417b20-0x41c2f0, are all its methods). Its methods' offsets are 0x20 lower than the object's: 0x412310's water images +0x1a8-+0x1b4 are the object's +0x1c8-+0x1d4, its chain link +0x190 is +0x1b0, its +0x104 is +0x124.

The constructor 0x416fd0(mode): mode +0xd4; chains vector +0xe8; obstacles +0xf8 (a pointer); +0x100, +0x104 (byte), +0x10c the last slot used, a vector of strings +0x110 (the words found, 0x41f7d0 saves them), +0x120, +0x124 the water level, +0x128 the balls over the level's maxBalls (at most 1, 0x416c60); a map<string> +0x130, a string +0x148, row and column groups +0x164/+0x174 and their free pool +0x184 (0x415c20, below), +0x194, a timer 0x478d20 at +0x1b8, the hint thread +0x1dc, a vector of strings +0x1e0 (0x414d60) and its index +0x1f4, the editor flag +0x1f8, +0x1fc, the level backup +0x200 (the editor's), a vector of strings +0x204. Named "WordBallSystem"; +0xdc the screen bounds (a Bounds, 441); flags 2 and 4; its key handler added to the root window (0x438f80); bounds (0, 0, 590, 441); the particle pools +0x1c0 (exploding-blossoms.xml) and +0x1c4 (sparkles.xml); +0x1ac an ImageNode of target_hit.png (pivot centre, hidden); +0x1b0 chainLink.png (centre); +0x1c8-+0x1d4 effects water1-4 (pivot 0, alpha mode (2, -128)); +0x1b4 the balls' layer (0x429b70(1), 200 bytes); then the HUDs (0x413f00). The destructor is 0x415920.

0x414b90 the physics step: the bounds, the rope and the follow constraints, each chain, every obstacle's collide, the ball pairs (a Collisions on the stack), then every breakable ball not gone acts as a box of its radius against the others (a Box on the stack at its position). 0x414d60: when the strings at +0x1e0 are not empty and (controller +0x4c > 0 or mode is not 1), a text node (0x4289d0 on controller +0x70 at (108, 573)) shows the next one (mode 1: in turn from +0x1f4, else at random), and controller +0x54 and +0x50 go up, +0x4c down. 0x415300 reset: stops the hint thread, +0x104/+0x100 cleared, HUD 0x422340, +0x110 cleared, HUD 0x423210(0), +0x124 = 0, let go (0x411ba0). 0x4153a0 clear: every ball's chain removed, state 3, flags 0, unlinked, its children removed (the selector with them); the Container part's and the layer's children removed; reset; in mode 2 outside the editor the level is deleted. 0x415c20 groups the balls not gone into rows (within 15 px of the row's first ball's y) and columns (x), at most 20 balls each (0x54-byte groups: 20 indices, count +0x50; the ball's +0xe8 row, +0xec column), then bubble-sorts each row by x and each column by y. 0x416300 addChain(index): a Chain (mode 2, not along) holding the index, pushed to +0xe8. 0x416c60 spawnWord(word): BALL_RUSTLE2, then one ball per letter (0x4140d0), each moved left by 44 per letter still to come, chained (the chain's word is the word), then +0x128. 0x4174f0 the same through 0x413860.

The editor (entered elsewhere, +0x1f8): 0x416400 keyDown, with a ball or obstacle followed: A-Z its letter (flags kept: breakable stays, else only stone), space a chain or a blank, 7 wildcard, 8 plain, 9 stone, 0 breakable, 2-4 the x2-x4 badges, '.' deletes it; without the editor: Esc and F toggle full screen ("optionToggleFullScreen" to controller +0xc), H controller 0x4070d0(1, mode), P pause or resume (controller 0x401580, 0x404220, 0x406520(1)). 0x417680 the editor HUD's actions (pickLetterBall, pickLetterBlock, pickRock, pickBlock, pickRamp0-270, addWord (wordEntry.xml), toggleGridSnap (controller +0x108), saveEditChanges and cancelEditChanges (0x416ba0), wordEntryDone (the first empty level word)). 0x416920 writes the balls back to the level as bubbles (a chain's letters in one); 0x416ba0 leaves the editor (save: write and drop the backup; cancel: the backup copied back (0x415460, the level's copy)), then clear.

## Reading the decompilation

Two scratch helpers (not in the repository) print functions: `fn.py <addr>...` (whole functions) and `fr.py <lo> <hi>`
(a range, compacted: no declarations or SEH bookkeeping), `vt.py <vtable>...` prints vtables side by side, and
`dis.sh <start> <end>` disassembles with llvm-objdump (E:/retools/llvm-mingw-*/bin) where Ghidra loses x87 float
code. They
live in the session's scratchpad; any copy works on `re/wordharmony/gh/all.c`.

Common helpers: 0x4024a0 string assign(const char*, n); 0x4023b0 string assign(string, pos, n); 0x436480 string
destructor; 0x4019b0 string compare(pos, n, const char*, n2); 0x435f20 App::instance; 0x401000 an empty function
(inlined destructors); 0x44648d `__purecall`; 0x445fa8 operator new.
