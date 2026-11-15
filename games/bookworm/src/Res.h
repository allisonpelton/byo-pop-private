#ifndef __BOOKWORM_RES_H__
#define __BOOKWORM_RES_H__

// BookWorm Deluxe's shared ids: images, fonts, sounds, songs, game modes, tile sets, dialogs.
// The original has no ResourceManager: three hard-coded file-name tables, indexed by these ids
// (re/bookworm/analysis/NAMES.md 4.1):
//   gImageNames 0x52e410 (149; the six that only registration drew are NULL in the port), loaded as GetImage(GetString("ImagePath", "images\\") + name)
//   gFontNames  0x52e3c0 (19),  loaded as new ImageFont(app, GetString("FontPath", "fonts\\") + name)
//   gSoundNames 0x52e668 (47),  loaded as LoadSound(id, GetString("SoundPath", "sounds\\") + name)
// Every id is used as mApp->mImages[IMAGE_X], mApp->mFonts[FONT_X], PlaySample(SOUND_X).

namespace Sexy
{

enum ImageId
{
	IMAGE_BACKDROP_TITLE = 0,
	IMAGE_BACKDROP_TITLE_LOADMETER = 1,
	// 2: registration only, taken out (the slot stays empty, so the ids keep their numbers)
	IMAGE_LOGO_TITLE = 3,
	IMAGE_TITLE_LOADMETER = 4,
	// 5: registration only, taken out (the slot stays empty, so the ids keep their numbers)
	// 6: registration only, taken out (the slot stays empty, so the ids keep their numbers)
	IMAGE_DIALOG = 7,
	IMAGE_DIALOGBUTTON = 8,
	// 9: registration only, taken out (the slot stays empty, so the ids keep their numbers)
	// 10: registration only, taken out (the slot stays empty, so the ids keep their numbers)
	// 11: registration only, taken out (the slot stays empty, so the ids keep their numbers)
	IMAGE_EDITBOX = 12,
	IMAGE_CURSOR_POINTER = 13,
	IMAGE_CURSOR_HAND = 14,
	IMAGE_CURSOR_DRAGGING = 15,
	IMAGE_CURSOR_TEXT = 16,                 // last image loaded by Init; the loading thread loads the rest
	IMAGE_BACKDROP_BOARD = 17,
	IMAGE_BACKDROP_BOARD_SCORCHED = 18,
	IMAGE_BACKDROP_BONUS = 19,
	IMAGE_BACKDROP_MENU = 20,
	IMAGE_BACKDROP_FAME = 21,
	IMAGE_BACKDROP_HELP = 22,
	IMAGE_BACKDROP_HELP_BOX = 23,
	IMAGE_BACKDROP_IHS = 24,
	IMAGE_BACKDROP_WB_SPEECH = 25,
	IMAGE_BACKDROP_WB_THOUGHT = 26,
	IMAGE_BACKDROP_WB_TEXT = 27,
	IMAGE_GAMESELECT_ACTION = 28,
	IMAGE_GAMESELECT_ACTION_OVER = 29,
	IMAGE_GAMESELECT_CLASSIC = 30,
	IMAGE_GAMESELECT_CLASSIC_OVER = 31,
	IMAGE_GAMESELECT_HALL = 32,
	IMAGE_GAMESELECT_HALL_OVER = 33,
	IMAGE_GAMESELECT_QUIT = 34,
	IMAGE_GAMESELECT_QUIT_OVER = 35,
	IMAGE_GAMESELECT_BUBBLE = 36,
	IMAGE_DIALOG_LEVELUP = 37,
	IMAGE_BUTTON_FAME_CONTINUE = 38,
	IMAGE_BUTTON_HELP_CONTINUE = 39,
	IMAGE_BUTTON_GOTO_IHS = 40,
	IMAGE_BUTTON_GOTO_FAME = 41,
	IMAGE_BUTTON_SUBMIT_IHS = 42,
	IMAGE_BUTTON_SUBMIT_IHS_HILITE = 43,
	IMAGE_BUTTON_HELP_TOPIC = 44,
	IMAGE_BUTTON_HELP_TOPIC_HILITE = 45,
	IMAGE_IHS_TAB_NAMES = 46,
	IMAGE_IHS_TAB_NAMES_HILITE = 47,
	IMAGE_IHS_TAB_INDICATOR = 48,
	IMAGE_IHS_ALIAS_WARNING = 49,
	IMAGE_BAR_LEVELUP_HEADER = 50,
	IMAGE_BAR_GAMEOVER_HEADER = 51,
	IMAGE_BAR_FOOTER_SCROLL = 52,
	IMAGE_BAR_FOOTER_CONTINUE = 53,
	IMAGE_RANKSTRIP = 54,                   // NULL after loading: copied to BookWormApp::mRankStripImage and deleted
	IMAGE_RANKFRAME = 55,
	IMAGE_LEVEL_UP_STAR = 56,
	IMAGE_FX_ASHFLAKE = 57,
	IMAGE_FX_EMBER_SM = 58,
	IMAGE_FX_EMBER_MED = 59,
	IMAGE_FX_EMBER_LRG = 60,
	IMAGE_FX_EMBER_SMOKE_SM = 61,
	IMAGE_FX_EMBER_SMOKE_MED = 62,
	IMAGE_FX_EMBER_SMOKE_LRG = 63,
	IMAGE_FX_PARTICLE_SM = 64,
	IMAGE_FX_PARTICLE_MED = 65,
	IMAGE_FX_PARTICLE_LRG = 66,
	IMAGE_FX_SPARK_SM = 67,
	IMAGE_FX_SPARK_MED = 68,
	IMAGE_FX_SPARK_LRG = 69,
	IMAGE_FX_SMOKE_SM = 70,
	IMAGE_FX_FLAME_LRG = 71,
	IMAGE_FX_FIRELINE = 72,
	IMAGE_FX_LEX_SWEAT_SEQ = 73,
	IMAGE_FX_LEX_SWEAT_TRICKLE = 74,
	IMAGE_TILE = 75,
	IMAGE_TILE_HILITE = 76,
	IMAGE_TILE_FIRE = 77,
	IMAGE_TILE_FIRE_HILITE = 78,
	IMAGE_TILE_GOLD = 79,
	IMAGE_TILE_GOLD_HILITE = 80,
	IMAGE_TILE_GREEN = 81,
	IMAGE_TILE_GREEN_HILITE = 82,
	IMAGE_TILE_SAPPHIRE = 83,
	IMAGE_TILE_SAPPHIRE_HILITE = 84,
	IMAGE_TILE_DIAMOND = 85,
	IMAGE_TILE_DIAMOND_HILITE = 86,
	IMAGE_TILE_CHARRED = 87,
	IMAGE_TILE_GEMHILITE = 88,
	IMAGE_TILE_SPIN_FX = 89,
	IMAGE_TILE_X = 90,
	IMAGE_FONT_TILE_LETTERS = 91,           // 26 rows A..Z (alpha file only: white)
	IMAGE_FONT_TILE_DIGITS = 92,            // 10 rows 1..10
	IMAGE_TILE_GREEN_GLOW = 93,
	IMAGE_TILE_GOLD_GLOW = 94,
	IMAGE_TILE_FIRE_GLOW = 95,
	IMAGE_TILE_FIRE_FX1 = 96,
	IMAGE_TILE_FIRE_FX2 = 97,
	IMAGE_ARROWS = 98,
	IMAGE_BONUSARROW_LEFT = 99,
	IMAGE_BONUSARROW_RIGHT = 100,
	IMAGE_BONUSARROW_TOP = 101,
	IMAGE_LEX_SHADOW = 102,
	IMAGE_LEX_HEADLESS = 103,
	IMAGE_LEX_GLOW_IDLE = 104,
	IMAGE_LEX_SMILE_IDLE = 105,
	IMAGE_LEX_SMILE_BLINK = 106,
	IMAGE_LEX_GIGGLE_1 = 107,
	IMAGE_LEX_GIGGLE_2 = 108,
	IMAGE_LEX_SLURP_1 = 109,
	IMAGE_LEX_SPEAK_AH = 110,
	IMAGE_LEX_SPEAK_EE = 111,
	IMAGE_LEX_SPEAK_OH = 112,
	IMAGE_LEX_SPEAK_PB = 113,
	IMAGE_LEX_SWEAT = 114,
	IMAGE_LEX_SWEAT_FACING = 115,
	IMAGE_LEX_BURNT = 116,
	IMAGE_LEX_BURNT_BLINK = 117,
	IMAGE_LEX_EATING = 118,
	IMAGE_LEXBIG_SPEAK_AH = 119,
	IMAGE_LEXBIG_SPEAK_EE = 120,
	IMAGE_LEXBIG_SPEAK_MM = 121,
	IMAGE_LEXBIG_SPEAK_OO = 122,
	IMAGE_LEXBIG_SPEAK_PB = 123,
	IMAGE_WB_TEXT_SHOUT_TAIL = 124,
	IMAGE_WB_TEXT_TALK_TAIL = 125,
	IMAGE_WB_TEXT_THINK_TAIL = 126,
	IMAGE_PROGRESS_GREEN = 127,
	IMAGE_PROGRESS_LAVENDER = 128,
	IMAGE_PROGRESS_LIME = 129,
	IMAGE_PROGRESS_MAGENTA = 130,
	IMAGE_PROGRESS_MULTI = 131,
	IMAGE_PROGRESS_NAVY = 132,
	IMAGE_PROGRESS_ORANGE = 133,
	IMAGE_PROGRESS_PURPLE = 134,
	IMAGE_PROGRESS_RED = 135,
	IMAGE_PROGRESS_TAN = 136,
	IMAGE_PROGRESS_TEAL = 137,
	IMAGE_PROGRESS_YELLOW = 138,
	IMAGE_PROGRESS_BURNT = 139,
	IMAGE_SPARKLE = 140,
	IMAGE_SPARKLE_LARGE = 141,
	IMAGE_DIALOGBUTTON_PLAIN = 142,
	IMAGE_SLIDERTRACK = 143,
	IMAGE_SLIDERTHUMB = 144,
	IMAGE_CHECKBOXES = 145,                 // left half unchecked, right half checked (BookWormApp::mCheckbox*Image)
	IMAGE_CHECKBOXES_HELP = 146,
	IMAGE_WAITBAR = 147,
	IMAGE_CLOUDS = 148,
	NUM_IMAGES = 149,

