#ifndef __PIZZAFRENZY_PIZZAFRENZY_H__
#define __PIZZAFRENZY_PIZZAFRENZY_H__

// The game's application (0x194 bytes, constructor 0x40c170, vtable 0x4f9a50; made by 0x40d0d0): the controller the
// engine's run loop starts. It keeps the settings, the scene, the sounds, the screens and the game's managers, and
// a state machine (+0x24) whose current state gets each frame's update and each screen's action.

#include "CityMap.h"
#include "CitySelectScreen.h"
#include "GameMode.h"
#include "GameScreen.h"
#include "GameStats.h"
#include "GenericApplication.h"
#include "GameMusicPlayer.h"
#include "GenericScreen.h"
#include "GenericUserSelectScreen.h"
#include "HighScoreScreen.h"
#include "Hud.h"
#include "ImageNode.h"
#include "Level.h"
#include "LevelStartScreen.h"
#include "MainMenu.h"
#include "NewToppingScreen.h"
#include "OptionScreen.h"
#include "Pizza.h"
#include "ScreenMgr.h"
#include "Settings.h"
#include "SoundMgr.h"
#include "SplatFactory.h"
#include "State.h"
#include "StoryScreen.h"
#include "TextItem.h"
#include "TileManifest.h"
#include "ToppingBookScreen.h"
#include "UserGameData.h"

#include <map>
#include <string>
#include <vector>

namespace engine
{
class Scene;
}

namespace pizzafrenzy
{

class PizzaFrenzy : public engine::GenericApplication
{
public:
	// The game's modes, by the number the application keeps in mGameMode
	enum
	{
		MODE_SPEED = 0,
		MODE_MEMORY = 1,
		MODE_CONCENTRATION = 2
	};

	PizzaFrenzy();             // 0x40c170
	~PizzaFrenzy() override;   // 0x40c6e0, 0x408b10

	static PizzaFrenzy* instance() { return sInstance; }  // 0x532900

	void configure(engine::AppConfig& theConfig) override;  // 0x04: 0x4050d0
	void init() override;                                   // 0x08: 0x409220
	void shutdown() override;                               // 0x14: 0x401df0
	void setMusicEnabled(bool theEnabled) override;         // 0x1c: 0x402340

	// The game's own slots
	virtual void update(engine::UpdateInfo& theInfo) override;  // 0x20: 0x401d90, from the application's update signal
	virtual void screenAction(std::string theAction);           // 0x24: 0x401030, from a screen's action signal
	virtual void windowCommand();                               // 0x28: 0x4059d0, SC_MINIMIZE and SC_MAXIMIZE
	virtual void windowActivate(bool theActive);                // 0x2c: 0x405a90, from the activate signal
	virtual void setFullScreen(bool theFullScreen);             // 0x30: 0x401090
	virtual void timerTick(int theMs);                          // 0x34: 0x4010b0, from the timer signal

	// 0x401570, from a screen's action signal (the loading state connects the screens it makes)
	void onScreenAction(std::string theAction) { screenAction(theAction); }

