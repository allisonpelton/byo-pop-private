#ifndef __BOOKWORM_BOOKWORMAPP_H__
#define __BOOKWORM_BOOKWORMAPP_H__

// BookWormApp.cpp: the application class of BookWorm Deluxe, the dead Math class embedded in it, the
// high-score entries and the free helpers of the module (WinMain's neighbours 0x40fe80-0x4100f0).
// Behaviour spec: re/bookworm/analysis/app.md; cross-module names: re/bookworm/analysis/NAMES.md.
//
// Original: class Sexy::BookWormApp : public Sexy::SexyApp (2004 SexyApp : ButtonListener), sizeof 0x1348
// (operator new in WinMain 0x40fe80), vftable 0x4fbda4 (68 slots: SexyApp's 67 + slot 67
// TitleScreenDone). sizeof(SexyApp) is 0x618, so BookWormApp's data starts at +0x618 with the embedded
// Math; offsets below are absolute (the port's base classes have other sizes, nothing depends on them).
// Ctor 0x41a0f0, dtor body 0x4187b0, scalar deleting dtor 0x41a510 (slot 14).
// Globals: gApp 0x5347d4 and gGameRand 0x5347d0 (both set by WinMain; port: gGameRand by Init, D17).

#include "SexyApp.h"
#include "MTRand.h"
#include "Res.h"
#include "UserManager.h"
#include <list>
#include <set>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TitleScreen;
class BWGameSelector;
class HelpWidget;
class FameWidget;
class Definitionator;
class StatKeeper;
class Image;
class MemoryImage;
class ImageFont;
class Dialog;
class Graphics;
class Color;
class MusicInterface;
class BookWormApp;
class AutoplayBot;

extern BookWormApp* gApp;   // 0x5347d4 (only WinMain used it; port: set by the ctor)
extern MTRand* gGameRand;   // 0x5347d0 the Board's letter / fire / special / bonus-word generator

// Original: class Sexy::Math (Math.cpp 0x42e330-0x42e3d0), sizeof 0xb48, vftable 0x50074c (1 slot: scalar
// deleting dtor 0x42e3a0). Embedded in BookWormApp at +0x618. Nobody reads its tables. Not in a Math.h of
// its own: on a case-insensitive file system that would shadow <math.h> (NAMES.md D19).
class Math
{
public:
	// +0x000 vfptr
	float			mSin[360];                  // +0x004 sin(mPi * i / 180), i = 0..359, stored as float
	float			mNegCos[360];               // +0x5a4 -cos(i * mPi / 180)
	float			mPi;                        // +0xb44 0x40490fda (3.1415925f)

	Math();                                     // 0x42e330
	virtual ~Math();                            // 0x42e390 body, 0x42e3a0 slot 0 (deleting)
};

// A high-score table row (0x40 bytes, copy ctor 0x412080). Containers: std::multiset with the two
// predicates below (insert places an entry after its equivalents, as VC7 does).
struct HighScoreEntry
{
	SexyString		mName;                      // +0x00 player name (UserManager's current user)
	SexyString		mWord;                      // +0x1c word ("" in the Classic / Action tables)
	int				mScore;                     // +0x38 score / word points / word length (Longest after a reload)
	mutable bool	mIsNew;                     // +0x3c added this session (SubmitHighScores); FameWidget clears it
	                                            // through the sets' const iterators: not part of either order

	HighScoreEntry() : mScore(0), mIsNew(false) {} // port: the original fills every field where it makes one
};

// 0x4115d0: a.mScore != b.mScore ? a.mScore > b.mScore : strcmp(a.mWord, b.mWord) < 0 (unsigned bytes).
// The original takes both arguments by value; const& gives the same results (D16).
struct HighScoreByScore
{
	bool operator()(const HighScoreEntry& a, const HighScoreEntry& b) const;
};

// 0x411570: a.mWord.size() != b.mWord.size() ? a.mWord.size() > b.mWord.size() : a.mScore > b.mScore
struct HighScoreByWordLength
{
	bool operator()(const HighScoreEntry& a, const HighScoreEntry& b) const;
};

