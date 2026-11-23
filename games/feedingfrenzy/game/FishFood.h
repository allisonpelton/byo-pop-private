#ifndef __FEEDINGFRENZY_FISHFOOD_H__
#define __FEEDINGFRENZY_FISHFOOD_H__

// The game's controller, FishFood (0x260 bytes, constructor 0x430ee0, vtable 0x4a6640). It is the application's
// controller (engine::App's +0x188): the run loop asks it for the window's configuration (configure, 0x42b8e0), then
// has it set the game up (init, 0x430b80). The controller keeps a state (+0x54) the application's update signal drives
// each frame (0x42fa80), and receives the screens' actions (0x42f190). It owns the screens, the score lists, the sound
// bank and the stages: the menu's stage behind the main menu, and the stage being played. The original's other two ways
// in, the screen saver's run and its settings dialog (the controller's slots 0x0c and 0x10, and the saver's stage
// list), are left out of the port, and so is the partner's message slot (0x20: 0x42eef0).

#include "Font.h"
#include "App.h"
#include "FadeContainer.h"
#include "FoodBank.h"
#include "FunFacts.h"
#include "GameMapLayout.h"
#include "GameSession.h"
#include "GameStrings.h"
#include "HighScoreEntryLayout.h"
#include "HighScoreLayout.h"
#include "MainMenuScreen.h"
#include "ScoreList.h"
#include "ScreenFader.h"
#include "ScreenLayout.h"
#include "Scene.h"
#include "Settings.h"
#include "Signal.h"
#include "Sound.h"
#include "SoundBank.h"
#include "StageInfo.h"

#include <memory>
#include <string>
#include <vector>

namespace feedingfrenzy
{

// The window and the screen it shows (the controller's configure, 0x42b8e0, says them)
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int FRAME_RATE = 30;
const int WINDOW_ICON = 5;

class StageMgr;

class FishFood : public engine::Controller, public engine::ScreenListener, public engine::HasSlots
{
public:
	FishFood();            // 0x430ee0
	~FishFood() override;  // 0x431310

	static FishFood* instance() { return sInstance; }  // the global 0x4c0880

	void configure(engine::AppConfig& theConfig) override;  // 0x04: 0x42b8e0
	void init() override;                                   // 0x08: 0x430b80
	void shutdown() override;                               // 0x14: 0x42c9d0

	// 0x42f190: a screen's action
	void screenAction(std::string theAction) override;

	// 0x42c0c0: the stage the game is in: the menu's in the loading and menu states, else the game's
	StageMgr* getStageMgr();
	// 0x42d230: a sound of the sound bank plays
	void playSound(const std::string& theName, float theVolume, float thePitch);

	// A popup screen to show at the next update (0x42ba60), with the time it fades in, and one hidden, fading out over
	// the time when there is one (0x42baf0)
	void showScreen(engine::ScreenLayout* theScreen, float theFadeSeconds);
	void hideScreen(engine::ScreenLayout* theScreen, float theFadeSeconds);

	// The logo's slide between its hidden and shown positions
	void slideLogoIn();   // 0x42bc40
	void slideLogoOut();  // 0x42bc90

	// The stage joins the screens (the root group)
	void addStage(StageMgr* theStage);
	// 0x42bf60 and 0x42bfe0: the game stage stops, with the pause screen, and plays on again
	void pauseGame();
	void pauseResume();
	// 0x42cf90: the stage's end: the foodbank is kept, and the state is the stage's end
	void finishStage();
	// 0x42dc90: the game goes to a stage of the manifest (a number from 0)
	void goToStage(int theIndex);
	// 0x42ee90: a new game in a mode (a GameMode), at a stage of the manifest
	void startGame(int theMode, int theStage);

	// The keys the application gives to the handlers added to it (0x444c00 and 0x443640): the last added first, until
	// one takes the key
	void addKeyHandler(engine::KeyHandler* theHandler);
	void removeKeyHandler(engine::KeyHandler* theHandler);

