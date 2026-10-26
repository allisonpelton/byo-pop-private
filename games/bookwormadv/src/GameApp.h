// Sexy::GameApp (constructor 0x443350, vtable 0x7adc44; its LuaLinkedClass part at +0x730): Bookworm Adventures'
// app. The game itself is mostly the scripts (main.lua and what it requires); the app loads them and the
// resources, owns the screens written in C++ (title, main menu, curtain, conversation panel, ...) and gives
// the scripts 73 methods (RegisterGameAppMethods, GameAppLua.cpp).
#pragma once

#include <set>
#include <string>
#include <vector>

#include "ArrowHint.h"
#include "LuaApp.h"

namespace JeffLib {
class SongPlayer;
class SoundPlayer;
}

namespace Sexy {

class Board;
class BookManager;
class ConversationPanel;
class CurtainTransition;
class CutScene;
class GameDialog;
class Image;
class Lex;
class LuaInterface;
class MainMenu;
class PopAnimManager;
class Profile;
class TitleScreen;
class TooltipWidget;
class TreasureScreen;
class Widget;

class GameApp : public LuaApp {
public:
	// The game's dialog ids (those that are ported)
	enum {
		DIALOG_OK = 0,           // DoOkDialog's
		DIALOG_OPTIONS = 1,      // the options (OptionsDialog)
		DIALOG_REALLY_QUIT = 2,  // the main menu's quit
		DIALOG_NEW_USER = 7,     // the new player's name (NewUserDialog): the conversation panel waits under it
		DIALOG_NO_WINDOWED = 13, // the options' "no windowed mode" message
		DIALOG_MENU = 17,        // the game's menu (MenuDialog)
		DIALOG_CONFIRM = 18,     // DoConfirmDialog's
		DIALOG_SKIP_MOVIE = 20,  // a click on a cut scene: skip it?
		DIALOG_CONTINUE = 21,    // "Continue" after the game lost the focus (the board stays muted under it)
	};

	// mTransitionState: what the curtain is doing (CurtainDone goes on from it)
	enum {
		TRANSITION_NONE = 0,
		TRANSITION_CUTSCENE = 1,       // closing over a screen to show a cut scene, then opening on it
		TRANSITION_OPENING = 2,        // closing (mCurtainLuaCallback when closed), then opening over a new screen
		TRANSITION_SAVED_GAME = 3,     // from the main menu or the chapter map to the game in progress
		TRANSITION_GAME_TO_MENU = 4,   // from a battle (or a mini-game) back to the main menu
		TRANSITION_TREASURE = 5,       // the treasure screen's continue
		TRANSITION_TO_MENU = 7,        // from the book back to the main menu (6 is the same)
		TRANSITION_NEW_GAME = 8,       // from the main menu to the book
		TRANSITION_TITLE_CURTAIN = 9,  // the title screen's: the callback when closed
	};

	// port: the loading thread runs in steps between frames (LoadingStep)
	enum LoadingPhase {
		LOADING_IDLE,
		LOADING_COMMON,   // the CommonGame group
		LOADING_GROUPS,   // the groups main.LoadRequiredScripts named
		LOADING_LEX,      // the word list
		LOADING_DONE,
	};

	// one of the chapter's resources, from the table of tables StartLoadChapterResources was given (keys in any
	// case); 0x40 bytes
	struct ChapterResource {
		std::string mString;        // MSTRING: a resource group, or a PAM file
		std::string mStrParameter;  // MSTRPARAMETER: the animation's Lua callback
		int mType = 0;              // MTYPE: CHAPTER_RES_GROUP, else (2) a PAM file
		int mIntParameter = 0;      // MINTPARAMETER: the animation's id
	};
	enum { CHAPTER_RES_GROUP = 1 };

