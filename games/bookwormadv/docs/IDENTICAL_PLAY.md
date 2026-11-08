# Bookworm Adventures: the identical-play pass

This is the report on comparing the port with the original exe, screen by screen, from 2026-10-31 to 2026-11-01. The
details of each finding are in `README.md` ("Deviations from the original", "Faults the pilot found", "Timing in the
pilot" and "Oddities"); this page gathers them for the user.

The pass is done on the game's side. What remains waits on the framework (the transformed sprites, which need its 2D
software path) or is an unexplained web timing item with no visible effect; both are described below.

## What was compared, and how

The comparison is a screenshot pilot (`tools/pilot/`) in four parts:

- the menus: the logo and title as a clip, the main menu, the focus lost and regained, the players dialog, the hall of
  fame, the mini-games' menu, the media player, the quit dialog, the options and the credits;
- the first battle: the map, the Enter! transition as a clip, the battle's intro, a word, the attacks as a clip, the
  battle menu, and the focus lost and regained;
- the screens: the tome and its list scrolled, and each mini-game from its welcome to its menu (Word Master with a
  guess, Letter Rip into its tutorial, Link n' Spell losing and regaining the focus);
- the won battle: the enemy's defeat, Cassandra's talk, the walk to the next battle and its hint.

One list of steps per part drives both sides by the game's own update count (`gen.py`), so that a click or a shot falls
at the same update. The original runs under `tools/origrun` inside Windows Sandbox, from a staged copy. Its random seeds
are made constants and the port gets the same seed, so both deal the same boards (`seedpatch.py`). The copy also gets one
patch so that it sees the mouse, which rundesk's desktop otherwise hides from it, and its hover effects can be compared.
The original's shots are timed exactly: origrun holds the game once it has drawn the update the shot is for (its
pending-draw flag and update step), so `shots/landed.tsv` names the update each shot shows. The port runs with
`-drawevery`, so it draws every update as the original's pace does here.

The original's loads run on a thread and last a different number of updates each run (see "Oddities"). The pilot measures
each load in the original's run (origrun's `--watch` on the loading screen's timer and the startup load's flags,
`loads.py`), and the port's test options `-chapterloadupdates` and `-startloadupdates` hold its loads just as long. Normal
play never uses those options.

`tools/shotdiff` compares the pictures, and every flagged box was looked at and judged. `lag.py` takes a port screenshot at
every update around each of the original's shots and finds which update of the port matches it. That separates timing
from real differences: a box that some port update matches exactly is timing, and one that no update matches is not.

The web build ran the same four parts in the Browser pane, and its pictures were compared with the native port's.

Sound and music are compared by their events, in order, not by recording them. The original plays its music through
BASS 2.3 and its sounds through DirectSound. In the sandbox, `tools/bassproxy` (a `bass.dll` in front of the game's
own) logs every BASS call, and origrun's DirectSound stand-in logs every buffer: its creation, its data, which buffer a
duplicate copies, and its volume, pan, frequency, plays and stops. The port logs the same events from the framework's
sound manager and music interface when `BYOPOP_SOUNDLOG` names a file (`PILOT_SOUND=on` in the pilot).
`tools/pilot/soundcmp.py` pairs the two sides' sounds by the order they are loaded, checks each pair's length and
format, and pairs the music by load order and checksum. It then compares the plays (with volume, pan, pitch and
looping), the volume changes, the stops, and the music's plays, positions, fades and stops.

In the menus all 92 sounds pair up, and the music matches event for event, every step of each fade included. The
sounds matched once two missing clicks were added (below), except one line: a click that had finished seconds before
is released at a slightly different moment, which changes nothing audible.

The other three parts were compared the same way. In the tome and mini-games all 147 sound events and all 1115 music
events are identical. In the first battle and the won battle all 103 sounds pair up, the music is identical (509
events each), and every play falls at the same update with the same volume, pan and pitch. What differs is when a
sound that has already finished is released. The original releases finished sounds as soon as the next one is played,
while the port's framework releases them a little later. So the logs place some of these releases differently. When the
game mutes and unmutes on losing and regaining the focus, the port also sets the volume of a few finished sounds it
still holds (a click, a grunt). Neither is audible, and the release is the shared framework's, so the port is left
as it is.

