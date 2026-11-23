#ifndef __FEEDINGFRENZY_STAGEENDSCREEN_H__
#define __FEEDINGFRENZY_STAGEENDSCREEN_H__

// The screen with a stage's results (StageEndScreen, 0x2f8 bytes, constructor 0x429a50, vtable 0x4a5a68). It shows what
// the player ate and counts it up a step at a time: in a bonus stage the share of the fish eaten and the time bonus,
// in the others the fish eaten of each kind that the stage had on the menu, with the food bank's count growing by each
// number; the timed game counts the time. A fun fact of the sea comes last. What shows next is set by a state, and the
// pause between two states by a wait.

#include "ImageFilter.h"
#include "Object.h"
#include "ResultCounter.h"
#include "SplatFactory.h"
#include "StageInfoScreen.h"

#include <string>

namespace engine
{
class TextItem;
}

namespace feedingfrenzy
{

// The kinds of fish a stage has on the menu (the five bubbles of the results)
const int RESULT_KIND_COUNT = 5;

// What the screen is doing (mState)
enum ResultsState
{
	RESULTS_IDLE = 0,           // nothing is counting
	RESULTS_BONUS_START = 2,    // the bonus stage's results wait before the share of the fish eaten counts
	RESULTS_BONUS_PERCENT = 3,  // the share of the fish eaten counts up
	RESULTS_TIME_BONUS = 4,     // the time bonus counts down
	RESULTS_FINAL_SCORE = 5,    // the time bonus counts into the score
	RESULTS_WAIT = 6,           // a pause before the next state
	RESULTS_FOOD_BANK = 7,      // the fish eaten count into the food bank
	RESULTS_TIME_MODE = 8,      // the time left counts into the time played
	RESULTS_MENU_BITES = 9,     // the counts of the kinds of fish show one after the other, each with a bite
	RESULTS_MENU_COUNTING = 10, // a kind of fish's count goes down into the food bank's
	RESULTS_RANK = 11,          // the food bank's rank shows
	RESULTS_FUN_FACT = 12,      // the fun fact fades in
	RESULTS_TIME_LEFT = 13      // the time left of the timed game shows
};

class StageEndScreen : public StageInfoScreen
{
public:
	explicit StageEndScreen(StageMgr* theStage);  // 0x429a50

	// 0x426920: the layout is loaded and the five texts of the menu are taken from it
	bool loadLayout(std::string theFile) override;

	// 0x4290f0: the screen is set up for the stage that ended: its kind (a StageType) and the game's mode (a GameMode);
	// the splat factory of the fish that were eaten is added to the screen, which keeps it
	void setUpResults(int theStageType, int theGameMode, engine::SplatFactory* theEatSplats);

	// 0x426f10: a kind of fish of the stage's menu: its picture goes in the bubble with the number theIndex (from 0),
	// and the level of the fish that are counted for it is kept
	void setMenuBubble(int theIndex, const std::string& theImage, float theLevel, engine::HsvShiftFilter* theShift);

	// 0x4292b0: the states of the counting
	void update(engine::UpdateInfo& theInfo) override;

private:
	// 0x4263d0: the texts of the menu are found in the layout
	void findMenuTexts();

	// 0x427a60: a fun fact of the sea goes in the text of the results
	void setUpFunFact();
	// 0x426440: the fish the stage ate are added to the food bank (not in the timed game), which is saved, and its
	// count is shown
	void setUpFoodBank();
	// 0x426750 (the normal game), 0x426560 (the timed game, which also takes the time left as the time played): the
	// results of a stage that is not a bonus stage, with the menu counted first
	void setUpMenuResults();
	// 0x427fd0 (the normal game), 0x4285e0 (the timed game): the results of a bonus stage: the share of the fish
	// eaten, and the time bonus
	void setUpBonusResults();

	// 0x4292b0 (the update's state of the bites): the count of the next kind of fish shows, with a splat of the
	// player's bite
	void showMenuBite();

	// The counters' slots; each plays the tick of a counting step
	void onBonusPercent(int theNumber);       // 0x426a10
	void onTimeBonusStep(int theNumber);      // 0x426b10
	void onTimeModeStep(int theNumber);       // 0x426be0
	void onFinalScoreStep(int theNumber);     // 0x426c70
	void onBonusFoodBankStep(int theNumber);  // 0x426d50
	void onMenuCountStep(int theNumber);      // 0x426dd0

	// The steps of the counting, each of which waits a while before the next state
	void startTimeBonusScore();         // 0x4291b0: the time bonus counts down; a stage with no time goes on
	void startFinalScore();             // 0x428c70: the time bonus is added to the score
	void startFoodBankCount();          // 0x428d50: the food bank's count grows by the fish eaten
	void startMenuCount(int theIndex);  // 0x428dc0: a kind of fish's count goes down into the food bank's
	void startTimeModeCount();          // 0x428e60: the time left is added to the time played
	void startMenuCounting();           // 0x429280: the counts of the menu go down into the food bank's
	void startRankUp();                 // 0x425eb0: the food bank's rank is shown
	void startFunFact();                // 0x425ee0: the fun fact fades in
	void startTimeLeft();               // 0x425f10: the time left of the timed game is shown
	// the state waits mWaitSeconds and goes on to theNextState
	void waitThen(int theNextState, float theSeconds);

	// The seconds the stage has left, in whole seconds
	int getWholeSecondsLeft() const;

	int mState;                                    // +0x168: a ResultsState
	int mNextState;                                // +0x16c: the state after the wait
	float mWaitSeconds;                            // +0x170
	int mStageType;                                // +0x174
	int mGameMode;                                 // +0x178
	int mScoreBase;                                // +0x17c: the score the bonus counts are added to
	int mFoodBankBase;                             // +0x180: the food bank's count the fish counted so far are added to
	int mTimeBase;                                 // +0x184: the time played that the time left is added to
	int mMenuIndex;                                // +0x188: the kind of fish that is counted
	int mMenuCount;                                // +0x18c: the fish eaten of that kind
	engine::TextItem* mMenuText[RESULT_KIND_COUNT];  // +0x190: the counts of the menu; not counted
	float mMenuLevels[RESULT_KIND_COUNT];            // +0x1a4: the level of each kind of fish; 0 when there is no kind
	ResultCounter mBonusPercentCounter;            // +0x1b8: the share of the fish eaten
	ResultCounter mTimeCounter;                    // +0x1f4: the time bonus, or the time of the timed game
	ResultCounter mFinalScoreCounter;              // +0x230: the time bonus into the score
	ResultCounter mFoodBankCounter;                // +0x26c: the fish eaten into the food bank
	ResultCounter mMenuCounter;                    // +0x2a8: a kind of fish into the food bank
	engine::Ref<engine::SplatFactory> mEatSplats;  // +0x2e4
};

} // namespace feedingfrenzy

#endif
