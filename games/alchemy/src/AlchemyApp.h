#ifndef __ALCHEMY_ALCHEMYAPP_H__
#define __ALCHEMY_ALCHEMYAPP_H__

// AlchemyApp: the application class of Alchemy Deluxe 1.6 (Alchemy.exe's game image, linked 2004-08-23).
// Behaviour spec: games/alchemy/docs/GAME_NOTES.md. Addresses in comments are the original's.
//
// Original: class AlchemyApp : public Sexy::SexyApp (2004 SexyApp, 72 virtual slots), vftable 0x4e5160, ctor
// 0x4118c9, dtor 0x41159c. The 2004 SexyApp is a ButtonListener; the port's SexyAppBase is too. The original's
// offsets are in the comments of the members (the port's base classes have other sizes).

#include "SexyApp.h"
#include "MTRand.h"
#include "Res.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TitleScreen;
class SelectorScreen;
class SidePanel;
class EffectsWidget;
class HelpWidget;
class HighScoreWidget;
class Image;
class MemoryImage;
class ImageFont;
class ButtonWidget;
class Dialog;
class AutoplayBot;
class StateLog;

enum DialogId
{
	DIALOG_NEW_GAME = 0,        // CONFIRM_NEW1/2: yes shows the selector
	DIALOG_SKILL = 1,           // the selector's skill change with a game in progress: yes starts a game at the pending skill
	DIALOG_QUIT = 2,            // CONFIRM_QUIT1/2
	DIALOG_HINT = 3,            // the wizard's hints (BUTTONS_FOOTER)
	DIALOG_OPTIONS = 4,         // OptionsDialog
	DIALOG_NAME_ENTRY = 7,      // ENTER_YOUR_NAME for a high score
	DIALOG_HIGH_SCORES = 0xb,   // the all-time table
	DIALOG_TUTORIAL = 0xc,      // CONFIRM_TUTORIAL1/2
	DIALOG_COLOR_DEPTH = 8,     // the full screen switch failed: the display's colour depth
	DIALOG_LOAD_GAME = 0x10     // CONFIRM_LOAD1/2: yes starts a game and loads the saved one
};

// Button ids of the screens (the app's ButtonPress/ButtonDepress; 2000 + dialog id is a dialog's yes, 3000 + id its no)
enum ButtonId
{
	BUTTON_NEW_GAME = 10,       // 0x411e2a
	BUTTON_TUTORIAL = 11,       // 0x411f27 (the selector's tutorial; the side panel has no button for it)
	BUTTON_OPTIONS = 12,        // 0x410f28
	BUTTON_FORGE = 13,          // 0x4061d6 (board: use the forge)
	BUTTON_QUIT = 14,           // 0x412017
	BUTTON_HINT = 15,           // 0x403c24 (board: the wizard's hint)
	BUTTON_SOUND = 16           // the sound toggle
};

class AlchemyApp : public SexyApp
{
public:
	enum
	{
		NUM_LOADING_TASKS = 101,        // mNumLoadingThreadTasks (LoadingThreadProc)
		NUM_SHAPE_SIZES = 20,           // the shrink animation of a piece
		NUM_SHAPE_IMAGES = 24,          // 22 shapes, the blank block and the skull
		NUM_BACKDROPS = 9,
		SCREEN_WIDTH = 640,
		SCREEN_HEIGHT = 480,
		BOARD_X = 0x96                  // the board area is 490 x 480 at x = 150; the side panel is the left 150
	};

	Image**					mImages;                // +0x6b0 new Image*[66] (0x108 bytes)
	MemoryImage*			mShapeImages[NUM_SHAPE_SIZES][NUM_SHAPE_IMAGES]; // +0x6b4 .. +0xe34
	ImageFont*				mAFont1;                // +0xe34 data\afont1.txt (the text font of the dialogs)
	ImageFont*				mFont1;                 // +0xe38 data\font1.txt
	ImageFont*				mScoreFont;             // +0xe3c data\scorefont.txt with IMAGE_SCOREFONT
	ImageFont*				mOverFont;              // +0xe40 data\overfont1.txt with IMAGE_OVERFONT1
	ImageFont*				mTitleFont;             // +0xe44 data\titlefont.txt
	ImageFont*				mLevelFont;             // +0xe48 data\levelfont1.txt with IMAGE_LEVELFONT1
	TitleScreen*			mTitleScreen;           // +0xe4c until the loading is done and the player starts
	SelectorScreen*			mSelector;              // +0xe50 the game selector (intro) screen or NULL
	Board*					mBoard;                 // +0xe54 the current game or NULL
	EffectsWidget*			mEffects;               // +0xe58 the full-screen particle overlay
	SidePanel*				mSidePanel;             // +0xe5c the left 150 pixels: forge gauge, buttons
	MemoryImage*			mBackdrops[NUM_BACKDROPS]; // +0xe60 the backdrop (crop of IMAGE_BACKDROP) and level2..9, gamma-corrected
	std::list<MemoryImage*>	mButtonImages;          // +0xe84 the CopyImage()s of the button strips
	std::list<ButtonWidget*> mButtons;              // +0xe90
	HelpWidget*				mHelp;                  // +0xea4 the help text over the board or NULL
	MemoryImage*			mSoundImages[6];        // +0xea8 the sound toggle's frames
	ButtonWidget*			mSoundButton;           // +0xea0
	HighScoreWidget*		mHighScores;            // +0xec0 the high-score screen over the board or NULL
	bool					mTutorialMode;          // +0xec4 the next board is the tutorial
	int						mSkillLevel;            // +0xec8 1 (beginner) .. 3
	int						mPendingSkill;          // +0xecc a skill chosen in the selector, applied when the game restarts
	bool					mTimeTrial;             // +0xed0 time-trial mode
	bool					mSelectorFlag;          // +0xed1 true until the first board starts
	bool					mNoPopLink;             // +0xed2 registry "NoPopLink": hides the "more games" link
	double					mDiff[4];               // +0x620 data\diff.txt: four difficulty constants
	std::string				mLastScoreName;         // +0x640 registry "LastScoreName"
	std::string				mLastInternetScoreName; // +0x65c registry "LastInternetScoreName" (kept, never shown)

