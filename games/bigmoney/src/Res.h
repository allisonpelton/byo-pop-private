#ifndef __BIGMONEY_RES_H__
#define __BIGMONEY_RES_H__

// Resource ids and name tables of Big Money Deluxe (data only, no code). There is no resources.xml:
// the exe hard-codes these tables.
//
// Images: WinBMApp::mImages (app+0x6e4, new Image*[NUM_IMAGES]) is indexed by ImageId. Entry i is loaded
//   by WinBMApp::LoadAppImage(i) as GetImage("images\\" + gImageNames[i], true); WinBMApp::Init loads
//   all of them, in index order, before the title screen. An "_name" file next to an image is its alpha
//   mask (merged by the image loader); IMAGE_CLOUDS exists only as _clouds.jpg (white + that alpha).
// Sounds: SoundManager::LoadSound(id, "sounds\\" + gSoundNames[id]) for id 0..NUM_SOUNDS-1 in
//   WinBMApp::LoadingThreadProc. Played with SexyAppBase::PlaySample(id).
// Music: WinBMApp::Init loads MUSIC_FILE as every SongId (14 channels of one Impulse Tracker module);
//   the PlayMusic/FadeIn offset is the order position the song starts at.
// Fonts: owned by WinBMApp (+0x6ec..+0x71c); the descriptor files are the FONT_FILE_* strings.
// Spec: re/bigmoney/analysis/app.md section 3.