**The question at exit after 3D was switched on.** The original asks it in SexyAppBase's destructor (0x53c120). It asks
only when the player changed the 3D setting during the session, the registry held no `Is3D` value yet, the card test
did not recommend 3D, and 3D is on. A second question comes when 3D seemed not to work. The port's renderer is always
recommended and working, so neither question can come up, on today's machines in the original either. Looking at it
showed a real difference, though: the original writes `Is3D` on exit only when the player changed the setting, and
the port wrote it every time. That is fixed (below).

### 3D mode

The original runs in 3D only on the user's visible desktop. Inside Windows Sandbox it cannot: the game runs in a
session with no display, or, with the sandbox's user logged on, on a remote display adapter whose Direct3D 7 device
fails, and dgVoodoo2 either crashes or falls back (`docs/ORIGRUN.md`). On the visible desktop it needs no wrapper: its
Direct3D test passes and its 3D flag stays on. The user allowed runs there with sound, and shots of the game's own
window (2026-11-01).

`tools/pilot/visrun.py` runs one of the pilot's scripts that way. It backs up the game's registry key and users folder,
puts in the pilot's start state with `Is3D` 1, and restores both afterwards. It drives the game as origrun does, by
its update counter, holding the game while it posts input or takes a shot, and it keeps the window on top and refuses a
shot while any other window overlaps it. The picture comes from the window itself (PrintWindow, called while the game
is released, as origrun does), never from the screen. An earlier version copied the screen, and in the menus run its
first frame, taken before the game's window was on screen, held the user's own windows; that picture was deleted, the
copy changed, and the menus part run again. visrun also logs any step where the game was not the foreground window. In
the screens and won-battle runs it never was (Windows does not hand a new program the focus while the user is busy), and
it made no difference to any picture. The copy must sit at a short path: the game builds its file paths in a 256-byte
buffer and dies at start under a worktree's path. The port ran the same scripts in 3D mode with the same load lengths
(the original's loads were far shorter on the desktop than in the sandbox: 4 to 15 updates for a chapter, 44 to 48 for
startup).

