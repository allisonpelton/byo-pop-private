#ifndef __PIZZAFRENZY_STORYSCREEN_H__
#define __PIZZAFRENZY_STORYSCREEN_H__

// The story screen (Iggle Pop's StoryScr, 0x5b0 bytes, constructor 0x442f50, vtable 0x4fe370; the application keeps it
// at +0xdc, from res/screenLayouts/storyScreen.xml): the screen of a level's story, which is also the screen of the
// level's end. The pulldown screen and the blackboard slide in over the city, the four characters say their lines in
// the speech bubbles (their text types itself out), and the story file's pictures come up on the screen and the
// blackboard. The level's end (the totals counting up, the stars, the longest combo, the toppings' upgrades and the
// bonus round's and the game's end totals) is the other part of it, with its own modes.

#include "FadeContainer.h"
#include "GameStats.h"
#include "GenericScreen.h"
#include "ImageNode.h"
#include "Interpolator.h"
#include "Level.h"
#include "Ramp.h"
#include "Sound.h"
#include "StorySequence.h"
#include "TextTyper.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

class StoryScreen : public engine::GenericScreen
{
public:
	StoryScreen();            // 0x442f50
	~StoryScreen() override;  // 0x441bd0, 0x4408a0

	// 0x16c: 0x43c9e0: the screen is as a new story has it, and the level's upgrades are hidden
	void resetTransitions() override;
	// 0x170: 0x43c490: the screen is shown, with nothing yet told to play
	void show() override;
	// 0x174: 0x43c4b0: the screen is going
	void hide() override;
	// 0x17c: 0x43cd10: the file of a story (a name in the game's archive) is read into the story that plays
	void setStory(const std::string& theStoryFile);
	// 0x180: 0x43cd40: the layout's nodes (the characters and their texts, the buttons, the pulldown and the
	// blackboard), and the screen's own sound
	void setup();
	// 0x184: 0x43c480: the level the level's end is of
	void setLevel(Level* theLevel) { mLevel = theLevel; }
	// 0x188: 0x43ed20: the groups are back out of the screen, the characters gone and the story's buttons as they start
	void reset();
	// 0x18c: 0x43c8d0: the story plays, from its first action
	void startStory();
	// 0x190: 0x43ee60: the story file's sequence is read, and the old one stops
	void buildStory();
	// 0x194: 0x441bf0: a frame of the screen: the story plays, or the level's end goes on
	void updateScreen(engine::UpdateInfo& theInfo);
	// 0x198: 0x43c550: the groups' ramps (0.4 seconds each) and the places the pulldown screen and the blackboard
	// come to from outside the screen
	void setUpMotion();
	// 0x19c: 0x43c870: the groups are where their ramps have put them
	void applyMotion();

	// 0x43e090: the game's stats, which the level's end shows, and the nodes of its totals; the loading state hands
	// them over
	void setGameStats(GameStats* theStats);
	// 0x443830: the level summary's totals are set up from the game's stats (the story file is read first): the
	// bonus is added to the score, the stars and the combo's pictures are made, the orders served go to the player's
	// food bank, and the game is saved
	void setUpSummary();
	// 0x43f3f0: the same for the bonus round's summary: the toppings' counts and bonus, the time's bonus, and the
	// bonus added to the score
	void setUpBonusSummary();
	// 0x43f980: the topping's upgrade (the upgrade at theIndex of its list) is shown (its picture, names and text),
	// and its story is made: Lisa tells the first upgrade of the game, then the click to continue
	void showUpgrade(Topping* theTopping, Upgrade* theUpgrade, int theIndex);
	// 0x440240: the story that follows a shown upgrade is made (Lorenzo tells the title the player has now, then the
	// click to continue)
	void startUpgradeStory();
	// 0x43ca70: the story is over: the groups are emptied, the sound stops, the sequence lets the screen go, and the
	// screen is reset
	void endStory();

	// The layout's listener, which this screen is (slot 0 of +0x1ac, 0x442ca0): the story's own buttons are met here
	// and every action goes on
	void screenAction(std::string theAction) override;
	// 0x158 (and 0x4414f0): a key goes to the story first
	void keyAction(int theKey) override;