typedef std::multiset<HighScoreEntry, HighScoreByScore> HighScoreByScoreSet;
typedef std::multiset<HighScoreEntry, HighScoreByWordLength> HighScoreByLengthSet;

class BookWormApp : public SexyApp
{
public:
	enum
	{
		NUM_LOADING_TASKS = 216,                // mNumLoadingThreadTasks (LoadingThreadProc)
		NUM_CLASSIC_SCORES = 10,
		NUM_ACTION_SCORES = 10,
		NUM_WORD_SCORES = 3                     // mBestWords, mLongestWords
	};

	Math					mMath;              // +0x618 (0xb48 bytes) built by the ctor, never read
	Board*					mBoard;             // +0x1160 current game (StartGame); after a game it stays, unparented, under the selector until RemoveBoard
	TitleScreen*			mTitleScreen;       // +0x1164 Init .. TitleScreenDone (the original ctor leaves it uninitialised; port: NULL)
	BWGameSelector*			mGameSelector;      // +0x1168 main menu or NULL (ShowGameSelector)
	HelpWidget*				mHelpWidget;        // +0x116c help screen or NULL (ShowHelp / HideHelp)
	FameWidget*				mFameWidget;        // +0x1170 hall of fame or NULL (ShowHallOfFame)
	Definitionator*			mDefinitionator;    // +0x1174 popdefs.txt, NULL until the loading thread made it
	UserManager*			mUserManager;       // +0x1178 profiles (ReadFromRegistry; the original ctor leaves it uninitialised; port: NULL)
	HighScoreByScoreSet		mClassicScores;     // +0x117c top 10 classic (registry Scores\Classic)
	HighScoreByScoreSet		mActionScores;      // +0x1188 top 10 action (Scores\Action)
	HighScoreByScoreSet		mBestWords;         // +0x1194 top 3 best-scoring words (Scores\Best)
	HighScoreByLengthSet	mLongestWords;      // +0x11a0 top 3 longest words (Scores\Longest)
	Image**					mImages;            // +0x11ac new Image*[NUM_IMAGES] (DDImages from GetImage); [IMAGE_RANKSTRIP] is NULL after loading
	ImageFont*				mFonts[NUM_FONTS];  // +0x11b0 .. +0x11f8
	Image*					mCheckboxOnImage;   // +0x11fc right half of IMAGE_CHECKBOXES (CopyImage): checked
	Image*					mCheckboxOffImage;  // +0x1200 left half: unchecked
	MemoryImage*			mTileSetImages[NUM_TILESETS]; // +0x1204 26 pre-drawn letter tiles per TileSetId (loading thread)
	Image*					mUnusedImages[4];   // +0x1228 zeroed by the ctor, deleted by the dtor, never set
	std::list<MemoryImage*>	mCopiedImages;      // +0x1238 owns the three copies below
	MemoryImage*			mTileSapphireImage; // +0x1244 copy of IMAGE_TILE_SAPPHIRE
	MemoryImage*			mTileDiamondImage;  // +0x1248 copy of IMAGE_TILE_DIAMOND
	MemoryImage*			mRankStripImage;    // +0x124c copy of IMAGE_RANKSTRIP (the original is deleted)
	bool					mInitDone;          // +0x1250 end of Init (TitleScreen, trial branch)
	int						mGameMode;          // +0x1254 GameMode of the current / last game; ctor 0
	int						mSelectedGameMode;  // +0x1258 mode picked in the selector (SelectGameMode); ctor -1
	int						mScoringMethod;     // +0x125c ctor 1, never changed (0 would select WordScore's dead length formula)
	std::set<SexyString>	mDictionary;        // +0x1260 wordlist.txt, upper case, length >= 3
	std::vector<SexyString>	mBonusWords;        // +0x126c bonus_word_dictionary.txt in file order, upper case
	std::set<SexyString>	mUsedBonusWords;    // +0x127c bonus words completed (Board::BonusWordScored); never read
	SexyString				mUnusedString1;     // +0x1288 never used
	SexyString				mUnusedString2;     // +0x12a4 assigned "" by the ctor, never used
	SexyString				mBonusWord;         // +0x12c0 next bonus word (Board::PickBonusWord)
	int						mGameOption;        // +0x12dc ctor 1; only copied to / from the selector; meaning unknown
	int						mUserTimesPlayed;   // +0x12e0 per-user "TimesPlayed"
	bool					mScrambled;         // +0x12e4 set by every Board::Scramble, never read
	bool					mWelcomePlayed;     // +0x12e5 the selector played lex_welcome (once per run)
	bool					mUnused12e6;        // +0x12e6 ctor true, no reader
	bool					mShowHelp;          // +0x12e7 option: help at game start (per user); ctor true
	bool					mShowHints;         // +0x12e8 option: hints (per user; saved games carry it); ctor true
	bool					mShowDefs;          // +0x12e9 option: definitions (per user); ctor true
	bool					mInternetOK;        // +0x12ea connecting accepted once this run (internet scores, not ported)
	bool					mUnused12eb;        // +0x12eb no reader
	bool					mWatchOutHintShown; // +0x12ec HINT_WATCH_OUT was scheduled (once per run)
	bool					mFireAlertHintShown;// +0x12ed HINT_FIRE_ALERT
	bool					mGreenTileHintShown;// +0x12ee HINT_GREEN_TILE
	bool					mUnused12ef;        // +0x12ef no reader
	bool					mScrambleHintShown; // +0x12f0 HINT_SCRAMBLE
	bool					mUnusedHintFlags[11]; // +0x12f1 ctor false, no reader
	std::vector<int>		mPausedSongs;       // +0x12fc songs paused by PauseAllMusic(true)
	int						mResumeUpdateCnt;   // +0x130c mUpdateCount when the board was last resumed (Board ignores input for 15 updates after; GotFocus)
	SexyString				mSEHInfo;           // +0x1310 appended to the crash report, never written (not ported)
	int						mPlayTicks;         // +0x132c ++ per Board update in play (Shutdown: minutes = / 6000)
	bool					mLevelReached;      // +0x1330 a level summary was shown this game (Board::ShowSummary)
	int						mPlaysToCount;      // +0x1334 ++ by ShutdownWidgets when mLevelReached (trial)
	int						mMaxExecutions;     // +0x1338 property MaxExecutions (0), unused
	int						mMaxPlays;          // +0x133c property MaxPlays (0)
	int						mMaxTime;           // +0x1340 property MaxTime (60, minutes)
	// +0x1344 4 bytes never touched (sizeof 0x1348)

