#ifndef __TYPERSHARK_WINWSAPP_H__
#define __TYPERSHARK_WINWSAPP_H__

// WinWSApp.cpp / WinWSAppScreens.cpp: the application class of Typer Shark Deluxe 1.02 (WinTS.exe, game PE 0x401000-0x4623f0).
//
// Original: class Sexy::WinWSApp : public SexyApp, vftable 0x52d734 (67 slots), ctor 0x4373b0, destructor body 0x4366a0
// (scalar deleting destructor 0x437960), sizeof 0x930; its own data runs from +0x618 to +0x92c. WinMain (0x401000) makes
// it (640 x 480), calls Init, Start and Shutdown and deletes it. The offsets below are the original's (code never
// depends on them; `grep -n "+0x8e8" games/typershark/src/*.h` finds a field).
//
// The application owns every long-lived object of the game: the image table (one Image* per ImageId of Res.h), the
// fonts, the screens and dialogs on the widget manager, the sound instances, the second (module) music interface, the
// user manager and the current user. gApp is the one global; gGameRand (DAT_00565384) is the generator all game
// draws go through.

#include "SexyApp.h"
#include "Res.h"
#include "Common.h"
#include "MTRand.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Sexy
{

class Image;
class ImageFont;
class MemoryImage;
class MusicInterface;
class SoundInstance;
class Dialog;
class DialogButton;
class Widget;

// the classes of the other translation groups (headers are included by the .cpp files only)
class Board;
class TitleScreen;
class MenuScreen;
class MapScreen;
class HighScoreScreen;
class StatsScreen;
class UserProfileMgr;   // the original calls it UserManager (0x427440 ctor, 0x41f090 dtor); users\user.idx
class UserProfile;      // one player (users\user%d.dat)
class LessonManager;
class TypingTutor;       // the widget at +0x72c (ctor 0x460b30, vftable 0x531544): a ButtonListener at +0x58
class StatusBar;         // the widget at +0x8dc (ctor 0x448a40, sizeof 0x88)
class KeyDialog;         // the plain message dialog (ctor 0x43ec30)
class HintDialog;        // the hint box (ctor 0x439ef0)
class OptionsDialog;     // ctor 0x43bf10
class AbyssModeDialog;   // ctor 0x43e7a0
class DeleteUserDialog;  // ctor 0x43dcb0

// the one generator all game draws go through (the original's DAT_00565384, made in Init)
extern MTRand* gGameRand;

// The 0x30-byte block the app keeps at +0x6f0 (operator new(0x30), read by GetPointsSoFar 0x41d7d0 and by the board
// 0x44d200): the first int is the points of the game so far. The rest is read by later groups.
struct AppStats
{
	int mPointsSoFar;
	int mReserved[11];
};

// The dialog ids the app (and the dialogs) use; the button ids that come back are 2000 + id (yes/ok) and 3000 + id
// (no/cancel). Ids that only a dialog class and ButtonDepress (0x431630) know are listed by the groups that own them.
enum WinWSDialogId
{
	DIALOG_WIN_GAME = 0x8d,             // WinGameDialog (0x43a4f0)
	DIALOG_MESSAGE = 0x1f4,             // the plain message dialogs (0x41bd80, 0x41c4f0)
	DIALOG_DIFFICULTY = 0x309,          // DifficultyDialog (0x43f0f0)
	DIALOG_OPTIONS = 0x30a,             // OptionsDialog (0x43bf10), made by ShowOptions (0x414150)
	DIALOG_REGISTER_30C = 0x30c,        // not ported (RegisterDialog)
	DIALOG_NEW_USER = 0x44d,            // NewUserDialog (0x43bcc0), made by ShowNewUserDialog (0x414270)
	DIALOG_SELECT_USER = 0x44e,         // SelectUserDialog "WHO ARE YOU?" (0x43edb0), made by 0x4128a0
	DIALOG_DELETE_USER = 0x44f,         // DeleteUserDialog "DELETE WHO?" (0x43dcb0), made by 0x4267d0
	DIALOG_STATS_CLOSE = 0x450,         // the Close button of the stats screen (0x41b9e0)
	DIALOG_ABYSS_MODE = 0x451,          // AbyssModeDialog "CONTINUE?" (0x43e7a0), made by 0x4129a0
	DIALOG_REGISTER = 600,              // not ported (RegisterDialog, 0x414020)
	DIALOG_HIGHSCORE_CLOSE = 0xccf,     // the "Close This Screen" button of the high score screen (0x41bbd0)
	DIALOG_GAME_OVER = 0x16062          // GameOverDialog (0x43adb0)
};

// The two shark image tables (+0x914 and +0x920): outer key the creature type, inner key the picture role (0 swim,
// 2 death, 3 mirrored swim, 4 red; the entity constructors read them: Shark 0x4101d0, Piranha 0x40fca0 and variants)
typedef std::map<int, Image*> ImageVariantMap;
typedef std::map<int, ImageVariantMap> ImageVariantTable;

class WinWSApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,                       // WinMain 0x401000 sets 0x280 x 0x1e0
		GAME_HEIGHT = 480,
		NUM_LOADING_TASKS = 0x137,              // ctor: mNumLoadingThreadTasks
		NUM_FONTS = 14
	};

	// ---- data from +0x618 ----
	KeyDialog*				mMessageDialog;         // +0x618 the plain message dialog (id 500) of 0x41bd80 / 0x41c4f0
	Dialog*					mDialog61c;             // +0x61c set and cleared by ButtonDepress (0x431630)
	Dialog*					mRegisterDialog;        // +0x620 not ported (0x414020 makes it)
	HintDialog*				mHintDialog;            // +0x624 the hint box (id 0x3f3) of 0x41c8c0; ButtonDepress reads it
	MenuScreen*				mMenuScreen;            // +0x628 (0x41d5c0)
	MapScreen*				mMapScreen;             // +0x62c the map of the levels (ctor 0x458d10, sizeof 0x238; 0x426440 makes it hidden, its +0x228 is the chosen level)
	AbyssModeDialog*		mAbyssModeDialog;       // +0x630 "CONTINUE?" (0x4129a0)
	Board*					mBoard;                 // +0x634 the game board; every game screen reads it
	HighScoreScreen*		mHighScoreScreen;       // +0x638 (0x41bbd0)
	StatsScreen*			mStatsScreen;           // +0x63c (0x41b9e0)
	int						mMaxPlays;              // +0x640 registry "MaxPlays" (trial limit; not ported, stays 0)
	DialogButton*			mHighScoreCloseButton;  // +0x644 "Close This Screen" (0x41bbd0)
	std::vector<int>		mPausedMusic;           // +0x648 (pad), +0x64c..+0x654 the songs LostFocus paused (0x426920)
	MusicInterface*			mMusic;                 // +0x658 the app's own module music interface (a second one besides SexyAppBase::mMusicInterface)
	LessonManager*			mLessonManager;         // +0x65c TODO: name from the lesson groups (0x4137d0 asks it something; dtor 0x421c50)
	bool					mFlag660;               // +0x660 ctor 0; read by 0x40c630 / 0x40cba0
	// +0x664 / +0x668: the letters the random-letter generator (0x41b4e0) has handed out
	int						mUsedUpperCount;        // +0x664 how many of them are A..Z
	int						mUsedLetterCount;       // +0x668 how many in all
	int						mPlayTimeTicks;         // +0x66c MaxTime accounting (0x41d910, 0x41d9d0; not ported)
	bool					mFlag670;               // +0x670 ctor 0
	bool					mFlag671;               // +0x671 ctor 0
	bool					mShouldCheckForUpdate;  // +0x672 LoadingThreadProc (0x435560): the update check; not ported, stays false
	bool					mFlag673;               // +0x673 trial flag (0x41d9d0, 0x41da80; not ported)
	DeleteUserDialog*		mDeleteUserDialog;      // +0x674 (0x4267d0)
	int						mMusicSlot;             // +0x678 which of the two songs plays (ChangeMusic 0x414350); ctor 1
	int						mMusicOrder;            // +0x67c the module order playing (ChangeMusic); ctor -1
	int						mField680;              // +0x680 ctor 0; read by ButtonDepress
	bool					mFlag684;               // +0x684 ctor 0; read by 0x42a870 / 0x42ee10
	bool					mFlag685;               // +0x685 ctor 0; read by 0x42a870 / 0x42ee10
	int						mOpenDialogCount;       // +0x688 dialogs open over the board (0x414020 / 0x414150 add one, the dialogs take it off)
	bool					mFlag68c;               // +0x68c ctor 0
	bool					mFlag68d;               // +0x68d ctor 0
	int						mField690;              // +0x690 ctor 0; read by 0x426c60, 0x42a870, 0x42ee10, 0x42f3c0, ButtonDepress
	std::string				mUsedLetters;           // +0x694 the letters handed out so far (0x41b4e0, 0x41d210)
	int						mField6b0;              // +0x6b0 read by 0x430b20 (config parser)
	std::string				mString6b4;             // +0x6b4 set by the config parsers (0x428d80, 0x4292b0); cleared in the dtor
	std::string				mString6d0;             // +0x6d0 set by the config parsers (0x429110); cleared in the dtor
	int						mTreasure;              // +0x6ec the treasure index (10..15) of the current game, 0 = not drawn yet (0x412830)
	AppStats*				mStats;                 // +0x6f0 (operator new(0x30))
	std::vector<std::string> mVector6f4;            // +0x6f4 (pad), +0x6f8: filled by the config parsers (0x4292b0)
	std::vector<std::string> mVector704;            // +0x704 (pad), +0x708: filled by the config parsers (0x429110)
	std::map<std::string, bool> mMap714;            // +0x714 filled by the config parsers (0x429110)
	std::map<std::string, bool> mMap720;            // +0x720 filled by the config parsers (0x4292b0)
	TypingTutor*			mTypingTutor;           // +0x72c the typing tutor (made by ButtonDepress; hidden by 0x41b9e0, taken away with the board by 0x41d5c0)
	std::vector<int>		mVector730;             // +0x730 (pad), +0x734..+0x73c: filled by the config (0x42a870, 0x42aad0, 0x42ee10); freed with free()
	int						mField740;              // +0x740 ctor -1; read by 0x42a870, 0x42aad0, 0x42ee10, 0x443a70
	SoundInstance*			mSoundInstance0;        // +0x744 GetSoundInstance(0) (LoadingThreadProc)
	SoundInstance*			mSoundInstance1;        // +0x748 GetSoundInstance(1)
	SoundInstance*			mSoundInstance1Copy;    // +0x74c the same pointer as +0x748
	DialogButton*			mStatsCloseButton;      // +0x750 "Close" button of the stats screen (0x41b9e0)
	// +0x754 .. +0x770: two std::vector of structs the config parsers fill (TODO: types from the config job)
	void*					mVector754[4];          // +0x754 (pad), begin, end, capacity (freed by 0x429780 then free())
	void*					mVector764[4];          // +0x764 (pad), begin, end, capacity
	// +0x774, +0x780, +0x78c: three maps the config parsers fill (TODO: types from the config job)
	std::map<int, std::vector<std::string> > mMap774;   // +0x774 the words by length (0x430b20 reads it through 0x42f930; cleared by 0x42fd10)
	std::map<std::string, bool> mMap780;            // +0x780
	std::map<std::string, bool> mMap78c;            // +0x78c
	// the left-facing and darkened copies of images that FUN_004337a0 makes (the entities read them)
	Image*					mRotorTorpedoDim;       // +0x79c IMAGE_ROTOR_TORPEDO, (200,200,200), mirrored
	Image*					mTorpedoSwimDim;        // +0x7a0 IMAGE_TORPEDO_SWIM
	Image*					mToxicSwimDim;          // +0x7a4 IMAGE_TOXIC_BASIC_SWIM
	Image*					mToxicGlowSwimDim;      // +0x7a8 IMAGE_TOXIC_GLOW_SWIM
	Image*					mGhostSharkSwimDim;     // +0x7ac IMAGE_SHARK_GHOST_SWIM
	Image*					mGhostSharkGlowDim;     // +0x7b0 IMAGE_SHARK_GHOST_GLOW_SWIM
	Image*					mCannonballDim;         // +0x7b4 IMAGE_CANNONBALL
	std::vector<Image*>		mColorizedFish;         // +0x7b8 (pad), +0x7bc: IMAGE_SMALLCARN then IMAGE_SMALLGOLDFISH in three colours each
	std::vector<Image*>		mSonarSweepImages;      // +0x7c8 (pad), +0x7cc: IMAGE_SONAR_SWEEP_0..4 mirrored and flipped
	std::vector<Image*>		mSonarSweep2Images;     // +0x7d8 (pad), +0x7dc: IMAGE_SONAR_SWEEP2_0..4 mirrored and flipped
	TitleScreen*			mTitleScreen;           // +0x7e8 the loading screen (the title screen made by Init)
	OptionsDialog*			mOptionsDialog;         // +0x7ec (0x414150)
	// +0x7f0 .. +0x89f: not used by any translated code

	// fonts (Init and LoadingThreadProc load them in this order: 0x8bc, 0x8c0, then 0x8a0 ...)
	ImageFont*				mFontGillSansMT10Bold;          // +0x8a0 data\GillSansMT10Bold.txt
	ImageFont*				mFontMyriadCondensed28Bold;     // +0x8a4 data\MyriadCondensedWeb28Bold.txt
	ImageFont*				mFontMyriadCondensed28BoldGL;   // +0x8a8 data\MyriadCondensedWeb28BoldGL.txt
	ImageFont*				mFontMyriadCondensed12Bold;     // +0x8ac data\MyriadCondensedWeb12Bold.txt
	ImageFont*				mFontMyriadCondensed14Bold;     // +0x8b0 data\MyriadCondensedWeb14Bold.txt
	ImageFont*				mFontSwissCheesed24;            // +0x8b4 data\SwissCheesed24.txt
	ImageFont*				mFontUnsteadyOversteer22;       // +0x8b8 data\UnsteadyOversteer22.txt
	ImageFont*				mFontRockwell13;                // +0x8bc data\Rockwell13.txt (dialog buttons and lines)
	ImageFont*				mFontRockwell13Bold;            // +0x8c0 data\Rockwell13Bold.txt (dialog headers)
	ImageFont*				mFontMyriadCondensed15;         // +0x8c4 data\MyriadCondensedWeb15.txt
	ImageFont*				mFontMyriadCondensed15b;        // +0x8c8 data\MyriadCondensedWeb15.txt again (a second ImageFont of the same file)
	ImageFont*				mFont12pt;                      // +0x8cc data\12ptfont.txt
	ImageFont*				mFont12ptGlow;                  // +0x8d0 data\12ptfontglow.txt
	ImageFont*				mFontCourierFinalDraft15Bold;   // +0x8d4 data\CourierFinalDraft15Bold.txt

	bool					mFlag8d8;               // +0x8d8 ctor 0; 0x414020 clears it; read by 0x42f3c0, 0x431320, ButtonDepress
	bool					mFlag8d9;               // +0x8d9 ctor 0
	StatusBar*				mStatusBar;             // +0x8dc the status bar shown with the board (made by 0x42f2a0; removed with the board by 0x41d5c0)
	SoundInstance*			mSoundInstance38;       // +0x8e0 GetSoundInstance(0x26)
	UserProfileMgr*			mProfileMgr;            // +0x8e4 (Init; deleted by 0x41f090 then free)
	UserProfile*			mUser;                  // +0x8e8 the current user (0x425f20), 0 when nobody is chosen
	bool					mFastCPU;               // +0x8ec LoadingThreadProc: PerfTimer::GetCPUSpeed() >= 695 (MHz); effects are cut down on slow machines
	bool					mHintsAllowed;          // +0x8ed ctor 1; read by 0x412d70 / 0x41c8c0 / 0x43d6b0 (the hint boxes may show)
	Image**					mImages;                // +0x8f0 one per ImageId (operator new(0x3e0) = 248 pointers)
	Image*					mRegisterImage;         // +0x8f4 images\register: not ported (stays NULL)
	bool					mFlag8f8;               // +0x8f8 ctor 1; Init clears it when the current user has a flag (+0x1dc) set
	int						mField8fc;              // +0x8fc ctor-less; read by 0x426c60, 0x429450, 0x42a870, 0x42ee10, ButtonDepress
	int						mField900;              // +0x900 same readers (and the board 0x44fe30)
	int						mField904;              // +0x904
	int						mField908;              // +0x908
	int						mField90c;              // +0x90c
	bool					mFlag910;               // +0x910 ctor 0
	ImageVariantTable		mSharkImages;           // +0x914 [type][(variant, 0)]: the shark and toxic-shark images, some copies owned (FUN_004337a0)
	ImageVariantTable		mPiranhaImages;         // +0x920 [type][(variant, 0)]: the piranha images
	// (sizeof 0x930)

