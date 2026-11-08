#ifndef __WORDHARMONY_GAME_H__
#define __WORDHARMONY_GAME_H__

// The game's controller (0x71c bytes, constructor 0x406980, vtable 0x480a14, its screen listener at +0x0c with vtable
// 0x480a10; the global at 0x4995b8): the screens, the levels, the scores, the sounds and the music, and the game's
// state machine (0x401000-0x409c00).
//
// The states (+0x1d8; a change asked for during a frame waits in +0x1dc, 9 for none):
//   0  the splash screen; the data and the screens are made while it shows
//   1  the main menu and its screens
//   2  a level's introduction on the load screen (the puzzles wait for a click)
//   3  (not used: setState(3) plays, as 5 does)
//   4  paused
//   5  playing
//   6  the game is over: the high score entry or the main menu
//   7  a level done: the bonus is counted up on the load screen, then the next level
//   8  the screen saver

#include "App.h"
#include "Dictionary.h"
#include "HighScores.h"
#include "Level.h"
#include "ScreenLayout.h"
#include "Settings.h"
#include <string>
#include <vector>

namespace engine
{
class FadeContainer;
class MusicSystem;
class Selector;
class SoundBank;
class TextEffects;
class TextItem;
}

class CongratsScreen;
class GameOverScreen;
class HighScoreEntryLayout;
class HighScoreScreen;
class LevelsSelectionScreen;
class SavedGame;
class ScreenSaver;
class WordBallSystem;

class Game : public engine::Controller, public engine::ScreenListener
{
public:
	Game();            // 0x406980
	~Game() override;  // 0x4073f0

	static Game* instance() { return sInstance; }  // 0x4995b8
#ifdef __EMSCRIPTEN__
	// the web page's volume setting (src/SiteProfiles.cpp) sets the volumes again through the game's own applyVolumes
	void siteApplyVolumes() { applyVolumes(); }
#endif

	// The controller's slots
	void configure(engine::AppConfig& theConfig) override;  // 0x04: 0x402eb0
	void init() override;                                   // 0x08: 0x406b30
	void shutdown() override;                               // 0x0c: 0x401750
	void update(engine::UpdateInfo& theInfo) override;      // 0x10: 0x409060
	void activate(bool theActive) override;                 // 0x14: 0x4070a0, the window's focus

	// The screen listener (0x407580): every screen's buttons
	void screenAction(std::string theAction) override;

	// 0x401240: the program's end
	void endProgram();
	// 0x401260: points to the score, and the score to the HUD
	void addScore(int thePoints);
	// 0x4012b0: a bonus text's points shown (the floating texts' callback)
	static void pointsShown(int thePoints);
	// 0x401290: a found word's floating text is done: mLettersLeft less its letters (theLetters), at least 0
	static void scoreShown(int theLetters);
	// 0x401490: the mouse over a main menu mode button (MODE_ values): that mode's description is the one shown
	void showModeInfo(int theMode);
	// 0x4014d0: the mouse off it: the description fades out
	void hideModeInfo(int theMode);
	// 0x401580
	bool isPaused();
	// 0x402b10: plays a sound of the sound bank by its name
	void playSound(std::string theName);
	// 0x402980: the letters in a random order
	static std::string shuffle(std::string theLetters);
	// 0x404220
	void resume();
	// 0x404e10: the game is over (theQuit: the player left it; theSave: with the game saved for later)
	void gameOver(bool theQuit, bool theSave);
	// 0x404f60: a random hint word of theMin to theMax letters
	std::string makeLetters(unsigned int theMin, unsigned int theMax);
	// 0x4050f0: theCount random letters
	std::string randomLetters(int theCount);
	// 0x4055f0: the next state of the state machine (+0x1d8)
	void setState(int theState);
	// 0x406430: the retry button: the level again, its points and hints given back (less a hint)
	void retry();
	// 0x406520
	void pause(bool theShowMenu);
	// 0x407280: the quit button
	void quit();
	// 0x4070d0: the help of a mode (theInGame: from the game, with its return button)
	void helpKey(bool theInGame, int theMode);
	// 0x4088f0: a found word into the list of the longest (+0x58)
	void noteWord(std::string theWord);
	// 0x408640: solitaire's words for a level of theCount balls
	void makeWords(int theCount, std::vector<std::string>& theWords);