	GameSession* getSession() { return &mSession; }  // 0x42beb0 (+0x58)
	GameStrings& getStrings() { return mStrings; }  // 0x42c730's table (+0x30)
	Settings& getSettings() { return mSettings; }   // 0x42bb40 (+0xd4)
	engine::SoundBank& getSoundBank() { return mSoundBank; }  // 0x42bc30 (+0x1cc)
	FoodBank& getFoodBank() { return mFoodBank; }   // 0x42be50 (+0x1a8)
	FunFacts* getFunFacts() { return mFunFacts.get(); }  // 0x42be00 (+0x208)
	// 0x42bdd0: the list of the normal or the timed game
	ScoreList& getScoreList(int theMode) { return theMode == GAME_MODE_TIMED ? mTimedScores : mNormalScores; }
	int getStageIndex() const { return mStageIndex; }  // 0x42be10 (+0x1e4)
	int getStageCount() const { return (int)mManifest.mStages.size(); }  // 0x42c0e0
	// 0x42c9b0: a stage of the manifest (a number from 0)
	StageInfo* getStageInfo(int theIndex) { return mManifest.mStages[theIndex].get(); }
	// 0x42c970: a change of the fish the stages bring (a number from 0); none when there is no such change
	FishChange* getFishChange(int theIndex)
	{
		if (theIndex < 0 || (int)mManifest.mFishChanges.size() <= theIndex)
			return nullptr;
		return &mManifest.mFishChanges[theIndex];
	}
	// 0x42be20: how far the game stage's load has got (0 to 1)
	float getLoadProgress();
	const std::string& getVersion() const { return mManifest.mVersion; }  // 0x42d2b0

	// port (test commands): the part of the game the controller is in, and the popup screen that shows over it, each as
	// a word ("" when there is no popup)
	const char* getStateName() const;
	const char* getPopupName() const;
	// port (test commands): the screen whose buttons a click reaches: the popup that shows, or the main menu
	engine::ScreenLayout* getClickScreen();
	// port (test commands): the furthest stage of both lists is the last of the manifest
	void unlockAllStages();
	// port (test commands): the name typed on the high score entry screen
	std::string getEnteredName();

private:
	// The controller's state (+0x54), the part of the game it is in
	enum State
	{
		FOOD_STATE_LOADING = 0,        // the splash screen up to the main menu
		FOOD_STATE_MENU = 1,           // the main menu and its screens
		FOOD_STATE_LOAD_MAP = 2,       // the game's stage loads behind the loading map
		FOOD_STATE_PLAYING = 4,        // the game's stage is played
		FOOD_STATE_STAGE_ENDED = 6,    // the stage is over: the game is won, or a name is asked for, or the menu returns
		FOOD_STATE_GAME_WON = 7,       // the screen of the won game shows
		FOOD_STATE_ENTERING_SCORE = 8, // a name is typed for a high score
		FOOD_STATE_NEXT_STAGE = 9      // the next stage replaces the stage that is played
	};

	void startup();                              // 0x430180
	void showSplash();                           // 0x42cc70
	void onAppUpdate(engine::UpdateInfo& theInfo);  // 0x42fa80
	void onAppActivate(bool theActive);          // 0x42c230
	void onKeyDown(int theKey);
	void onKeyUp(int theKey);
	void updateLoading(engine::UpdateInfo& theInfo);  // 0x42ec30
	void updateLoadMap(engine::UpdateInfo& theInfo);  // 0x42c760
	void updateStageEnded();                     // 0x42fa80, state 6
	void updateNextStage();                      // 0x42fa80, state 9
	void enterMenu();                            // 0x42c3a0 (case 1)
	void startStage();                           // 0x42c3a0 (case 5)

	void createScreens();                        // 0x42e420
	void createMainMenu();                       // 0x42d980
	void loadGlobalSounds();                     // 0x42d190
	void fillScoreLists();                       // 0x42d0c0
	void rebuildRoot();                          // 0x42b980: the menu's stage and screen, the music
	void showLoadMap();                          // 0x42ba00
	void removeGameStage();                      // 0x42fa80 (states 6 and 9): the game's stage is let go
	// 0x42fa80 (state 6) and 0x42f190 (gameWonDone): the screen for a name is shown when the game's score is on the
	// list (its rank under the limit); false when it is not
	bool showHighScoreEntry(int theRankLimit);