namespace Sexy
{

enum ImageId
{
	IMAGE_CURSOR_POINTER = 0,       // cursor_pointer    54x54 +mask   SetCursorImage(CURSOR_POINTER)
	IMAGE_CURSOR_HAND = 1,          // cursor_hand       58x58 +mask   CURSOR_HAND
	IMAGE_CURSOR_DRAGGING = 2,      // cursor_dragging   37x37 +mask   CURSOR_DRAGGING
	IMAGE_CURSOR_TEXT = 3,          // cursor_text       9x21          CURSOR_TEXT
	IMAGE_TITLEBACK = 4,            // titleback.jpg     640x480       TitleScreen
	IMAGE_BARANDLOGO = 5,           // barandlogo        540x149 +mask TitleScreen
	IMAGE_GOLDBAR = 6,              // goldbar.jpg       370x76 +mask  TitleScreen loading bar
	IMAGE_MRMONEYBAGS = 7,          // mrmoneybags.jpg   613x403 +mask TitleScreen
	IMAGE_TITLECIGARTIP = 8,        // titlecigartip     35x50 +mask   TitleScreen
	IMAGE_TITLEEYEGLOW = 9,         // titleeyeglow      163x99        TitleScreen (additive)
	IMAGE_BIGMONEYLOGO = 10,        // bigmoneylogo.jpg  472x239 +mask TitleScreen
	IMAGE_DELUXOR = 11,             // deluxor           155x150 +mask TitleScreen (HSL badge)
	IMAGE_DOLLARSTRIP = 12,         // dollarstrip       640x20 +mask  TitleScreen
	IMAGE_LEVELFONT = 13,           // levelfont         164x19 +mask  WinBMApp::mLevelFont strip (legacy font)
	IMAGE_FLOATFONT = 14,           // floatfont         223x20 +mask  unused (floatfont.txt loads its own)
	IMAGE_LEVEL = 15,               // level             51x21 +mask   Board
	IMAGE_LEFTBAR = 16,             // leftbar.jpg       640x480 +mask Board, HelpWidget, OverlayWidget
	IMAGE_BACKDROP = 17,            // backdrop.jpg      406x465       Board
	IMAGE_RIGHTBAR1 = 18,           // rightbar1.jpg     56x479        Board
	IMAGE_RIGHTBAR2 = 19,           // rightbar2.jpg     80x480 +mask  Board
	IMAGE_COIN = 20,                // coin              400x40 +mask  Board
	IMAGE_STACK = 21,               // stack             40x372        Board
	IMAGE_COINS = 22,               // coins             580x145 +mask Board (29x29 cells); source of WinBMApp::mBrightCoinsImage
	IMAGE_HORZCOINS = 23,           // horzcoins         464x145 +mask Board
	IMAGE_BAG = 24,                 // bag               132x36 +mask  Board
	IMAGE_EXPLOSION = 25,           // explosion         675x91        Board
	IMAGE_DOLLAR = 26,              // dollar            1056x33 +mask Board
	IMAGE_SPARKLE = 27,             // sparkle           280x20        Board
	IMAGE_OLDDIALOG = 28,           // olddialog         192x192 +mask NewDialog (DoDialog dialogs), CreditsDialog
	IMAGE_DIALOG = 29,              // dialog.jpg        192x192 +mask MoneyDialog subclasses
	IMAGE_DIALOGLIGHT = 30,         // dialoglight       14x14         MoneyDialog lights
	IMAGE_DIALOGGLOW = 31,          // dialogglow        22x22         MoneyDialog glow (additive)
	IMAGE_DIALOGBUTTON = 32,        // dialogbutton      96x36         every dialog's button component image
	IMAGE_BUTTONSTATES = 33,        // buttonstates.jpg  108x174       side buttons: rows of 29 (over 0..2, down 3..5)
	IMAGE_SLIDERTRACK = 34,         // slidertrack       209x20        OptionsDialog
	IMAGE_SLIDERTHUMB = 35,         // sliderthumb.png   36x31         OptionsDialog
	IMAGE_CHECKBOXES = 36,          // checkboxes.jpg    44x23         -> WinBMApp::mCheckboxUnchecked/CheckedImage
	IMAGE_RANKSTRIP = 37,           // rankstrip         100x2700      Board stats panel
	IMAGE_GAMEOVER = 38,            // gameover          333x181 +mask Board
	IMAGE_COUNTDOWN = 39,           // countdown.jpg     160x480 +mask Board
	IMAGE_BARBAGDISPLAY = 40,       // barbagdisplay.jpg 168x43        Board
	IMAGE_MOUTHS_UNUSED = 41,       // mouths            184x816 +mask unused (the table lists mouths twice)
	IMAGE_LEVELUP = 42,             // levelup           300x189 +mask Board
	IMAGE_EDITBOX = 43,             // editbox.png       96x32         RecordScoreDialog
	IMAGE_BAGHOLDER = 44,           // bagholder         41x34 +mask   Board
	IMAGE_GLOWBAG = 45,             // glowbag           64x64 +mask   Board
	IMAGE_WAITBAR = 46,             // waitbar           32x16         UpdateCheckDialog (not ported)
	IMAGE_SELECTBACK = 47,          // selectback.jpg    467x480       GameSelector, StartButton; selector tiles
	IMAGE_GAMETYPESTEXT = 48,       // gametypestext     465x248 +mask selector tiles (3 columns, pitch 154)
	IMAGE_ILLUSTRATIONS = 49,       // illustrations.jpg 452x128 +mask selector tiles (3 x 144, pitch 154)
	IMAGE_SKILLSTEXT = 50,          // skillstext        467x52 +mask  skill tiles
	IMAGE_SKILLSPRESSED = 51,       // skillspressed.jpg 467x52        skill tiles (down)
	IMAGE_BOTTOMLINE = 52,          // bottomline.jpg    436x18 +mask  Board
	IMAGE_COINGLOW = 53,            // coinglow          45x45 +mask   Board
	IMAGE_MRMONEY = 54,             // mrmoney           184x136 +mask -> WinBMApp::mMrMoneyImages
	IMAGE_MOUTHS = 55,              // mouths            184x816 +mask -> WinBMApp::mMrMoneyImages (6 frames of 136)
	IMAGE_BILL = 56,                // bill              1920x50 +mask Board (16 frames of 120); -> WinBMApp::mBillImages
	IMAGE_GAMEDIFFTYPE = 57,        // gamedifftype      100x135 +mask Board
	IMAGE_STARTTEXT = 58,           // starttext         280x100 +mask StartButton
	IMAGE_FLOATLOGO = 59,           // floatlogo         156x76 +mask  Board, HelpWidget, OverlayWidget
	IMAGE_PAUSEBACKDROP = 60,       // pausebackdrop     489x374       Board
	IMAGE_STARTARROWS = 61,         // startarrows       30x60 +mask   StartButton; -> WinBMApp::mStartArrowsMirrored
	IMAGE_INSTRUCTOARROWS = 62,     // instructoarrows   50x250        Board
	IMAGE_INSTRUCTOTEXT = 63,       // instructotext     200x450       Board
	IMAGE_HELPCIRCLES = 64,         // helpcircles.jpg   104x660       HelpWidget
	IMAGE_CLOUDS = 65,              // clouds: only _clouds.jpg 512x512 CreditsDialog
	IMAGE_BIGARROW = 66,            // bigarrow          372x87        HelpWidget
	IMAGE_NOMOREMOVES = 67,         // nomoremoves       333x181 +mask Board
	NUM_IMAGES = 68
};

// @0x5285b8 (const char*[71] in the original; the trial and register images are left out)
inline constexpr const char* gImageNames[NUM_IMAGES] =
{
	"cursor_pointer", "cursor_hand", "cursor_dragging", "cursor_text", "titleback", "barandlogo",
	"goldbar", "mrmoneybags", "titlecigartip", "titleeyeglow", "bigmoneylogo", "deluxor", "dollarstrip",
	"levelfont", "floatfont", "level", "leftbar", "backdrop", "rightbar1", "rightbar2",
	"coin", "stack", "coins", "horzcoins", "bag", "explosion", "dollar", "sparkle", "olddialog", "dialog",
	"dialoglight", "dialogglow", "dialogbutton", "buttonstates", "slidertrack", "sliderthumb",
	"checkboxes", "rankstrip", "gameover", "countdown", "barbagdisplay", "mouths", "levelup", "editbox",
	"bagholder", "glowbag", "waitbar", "selectback", "gametypestext", "illustrations", "skillstext",
	"skillspressed", "bottomline", "coinglow", "mrmoney", "mouths", "bill", "gamedifftype", "starttext",
	"floatlogo", "pausebackdrop", "startarrows", "instructoarrows", "instructotext",
	"helpcircles", "clouds", "bigarrow", "nomoremoves"
};

// Files are .ogg except piecehit, bonusdrop and explode (.au)
enum SoundId
{
	SOUND_CLEARCOINS = 0,
	SOUND_CLEARCOINS2 = 1,
	SOUND_CLEARCOINS3 = 2,
	SOUND_PIECEHIT = 3,
	SOUND_MOVEUP = 4,
	SOUND_BONUSDROP = 5,
	SOUND_BAGHIT = 6,
	SOUND_GOTBAG = 7,
	SOUND_GETREADY = 8,
	SOUND_GO = 9,
	SOUND_BUTTONCLICK = 10,     // every WinBMApp::ButtonPress (including dialog buttons)
	SOUND_TONE1 = 11,
	SOUND_TONE2 = 12,
	SOUND_TONE3 = 13,
	SOUND_TONE4 = 14,
	SOUND_TONE5 = 15,
	SOUND_TONE6 = 16,
	SOUND_TONE7 = 17,
	SOUND_TONE8 = 18,
	SOUND_TONE9 = 19,
	SOUND_TONEPUZZLE = 20,
	SOUND_WARNING = 21,
	SOUND_GAMEOVERTONE = 22,
	SOUND_EXPLODE = 23,
	SOUND_GAMEOVER = 24,
	SOUND_LEVELUP = 25,
	SOUND_GOODJOB = 26,
	SOUND_EXCELLENT = 27,
	SOUND_INCREDIBLE = 28,
	SOUND_AWESOME = 29,
	SOUND_UNBELIEVABLE = 30,
	SOUND_BIGMONEY = 31,
	SOUND_THREE = 32,
	SOUND_TWO = 33,
	SOUND_ONE = 34,
	SOUND_BAD = 35,             // invalid registration code (not ported)
	SOUND_COINTUBE = 36,
	SOUND_WELCOME = 37,         // GameSelector update 100, first selector of the run
	SOUND_SPEEDBONUS = 38,
	SOUND_BONUS = 39,
	SOUND_SUPERBONUS = 40,
	SOUND_MEGAMONEY = 41,
	SOUND_ACTION = 42,          // 42 + GameType
	SOUND_STRATEGY = 43,
	SOUND_PUZZLE = 44,
	SOUND_WAKEUP = 45,
	SOUND_ANEWHIGHSCORE = 46,
	SOUND_LOVEIT = 47,          // GameSelector update 100, 1 in 10 after the first selector
	NUM_SOUNDS = 48
};

// @0x5286d8 (const char*[48])
inline constexpr const char* gSoundNames[NUM_SOUNDS] =
{
	"clearcoins", "clearcoins2", "clearcoins3", "piecehit", "moveup", "bonusdrop", "baghit", "gotbag",
	"getready", "go", "buttonclick", "tone1", "tone2", "tone3", "tone4", "tone5", "tone6", "tone7",
	"tone8", "tone9", "tonepuzzle", "warning", "gameovertone", "explode", "gameover", "levelup",
	"goodjob", "excellent", "incredible", "awesome", "unbelievable", "bigmoney", "three", "two", "one",
	"bad", "cointube", "welcome", "speedbonus", "bonus", "superbonus", "megamoney", "action", "strategy",
	"puzzle", "wakeup", "anewhighscore", "loveit"
};

// One module, loaded as every song id (WinBMApp::Init)
inline constexpr const char* MUSIC_FILE = "music\\QualityTime_BigMoney.it";

enum SongId
{
	SONG_PUZZLE = 0,            // order 0   Puzzle game music
	SONG_PUZZLE_LEVELUP = 1,    // order 23  Board level-up jingle
	SONG_PUZZLE_PAUSE = 2,      // order 24  Board pause music
	SONG_ACTION = 3,            // order 26  Action game music
	SONG_ACTION_LEVELUP = 4,    // order 40
	SONG_ACTION_PAUSE = 5,      // order 41
	SONG_STRATEGY = 6,          // order 44  Strategy game music
	SONG_STRATEGY_LEVELUP = 7,  // order 63
	SONG_STRATEGY_PAUSE = 8,    // order 64
	SONG_GAMEOVER = 9,          // order 67
	SONG_UNUSED10 = 10,         // loaded, never played
	SONG_TITLE = 11,            // order 87  title; keeps playing into the game selector
	SONG_SELECTOR = 12,         // order 89  game selector; never heard (ShowGameSelector)
	SONG_UNUSED13 = 13,         // loaded, never played
	NUM_SONGS = 14
};

// Start orders of the songs (the PlayMusic/FadeIn offsets the game uses)
enum
{
	ORDER_PUZZLE = 0,
	ORDER_PUZZLE_LEVELUP = 23,
	ORDER_PUZZLE_PAUSE = 24,
	ORDER_ACTION = 26,
	ORDER_ACTION_LEVELUP = 40,
	ORDER_ACTION_PAUSE = 41,
	ORDER_STRATEGY = 44,
	ORDER_STRATEGY_LEVELUP = 63,
	ORDER_STRATEGY_PAUSE = 64,
	ORDER_GAMEOVER = 67,
	ORDER_TITLE = 87,
	ORDER_SELECTOR = 89
};

// Font descriptor files (string literals in WinBMApp::Init)
inline constexpr const char* FONT_FILE_1 = "data\\bigmoneyfont1.txt";
inline constexpr const char* FONT_FILE_3 = "data\\bigmoneyfont3.txt";
inline constexpr const char* FONT_FILE_SCORE = "data\\scorefont.txt";   // default point size 25
inline constexpr const char* FONT_FILE_LEVEL = "data\\levelfont.txt";   // legacy format, image IMAGE_LEVELFONT
inline constexpr const char* FONT_FILE_FLOAT = "data\\floatfont.txt";

// WinBMApp::mGameType (+0x818, registry GameType)
enum GameType
{
	GAMETYPE_ACTION = 0,        // timed: ModalOpen/ModalClose pause only this type
	GAMETYPE_STRATEGY = 1,
	GAMETYPE_PUZZLE = 2,
	NUM_GAMETYPES = 3
};

// Dialog ids (SexyAppBase dialog map). YES/OK/footer presses arrive in WinBMApp::ButtonDepress as
// 2000+id, NO/CANCEL as 3000+id (id % 10000 decides which).
enum DialogId
{
	DIALOG_NEW_GAME = 0,                // "end the current game?" YES_NO -> ShowGameSelector
	DIALOG_OPTIONS = 1,                 // OptionsDialog (MoneyDialog, FOOTER "Done")
	DIALOG_RECORD_SCORE = 3,            // RecordScoreDialog (MoneyDialog, non-modal, OK_CANCEL)
	DIALOG_CONTINUE_GAME = 5,           // "continue your last game?" YES_NO
	DIALOG_QUIT = 12,                   // YES_NO -> Shutdown
	DIALOG_HIGH_SCORES = 13,            // HighScoreDialog (MoneyDialog)
	DIALOG_CREDITS = 14,                // CreditsDialog
	DIALOG_INFO = 16                    // FOOTER: the "enter a name" notice
};

// Side ButtonWidgets made by WinBMApp::TitleScreenIsFinished (WinBMApp is their ButtonListener)
enum ButtonId
{
	BUTTON_NEW_GAME = 0,        // Rect(30, 126, 108, 29) -> ConfirmNewGame
	BUTTON_OPTIONS = 1,         // Rect(31, 189, 108, 29) -> ShowOptionsDialog
	BUTTON_QUIT = 2             // Rect(30, 444, 108, 29) -> ConfirmQuit
};

}

#endif