All four parts were compared in 3D. The first battle matches. Both sides draw the midground island behind the enemy, deal the same board, and of the
twelve stills nine differ only faintly. The transformed sprites that differ in 2D (the enemies, the intro's spear, the
map's rays) match in 3D, since both sides draw them through a 3D renderer.
What shotdiff still flagged is timing, which `lag.py` (now with `PILOT3D=on`) confirmed. The first is the Trojan
Spearman's idle pose in the focus-regained shot and in seven frames of the attacks clip. The second is Lex's blink in
one frame. The third is the map's rays in the Enter! transition. Each of the original's pictures matches a port update
within one to four updates of its own exactly, but for the window's rounded corners on the user's desktop. That is the
same spread as the original's click timing in the sandbox runs.

The won battle's eight stills differ only faintly. Of the screens' seventeen, one is flagged: the Submit! button's
sparkle in Word Master, which matches the port one update earlier. The menus' title clip matches at lag 0 but for
Lex's place while the startup load runs, as in 2D. Both credits shots match the port one to six updates later (the
credits' shake and the fire bowl). The media player's spectrum is the one real difference, and it is not the port's: on
the desktop the original played real sound and drew its spectrum, while the port ran under rundesk's silent driver, and
the 2D comparison masks it for the same reason. Nothing in 3D mode needed a fix.

## Changes the pass made

| Difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| A hill (the island) stood behind the enemy in the port's battles | The arena script draws the midground animation only when the app reports 3D, and the original's 3D mode is the registry's `Is3D` (the user's is 0) | The game's 3D mode is read from and written to `Is3D`, set by the Options box, and is what the scripts and the game ask | 7b60a5a |
| The tome stayed locked on the main menu | The profile load (0x48a390) calls cheat.lua's `_debug_profile_loaded_value` for each value and `_debug_profile_done_loading` at the end; the second unlocks the bestiary past the first book | `Profile::Load` makes both calls; the hall of fame's loads stay quiet, as the original's | b324e42 |
| Stretched buttons (the battle menu, every game dialog) had stretched end caps, and the new player's name sat in a stretched frame | The original draws them with `DrawImageBox` (corners at their own size, edges and middle tiled) | Drawn as boxes | 71de1d5 |
| The hall of fame's arena list showed 0:14:40 for the ninth default run, where the original shows 0:14:50 | The original's table (MakeTimeList, 0x4632a0) has 89000 hundredths | 89000 | 9392f85 |
| The mini-games showed an empty plate where the original has a greyed Submit! button | `SetAttackDisabled` and `ShowImmuneAttack` (0x41a070, 0x41a3e0) size the cels from the picture the button has | The same | 5abf844 |
| A hint's lines stood 19 pixels apart | ArrowHint::Draw (0x4036c0) steps them by the font's ascent, 14 (0x403953) | Steps by the ascent | e389b58 |
| The main menu's "change player" link and the answers of the game's dialogs (the New Player dialog's Cancel, the quit dialog's) made no click | MainMenu's `ButtonPress` (0x47d540) plays SOUND_CLICK2, and GameApp's (0x44b5e0), which a dialog's yes and no reach as 2000 or 3000 plus its id, plays SOUND_CLICK1 (SOUND_CLICK2 for buttons 1 and 11) | The same | b0dfa0a |
| The port wrote `Is3D` to the registry at every exit, so a player who never touched the setting got one, and the game no longer ran its own 3D test at the next start | The original writes it in SexyAppBase's destructor (0x53c120) only under `mUserChanged3DSetting`, which a change of the 3D mode sets | Writes `Is3D` only when the Options box changed the 3D mode this session | f51d39b |
| Every mini-game played from the Adventure map ended the game with "Expected boolean in LuaClassRet" once it was over (the user, 2026-11-07) | At a mini-game's end its script opens the curtain with the callback `MiniGame.DoConvPanel`, which returns nothing; CurtainDone (0x44d360) reads a boolean from the callback only when it returned a result | The same (the port read one always). The menu tent's mini-games pass no callback, so only the map's crashed | f58fe80c |
| The music could stop for good after a chapter boss when the game lost the focus (the user, 2026-11-07) | Not the original's: after a win the scripts queue the map's song for when the treasure or victory song reaches order 41 (`PlaySongWhenDone`), and the original's loop always sees that order. The port's main loop stops in a hidden or minimized browser tab while the music plays on, so the song passed the order and ended unseen, and the queued song never started | A wait whose song has stopped also plays the queued song; in normal play the song reaches its order first. Reproduced natively with the new `stall` test command (60 s during the treasure song) | b6a51d0c |
| A short hitch at the start of every curtain (Volume 2's session found it; the user's "curtain lag") | The original makes three flipped and mirrored copies of the curtain picture for every curtain, and the first frame uploads them as new textures | The copies are made once and kept by the app (freed at its end) | 4ee9c3db |

The pass also changed the test tools: the pilot's scripts, the sandbox runs, exact shot timing, `lag.py`, and the load
holds (39aaca5, 8607ebf). Besides those, it made shared origrun changes (sandbox capture, `--watch`), each merged by the
coordinator.