	NUM_INIT_IMAGES = IMAGE_CURSOR_TEXT + 1 // images 0..16 are loaded by Init, the rest by the loading thread
};

enum FontId
{
	FONT_VERDANA10BOLD = 0,
	FONT_VERDANA12BOLD = 1,
	FONT_VERDANA14BOLD = 2,
	FONT_VERDANA18BOLDCOLOR = 3,
	FONT_LOCKERGNOME14OUTLINE = 4,          // last font loaded by Init; the loading thread loads the rest
	FONT_VERDANA18BOLD = 5,
	FONT_SCRIPPSCOLLEGEOS13BOLD = 6,
	FONT_SCRIPPSCOLLEGEOS15BOLD = 7,
	FONT_SCRIPPSCOLLEGEOS18BOLD = 8,
	FONT_LOCKERGNOME18 = 9,
	FONT_LOCKERGNOME24 = 10,
	FONT_COURIERFINALDRAFT20BOLD = 11,
	FONT_GOLDSCORE = 12,
	FONT_GOLDSCOREHEATED = 13,
	FONT_LOCKERGNOMESMALLCOLOR = 14,
	FONT_LEGACYSERIFMDSCITCTT12 = 15,
	FONT_MYRIADWEB10 = 16,
	FONT_MYRIADCONDENSEDWEB11 = 17,
	FONT_MYRIADWEB12BOLD = 18,
	NUM_FONTS = 19,