	engine::Ref<engine::Scene> mScene;                 // +0x20
	engine::StateMgr mStates;                          // +0x24
	engine::Ref<engine::SoundMgr> mSounds;             // +0x3c
	engine::Ref<engine::ScreenMgr> mScreens;           // +0x40
	engine::Ref<Settings> mSettings;                   // +0x44
	int mCurrentLevel;                                 // +0x48: the level of the current city the game is on
	int mCurrentCity;                                  // +0x4c: the city the game is in
	int mDay;                                          // +0x54: the day of the game, from 1, over all the cities
	int mGameMode;                                     // +0x58: 0 speed, 1 memory, 2 concentration
	engine::Ref<engine::GenericScreen> mSplashScreen;  // +0x5c
	engine::Ref<engine::ImageNode> mLoadBar;           // +0x60: the splash screen's load bar, shown as far as loaded
	int mLoadBarWidth;                                 // +0x64
	int mLoadBarHeight;                                // +0x68
	engine::Ref<engine::GenericUserSelectScreen> mUserSelect;  // +0x6c
	engine::Ref<engine::GenericScreen> mNewUserEntry;        // +0x70: the node "nameEntry" takes the name
	engine::Ref<engine::GenericScreen> mDeleteUserConfirm;   // +0x74
	engine::Ref<engine::TextItem> mDeleteUserText;           // +0x78: its question
	std::string mDeleteUserTemplate;                         // +0x7c: the question as the layout has it
	engine::Ref<MainMenu> mMainMenu;                         // +0x98
	engine::Ref<OptionScreen> mOptions;                      // +0x9c
	engine::Ref<HighScoreScreen> mHighScores;                // +0xa0
	engine::Ref<engine::GenericScreen> mHighScoreResetConfirm;  // +0xa4
	engine::Ref<engine::GenericScreen> mRestoreGameConfirm;  // +0xa8
	engine::Ref<engine::GenericScreen> mAppQuitConfirm;      // +0xac
	engine::Ref<engine::GenericScreen> mGameExitConfirm;     // +0xb0
	engine::Ref<engine::GenericScreen> mUnlockNotice;        // +0xb4: the designer's, shown while it is locked
	std::string mUnlockNoticeTemplate;                       // +0xb8: the node "notifyText"'s text as the layout has it
	engine::Ref<engine::GenericScreen> mPauseMenu;           // +0xd4
	engine::Ref<StoryScreen> mStoryScreen;                   // +0xdc: the level's story, and its end
	engine::Ref<ToppingBookScreen> mToppingBook;             // +0xe0: opened from the menu and the pause menu
	engine::Ref<LevelStartScreen> mLevelStartScreen;         // +0xe4: the vans' toppings
	engine::Ref<NewToppingScreen> mNewToppingScreen;         // +0xec: the toppings the player has not been shown
	engine::Ref<CitySelectScreen> mCitySelect;               // +0xf0
	engine::Ref<GameScreen> mDecorateScreen;                 // +0xf4: the bonus round's screen (decoratePizza.xml)
	engine::Ref<GameScreen> mGameScreen;                     // +0x114
	engine::Ref<Hud> mHud;                                   // +0x118
	engine::Ref<CityMap> mCityMap;                           // +0x11c: the city on the game screen
	engine::Ref<GameStats> mGameStats;                       // +0x120: what the game being played has counted
	engine::Ref<TileManifest> mTileManifest;                 // +0x124: the game's data
	engine::Ref<UserGameData> mUserGameData;                 // +0x128: the current player's
	engine::Ref<engine::SplatFactory> mSplatFactory;         // +0x12c: the scene's, loaded from the splats' file
	engine::Ref<GameMode> mMode;                             // +0x110: the game of the mode that is being played
	engine::Ref<GameMusicPlayer> mMusicPlayer;               // +0x130
	// +0x137: set by the game's states (0x402730 and 0x4027d0), cleared by 0x402790, which the translated states do not
	// reach: a game is on the screen. The water of the roads is only drawn while it is set.
	bool mGameScreenActive = false;
	bool mBonusRound;                                        // +0x136: the level being played is a bonus round
	std::vector<engine::Ref<City>> mCities;                  // +0x138: the level manifest's cities, in order
	std::vector<engine::Ref<Pizza>> mPizzas;                 // +0x148: the pizza manifest's pizzas, in its order
	std::map<int, engine::Ref<Pizza>> mPizzaById;            // +0x158: the pizza manifest's pizzas by their numbers
	std::vector<engine::Ref<Pizza>> mDesignedPizzas;         // +0x164: the pizzas the player designed
	std::map<int, engine::Ref<Topping>> mPizzaToppings;      // +0x174: the topping each pizza of the player's is
	int mPizzaMaxId = 0;                                     // +0x180: the number the next pizza takes
	bool mShowHighScores;                                    // +0x134: the menu shows the high scores when it starts
	bool mShowCitySelect;                                    // +0x135: the menu shows the city select when it starts

	// The debug flag at 0x532908 (set to false whenever the menu starts with a player): the pizza designer is open
	// without the toppings it needs
	static bool sDesignerUnlockOverride;

