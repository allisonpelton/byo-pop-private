#ifndef __RES_H__
#define __RES_H__

// Resource ids and name tables of Bejeweled Deluxe 1.87 (data only, no code).
//
// Images: WinDMApp::mImages (app+0x790, new Image*[NUM_IMAGES]) is indexed by ImageId. Entry i is
//   loaded by WinDMApp::LoadAppImage(i) as GetImage("images\\" + gImageNames[i], true) followed by
//   GenerateDDSurface(). WinDMApp::Init loads 0..NUM_TITLE_IMAGES-1 synchronously; the loading
//   thread (WinDMApp::LoadingThreadProc) loads NUM_TITLE_IMAGES..NUM_IMAGES-1.
// Sounds: SoundManager::LoadSound(id, "sounds\\" + gSoundNames[id]) for id 0..NUM_SOUNDS-1
//   (loading thread). Played through SexyAppBase::PlaySample(id[, pan]).
// Music: MusicInterface::LoadMusic(id, gMusicFiles[id]) in WinDMApp::Init. The files on disk are
//   capitalised differently (1984-Network.s3m ...), so lookup must be case-insensitive.
// Fonts: owned by WinDMApp (see WinDMApp.h, +0x7c4..+0x7d4); gFontFiles lists the descriptor files.

namespace Sexy
{

enum ImageId
{
	IMAGE_TITLESCREEN = 0,              // titlescreen6       640x480 title background
	IMAGE_TITLEGREENBAR = 1,            // titlegreenbar2     340x24 loading bar
	IMAGE_TRIALCIRCLE = 2,              // trialcircle2       105x107 (unregistered only)
	IMAGE_CURSOR = 3,                   // cursor1            CURSOR_POINTER
	IMAGE_POINTER = 4,                  // pointer2           CURSOR_HAND
	IMAGE_OVERHAND = 5,                 // overhand           CURSOR_DRAGGING
	IMAGE_TYPER = 6,                    // typer              CURSOR_TEXT
	IMAGE_ALPHABACK = 7,                // alphaback1         640x480 playfield frame (alpha)
	IMAGE_GEMSHEET = 8,                 // gemsheet6          780x728: 15 frames x 52 px, 2 rows (normal, glow) per color, 104 px per color
	IMAGE_TARGET = 9,                   // target1            6 frames x 52 px selection cursor
	IMAGE_DIALOGBOX = 10,               // dialogbox2         Dialog component image
	IMAGE_BOXBUTTON = 11,               // boxbutton2         DialogButton component image
	IMAGE_WAITBAR = 12,                 // waitbar            UpdateCheckDialog progress (not ported)
	IMAGE_OVER_NEWGAME_PUSHED = 13,     // over2-newgame-pushed1
	IMAGE_OVER_NORMAL_PUSHED = 14,      // over3-normal-pushed1
	IMAGE_OVER_NORMAL_SELECTED = 15,    // over7-normal-selected1
	IMAGE_OVER_TIMETRIAL_PUSHED = 16,   // over4-timetrial-pushed1
	IMAGE_OVER_TIMETRIAL_SELECTED = 17, // over8-timetrial-selected1
	IMAGE_OVER_OPTIONS_PUSHED = 18,     // over5-options-pushed1
	IMAGE_OVER_QUIT_PUSHED = 19,        // over6-quit-pushed1
	IMAGE_TIMERBAR = 20,                // timerbar           source of WinDMApp::mTimerBarImages
	IMAGE_ALPHADIGITS = 21,             // alphadigits4       font strip (point popups)
	IMAGE_SPARKLE = 22,                 // sparkle            14 frames x 40 px (hint sparkles)
	IMAGE_SCOREFONT = 23,               // scorefont1         font strip (score)
	IMAGE_OVER_HINT = 24,               // over1-hint1        hint button glow / pushed image
	IMAGE_SLIDER_TRACK = 25,            // slider-track
	IMAGE_SLIDER_THUMB = 26,            // slider-thumb
	IMAGE_CHECKBOX = 27,                // checkbox1          62x32: left half checked, right half unchecked
	IMAGE_CHECKERTEXTURES = 28,         // checkertextures2   5 textures x 102 px (two 51x51 cells each)
	IMAGE_OVERFONT = 29,                // overfont2          font strip (banner text)
	IMAGE_BLACKHOLE = 30,               // blackhole          9 frames x 256 px (level-complete vortex)
	IMAGE_HOLEMASK = 31,                // holemask           vortex overlay
	IMAGE_ELECT = 32,                   // elect              128 px wide level-wipe strip
	IMAGE_EDITBOX = 33,                 // editbox            96x32 3-slice edit frame
	IMAGE_SWAPPER1 = 34,                // swapper1           tutorial swap arrows
	IMAGE_SWAPPER2 = 35,                // swapper2           tutorial hand
	NUM_IMAGES = 36,
	NUM_TITLE_IMAGES = 7                // images loaded by Init before the title screen
};

// @0x536250 (const char*[36])
inline constexpr const char* gImageNames[NUM_IMAGES] =
{
	"titlescreen6", "titlegreenbar2", "trialcircle2", "cursor1", "pointer2", "overhand", "typer",
	"alphaback1", "gemsheet6", "target1", "dialogbox2", "boxbutton2", "waitbar",
	"over2-newgame-pushed1", "over3-normal-pushed1", "over7-normal-selected1",
	"over4-timetrial-pushed1", "over8-timetrial-selected1", "over5-options-pushed1",
	"over6-quit-pushed1", "timerbar", "alphadigits4", "sparkle", "scorefont1", "over1-hint1",
	"slider-track", "slider-thumb", "checkbox1", "checkertextures2", "overfont2", "blackhole",
	"holemask", "elect", "editbox", "swapper1", "swapper2"
};

enum SoundId
{
	SOUND_GOTSET = 0,       // gotset2      match of exactly 3 (longest run of the pass == 3)
	SOUND_GOTSETBIG = 1,    // gotsetbig2   match with a run of 4+
	SOUND_COMBO2 = 2,       // combo22      cascade level 1 (played on every scoring pass)
	SOUND_COMBO3 = 3,       // combo32
	SOUND_COMBO4 = 4,       // combo42
	SOUND_COMBO5 = 5,       // combo52
	SOUND_COMBO6 = 6,       // combo52 (sic: the table repeats combo52; combo62.ogg ships but is never loaded)
	SOUND_BAD = 7,          // bad2         illegal swap / invalid registration
	SOUND_GEMONGEM = 8,     // gemongem2    gems landing (throttled to one per 10 ticks)
	SOUND_EXPLODE = 9,      // explode2     end of the game-over shake
	SOUND_CLICK = 10,       // click2       (loaded, never played by game code)
	SOUND_SELECT = 11,      // select       gem selected (panned)
	SOUND_MENUCLICK = 12,   // menuclick    every app ButtonPress and every OptionsDialog button press
	SOUND_MULTISHOT = 13,   // multishot    two or more runs in one scoring pass
	SOUND_GO = 14,          // go1          tick 150 of a new board
	SOUND_LEVELUP = 15,     // levelup1     vortex: pieces pass radius 80
	SOUND_GAMEOVER = 16,    // gameover1    all pieces fell off after game over
	SOUND_FIRECRACKLE = 17, // firecrackle  vortex finished, level wipe starts
	SOUND_WARNING = 18,     // warning1     timed mode, bar low
	SOUND_WHIRLPOOL = 19,   // whirlpool1   looping SoundInstance during the vortex / wipe
	SOUND_EXCELLENT = 20,   // excellent1   move score crosses 120 * multiplier
	NUM_SOUNDS = 21
};

// @0x5362e0 (const char*[21])
inline constexpr const char* gSoundNames[NUM_SOUNDS] =
{
	"gotset2", "gotsetbig2", "combo22", "combo32", "combo42", "combo52", "combo52", "bad2",
	"gemongem2", "explode2", "click2", "select", "menuclick", "multishot", "go1", "levelup1",
	"gameover1", "firecrackle", "warning1", "whirlpool1", "excellent1"
};

enum MusicId
{
	MUSIC_NORMAL = 0,   // played (FadeIn) while a normal-mode board exists
	MUSIC_TIMED = 1,    // played while a timed-mode board exists
	MUSIC_TITLE = 2,    // title screen (PlayMusic(2, 0, false) in Init, FadeOut(2, true, 0.004) on leaving)
	NUM_MUSIC = 3
};

// string literals in WinDMApp::Init
inline constexpr const char* gMusicFiles[NUM_MUSIC] =
{
	"music\\1984-network.s3m", "music\\1981-datajack.s3m", "music\\1980-autonomus.s3m"
};

enum FontId
{
	FONT_ALPHADIGITS = 0,  // app+0x7c4 ImageFont(mImages[IMAGE_ALPHADIGITS], "data\\alphadigits4.txt")  point popups
	FONT_SCOREFONT = 1,    // app+0x7c8 ImageFont(mImages[IMAGE_SCOREFONT], "data\\scorefont1.txt")     score
	FONT_HEADER = 2,       // app+0x7cc ImageFont(app, "data\\afont1.txt")    dialog headers, HUD level, pause text
	FONT_MAIN = 3,         // app+0x7d0 ImageFont(app, "data\\mainfont1.txt") dialog lines/buttons, most text
	FONT_OVERFONT = 4,     // app+0x7d4 ImageFont(mImages[IMAGE_OVERFONT], "data\\overfont2.txt")       banner text
	NUM_FONTS = 5
};

inline constexpr const char* gFontFiles[NUM_FONTS] =
{
	"data\\alphadigits4.txt", "data\\scorefont1.txt", "data\\afont1.txt", "data\\mainfont1.txt",
	"data\\overfont2.txt"
};

// Image used by the (Image*, file) ImageFont ctor, or -1 for the (SexyAppBase*, file) ctor.
inline constexpr int gFontImageIds[NUM_FONTS] =
{
	IMAGE_ALPHADIGITS, IMAGE_SCOREFONT, -1, -1, IMAGE_OVERFONT
};

}

#endif