	NUM_INIT_FONTS = FONT_LOCKERGNOME14OUTLINE + 1
};

// The rising notes are played as SOUND_UP1 + min(n, 7). Id 24 is a second "buttonclick" entry.
enum SoundId
{
	SOUND_BUTTONCLICK = 0,
	SOUND_GAMEOVERTONE = 1,
	SOUND_BAD = 2,
	SOUND_BLANK = 3,
	SOUND_UP1 = 4,
	SOUND_UP2 = 5,
	SOUND_UP3 = 6,
	SOUND_UP4 = 7,
	SOUND_UP5 = 8,
	SOUND_UP6 = 9,
	SOUND_UP7 = 10,
	SOUND_UP8 = 11,
	SOUND_UP9 = 12,
	SOUND_NEXTSELECT = 13,
	SOUND_WORDCOMPLETE = 14,
	SOUND_BONUSCOMPLETE = 15,
	SOUND_BUZZER = 16,
	SOUND_PIECE_HIT = 17,
	SOUND_CHOMP = 18,
	SOUND_FLAMETILE = 19,
	SOUND_WARNING = 20,
	SOUND_TILEBURN = 21,
	SOUND_APPEAR2 = 22,
	SOUND_GOLDTILE = 23,
	SOUND_BUTTONCLICK_2 = 24,
	SOUND_REDTILECLEARED = 25,
	SOUND_SPINSUPERBONUS = 26,
	SOUND_BONUSWORD = 27,
	SOUND_SCREENBURNGAMEOVER = 28,
	SOUND_LEX_ASTONISHING = 29,
	SOUND_LEX_AWESOME = 30,
	SOUND_LEX_BURP = 31,
	SOUND_LEX_CAREFUL = 32,
	SOUND_LEX_DANGER = 33,
	SOUND_LEX_EXCELLENT = 34,
	SOUND_LEX_FANTASTIC = 35,
	SOUND_LEX_GETREADY = 36,
	SOUND_LEX_GO = 37,
	SOUND_LEX_INCREDIBLE = 38,
	SOUND_LEX_LETSGO = 39,
	SOUND_LEX_LEVELUP = 40,
	SOUND_LEX_LOOKOUT = 41,
	SOUND_LEX_SCRAMBLE = 42,
	SOUND_LEX_UNBELIEVABLE = 43,
	SOUND_LEX_VERYGOOD = 44,
	SOUND_LEX_WATCHOUT = 45,
	SOUND_LEX_WELCOME = 46,
	NUM_SOUNDS = 47
};

// One MO3 file ("music\\Bookworm1.mo3") loaded five times as five independent songs. The PlayMusic /
// FadeIn offsets are MO3 order numbers (row 0); the last bool of PlayMusic / FadeIn is noLoop.
enum SongId
{
	SONG_MENU = 0,                          // title, game selector (ORDER_MENU)
	SONG_CLASSIC = 1,                       // classic game (ORDER_CLASSIC)
	SONG_ACTION = 2,                        // action game (ORDER_ACTION)
	SONG_JINGLE = 3,                        // game-over jingle (no loop), hall of fame (ORDER_FAME)
	SONG_DANGER = 4,                        // danger theme
	NUM_SONGS = 5
};

enum MusicOrder
{
	ORDER_CLASSIC = 0,
	ORDER_MENU = 27,
	ORDER_ACTION = 37,
	ORDER_DANGER_CLASSIC = 51,
	ORDER_FAME = 54,
	ORDER_GAMEOVER_ACTION = 59,
	ORDER_GAMEOVER_CLASSIC = 61,
	ORDER_DANGER_ACTION = 65
};

// BookWormApp::mGameMode (selector, Board, BoardDraw, LevelUpWidget)
enum GameMode
{
	GAMEMODE_CLASSIC = 0,
	GAMEMODE_ACTION = 1
};

// BookWormApp::mTileSetImages: 26 pre-drawn letter tiles (A..Z, left to right) per tile kind, built by
// the app's loading thread, drawn by BoardDraw and FlyingPieceList
enum TileSetId
{
	TILESET_NORMAL = 0,
	TILESET_NORMAL_HILITE = 1,
	TILESET_GREEN = 2,
	TILESET_GREEN_HILITE = 3,
	TILESET_GOLD = 4,
	TILESET_GOLD_HILITE = 5,
	TILESET_FIRE = 6,
	TILESET_FIRE_HILITE = 7,
	TILESET_CHARRED = 8,
	NUM_TILESETS = 9
};

// Dialog ids (the SexyApp dialog map). YES / OK / footer presses reach BookWormApp::ButtonDepress as
// Dialog2004::ID_YES_2004 + id (2000 + id), NO / CANCEL as ID_NO_2004 + id (3000 + id). Button modes are
// the port's Dialog::BUTTONS_NONE / YES_NO / OK_CANCEL / FOOTER (0..3).
enum DialogId
{
	DIALOG_UNUSED_0 = 0,                    // never created; 2000 -> KillDialog(0) + DoGameSelector
	DIALOG_OPTIONS = 1,                     // OptionsDialog (DoOptions)
	// 2 the RegisterDialog: taken out (registration)
	DIALOG_UNUSED_3 = 3,                    // never created; killed by NewGame / DoGameSelector
	// 4 the trial-expired dialog: taken out
	DIALOG_CONTINUE = 5,                    // continue the saved game? (TryConfirmLoad)
	// 6 the internet connection question, 7 and 8 the update check: not ported (online features)
	// 9 the invalid registration code: taken out
	// 11 the wait message of OpenURL and 12 the URL to open by hand (URLOpenFailed): taken out
	DIALOG_QUIT = 13,                       // quit confirmation (ConfirmQuit)
	DIALOG_HINT = 15,                       // HintDialog (DoHintDialog) and the Board's can't-scramble messages
	DIALOG_CREDITS = 16,                    // CreditsDialog
	// 17 the Board's and Fame's registration upsells: taken out
	DIALOG_MESSAGE = 18,                    // OK-only messages (new-user name checks, Options warnings)
	DIALOG_SELECT_USER = 19,                // SelectUserDialog
	DIALOG_DELETE_USER = 20,                // DeleteUserDialog
	DIALOG_CONFIRM_DELETE_USER = 21,        // delete confirmation (DeleteUserDialog)
	DIALOG_NEW_USER = 22,                   // NewUserDialog
	// 23 the internet alias, 24 to 27 the Hall of Fame's resubmit message, wait dialog, error and warning: not ported
	DIALOG_LEAVE_GAME = 28                  // leave-game confirmation (ConfirmEndGame)
};

// Button ids heard by more than one class
enum
{
	BUTTON_HINT_DISABLE = 26                // HintDialog's disable-hints button (also decoded by the app)
};

// The file-name tables (defined in BookWormApp.cpp)
extern const char* const gImageNames[NUM_IMAGES];   // 0x52e410
extern const char* const gFontNames[NUM_FONTS];     // 0x52e3c0
extern const char* const gSoundNames[NUM_SOUNDS];   // 0x52e668

}

#endif