	// 0x401010 (slot 0x44): the game ends
	void quitApplication();
	// 0x402370 (slot 0x40): the game is saved in the mode's saved game
	void saveGame();
	// 0x409eb0 (slot 0x38): the level manifest's cities are added in order
	void addCity(City* theCity) { mCities.push_back(theCity); }
	// 0x409990: the pizza of the manifest's number (an empty one, which stays, for a number the manifest has not given)
	Pizza* getPizza(int theId) { return mPizzaById[theId]; }
	// 0x4099b0: a pizza of the manifest is kept, in the list and by its number
	void addPizza(Pizza* thePizza);
	// 0x40c700: a game of the mode starts in the city, from the saved one when the mode has one
	void startGame(int theMode, int theCity);
	// 0x4023d0: the city select is shown, after its slot 0x180
	void showCitySelect();
	// 0x403bc0: the game goes on to the next level, and to the next city after the last level of one
	void advanceLevel();
	// 0x409f30: the level starts: its story first when the player has not seen it, then the level itself
	void startLevel();
	// 0x40dab0, and the state 0x402d70 after it; translated next
	void startPizzaDesigner();
	// The +0xfc object's slot 0xc, and the state 0x402e00 after it; translated next
	void startPizzaEditor();

	// 0x408090: the toppings the level gives are the player's, which the player is shown when they are new
	void giveToppings();
	// 0x407fc0: the level's start is shown, and a bonus round goes on at once
	void showLevelStart();
	// 0x4022f0: the mode's game is let go with the level it played and the city on the game screen
	void stopGame();
	// 0x407080: the game is paused, and the pause menu is shown; the state that paused is the one that comes back
	void pauseGame(engine::State* theNextState);
	// 0x407370: the level is over: the level's end is shown, and the next level or the game's end follows
	void endLevel();
	// +0x188: the window has the focus
	bool isActive() const { return mActive; }

	// 0x403bb0: the city that is played
	City* getCurrentCity() { return mCities[mCurrentCity]; }

	// 0x406dc0: the level is set up for play: the mode's game with the level's city, or the bonus round
	void setUpLevel();

	// 0x405270: the level's city file is loaded into the game screen's map and the screen draws it; the city select
	// draws the postcards from it
	void loadCityFile(const std::string& theFile, GameScreen* theScreen);

private:
	// 0x403b60: a game starts at the first level of the city, on the day after the days the cities before it have
	void beginCity(int theCity);
	// 0x404660: the checked subscript of the level lists, whose error is an out_of_range exception
	Level* getCurrentLevel();
	// What the game itself does, translated next with it: each makes what the original's function makes and puts the
	// game's state in place. Their callers here are startLevel (story), giveToppings (new topping), showLevelStart
	// (level start) and setUpLevel (the others).
	// 0x402ba0: the story screen (+0xdc, slot 0x17c) reads the story's file, and the story state starts
	void enterStoryState(Level* theLevel, const std::string& theStory);
	// 0x402ce0: the new topping screen (+0xec, slot 0x184), and its state
	void enterNewToppingState();
	// 0x402c40: the level start screen (+0xe4, slot 0x180) shows the level, and its state starts
	void enterLevelStartState(Level* theLevel);
	// 0x459510, 0x460610 and 0x45b3e0 (the speed, memory and concentration mode's game, which the mode number picks),
	// kept at +0x110; its slot 0x4 (the game screen, the hud and the stats) and slot 0x10 (the level), then the state
	// 0x4029b0 (GameState.h). 
	void enterModeGame(Level* theLevel);
	// 0x45f830 (BonusGame.h), kept at +0x110: its slot 0xbc (the decorating screen at +0xf4 and the stats) and slot
	// 0x10 (the level), then the state 0x402a40 (BonusStates.h)
	void enterBonusRound(Level* theLevel);

	void onAppUpdate(engine::UpdateInfo& theInfo) { update(theInfo); }  // 0x401560
	void onWindowCommand() { windowCommand(); }                          // 0x401580
	void onActivate(bool theActive) { windowActivate(theActive); }       // 0x401590
	void onTimer(int theMs) { timerTick(theMs); }                        // 0x4015a0

	static PizzaFrenzy* sInstance;

	std::string mTitle;  // the configuration's title (the string table's 0xcb)
	bool mActive;        // +0x188: the window has the focus
};

} // namespace pizzafrenzy

#endif
