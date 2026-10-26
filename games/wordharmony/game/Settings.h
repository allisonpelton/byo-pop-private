#ifndef __WORDHARMONY_SETTINGS_H__
#define __WORDHARMONY_SETTINGS_H__

// The player's settings (constructor 0x40a000, the options in the registry 0x409d50-0x40aec0), kept under
// Software\GameHouse\WordHarmony: sound, music and full screen (with the options screen's check boxes), the highest
// level reached in each mode, whether each mode's help shows at its start, the default high score name, the link
// settings and the screensaver flag, and a saved game per mode. The saved games (the registry strings
// PuzzleSavedGame, SolitaireSavedGame and ActionSavedGame, the saved game's XML) come with the save code: load and
// save leave them out for now.

#include <string>

namespace engine
{
class CheckBox;
class ScreenLayout;
}

class SavedGame;

class Settings
{
public:
	Settings();  // 0x40a000

	// 0x40a110: the options screen's check boxes set from the settings, the window's mode too
	void setScreen(engine::ScreenLayout* theScreen);
	// 0x40a200
	void setDefaultScoreName(std::string theName) { mDefaultScoreName = theName; }
	// 0x40a250
	void load();
	// 0x40aa10: the check boxes' states and the rest written
	void save();

	// 0x409db0
	bool isSoundEnabled();
	// 0x409dc0
	bool isMusicEnabled();
	// 0x409dd0: the box set from the window first
	bool isFullScreen();
	// 0x409d50: theLevel when it is higher than the mode's highest
	void setHighestLevel(int theMode, int theLevel);
	// 0x409d70: the mode's saved game replaced (the old one deleted)
	void setSavedGame(int theMode, SavedGame* theGame);
	// 0x409e80
	SavedGame* getSavedGame(int theMode) { return mSavedGames[theMode]; }

	bool getShowPuzzleHelp() { return mShowPuzzleHelp; }        // 0x409e90
	bool getShowActionHelp() { return mShowActionHelp; }        // 0x409ea0
	bool getShowSolitaireHelp() { return mShowSolitaireHelp; }  // 0x409eb0
	void setShowPuzzleHelp(bool theShow) { mShowPuzzleHelp = theShow; }        // 0x409ec0
	void setShowActionHelp(bool theShow) { mShowActionHelp = theShow; }        // 0x409ed0
	void setShowSolitaireHelp(bool theShow) { mShowSolitaireHelp = theShow; }  // 0x409ee0

	std::string mDefaultScoreName;     // +0x00
	engine::ScreenLayout* mScreen;     // +0x1c: the options screen
	engine::CheckBox* mSoundBox;       // +0x20
	engine::CheckBox* mMusicBox;       // +0x24
	engine::CheckBox* mFullScreenBox;  // +0x28
	int mHighestLevels[3];             // +0x2c: by mode (puzzle 11, solitaire 15, action 10)
	bool mShowPuzzleHelp;              // +0x38
	bool mShowSolitaireHelp;           // +0x39
	bool mShowActionHelp;              // +0x3a
	SavedGame* mSavedGames[3];         // +0x3c: by mode
	bool mShowLink;                    // +0x48
	std::string mVisitURL;             // +0x4c: "NO URL"
	std::string mILink;                // +0x68
	bool mSaverEnabled;                // +0x84
	bool mSoundEnabled;                // +0x85
	bool mMusicEnabled;                // +0x86
	bool mFullScreen;                  // +0x87
	// +0x88: the registry key (Software\GameHouse\WordHarmony); the port goes through engine::Host
};

#endif