	// The popup screens: one waiting to show (+0x7c, with the fade-in time at +0x84)
	void updateScreens();                                                    // 0x42ba80

	// The logo's slide between its hidden and shown positions (+0xb4 with its state at +0xb8)
	void updateLogo(engine::UpdateInfo& theInfo);        // 0x42bcd0

	void applySettings();  // 0x42bec0: the sound and music volumes, the full screen
	void updateCursorLock();  // 0x42c060: the pointer is held and hidden while the game's stage is played

	static FishFood* sInstance;

	engine::Ref<StageMgr> mGameStage;        // +0x20: the stage being played
	engine::Ref<StageMgr> mMenuStage;        // +0x24: the stage behind the menu
	StageInfo mMenuStageInfo;                // +0x28: the menu stage's entry (made when the stage loads)
	int mState;                              // +0x54
	float mStateSeconds;                     // +0x5c: the seconds left of the state's wait
	int mStep;                               // +0x60: the step inside the state
	engine::Ref<engine::FadeContainer> mSplash;   // +0x64
	float mMapWait;                          // +0x74: the seconds the loading map shows before the stage loads
	engine::Ref<engine::Scene> mScene;       // +0x8c
	engine::Ref<engine::Container> mRoot;    // +0x90: "RootGroup", the screens and the stages
	engine::Ref<engine::Container> mPopups;  // +0x94: "PopupGroup", the screens over them
	engine::Ref<ScreenFader> mFader;         // +0x88
	engine::Ref<engine::Container> mLogoGroup;  // +0xb4: the logo and its "TM"
	engine::Ref<engine::ScreenLayout> mShowingScreen;  // +0x7c
	engine::ScreenLayout* mPopup;            // port (test commands): the popup screen that shows
	float mShowFadeSeconds;                  // +0x84
	int mLogoState;                          // +0xb8: 0 hidden, 1 sliding in, 2 shown, 3 sliding out
	float mLogoSecondsLeft;                  // +0xbc
	float mLogoSlideSeconds;                 // +0xc0
	engine::Point mLogoHiddenPosition;       // +0xc4
	engine::Point mLogoShownPosition;        // +0xcc

	// The screens (each is made once, by createScreens)
	engine::Ref<MainMenuScreen> mMainMenu;                 // +0x70
	engine::Ref<GameMapLayout> mMapScreen;                 // +0xb0
	engine::Ref<GameMapLayout> mLoadMapScreen;             // +0x234
	engine::Ref<engine::ScreenLayout> mOptionsScreen;      // +0x98
	engine::Ref<HighScoreLayout> mHighScoreScreen;         // +0xa4
	engine::Ref<HighScoreEntryLayout> mHighScoreEntryScreen;  // +0xa8
	engine::Ref<engine::ScreenLayout> mGameWonScreen;      // +0xac
	engine::Ref<engine::ScreenLayout> mPauseScreen;        // +0x9c
	engine::Ref<engine::ScreenLayout> mCreditsScreen;      // +0x1c4
	std::unique_ptr<FunFacts> mFunFacts;                   // +0x208
	engine::Sound* mMenuMusic;                             // +0x1ac

	GameSession mSession;                    // +0x58: the game being played
	Settings mSettings;                      // +0xd4
	GameStrings mStrings;                    // +0x30
	StageManifest mManifest;                 // +0x1ec, +0x1fc, +0x214
	int mStageIndex;                         // +0x1e4: the stage of the manifest the game is at (from 0)
	bool mCursorLocked;                      // +0x1d0: the pointer is held and hidden
	int mGameMode;                           // +0x20c: how the game is played (a GameMode)
	ScoreList mNormalScores;                 // +0x140
	ScoreList mTimedScores;                  // +0x174
	FoodBank mFoodBank;                      // +0x1a8
	bool mScoresLoaded;                      // +0x230: the lists and the foodbank are read
	engine::SoundBank mSoundBank;            // +0x1cc
	std::vector<engine::KeyHandler*> mKeyHandlers;  // the application's key handlers (+0x108)
};

} // namespace feedingfrenzy

#endif