	// port: the game's test hooks (the framework's -seed, -script, -fast and -trace are in SexyAppBase.h).
	// None of them changes the game or its generators.
	//   -autoplay=<seed>         a bot with its own RNG plays through script input until the game is over
	//                            (games/bookworm/src/testhooks/Autoplay.h);
	//   -autoplaymode=<mode>     classic (default), action, or both (Classic, then Action);
	//   -autoplaycap=<n>         quits at update n (default 200000);
	//   -autoplaylog=<file>      writes its input as a -script file, with comments on what happened;
	//   -autoplayshots=<prefix>  screenshots <prefix><mode>-levelup<n>.png, -gameover.png, -fame.png and,
	//                            every -autoplayshotwords=<n> words (default 25, 0: none), -word<n>.png;
	//   -autoplayhints=1         plays the word solver's first hint every move (the mod "solver" on)
	bool					mAutoplay;
	unsigned long			mAutoplaySeed;
	std::string				mAutoplayMode;
	int						mAutoplayCap;
	std::string				mAutoplayLogFileName;
	std::string				mAutoplayShotPrefix;
	int						mAutoplayShotWords;
	bool					mAutoplayHints;
	AutoplayBot*			mAutoplayBot;

public:
	BookWormApp();                                              // 0x41a0f0
	virtual ~BookWormApp();                                     // 0x4187b0 body, 0x41a510 slot 14