	// port: the game's test hooks (the framework's -seed, -script, -fast and -trace are in SexyAppBase.h).
	//   -autoplay=<seed>         a bot with its own RNG plays through script input until the game is over
	//                            (testhooks/Autoplay.h); -autoplaymode=strategic|timetrial|tutorial;
	//   -autoplaycap=<n>         quits at update n (default 20000);
	//   -autoplaylog=<file>      writes its input as a -script file, with comments on what happened;
	//   -autoplayshots=<prefix>  screenshots at the game's milestones
	bool					mAutoplay;
	unsigned long			mAutoplaySeed;
	std::string				mAutoplayMode;
	int						mAutoplayCap;
	int						mAutoplayGames;
	std::string				mAutoplayLogFileName;
	std::string				mAutoplayShotPrefix;
	AutoplayBot*			mAutoplayBot;
	//   -autoplayui              the bot plays through the mouse: clicks on the buttons, the way a -script does, and writes
	//                            -autoplayscript=<file> (the clicks and moves as script lines)
	//   -autoplaystart=<n>       the bot starts at update n (the original's loading takes a while)
	//   -statelog=<file>         each change of the board's state, score, moves, forged pieces, boards cleared and the
	//                            piece to place, with the update it came at (the identical-play pass compares it with
	//                            the original's values)
	//   -autoplayskill=<0..2>    (with -autoplayui) clicks that skill level's button before the mode's
	//   -autoplaymiss=<n>        (with -autoplayui) one piece in n is put on a cell it cannot go to
	//   -autoplayextra=<n>       (with -autoplayui) before one piece in n the bot clicks the hint button or presses a key (the pause)
	bool					mAutoplayUi;
	int						mAutoplaySkill;
	int						mAutoplayMiss;
	int						mAutoplayExtra;
	int						mAutoplayStart;
	std::string				mAutoplayScriptFileName;
	std::string				mStateLogFileName;
	StateLog*				mStateLog;

public:
	AlchemyApp();                                           // 0x4118c9
	virtual ~AlchemyApp();                                  // 0x41159c

	virtual void			Init() override;                // 0x412d69 slot 17
	virtual void			LoadingThreadProc() override;   // 0x4124cf slot 18
	virtual void			WriteToRegistry() override;     // 0x411a41 slot 19
	virtual void			ReadFromRegistry() override;    // 0x411b43 slot 20
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override; // 0x410bb6
	virtual void			ButtonPress(int theId) override;          // 0x410db6
	virtual void			ButtonDepress(int theId) override;        // 0x413eea
	virtual bool			KillDialog(int theDialogId) override;     // 0x4111fa slot 61
	virtual void			ModalOpen() override;                     // 0x410cb9 slot 63: pauses the board
	virtual void			ModalClose() override;                    // 0x410cd3 slot 64
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	virtual MusicInterface*	CreateMusicInterface() override;
	virtual void			GetShutdownSaves(std::vector<ShutdownSave>* theSaves) override;
	virtual void			ShutdownHook() override;                  // port (web): ~AlchemyApp's save (the app is never deleted)
	virtual void			UpdateFrames() override;                  // port: runs the test hook's bot
	void					TestMouseMoveTo(int theX, int theY) { MouseMoveTo(theX, theY); }  // port: the bot's pointer, as a script's

	using SexyApp::KillDialog;

	bool					LoadImage(int theIndex);                  // 0x4123f1
	void					TitleScreenDone();                        // 0x4144f1: the side panel, the buttons, the selector
	ButtonWidget*			AddPanelButton(int theId, const Rect& theRect, Image* theStrip, const Rect& theOverRect, const Rect& theDownRect); // 0x41448f + 0x4143e3
	void					SetMute(bool wantMuted);                  // 0x410d2a slot 71: mutes or unmutes, then the sound button follows
	void					UpdateSoundButton();
	void					SetTimeTrial(bool isTimeTrial);           // 0x410d01
	void					SetSkillLevel(int theSkill);              // 0x410ced

	void					ShowSelector();                           // 0x411357
	void					StartGame();                              // 0x41122e
	void					ShowHighScores();                         // 0x411138
	void					ShowAllTimeScores();                      // 0x410dc3
	void					ShowHelp();                               // 0x410e37
	void					HideHelp();                               // 0x410ee0
	void					ShowOptions();                            // 0x410f28
	void					ShowNameEntry(const SexyString& theLines); // 0x4110bf
	void					ConfirmNewGame();                         // 0x411e2a
	void					ConfirmTutorial();                        // 0x411f27
	void					ConfirmQuit();                            // 0x412017
	void					ConfirmLoad();                            // 0x411d4f
	void					RemoveWidgetAndDelete(Widget* theWidget);

	void					AutoplayShot(const std::string& theFileName) { SaveScreenshot(theFileName); } // port: the bot's screenshots
	bool					HasSavedGame();                           // 0x4010fc: savegame.dat opens
	bool					DeleteSaveGame();                         // 0x40111e
};

extern AlchemyApp* gApp;

}

#endif