The user's reports of 2026-11-07 were checked with the user's own words in mind. The crash and the hitch were found by
reading the original. The lost music never showed in scripted runs: a `-script` run drops the window's real focus and
minimize events, so the game's `focus`, `minimize` and `restore` script commands copy the framework's handling of them.
Focus losses and minimizes at seven points from the boss's death to the map all brought the sound back, as did the
mute bookkeeping (Board::Pause matches the original's). What the user saw is the browser's: a hidden tab stops the
game while its music plays on. The fix was checked natively with `stall`, not yet in a hidden browser tab.

**Real input.** The pass played with real input in the Browser pane (`?silent=1`, requestAnimationFrame shimmed): DOM
mouse and key events on the canvas, from a new player's name through the introduction to the first battle. Natively,
`tools/realrun.py` drives the port's window under rundesk with posted messages: clicks, keys, and a real minimize and
restore. SDL 2 reads Windows' cursor, which sits in a corner of rundesk's desktop, so a click is posted as a burst (a
move with the press, a move with the release) or it lands at the corner; a press held across frames (a drag) waits
for the framework's `-realinput`.

The real-input play of 2026-11-07 and 2026-11-08 went further. Natively, `realrun.py` played a new player from the name
box through the introduction, the first battle's tutorial and the first three chapters, all won, and from a saved
profile the fourth and fifth chapters, won again after the last driver fix (each in five to six minutes of real time).
A minimize during the victory song, in full screen, brought the music back. In the Browser pane `tools/driver.js`
played its way to the sixth chapter, across reloads of the page. Everything that went wrong was the drivers', not the port's:
the tutorial lets one tile be picked at a time and waits on its own arrows, a dialog already up when a driver started
was left unanswered, an arrow's line swallowed the board line after it, and a lost turn that showed for one update while
a turn changed made a driver click a tile into the next word (the board log now names a lost turn only once it has held
for 30 updates). A word spelled twice in a driver's list is its retry: the board went busy for a moment after it was
logged, the first clicks landed then, the game refused the half word with its own dialog, and the driver spelled it
again.

Since 2026-11-08 the port answers the standard `move` command from its own bot (`GameApp::MoveForTests`: the dialog,
conversation, tutorial, lost turn, arrow, overlay, word, guess, treasure or map click a driver needs next), and
`tools/movedriver.py` is the thin loop that plays by it on the live channel natively (`state`, `move`, the move's input
as real SDL events through `-commands` with an empty `-realinput`). Making it found one fault in the port's test hooks,
none in the game: a test command that waits for its screen tried again through the `-script` list, and a non-empty
script list makes the framework drop every real event, so after one waiting `goto` no real click reached the game. On
the live channel the retries now have their own queue; a `-script` run keeps them in the script as before.

## Oddities skipped

**How long the loading screens stay up.** The original loads at startup and before each chapter on a thread, so its
screens last as long as the machine takes. In the sandbox runs the startup load took 134 to 263 updates and the first
chapter's 52 to 85. The chapter's screen draws a hint, one `Rand()`, every 50 updates, so a load of 100 updates or more
would give the next battle another board. The port loads in 20 ms slices and shows the screens for its own time. Copying
the original would mean loading at its thread's speed on each machine. To see it, watch the screen's timer with origrun:
`--watch "[[0x8259e4]+0x83c]+0xb0"`. Within a held load, the loading bar and Lex's place on the title still move at each
side's own pace (a few hundred pixels in a few frames).

## Deviations that remain

**Waiting on the framework's 2D software path.** Sprites drawn through a transform keep their place but are sampled
differently: the battle's enemies (the Trojan Spearman's idle, and the Trojan Warrior walking in), the battle intro's
spear, Lex in Link n' Spell, the credits' creatures and fire bowl, and the map's rays. With `Is3D` 0 the original draws
them with SexyAppFramework's software rasterizer, and the port draws them through its renderer. No update of the port
matches them, so they are not timing. They wait on the framework's software drawing of transformed sprites when 3D is
off. The word bar's scaled tiles are the same kind: the original's are sharp, the port's smoothed (faint only).

**Timing.** The Attack and Submit! buttons' sparkle and glow start with a click, and the original's click lands an update
or two apart from run to run. These are measured with `lag.py` and listed in the README.

**Pixels without a visible effect.** The JPEG 2000 art (the main menu's and the mini-games' backgrounds) is decoded by
OpenJPEG, while the original links j2k-codec. The two differ by about 2 levels a channel, and by more than 8 on 0.65% of
the picture, at edges.

**Masked.** The media player's spectrum is black in the original under origrun, whose silent sound stand-in hands it no
samples, so it is masked.

**The web build.** It matches the native port but for the media player's spectrum, which follows the browser's audio
clock. Also, in two of three web runs, the main menu's library glow was three updates ahead after the media player. That
cause is not found. The glow pulses by the update count. The one load in the port that works in wall-clock slices,
the chapter's and the startup's, is held to the original's length in these runs. Nothing else on the way from the
media player back to the menu reads the clock, so the music's timing in the browser and the pane's focus remain the
candidates. Finding it needs more web runs, and since three updates of a glow have no visible effect it waits.