	Dictionary mDictionary;                 // +0x10: config\hintList.txt
	WordTree mWords;                        // +0x14: config\matchList.txt
	std::vector<std::string> mHintWords;    // +0x1c: config\hintList.txt
	int mScore;                             // +0x2c
	int mTime;                              // +0x30: the seconds played, at the last level's end
	int mLevelPoints;                       // +0x38: the level's points (the retry takes them back)
	int mLettersLeft;                       // +0x3c: the HUD's balls remaining, the level's mBallCriteria less
	                                        // mLettersUsed (and lowered as the found words' floating texts end)
	int mLettersUsed;                       // +0x40: the found words' lengths, and solitaire's wildcards
	int mBonus;                             // +0x44: the level end's bonus, counted down
	unsigned int mWildcards;                // +0x48: wildcards to add to the next level's letters
	int mHints;                             // +0x4c: hints left
	int mHintsUsed;                         // +0x50: in the game
	int mLevelHintsUsed;                    // +0x54: in the level
	std::vector<std::string> mLongestWords; // +0x58: the longest words found (0x4088f0), five at most
	float mCountUp;                         // +0x68: the bonus count's time
	bool mContinue;                         // +0x6c: the load screen's click (or no wait)
	engine::TextEffects* mTextEffects;      // +0x70: config\splats.xml
	Settings mSettings;                     // +0x74
	engine::ScreenLayout* mEditorHud;       // +0x104: config\EditorHUD.xml (the editor's, never shown)
	bool mGridSnap;                         // +0x108: the editor's grid
	// +0x10c: the beta's play time upload (InternetOpen; init gives it its URL, nothing sends): left out
	float mLevelTime;                       // +0x18c: the level's seconds
	engine::Node* mNextRoot;                // +0x190: to be the root window's only child at the next update
	engine::MusicSystem* mPuzzleMusic;      // +0x194: config\music.xml (puzzle and solitaire)
	engine::MusicSystem* mActionMusic;      // +0x198: config\drums.xml
	engine::MusicSystem* mGameOverMusic;    // +0x19c
	engine::MusicSystem* mMenuMusic;        // +0x1a0
	engine::MusicSystem* mMusic;            // +0x1a4: the one playing
	std::vector<Level*> mLevels;            // +0x1a8: config\levels.xml (puzzle)
	std::vector<Level*> mActionLevels;      // +0x1b8: config\freematch-levels.xml (templates)
	std::vector<Level*> mSolitaireLevels;   // +0x1c8: config\solitaire-levels.xml (templates)
	int mState;                             // +0x1d8
	int mNextState;                         // +0x1dc: 9 for none
	float mTimer;                           // +0x1e0
	float mSplashTime;                      // +0x1e4
	engine::Selector* mSplash;              // +0x1e8
	engine::ScreenLayout* mMainMenu;        // +0x1ec
	engine::ScreenLayout* mOptions;         // +0x1f4
	HighScoreEntryLayout* mHighScoreEntry;  // +0x1f8
	int mSaverMode;                         // +0x1fc: the command line's: 0 the game, 1 /s, 2 /c, 3 /p
	ScreenSaver* mScreenSaver;              // +0x200
	engine::FadeContainer* mShowNext;       // +0x204: added to the root window and faded in at the next update
	engine::Node* mRemoveNext;              // +0x208: removed from the root window at the next update
	HighScoreScreen* mHighScoreScreen;      // +0x20c
	engine::ScreenLayout* mLevelEnd;        // +0x210
	engine::ScreenLayout* mPausedScreen;    // +0x214
	GameOverScreen* mGameOverScreen;        // +0x218
	CongratsScreen* mCongratsScreen;        // +0x21c
	LevelsSelectionScreen* mLevelsSelection;      // +0x220: the puzzles' (10 buttons of 10)
	LevelsSelectionScreen* mDifficultySelection;  // +0x224: freematch's (3 of 4)
	LevelsSelectionScreen* mSolitaireSelection;   // +0x228: solitaire's (3 of 7)
	engine::ScreenLayout* mPuzzleHelp;      // +0x22c
	engine::ScreenLayout* mActionHelp;      // +0x230
	engine::ScreenLayout* mSolitaireHelp;   // +0x234
	engine::ScreenLayout* mPuzzleBrief;     // +0x238: the main menu's mode descriptions
	engine::ScreenLayout* mActionBrief;     // +0x23c
	engine::ScreenLayout* mSolitaireBrief;  // +0x240
	engine::ScreenLayout* mNewGameScreen;   // +0x244: continue the saved game or start again
	engine::ScreenLayout* mQuitGameScreen;  // +0x248
	HighScores mPuzzleScores;               // +0x24c
	HighScores mSolitaireScores;            // +0x27c
	HighScores mActionScores;               // +0x2ac
	engine::ScreenLayout* mLoadScreen;      // +0x2dc
	engine::TextItem* mLoadTexts[5];        // +0x2e0: text1 to text5
	engine::Node* mContinueText;            // +0x2f4
	engine::Node* mContinueButton;          // +0x2f8
	engine::TextItem* mScoreText;           // +0x2fc
	// +0x300: a frame rate display (0x42a240, "FPS:" and "PPS:"), made and never shown: left out
	bool mQuitting;                         // +0x304
	int mLevel;                             // +0x308: the level played
	Level* mCurrentLevel;                   // +0x30c
	Level mBaseLevel;                       // +0x310: not used
	SavedGame* mSavedGame;                  // +0x6f8: the game continued
	int mTemplate;                          // +0x6fc: the freematch or solitaire level's template
	WordBallSystem* mBallSystem;            // +0x700
	engine::FadeContainer* mGameScreen;     // +0x704
	int mMode;                              // +0x708: the game's mode (the ball system's MODE_ values)
	bool mRetrying;                         // +0x70c: the level again with the same template
	engine::SoundBank* mSounds;             // +0x710: config\globalSounds.xml