	std::vector<ChapterResource> mChapterResources;  // +0x788
	JeffLib::SoundPlayer* mSoundPlayer = nullptr;  // +0x7b4
	TitleScreen* mTitleScreen = nullptr;           // +0x7b8
	BookManager* mBookManager = nullptr;           // +0x7bc, the game screen (gBookManager)
	TreasureScreen* mTreasureScreen = nullptr;     // +0x7c0 (gTreasureScreen)
	MainMenu* mMainMenu = nullptr;                 // +0x7c4
	PopAnimManager* mPopAnimManager = nullptr;     // +0x7c8, the scripts' animations
	CutScene* mCutScene = nullptr;                 // +0x7d0
	std::string mCutSceneFile;                     // +0x7d4: the last PlayCutScene's
	std::string mCutSceneCallback;                 // +0x7f0
	std::string mCutSceneCaption;                  // +0x80c
	CurtainTransition* mCurtain = nullptr;         // +0x828
	Widget* mBookScreen = nullptr;                 // +0x82c: the tome's or the media player's (not ported yet)
	ButtonListener* mConfirmListener = nullptr;    // +0x838: a widget's ButtonListener part, in the original
	Widget* mChapterLoadScreen = nullptr;          // +0x83c: a chapter's loading screen (not ported yet)
	std::string mCurtainLuaCallback;               // +0x840: the scripts' function called when the curtain is done
	std::string mDialogCallback;                   // +0x878: the scripts' function told the answer to dialog 0 or 18
	Widget* mHallOfFame = nullptr;                 // +0x89c (not ported yet)
	Widget* mCredits = nullptr;                    // +0x8a0: the credits, which take the keys (not ported yet)
	void* mCurtainCallbackObject = nullptr;        // +0x894
	void (*mCurtainCallback)(void*) = nullptr;     // +0x898 (a member function in the original)
	int mTransitionState = TRANSITION_NONE;        // +0x8ec
	int mTotalBooks = 0;                           // +0x8f8, from main.LoadRequiredScripts
	int mRegTotalBooks = 0;                        // +0x8fc, the registry's TotalBooks
	int mAppTime = 0;                              // +0x900, the registry's AppTime, counting updates
	int mUserUpdates = 0;                          // +0x97c, updates since the player was selected
	bool mNukeProfile = false;                     // +0x910: the scripts' _NukeProfile once the book is up
	bool mChapterInitQueued = false;               // +0x911: the player's ChapterInit once the curtain is open
	bool mPlayIntro = false;                       // +0x912: the intro's: the new player's name once it is open
	bool mLoadingChapter = false;                  // +0x913: loading a chapter's resources (0x44e040)
	bool mTitleScreenDone = false;                 // +0x914
	bool mTitleScreenDoneQueued = false;           // +0x915
	bool mShowTilePips = true;                     // +0x916: CanShowTilePips' answer when the profile has no EnablePips
	bool mFullCurtain = false;                     // +0x917: a transition of the whole curtain (0x44d080)
	bool mLoadingDone = false;                     // +0x919
	bool mNoMusic = false;                         // +0x91a
	bool mTitleToMainMenu = false;                 // +0x91b
	bool mQueuedHydraLockout = false;              // +0x91d (not ported yet: the trial's)
	bool mTimedMode = false;                       // +0x96e: playing the timed mode (its TM_ profile keys)
	bool mMiniGameFromMenu = false;                // +0x96f: a mini-game started from the main menu's list (0x4819a0)
	bool mQuitting = false;                        // +0x91c: leaving the game (0x451c20; 0x451920 clears it)
	int mBattleUpdates = 0;                        // +0x974: updates of battle (not of a level-up or a chapter's end)
	int mMiniGameUpdates = 0;                      // +0x978: updates of the adventure's mini-games
	std::vector<std::string> mStartupGroups;       // +0x924 (0x38-byte entries; the group's name at +0x1c)
	Lex* mLex = nullptr;                           // +0x940
	ConversationPanel* mConversationPanel = nullptr;  // +0x944
	Board* mBoard = nullptr;                       // +0x948: the battle's or a mini-game's (gBoard)
	LuaInterface* mLuaInterface = nullptr;         // +0x94c
	Profile* mProfile = nullptr;                   // +0x950, the player's
	ArrowHintManager* mArrowHintManager = nullptr; // +0x964: the tutorial's arrows (made by LoadingThreadCompleted)
	bool mKillCutSceneWhenDone = false;            // +0x96d
	// port: the original's music interface was a JeffLib::SexyBassMusicInterface; this does what it added
	JeffLib::SongPlayer* mSongPlayer = nullptr;
	std::string mProfileErrors;                    // +0x8c8: CheckProfiles' report of the profiles it deleted

	GameApp();
	~GameApp() override;

	const char* LuaGetRegisteredName() override { return "GameApp"; }

	void Init() override;                   // 0x445a50
	void LoadingThreadProc() override;      // 0x4473e0
	void LoadingThreadCompleted() override; // 0x4481a0
	bool UpdateAppStep(bool* updated) override;
	void UpdateFrames() override;           // 0x44aa50
	void Shutdown() override;               // 0x445960
	// port: the test scripts' "word" (TestHooks.cpp)
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// 0x44f5c0: through the sound player, not again within 5 updates
	void PlaySample(int theSoundNum) override;
	// 0x44f600: the same at a pan
	void PlaySample(int theSoundNum, int thePan) override;

