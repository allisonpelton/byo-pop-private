// Sexy::HallOfFame (0x14c bytes, constructor V1:0x45e840, vtables 0x6fc75c and 0x6fc87c (its ButtonListener part at
// +0x88); code V1:0x45e840-0x4721d0): the screen of the best players. Every profile in users/*.bwa is loaded and
// ranked in three tabs: the adventure (the top ten words ever spelled, and the Lex power rating of the players),
// the arena (the best timed runs and the most bonuses) and the mini-games (three games a page: each one's best
// scores). The lists begin with ten entries made by the game's authors, which the players' own push down. The
// player's own new entries are shown in a colour that cycles. Laid out from a data/hof_*.xml file (XLayout).
//
// Made by GameApp (V1:0x450760 from the main menu, V1:0x450850 after a timed game, V1:0x450aa0 after a mini-game, V1:0x450c30
// after the end of the game; GameAppHallOfFame.cpp).
#pragma once

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "Widget.h"

namespace Sexy {

class FadeButton;
class GameApp;
class Image;
class Profile;
class XLayout;

class HallOfFame : public Widget, public ButtonListener {
public:
	// mMode: the tab, and for the mini-games the page (three games a page) from 2 on
	enum { MODE_ADVENTURE = 0, MODE_ARENA = 1, MODE_MINI_GAMES = 2 };
	// the buttons' ids
	enum {
		BUTTON_MAIN_MENU = 0,
		BUTTON_PLAY_AGAIN = 1,
		BUTTON_TAB_ADVENTURE = 2,
		BUTTON_TAB_ARENA = 3,
		BUTTON_TAB_MINI_GAMES = 4,
		BUTTON_PREV = 5,
		BUTTON_NEXT = 6,
	};

	// A row of the top words: a player's name and word, the new ones in the colour that cycles (0x3c bytes)
	struct WordRow {
		std::string mName;       // +0x00
		std::string mWord;       // +0x1c
		bool mNew = false;       // +0x38
	};
	// A row of the Lex power ratings: the name and the picture's cel of the rating (0x24 bytes)
	struct PowerRow {
		std::string mName;       // +0x00
		int mCel = 0;            // +0x1c
		bool mChanged = false;   // +0x20: the player's own, when the rating changed since the screen was last seen
	};
	// A row of the best timed runs (0x60 bytes): the name, the last boss reached with the time it took
	struct TimeRow {
		std::string mName;       // +0x00
		std::string mBoss;       // +0x1c: the boss of the stage
		std::string mTime;       // +0x38: "(h:mm:ss)"
		Image* mImage = nullptr; // +0x54: the arena's picture
		int mCel = 0;            // +0x58
		bool mNew = false;       // +0x5c
	};
	// A row of the most bonuses (0x58 bytes) and of a mini-game's best scores (the same shape)
	struct ScoreRow {
		std::string mName;       // +0x00
		std::string mFirst;      // +0x1c: the experience, or the score
		std::string mSecond;     // +0x38: the number of bonuses, or of words
		bool mNew = false;       // +0x54
	};
	// A mini-game of the list (0x40 bytes)
	struct MiniGame {
		std::string mSlot;       // +0x00: the layout's node of the page's place for it ("Slot1" to "Slot3")
		std::string mClass;      // +0x1c: the game's class in the scripts
		Image* mBanner = nullptr;  // +0x38
		bool mScoreAndWords = false;  // +0x3c: scores and words, else words only
	};

	XLayout* mLayout;                 // +0x8c
	GameApp* mApp;                    // +0x90
	FadeButton* mMainMenuButton;      // +0x94: id 0 ("Continue" after the end of the game)
	FadeButton* mPlayAgainButton;     // +0x98: id 1
	FadeButton* mTabs[3];             // +0x9c: ids 2 to 4
	FadeButton* mPrevButton;          // +0xa8: id 5
	FadeButton* mNextButton;          // +0xac: id 6
	std::vector<WordRow> mWordRows;   // +0xb4
	std::vector<PowerRow> mPowerRows; // +0xc4
	std::vector<TimeRow> mTimeRows;   // +0xd4
	std::vector<ScoreRow> mBonusRows; // +0xe4
	std::vector<std::vector<ScoreRow> > mMiniGameRows;  // +0xf4: by mini-game
	std::vector<MiniGame> mMiniGames; // +0x104
	// port: +0x114 is a vector that nothing ever filled
	std::string mMiniGameName;        // +0x120: the mini-game the player just played ("Play Again" plays it again)
	int mMode;                        // +0x13c
	int mNumMiniGames;                // +0x140: main.GetNumMiniGames
	int mCounter;                     // +0x144: counts 0 to 254 for the colour that cycles
	bool mGameOver;                   // +0x148: shown at the end of a game: Play Again beside the main menu's button
	bool mTimed;                      // +0x149: after a timed game: Play Again plays the timed mode again
	bool mEndOfGame;                  // +0x14a: after the end of the adventure: "Continue" in place of the main menu's button
	bool mFromArena;                  // +0x14b: shown after a timed game: the main menu comes back under it

	// theMode: the tab to begin with. theName: the mini-game that was just played (or empty)
	HallOfFame(GameApp* theApp, int theMode, bool theGameOver, bool theTimed, const std::string& theMiniGameName,
	           bool theEndOfGame);  // V1:0x45e840
	~HallOfFame() override;          // 0x450f40 (0x450f60)

	// V1:0x460550: the curtain over the screen is closed: the main menu comes back (after a timed game) and the
	// screen goes
	void CurtainClosed();

	// vf70 in the original (0x459560; the curtain's callback is the thunk V1:0x477cd0, which calls it): once the
	// curtain has closed over this screen, the mini-games' menu comes for the game just played and this goes
	void ShowMiniGamesMenu();

	// Widget
	void Update() override;           // 0x452b90
	void Draw(Graphics* g) override;  // 0x451de0

	// ButtonListener
	void ButtonPress(int theId) override;    // 0x4721d0: the click
	void ButtonDepress(int theId) override;  // 0x452c00

private:
	void SetTab(int theMode);   // V1:0x460950: the tab buttons look, and mMode
	void LayOut();              // V1:0x45edc0: the layout of the tab, the buttons and the mini-games' banners
	void LoadLists();           // V1:0x460b10: every profile loaded, the lists made
	// the lists (each puts its rows into this screen's)
	void MakeWordList(const std::vector<Profile*>& theProfiles);        // V1:0x460ea0: the top words
	void MakePowerList(const std::vector<Profile*>& theProfiles);       // V1:0x4623f0: the Lex power ratings
	void MakeTimeList(const std::vector<Profile*>& theProfiles);        // V1:0x4632a0: the best timed runs
	void MakeBonusList(const std::vector<Profile*>& theProfiles);       // V1:0x4644d0: the most bonuses
	void MakeMiniGameLists(const std::vector<Profile*>& theProfiles);   // V1:0x465290: the mini-games' best scores
};

}  // namespace Sexy