	static Game* sInstance;
	// port: -splashhold keeps the original's two seconds of splash (updateSplash), for comparisons with it
	static bool sSplashHold;

private:
	// 0x401090: the splash screen
	void makeSplash();
	// 0x401170: the screens waiting to be shown or removed
	void switchScreens();
	// 0x4012e0: the sound and music volumes from the options
	void applyVolumes();
	// 0x401340
	HighScores* getHighScores(int theMode);
	// 0x401370: a freematch level of the template's balls, faster as theLevel goes
	void setActionLevel(int theLevel);
	// 0x401410: a solitaire level of theLevel + 4 balls
	void setSolitaireLevel(int theLevel);
	// 0x401590: the screen saver's command line
	void readCommandLine();
	// 0x401790: the main menu, with its music
	void showMainMenu();
	// 0x401820: the game screen, with the mode's music
	void showGameScreen();
	// 0x4018f0: the game over or (the puzzles all done) the congratulations
	void showGameOver();
	// 0x401cb0: one of the load screen's lines
	void setLoadText(std::string theText, int theIndex);
	// 0x401d20 and 0x401de0: a level copied from a template (theTemplate < 0: a random one)
	void makeActionLevel(int theLevel, int theTemplate);
	void makeSolitaireLevel(int theLevel, int theTemplate);
	// 0x401f00: the level to play
	void chooseLevel(int theTemplate);
	// 0x403290: the options screen
	void makeOptions();
	// 0x403370: the screen saver, or its message and the end
	void startScreenSaver();
	// 0x4034c0: the sounds and the music
	void loadSounds();
	// 0x4036c0: the load screen's lines emptied
	void clearLoadTexts();
	// 0x403760: names in the score tables with fewer than five
	void addDefaultScores();
	// 0x403a30: the level end's banner
	void showLevelEnd();
	// 0x403b30: the game screen, the ball system and the screens over it
	void makeGameScreen();
	// 0x404170: the load screen (theAtOnce: the continue line without a fade)
	void showLoadScreen(bool theShow, bool theAtOnce);
	// 0x404320: the main menu's game link
	// 0x4047d0: a level's start: its introduction on the load screen
	void startLevel();
	// 0x4049b0: a level's end: the bonus lines
	void endLevel();
	// 0x404db0: the game's end
	void endGame();
	// 0x405360: the high score tables shown, with default names
	void showHighScores();
	// 0x405800: the menus and their screens
	void makeMenus();
	// 0x406da0: a new game of theMode from theLevel
	void newGame(int theMode, int theLevel);
	// 0x406f30: a saved game continued
	void continueGame(SavedGame* theGame);
	// 0x408190: the levels, the words, the sounds and the editor's HUD
	void loadData();
	// 0x408fd0: the splash screen's update
	void updateSplash(engine::UpdateInfo& theInfo);
};

#endif