	// What the sequence asks of the screen (the actions' calls, StorySequence.h)
	// 0x43cae0: the character's portrait and bubble show, and no other's; the text of the character (null for a name
	// that is no character's)
	engine::TextTyper* showSpeaker(const std::string& theSpeaker);
	// 0x441170: an event of the story: what the screen shows, hides or does
	void handleEvent(const std::string& theEvent, const std::string& theParam);

private:
	// 0x43c920: the quit button is shown and takes the mouse, or is hidden
	void showQuitButton(bool theShow);

	// 0x43e090 (the end of): the nodes of the totals, by their names in the layout, and the text of the toppings' line
	// as the layout has it
	void findTotalNodes();
	// 0x43cd40 (the end of): levelUpgradeGroup, and in it toppingImage, toppingName, levelText, toppingClass and
	// nextLevelText (their texts as the layout has them are formats) and the typer of toppingDescription (290 wide),
	// which goes in the group
	void findUpgradeNodes();
	// 0x43c9e0, 0x43ca70 and 0x441170 ("hideUpgrades"): the upgrades' group is hidden
	void hideUpgrades() { mLevelUpgradeGroup->setVisible(false); }
	// 0x441bf0, cases 4 to 0x15: a frame of the level's end (the totals counting up, the stars, the combo, and the
	// bonus report, one case for each step, each waiting 0.3 or 0.5 seconds for the next)
	void updateLevelEnd(engine::UpdateInfo& theInfo);
	// 0x441170 ("showScores"): the level's end starts with its totals
	void showScores() { mMode = MODE_SHOW_REVENUE; }
	// 0x43f0f0, from 0x441170 ("showFunFact") and from the key F (0x4414f0) while the debug flag is set: Lorenzo says
	// a fun fact of the manifest's (one random draw)
	void showFunFact();
	// 0x441170 ("showBonus"): the bonus report starts
	void showBonus() { mMode = MODE_BONUS; }
	// 0x43f240 and 0x43c8f0, from 0x441170 ("gameWon"): the game's end is shown with its totals
	void showGameWon();
	// 0x441650, from 0x442ca0 ("continue" in modes 4 to 0xb): the totals go to their final numbers
	void continueScores();
	// 0x43f7f0, from 0x442ca0 ("continue" in modes 0xd to 0x15): the bonus report goes to its final numbers
	void continueBonus();
	// 0x4406c0: the orders delivered so far (of the level's orders) and the food bank's number
	void showOrderCounts();
	// 0x443510: the stars of the rating, one for each 20 of the happiness and a half star for a rest of 10 or more;
	// each is hidden until it is shown, and adds its points to the score
	void setUpStars();
	// 0x443290: a picture of the longest combo's topping for each of the combo, in a row going left, each hidden until
	// it is shown and adding its points to the score
	void setUpComboIcons();
	// 0x441bf0 (cases 7, 9 and 0x15): what the seconds at the rate count goes from the rest to the counted, and no
	// more than the rest
	void countUp(int& theRest, int& theCounted, float theSeconds, float theRate);
	// 0x441bf0: the sound of the count plays from its start when it is not playing
	void keepRollupSoundPlaying();
	// 0x441bf0, 0x441650, 0x43f7f0: the sound of the count stops
	void stopRollupSound();

	// The four characters of the story, in the order of the layout's nodes
	enum
	{
		SPEAKER_PAULA = 0,
		SPEAKER_LISA = 1,
		SPEAKER_LORENZO = 2,
		SPEAKER_NICCOLO = 3,
		SPEAKER_COUNT = 4
	};