	// ButtonListener (dialog buttons are forwarded here by Dialog2004 as 2000 + id / 3000 + id)
	virtual void	ButtonPress(int theId) override;            // 0x410b30 slot 3
	virtual void	ButtonDepress(int theId) override;          // 0x417080 slot 4

	virtual void	UpdateFrames() override;                    // 0x4100f0 slot 7
	virtual void	LoadingThreadCompleted() override;          // 0x4169d0 slot 13 (update check: not ported)
	virtual void	Init() override;                            // 0x414a10 slot 16
	virtual void	LoadingThreadProc() override;               // 0x41a8a0 slot 17
	virtual void	WriteToRegistry() override;                 // 0x412000 slot 18
	virtual void	ReadFromRegistry() override;                // 0x41a540 slot 19
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
						const SexyString& theDialogLines, const SexyString& theDialogFooter,
						int theButtonMode) override;            // 0x410970 slot 20
	virtual void	URLOpenFailed(const std::string& theURL) override;   // 0x414fa0 slot 24
	virtual void	URLOpenSucceeded(const std::string& theURL) override; // 0x410b10 slot 25
	virtual bool	OpenURL(const std::string& theURL, bool shutdownOnOpen = false) override; // 0x415100 slot 26
	// 0x416090 slot 32 GetGameSEHInfo not ported (crash report)
	virtual void	Shutdown() override;                        // 0x410ca0 slot 33
	virtual void	SetMusicVolume(double theVolume) override;  // 0x410110 slot 47
	virtual Dialog*	DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
						const SexyString& theDialogLines, const SexyString& theDialogFooter,
						int theButtonMode) override;            // 0x4103e0 slot 55
	virtual bool	KillDialog(int theDialogId) override;       // 0x4110b0 slot 58
	virtual void	ModalOpen() override;                       // 0x410ad0 slot 60
	virtual void	ModalClose() override;                      // 0x410af0 slot 61
	virtual void	GotFocus() override;                        // 0x419000 slot 62
	virtual void	LostFocus() override;                       // 0x419040 slot 63
	virtual void	TitleScreenDone();                          // 0x414ef0 slot 67 (TitleScreen::ButtonDepress)

	// port: the 2004 SexyApp makes a BassMusicInterface itself (no virtual); here an SDLMusicInterface
	// with BASS 2.0's music gain (docs/FRAMEWORK_2004.md section 6, 4.2)
	virtual MusicInterface* CreateMusicInterface() override;

	// port: the game's texts are read from the player's Bookworm.exe (GameText.h) right after the partner
	// properties, before ReadFromRegistry (the default high scores) and anything that shows text
	virtual void	InitPropertiesHook() override;

	// port (web): the page can be closed or crash without Shutdown, which is where the original saves a
	// game in progress (see SexyAppBase::UpdateUserDataBackup)
	virtual void	GetShutdownSaves(std::vector<ShutdownSave>* theSaves) override;
	virtual void	WriteShutdownRegistry() override;

	// port: the test hooks' command-line switches (see mAutoplay); everything else goes to the framework
	virtual void	HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	// Screens and widgets
	void			ShutdownWidgets();                          // 0x40ffa0 (Shutdown) saves a game in progress
	void			RemoveBoard();                              // 0x410180
	void			ShowHelp();                                 // 0x4101c0
	void			HideHelp();                                 // 0x4102d0
	void			RemoveGameSelector();                       // 0x410360
	void			RemoveFameWidget();                         // 0x4103a0
	void			StartGame();                                // 0x410d10 new Board for mGameMode
	void			ShowGameSelector();                         // 0x410f00
	void			ShowHallOfFame();                           // 0x411020
	void			StartGameForUser();                         // 0x413c80 LoadUserSettings + StartGame (+ help)
	void			SelectGameMode(int theMode);                // 0x4153c0 the continue-game dialog or start
	void			GoToMainMenu();                             // 0x4157e0 the Board's menu button

	// Dialogs
	void			ShowSelectUserDialog();                     // 0x410520 (DIALOG_SELECT_USER)
	void			ShowHint(int theHintId, const SexyString& theHeader, const SexyString& theLines,
						const SexyString& theFooter, int theButtonMode); // 0x4105d0 (DIALOG_HINT)
	void			ShowCredits();                              // 0x4106c0 (DIALOG_CREDITS)
	void			ShowOptions();                              // 0x4107c0 (DIALOG_OPTIONS)
	void			ShowNewUserDialog(const SexyString& theText); // 0x4108c0 (DIALOG_NEW_USER)
	void			ShowQuitDialog();                           // 0x415660 (DIALOG_QUIT)
	void			ShowDeleteUserDialog(std::vector<SexyString> theNames); // 0x418590 (DIALOG_DELETE_USER)
	void			ShowRegisterDialog();                       // 0x413cb0 not ported: does nothing (registered build)
	bool			IsInternetScoresEnabled();                  // 0x4152a0 false (internet scores not ported)
	// Not ported (registration / internet high scores / update check): ShowInvalidCodeDialog 0x413f30,
	// ShowAliasDialog 0x414080, ConfirmInternetConnect 0x415a20, AskForUpdateCheck 0x415c40,
	// DoUpdateCheck 0x415de0

	// Resources
	bool			CheckRegistryAccess();                      // 0x414910
	bool			LoadImage(int theImageId);                  // 0x4144b0
	bool			LoadFont(int theFontId);                    // 0x4146d0
	bool			LoadWordList(const SexyString& theFileName, std::set<SexyString>* theSet,
						std::vector<SexyString>* theVector);    // 0x41a5b0
	bool			LoadDictionary(const SexyString& theFileName, std::set<SexyString>* theSet); // 0x41a860
	bool			LoadBonusWords(const SexyString& theFileName, std::vector<SexyString>* theVector); // 0x41a880

	// Users, saved games, high scores
	void			LoadUserSettings();                         // 0x413a40
	bool			SaveCurrentUser();                          // 0x412100 (OptionsDialog)
	bool			HasSavedGame(const SexyString& theUserName, int theMode); // 0x412190
	// port: the saved-game file of a player and mode, "players/<name>/classic.dat" or ".../action.dat"
	// (relative: the framework's file helpers put it in the user dir), "" for other modes. The original
	// builds the same path in HasSavedGame and in Board's SaveGame / LoadGame / DeleteSavedGame (NAMES L4).
	static SexyString GetSaveGamePath(const SexyString& theUserName, int theMode);
	void			LoadHighScores();                           // 0x419080
	void			AddDefaultHighScores();                     // 0x418bd0
	void			SaveHighScores();                           // 0x412ec0
	void			SubmitHighScores(StatKeeper* theStats);     // 0x419cd0 (Board, game over)

	// Misc
	void			PauseAllMusic(bool pause);                  // 0x4184d0
	SexyString		FormatTime(int theSeconds);                 // 0x411cd0 "[d:][hh:]mm:ss", "" without a board
	SexyString		GetLevelTimeString(const SexyString& thePrefix); // 0x411ea0
	SexyString		GetTotalTimeString(const SexyString& thePrefix); // 0x411f50

	using SexyApp::ButtonPress;
	using SexyApp::KillDialog;
};

// Free helpers of the module (cdecl in the original)
void		DrawShadowedString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor,
				bool theShadow, const Color& theShadowColor);  // 0x410090 (FameWidget)
SexyString	TruncateToWidth(Graphics* g, const SexyString& theString, int theMaxWidth,
				const SexyString& theEllipsis);                 // 0x412da0 (FameWidget)
void		TrimString(SexyString& theString, const SexyString& theChars); // 0x4122e0 both ends
void		UpperCaseString(SexyString& theString);             // 0x4112f0 CRT toupper on every char
void		OptimizeImage(Image* theImage, const SexyString& theName); // 0x411220 (EDI = image, EAX = &name)

}

#endif