public:
	WinWSApp();                                                         // 0x4373b0
	virtual ~WinWSApp();                                                // 0x4366a0 (scalar deleting 0x437960, slot 14)

	// ---- virtual overrides ----
	virtual void			InitPropertiesHook() override;                          // port: loads properties/texts.xml and the game texts
	virtual void			Init() override;                                        // 0x428500 slot 16
	virtual void			LoadingThreadProc() override;                           // 0x435560 slot 17
	virtual void			LoadingThreadCompleted() override;                      // 0x420130 slot 13
	virtual void			UpdateFrames() override;                                // 0x4127a0 slot 7
	virtual void			SetMusicVolume(double theVolume) override;              // 0x4127c0 slot 47
	virtual void			ButtonPress(int theId) override;                        // 0x412820 slot 3
	virtual void			ButtonDepress(int theId) override;                      // 0x431630 slot 4 (WinWSAppButtons.cpp)
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter,
								int theButtonMode) override;                        // 0x412b30 slot 20
	virtual void			WriteToRegistry() override;                             // 0x412d20 slot 18
	virtual void			GotFocus() override;                                    // 0x426a10 slot 62
	virtual void			LostFocus() override;                                   // 0x4269e0 slot 63
	virtual MusicInterface*	CreateMusicInterface() override;                        // port: the module music interface (BASS in the original)
	// 0x41d910 slot 33 (Shutdown) only counted the MaxTime trial minutes: not ported, SexyApp::Shutdown runs

	// ---- helpers (their original names are FUN_ addresses) ----
	bool					ChangeMusic(int theOrder);                              // 0x414350
	void					PauseMusic(bool thePause);                              // 0x426920 (true: pause what plays and remember it, false: resume it)
	int						GetTreasureIndex();                                     // 0x412830 (10..15, drawn from gGameRand the first time)
	std::string				GetTreasureName();                                      // 0x41b920 (the game text of the treasure)
	std::string				RandomLetter(bool theOnlyLetters);                      // 0x41b4e0 (a character not handed out yet; "" when none is left)
	void					RemoveLetter(char theLetter);                           // 0x41d210 (takes a character out of mUsedLetters)
	int						GetPointsSoFar();                                       // 0x41d7d0

	// loading
	bool					CheckRegistryAccess();                                  // 0x41ac70
	bool					LoadImages();                                           // 0x41ad70 (images 0..0xf1; Init loads 0xf2..0xf7)
	void					MakeDerivedImages();                                    // 0x4337a0 (colourised, mirrored and flipped copies)
	bool					LoadFont(ImageFont*& theFont, const char* thePath);     // the font-loading step of Init / LoadingThreadProc

	// the data-file loaders of LoadingThreadProc (written by the config / word-list jobs; each returns false when the file is bad)
	bool					LoadWaves();                                            // 0x434740 data\waves.txt
	bool					LoadWordLists();                                        // 0x430d20 the word lists (string 0x52d478)
	bool					LoadGemNames();                                         // 0x42b0c0 data\gemnames.txt
	bool					LoadShipNames();                                        // 0x42b400 data\shipnames.txt
	bool					LoadThemes();                                           // 0x434d60 the themes (string 0x52d448)

	// screens and dialogs (WinWSAppScreens.cpp)
	void					ShowSelectUserDialog();                                 // 0x4128a0 (dialog 0x44e)
	void					ShowAbyssModeDialog();                                  // 0x4129a0 (dialog 0x451)
	int						GetUserField34();                                       // 0x412ae0 (a number of the current user)
	void					RemoveMenuScreen();                                     // 0x412af0 (the widget at +0x628)
	void					ActivateTypingTutor();                                  // 0x412c90 (ButtonDepress(0x58) of the tutor, then its focus)
	void					MarkMenuScreenDirty();                                  // 0x412cc0
	void					BringStatsCloseButtonToFront();                         // 0x412ce0
	void					FocusTypingTutor();                                     // 0x412d00
	void					SetUserHints(bool theHints);                            // 0x412d50 (the current user's hints setting)
	bool					HintsEnabled();                                         // 0x412d70 (a user is chosen, has hints on, and the game allows them)
	void					ShowOptions();                                          // 0x414150 (dialog 0x30a)
	void					ShowNewUserDialog(const std::string& theText);          // 0x414270 (dialog 0x44d)
	void					ShowStatsScreen(int theWhich);                          // 0x41b9e0
	void					ShowHighScoreScreen();                                  // 0x41bbd0
	void					ShowMessageDialog(std::string theHeader, std::string theLines, std::string theButtonText, bool theFlatten); // 0x41bd80 (dialog 500)
	void					ShowConfirmDialog(std::string theHeader, std::string theLines, std::string theFooter, bool theFlatten,
								std::string theYesLabel, std::string theNoLabel);   // 0x41c4f0 (dialog 500 with two buttons)
	void					ShowHintDialog(std::string theLines, std::string theHeader); // 0x41c8c0 (dialog 0x3f3; the header is "" in the original's callers)
	void					ShowLevelIntro();                                       // 0x41cc60 (the hint box of the level that starts)
	void					ShowNoWindowedModeMessage();                            // 0x41d030 (dialog 0x30b)
	void					ShowMenuScreen();                                       // 0x41d5c0
	void					ShowMapScreen();                                        // 0x426440 (after loading: the title screen goes)
	void					ShowDeleteUserDialog(const std::vector<std::string>& theNames); // 0x4267d0 (dialog 0x44f)
	void					StartBoard();                                           // 0x431320 (a new Board for the chosen level)

	// the game's start, restart and end, and the words of a game (WinWSAppGame.cpp)
	void					ResetGame(bool theKeepGameMode);                        // 0x42a870 (ends the game, then ShowMenuScreen)
	void					RestartBoard();                                         // 0x42ee10 (new lesson manager, board and status bar)
	void					CreateBoard();                                          // 0x42f3c0 (new lesson manager and board for the chosen mode)
	void					ReturnToMap();                                          // 0x429450 (a level is over: the map comes back)
	void					SaveGameProgress(bool theFlag, bool theWithDouble);     // 0x426c60 (the totals go into the user, the profile is written)
	void					CopyMapListsToUser();                                   // 0x426a40 (the map screen's two lists go into the user)
	std::string				GetWordA();                                             // 0x4292b0 (the word at +0x6b4; drawn when it is empty)
	std::string				GetWordB();                                             // 0x429110 (the word at +0x6d0; drawn when it is empty)
	std::string				GetWord(int theMinLength, int theMaxLength);            // 0x428d80 (first letter not used yet; lists at +0x758)
	std::string				GetThemeWord(int theMinLength, int theMaxLength, int theMinTheme, int theMaxTheme); // 0x42aad0 (lists at +0x768)
	std::string				GetLongWord(int theMinLength, int theMaxLength);        // 0x430b20 (lists at +0x774, lengths 3 .. 0x15)
};

extern WinWSApp* gApp;

}

#endif