	// The modes of the screen (+0x37c): nothing plays, shown, the story, hidden; the level's end has the others, each
	// named for what it shows when the screen gets to it: the totals, then the rows of the bonus report
	enum
	{
		MODE_RESET = 0,
		MODE_SHOWN = 1,
		MODE_STORY = 2,
		MODE_HIDDEN = 3,
		MODE_SHOW_REVENUE = 4,
		MODE_SHOW_TARGET = 5,
		MODE_SHOW_BONUS_SALES = 6,
		MODE_COUNT_BONUS_SALES = 7,
		MODE_SHOW_ORDERS = 8,
		MODE_COUNT_SERVED = 9,
		MODE_SHOW_STARS = 10,
		MODE_SHOW_COMBO = 0xb,
		MODE_SCORES_DONE = 0xc,
		MODE_BONUS = 0xd,  // the bonus report's first row
		MODE_SHOW_GOOD = 0xe,
		MODE_SHOW_OKAY = 0xf,
		MODE_SHOW_MISSED = 0x10,
		MODE_SHOW_TOPPING_BONUS = 0x11,
		MODE_SHOW_TIME_LEFT = 0x12,
		MODE_SHOW_TIME_BONUS = 0x13,
		MODE_SHOW_BONUS_TOTAL = 0x14,
		MODE_COUNT_BONUS_TOTAL = 0x15,
		MODE_LAST_END = 0x15
	};

	// A character: the portrait (a container, which fades in), the speech bubble and the text typer that sits in the
	// portrait (copied from the layout's text node of the character, which is emptied)
	struct Speaker
	{
		engine::Ref<engine::FadeContainer> mPortrait;
		engine::Ref<engine::Node> mBubble;
		engine::Ref<engine::TextTyper> mText;
	};
	void setUpSpeaker(Speaker& theSpeaker, const char* theName, const char* theBubble, const char* theText,
	                  float theWidth);

	engine::Ref<engine::Node> mPulldownGroup;      // +0x1d4: "pulldownScreenGroup", which comes down from above
	engine::Ref<engine::Node> mBlackboardGroup;    // +0x1d8: "blackboardGroup", which comes in from the right
	engine::Ref<engine::Container> mPulldown;      // +0x1dc: "pulldownScreen", where the story's pictures go
	engine::Ref<engine::Container> mBlackboard;    // +0x1e0: "blackboard"
	Speaker mSpeakers[SPEAKER_COUNT];              // +0x1e4: Paula, Lisa, Lorenzo and Niccolo
	engine::Ref<engine::Node> mSkipString;         // +0x214: "skipString", the hint that the story can be skipped
	engine::Ref<StorySequence> mSequence;          // +0x218: the story's actions
	std::string mStoryFile;                        // +0x21c: where the story's file is

	// The level's end's nodes, by the layout's names
	engine::Ref<engine::Node> mStatsGroup;         // +0x238: "stats", the level's report
	engine::Ref<engine::TextItem> mActualRevenue;  // +0x23c: "actualRevenue", the tips of the level
	engine::Ref<engine::TextItem> mSalesTarget;    // +0x240: "salesTarget", the level's goal
	engine::Ref<engine::TextItem> mBonusSales;     // +0x244: "bonusSales", what the tips are over the goal
	engine::Ref<engine::TextItem> mTotalScore;     // +0x248: "totalScore"
	engine::Ref<engine::TextItem> mOrdersDelivered;  // +0x24c: "ordersDelivered", as delivered/ordered
	engine::Ref<engine::TextItem> mFoodBank;       // +0x250: "foodBank", the orders the player has served
	engine::Ref<engine::Container> mServiceRating;  // +0x254: "serviceRating", where the stars go
	std::vector<engine::Ref<engine::ImageNode>> mStars;  // +0x258: the stars, from the right-most (the first)
	engine::Ref<engine::Container> mComboGroup;    // +0x268: "comboGroup", where the combo's pictures go
	engine::Ref<engine::TextItem> mLongestCombo;   // +0x26c: "longestCombo", the number of the combo's pictures shown
	std::vector<engine::Ref<engine::ImageNode>> mComboIcons;  // +0x270: the pictures of the combo
	engine::Sound* mRollupSound;                   // +0x280: "menu_rollup", which loops while a total counts up
	engine::Ref<engine::Node> mBonusStats;         // +0x284: "bonusStats", the bonus round's report
	engine::Ref<engine::TextItem> mPerfectToppings;  // +0x288: "perfectToppings"
	std::string mToppingsFormat;                   // +0x28c: its text as the layout has it, which each line of the
	                                               // toppings uses
	engine::Ref<engine::TextItem> mGoodToppings;   // +0x2a8: "goodToppings"
	engine::Ref<engine::TextItem> mOkayToppings;   // +0x2ac: "okayToppings"
	engine::Ref<engine::TextItem> mMissedToppings;  // +0x2b0: "missedToppings"
	engine::Ref<engine::TextItem> mToppingBonus;   // +0x2b4: "toppingBonus"
	engine::Ref<engine::TextItem> mTimeLeft;       // +0x2b8: "timeLeft"
	engine::Ref<engine::TextItem> mTimeLeftBonus;  // +0x2bc: "timeLeftBonus"
	engine::Ref<engine::TextItem> mBonusTotal;     // +0x2c0: "bonusTotal"
	engine::Ref<engine::TextItem> mBonusTotalScore;  // +0x2c4: "bonusTotalScore"
	engine::Ref<engine::Node> mGameWon;            // +0x2c8: "gameWon", the game's end report
	engine::Ref<engine::TextItem> mTotalSales;     // +0x2cc: "totalSales"
	engine::Ref<engine::TextItem> mElapsedTime;    // +0x2d0: "elapsedTime", the time the game took
	engine::Ref<engine::TextItem> mFinalFoodBank;  // +0x2d4: "finalFoodBank"
	GameStats* mGameStats;                         // +0x2d8: what the level's end shows

