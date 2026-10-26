// Sexy::GameApp (constructor 0x443350, vtable 0x7adc44; its LuaLinkedClass part at +0x730): Bookworm Adventures'
// app. The game itself is mostly the scripts (main.lua and what it requires); the app loads them and the
// resources, owns the screens written in C++ (title, main menu, curtain, conversation panel, ...) and gives
// the scripts 73 methods (RegisterGameAppMethods, GameAppLua.cpp).
#pragma once

#include <string>
#include <vector>

#include "LuaApp.h"

namespace JeffLib {
class SongPlayer;
class SoundPlayer;
}

namespace Sexy {

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
class TreasureScreen;
class Widget;

class GameApp : public LuaApp {
public:
	// The game's dialog ids (those that are ported)
	enum {
		DIALOG_OK = 0,           // DoOkDialog's
		DIALOG_REALLY_QUIT = 2,  // the main menu's quit
		DIALOG_MENU = 7,         // the game's menu (not ported yet): the conversation panel waits under it
		DIALOG_CONFIRM = 18,     // DoConfirmDialog's
		DIALOG_SKIP_MOVIE = 20,  // a click on a cut scene: skip it?
	};

	// mTransitionState: what the curtain is doing (CurtainDone goes on from it)
	enum {
		TRANSITION_NONE = 0,
		TRANSITION_CUTSCENE = 1,       // closing over a screen to show a cut scene, then opening on it
		TRANSITION_OPENING = 2,        // closing (mCurtainLuaCallback when closed), then opening over a new screen
		TRANSITION_SAVED_GAME = 3,     // from the main menu to the book of a game in progress
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
	std::vector<std::string> mStartupGroups;       // +0x924 (0x38-byte entries; the group's name at +0x1c)
	Lex* mLex = nullptr;                           // +0x940
	ConversationPanel* mConversationPanel = nullptr;  // +0x944
	Widget* mBoard = nullptr;                      // +0x948: the battle's (not ported yet)
	LuaInterface* mLuaInterface = nullptr;         // +0x94c
	Profile* mProfile = nullptr;                   // +0x950, the player's
	Widget* mArrowHintManager = nullptr;           // +0x964: the tutorial's arrows (not ported yet)
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
	// 0x44df60: from the main menu to the book of the game in progress, then gBookManager:StartGame
	void StartSavedGameTransition();
	// 0x44df10: from the book back to the main menu
	void StartToMenuTransition();
	// 0x44e000: the treasure screen's continue, gTreasureScreen:ContinueButtonPressed once closed
	void StartTreasureTransition();

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
	// 0x44c440: the tooltip under the mouse (tooltips are not ported yet)
	void UpdateTooltipHover(int x, int y) {}

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

	void RegisterGameAppMethods();  // the Lua class GameApp (part of Init)
};

extern GameApp* gApp;

}  // namespace Sexy