	// Dialogs. Every one is a GameDialog (NewDialog, 0x443f00), laid out at least 348 wide.
	Dialog* NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	                  const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;
	// 0x451ec0: two of them get a white header, wider margins and room for their buttons
	Dialog* DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	                 const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;
	// 0x444950: a yes or no question under the "are you sure" banner (dialog DIALOG_CONFIRM unless theId says
	// otherwise), centred. For DIALOG_CONFIRM, the scripts' function theCallback (when not empty) is called with
	// the answer, and theListener hears ButtonDepress(1018) for yes and (3018) for no.
	GameDialog* DoConfirmDialog(int theId, const SexyString& theHeader, const SexyString& theLines,
	                            ButtonListener* theListener, const std::string& theCallback);
	// 0x44b620: the battle's board over the book, its grid and engines started
	void StartBattle();
	// 0x4456a0: the game's menu (MenuDialog, dialog DIALOG_MENU) over the paused board. theFromTreasure: it is the
	// treasure screen's (its "Main Menu" button goes back with the transition 0x44ded0).
	void ShowGameMenu(bool theFromTreasure);
	// 0x4455d0: the options (OptionsDialog, dialog DIALOG_OPTIONS) over the paused board
	void ShowOptionsDialog();
	// 0x4457b0: the options are done: theApply takes the checkboxes' answers (screen mode, 3-D, cursors, tips);
	// the dialog goes, and the board goes on
	void CloseOptionsDialog(bool theApply);
	// 0x44f7e0: whether the tiles show their letter values (the board's and the profile's EnablePips)
	void SetShowTilePips(bool theShow);
	// 0x444b90: asks the new player's name (NewUserDialog, dialog DIALOG_NEW_USER) with a name no one has yet.
	// theButtonMode: BUTTONS_FOOTER for the intro's (only OK), BUTTONS_OK_CANCEL from the players' list.
	void ShowNewUserDialog(int theButtonMode);
	// 0x4450a0: the name is answered (OK or cancel): the profile, the conversation and the movie go on
	void NewUserDialogDone();
	// 0x4452d0: the old data of a player of that name goes (users/data/<name>.dat)
	void DeleteUserData(const std::string& theName);
	// 0x44ded0: from the treasure screen's menu back to the main menu (TRANSITION_TO_MENU - 1)
	void StartTreasureToMenuTransition();
	// 0x443f90: a message with an OK button (dialog 0), under a banner when there is one; OK calls the scripts'
	// function theCallback (when not empty) with true
	GameDialog* DoOkDialog(const SexyString& theHeader, const SexyString& theLines, const std::string& theCallback,
	                       Image* theBanner);
	// 0x44ad10: the dialogs' answers (2000 + id for yes, 3000 + id for no), and the buttons of the screens
	// that send theirs here
	void ButtonDepress(int theId) override;

	// port: SexyAppBase::StartLoadingThread (the title screen's call), loading in steps between the frames, so
	// that the title screen goes on while the resources load, natively and in the browser (no threads)
	void StartLoading();

	// The title screen's Lex has walked off (0x448d70)
	void TitleScreenDone();
	// 0x448f10: whether the player has a profile past the beginning (the title then goes to the main menu)
	bool HasPlayedGameBefore();
	// 0x447830: loads every users/*.bwa and deletes the ones that do not load (mProfileErrors says why). False
	// when it deleted one.
	bool CheckProfiles();
	// 0x4488c0: the player's profile (mProfile) from users/<name>.bwa; false (and no profile) when it does not load
	bool SelectUser(const std::string& theName);
	// 0x4494f0
	void ShowMainMenu(bool theFromTitle, bool theToBack);
	// 0x449160: a new player: a profile, the intro, then the first book
	void StartIntro(int theMode);