	// The level's end's counters
	float mTimer;                                  // +0x2dc: the seconds until the level's end goes on
	float mCountInterval;                          // +0x2e0: set to 0.15 and read by nothing
	int mStarsShown;                               // +0x2e4: how many stars are shown
	int mBonusLeft;                                // +0x2e8: what is left of the bonus to count into the score
	int mTimeBonus;                                // +0x2ec: the bonus round's time bonus
	int mBonusAmount;                              // +0x2f0: the bonus the level or the bonus round gave
	int mScoreShown;                               // +0x2f4: the score as it shows
	int mServedLeft;                               // +0x2f8: what is left of the orders served to count
	int mServedTotal;                              // +0x2fc: the orders served this level
	int mLongestComboCount;                        // +0x300: the longest combo of the level
	engine::Ref<Topping> mLongestComboTopping;     // +0x304: the topping of the longest combo, null for none
	int mComboShown;                               // +0x30c: how many of the combo's pictures are shown
	int mFoodBankCount;                            // +0x310: the orders the player has served, as shown
	engine::Ref<engine::FadeContainer> mLevelUpgradeGroup;  // +0x314: "levelUpgradeGroup", a topping's upgrade
	engine::Ref<engine::TextTyper> mUpgradeDescription;     // +0x318: the text of "toppingDescription", typed
	engine::Ref<engine::TextItem> mToppingName;    // +0x31c: "toppingName", the name of the upgrade
	engine::Ref<engine::ImageNode> mToppingImage;  // +0x320: "toppingImage"
	engine::Ref<engine::TextItem> mLevelText;      // +0x324: "levelText"
	engine::Ref<engine::TextItem> mToppingClass;   // +0x328: "toppingClass", the name of the topping
	engine::Ref<engine::TextItem> mNextLevelText;  // +0x32c: "nextLevelText"
	std::string mNextLevelFormat;                  // +0x330: its text as the layout has it
	std::string mLevelFormat;                      // +0x34c: the level text's, as the layout has it
	engine::Ref<engine::Node> mQuitButtonGroup;    // +0x368: "quitButtonGroup"
	engine::Ref<engine::Node> mQuitButton;         // +0x36c: "quitButton"
	engine::Ref<engine::Node> mContinueButtonGroup;  // +0x370: "continueButtonGroup"
	engine::Ref<engine::Node> mContinueButton;     // +0x374: "continueButton"
	Level* mLevel;                                 // +0x378: the level the level's end is of
	int mMode;                                     // +0x37c
	engine::Ramp mPulldownRamp;                    // +0x380: how far down the pulldown screen has come
	engine::Ramp mBlackboardRamp;                  // +0x3a4: how far in the blackboard has come
	engine::PositionInterpolator mPulldownMotion;    // +0x3c8: the pulldown group's places over the ramp
	engine::PositionInterpolator mBlackboardMotion;  // +0x418: the blackboard group's places over the ramp
	// +0x468 to +0x5ac: nothing translated uses it
};

} // namespace pizzafrenzy

#endif
