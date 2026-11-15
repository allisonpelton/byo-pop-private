#ifndef __PIZZAFRENZY_STORYSCREEN_H__
#define __PIZZAFRENZY_STORYSCREEN_H__

// The story screen (Iggle Pop's StoryScr, 0x5b0 bytes, constructor 0x442f50, vtable 0x4fe370; the application keeps it
// at +0xdc, from res/screenLayouts/storyScreen.xml): the screen of a level's story, which is also the screen of the
// level's end. The pulldown screen and the blackboard slide in over the city, the four characters say their lines in
// the speech bubbles (their text types itself out), and the story file's pictures come up on the screen and the
// blackboard. The level's end (the totals, the tips, the frenzy and the toppings' upgrades) is the other part of it:
// the nodes, members and methods of that part are translated with the level's end, and each place the story's part
// reaches it calls one empty method that says so.

#include "GameStats.h"
#include "GenericScreen.h"
#include "Interpolator.h"
#include "Level.h"
#include "Ramp.h"
#include "Sound.h"
#include "StorySequence.h"
#include "TextTyper.h"

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

	// The level's end, translated with it. Each is a place the story's part of the screen reaches it.
	// 0x43e090 (the end of): the nodes of the totals (stats, actualRevenue, salesTarget, bonusSales, totalScore,
	// ordersDelivered, foodBank, serviceRating, comboGroup, longestCombo, bonusStats, perfectToppings and the others)
	void findTotalNodes();
	// 0x43cd40 (the end of): levelUpgradeGroup, and with the level's end toppingImage, toppingName, levelText,
	// toppingClass, nextLevelText (with their texts as the layout has them) and the typer of toppingDescription (290
	// wide), which goes in the group
	void findUpgradeNodes();
	// 0x43c9e0, 0x43ca70 and 0x441170 ("hideUpgrades"): the upgrades' group is hidden
	void hideUpgrades() { mLevelUpgradeGroup->setVisible(false); }
	// 0x441bf0, cases 4 to 0x15: a frame of the level's end (the totals counting up, the grade, the toppings and the
	// bonus, one case for each step, each waiting 0.3 or 0.5 seconds for the next)
	void updateLevelEnd(engine::UpdateInfo& theInfo);
	// 0x441170 ("showScores"): the level's end starts with its totals (mode 4)
	void showScores();
	// 0x43f0f0, from 0x441170 ("showFunFact") and from the key F (0x4414f0) while the debug flag is set: the coupons'
	// fun fact is said by Lorenzo
	void showFunFact();
	// 0x441170 ("showBonus"): the bonus part of the level's end starts (mode 0xd)
	void showBonus();
	// 0x43f240 and 0x43c8f0, from 0x441170 ("gameWon"): the game's end is shown with its totals
	void showGameWon();
	// 0x441650, from 0x442ca0 ("continue" in modes 4 to 0xb): the totals go to their final numbers
	void continueScores();
	// 0x43f7f0, from 0x442ca0 ("continue" in modes 0xd to 0x15): the bonus goes to its final numbers
	void continueBonus();

	// The four characters of the story, in the order of the layout's nodes
	enum
	{
		SPEAKER_PAULA = 0,
		SPEAKER_LISA = 1,
		SPEAKER_LORENZO = 2,
		SPEAKER_NICCOLO = 3,
		SPEAKER_COUNT = 4
	};

	// The modes of the screen (+0x37c): nothing plays, shown, the story, hidden; the level's end has the others
	enum
	{
		MODE_RESET = 0,
		MODE_SHOWN = 1,
		MODE_STORY = 2,
		MODE_HIDDEN = 3,
		MODE_SCORES = 4,
		MODE_BONUS = 0xd,
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
	// +0x238 to +0x2d4: the nodes of the level's end's totals (translated with it)
	GameStats* mGameStats;                         // +0x2d8: what the level's end shows
	engine::Ref<engine::Node> mLevelUpgradeGroup;  // +0x314: "levelUpgradeGroup", a topping's upgrade
	// +0x2dc to +0x37c: the level's end's counters and nodes (translated with it)
	engine::Sound* mRollupSound;                   // +0x280: "menu_rollup", which loops while a total counts up
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
	// +0x468 to +0x5ac: the level's end's interpolators (translated with it)
};

} // namespace pizzafrenzy

#endif