	// 0x44f540: whether there is a curtain that is closing or opening
	bool IsCurtainMoving() const;
	// 0x44d120: the curtain, made and put in front of everything when there is none
	CurtainTransition* GetCurtain();
	// 0x44d360: the curtain finished closing or opening
	void CurtainDone();
	// 0x44cfb0: closes the curtain (or opens it) at a priority (below 0: the curtain's own); theCallback, a
	// scripts' function, is called when it is done
	void DoCurtainTransition(bool theClose, const std::string& theCallback, int thePriority);
	// 0x44de70: from the main menu to a new game's book (TRANSITION_NEW_GAME)
	void StartNewGameTransition();
	// 0x44df60 (the scripts' DoCSToGameTransition): from the main menu or the chapter map to the game in
	// progress, gBookManager:StartGame once closed
	void StartSavedGameTransition();
	// 0x44df10 (DoCSToMMTransition): from the book back to the main menu
	void StartToMenuTransition();
	// 0x44dfb0 (DoGameToMMTransition): from a battle back to the main menu, the curtain over everything
	void StartGameToMenuTransition();
	// 0x44e000: the treasure screen's continue, gTreasureScreen:ContinueButtonPressed once closed
	void StartTreasureTransition();
	// 0x44d080: the whole curtain closes and opens again (a TRANSITION_CUTSCENE with mFullCurtain), theCallback
	// (a scripts' function) once it is open
	void DoFullCurtainTransition(const std::string& theCallback);
	// 0x44bb90: the battle (or a mini-game) ends and the chapter map comes back, told to the scripts' BookManager
	// ReturnToChapterSelect. Unless theFromMainMenu, the book then plays the end of the book (when that answered
	// 0) or its arrow hints.
	void ReturnToChapterSelect(bool theLexWon, bool theFromOptions, bool theNoMusic, bool theFromMainMenu);
	// 0x44e040 (the scripts' StartLoadChapterResources(theContinuing, theResources)): loads the chapter's resource
	// groups and animations (mChapterResources), then starts the battle unless theContinuing (a timed mode's
	// checkpoint). Pushes true when done, false when the app is shutting down.
	int StartLoadChapterResources(lua_State* L);
	// 0x44a920: the treasure screen and its buttons, hidden and disabled
	void HideTreasureScreen();

	// 0x44a0e0: the book and the treasure screen, made again; theHidden keeps the book hidden (behind the
	// treasure screen) and leaves its scripts unloaded
	void InitGameScreen(bool theHidden);
	void RemoveBookManager();  // 0x449340
	void HideBookManager();    // 0x44a830: the book and its buttons
	void RemoveMainMenu();     // 0x449300
	// 0x44cd00: the Flash movie theFile with theCaption, after the curtain; theCallback (a scripts' function)
	// once it has closed over the movie's end. theHide keeps it hidden until the curtain is closed.
	void PlayCutScene(const std::string& theFile, const std::string& theCallback, const std::string& theCaption,
	                  bool theKillWhenDone, bool theHide);
	void CloseCutScene();  // 0x44cf70
	// 0x448ff0: the main menu's adventure, when the profile says the scripts' intro cut scene is due
	void PlayMainMenuCutScene();
	// 0x44c2d0 (convpanel.New): a new conversation panel in place of the old one
	void NewConversationPanel();
	// 0x450460: a conversation panel is active
	bool ShowingConversationPanel() const;
	// 0x450d20: the curtain in front, and above an active conversation panel's priority
	void KeepCurtainOnTop();

	// ---- tooltips (TooltipWidget.cpp) ----
	std::vector<TooltipWidget*> mTooltips;        // +0x79c: every tooltip made by MakeTooltip (the app owns them)
	std::set<TooltipWidget*> mDisabledTooltips;   // +0x7ac: those EnableTooltips(false) disabled (to enable again)
	TooltipWidget* mHoverTooltip = nullptr;       // +0x7cc: the tooltip showing under the mouse
	TooltipWidget* MakeTooltip();                      // 0x44bf00, the scripts' MakeTooltip
	bool RemoveTooltip(TooltipWidget* theTooltip);     // 0x44c080, the scripts' RemoveTooltip
	void RemoveAllTooltips();                          // 0x44c1a0
	// 0x44c440: the tooltip under the mouse shows, the one before it hides
	void UpdateTooltipHover(int x, int y);
	void TurnOffTooltips();                            // 0x44c570, the scripts' TurnOffTooltips
	void EnableTooltips(bool theEnable);               // 0x44c590, the scripts' EnableTooltips

private:
	LoadingPhase mLoadingPhase = LOADING_IDLE;
	size_t mLoadingGroup = 0;
	bool mLoadingGroupStarted = false;

	// One step of the loading thread's work, until theDeadline (SDL ticks; 0: all of it). True when done.
	bool LoadingStep(unsigned int theDeadline);
	void LoadingFailed();
	// the scripts' function of dialog 0 or 18, told the answer (then forgotten)
	void CallDialogCallback(bool theAnswer);
	// CurtainDone's calls: the curtain's callback, and the scripts' function it was given; false when the app is
	// shutting down
	bool CallCurtainCallback();
	bool CallCurtainLuaCallback();
	void RemoveBoard();  // 0x4493f0
	// 0x44eae0 (the original's loading thread, with mLoadingChapter): mChapterResources loaded; false and
	// theError when one fails
	bool LoadChapterResources(std::string& theError);
	// ReturnToChapterSelect's: the groups of mChapterResources deleted, its animations too
	void FreeChapterResources();

	void RegisterGameAppMethods();  // the Lua class GameApp (part of Init)
};

extern GameApp* gApp;

}  // namespace Sexy
