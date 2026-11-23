#ifndef __FEEDINGFRENZY2_STAGECOMPLETESCREEN_H__
#define __FEEDINGFRENZY2_STAGECOMPLETESCREEN_H__

// The screens after a stage (StageCompleteScreen, its constructor 0x490c40 (feedingfrenzy2)), the base of the normal
// stage's end (StageCompleteScreenNormal, the application's +0x9c) and the bonus stage's (StageCompleteScreenBonus,
// +0xa0): the layout read with the shell's theme, with the splats of
// resource/shell/stageCompleteSplats.xml and a group for its effects on top. While the screen is enabled, a press of
// the mouse anywhere finishes the count running at once.
//
// The normal stage's end (0x290 bytes, its constructor 0x4a15b0 (feedingfrenzy2)) counts the stage up in steps, each
// a state of its own machine: the fish eaten of each kind the stage shows (up to five), then the bonuses picked up,
// then the barracudas, each step's number popping in with an effect, a chomp's splat and a sound; then the food bank
// counts up to its new total, the rank's header and the rank the total earns come in, and a fun fact (or, after the
// final boss, the alien's unlocking) fades in. A boss stage shows the boss's own layout and its bonus in place of the
// fish. Its buttons' actions are the stage-complete state's.

#include "Container.h"
#include "GenericScreen.h"
#include "KeyInterpolator.h"
#include "Splat.h"
#include "State.h"
#include "TextItem.h"
#include "TimeDriver.h"

class FishSpawn;
class Stage;

class StageCompleteScreen : public engine::GenericScreen
{
public:
	StageCompleteScreen() {}

	// 0x405270 (feedingfrenzy2): the layout read with the shell's theme, then the splats made and read and the effects'
	// group (FxGroup) made, the group added and then the splats
	void loadScreen(const std::string& thePath);
	// 0x90 (0x4a4550 (feedingfrenzy2)): the layout's, and the engine's presses of the mouse heard
	void enable() override;
	// 0x94 (0x48c050 (feedingfrenzy2)): the layout's, and the presses no longer heard
	void disable() override;

	// 0x180: the screen set for the stage of the id, from the food bank before it, and its count begun
	virtual void setup(const std::string& theStageId, Stage* theStage, int theFoodBank) = 0;
	// 0x184: the food bank with the stage's food added
	virtual int getTotal() = 0;
	// 0x188: the count's machine updated
	virtual void updateCount(engine::UpdateContext* theContext) = 0;
	// 0x190 and 0x194: the debug buttons' next and previous fun fact (their buttons are hidden and take no input)
	virtual void nextFunFact() = 0;
	virtual void previousFunFact() = 0;
	// 0x198: the count running finished at once
	virtual void skip() = 0;

protected:
	// 0x4c2830 (feedingfrenzy2): through the screen's 0x18c (0x4a25d0 (feedingfrenzy2)), its 0x198
	void mouseDownSlot(const engine::Vector2<int>& thePoint) { skip(); }

	engine::Ref<Stage> mStage;                   // +0x1b4
	engine::Ref<SplatFactory> mSplats;           // +0x1b8
	engine::Ref<engine::Container> mFxGroup;     // +0x1bc
};

class StageCompleteScreenNormal : public StageCompleteScreen
{
public:
	StageCompleteScreenNormal();

	// 0x180 (0x53c106 (feedingfrenzy2)): the food bank kept as the total too, the components found, the quit button
	// hidden after the game's last stage; a normal stage counts from its fish, a boss stage shows its layout and
	// bonus, fills the slots and counts from the food bank
	void setup(const std::string& theStageId, Stage* theStage, int theFoodBank) override;
	// 0x184 (0x4a4820 (feedingfrenzy2))
	int getTotal() override { return mTotal; }
	// 0x188 (0x5019bd (feedingfrenzy2))
	void updateCount(engine::UpdateContext* theContext) override { mStateMgr.update(theContext); }
	// 0x190 and 0x194 (0x539916 and 0x5398da (feedingfrenzy2)): the next or previous fact of the fun facts in the
	// fact's text: not translated, as nothing reaches their buttons
	void nextFunFact() override {}
	void previousFunFact() override {}
	// 0x198 (0x50197f (feedingfrenzy2)): the current state's 0x1c, when there is one
	void skip() override;

private:
	// The count's states (StageCompleteScreenNormal::StageCompleteStateNormal, an AbstractState of the screen)
	class CountState : public engine::State
	{
	public:
		// 0x516641 (feedingfrenzy2), through 0x516539 (feedingfrenzy2)
		explicit CountState(StageCompleteScreenNormal* theScreen) : mScreen(theScreen) {}
		// 0x1c: the count finished at once (pure in the base)
		virtual void skip() = 0;

	protected:
		StageCompleteScreenNormal* mScreen;  // +0x0c
	};
	class CountFishState;
	class CountBubbleState;
	class CountBarracudaState;
	class FoodBankState;
	class ShowFunFactState;

	enum
	{
		SLOT_COUNT = 5
	};

	// 0x5310c2 (feedingfrenzy2): the screen's components found by name
	void findComponents();
	// 0x53ad86 (feedingfrenzy2): the slots filled from the stage's fish spawns shown on its end, the rest hidden; the
	// bonuses and barracudas counted, the slots and the bonus' centred, every number hidden at 0, the total reckoned,
	// the food bank shown, the rank found and hidden, and the timed mode's clock shown
	void fill();
	// 0x516764 (feedingfrenzy2): the slot's fish eaten (none when the spawn is worth no food); the first slot of a rank
	// and of the water or the air also counts the fish of that rank and place the end does not show
	int countSlot(int theSlot);
	// 0x5166c2 (feedingfrenzy2): the stage's spawns of a class whose name holds Bonus, and the bonuses of them picked
	// up
	void countBonuses();
	// 0x51d345 (feedingfrenzy2): the food of the barracudas eaten
	int countBarracudas();
	// 0x53ac55 (feedingfrenzy2): each filled slot's number shown
	void finishFish();
	// 0x53abf5 (feedingfrenzy2): the bonuses' number shown, when the stage has any
	void finishBonuses();
	// 0x539952 (feedingfrenzy2): the total in the food bank's text, it, the rank's header and the rank shown
	void finishFoodBank();
	// 0x50194e (feedingfrenzy2): the barracudas' bonus shown at its size, when there is one
	void finishBarracudas();
	// The slot's number popping in (0x540a9f and 0x540881 (feedingfrenzy2), the same steps): the shell theme's effect
	// fishCountPopIn at the text, the chomp's splat 30 below it, the number set and shown, and the chomp's sound
	void popIn(engine::TextItem* theText, int theNumber);

	engine::Ref<FishSpawn> mSlotSpawns[SLOT_COUNT];        // +0x1a0: null past the last filled
	int mFoodBank;                                         // +0x1c0: before the stage
	int mTotal;                                            // +0x1c4: after it
	int mBonusesTaken;                                     // +0x1c8: the bonuses picked up, each one food
	int mBonusSpawns;                                      // +0x1cc: the stage's spawns of bonuses
	engine::TextItem* mFishCounts[SLOT_COUNT];             // +0x1d0: fishCount0 to fishCount4
	engine::Container* mFishImages[SLOT_COUNT];            // +0x1e4: fishImg0 to fishImg4
	int mSlotCounts[SLOT_COUNT];                           // +0x1f8
	engine::TextItem* mBonusCountText;                     // +0x20c: bonusCount
	engine::Component* mBonusImageTimed;                   // +0x210: bonusImgTimed
	engine::Component* mBonusImageNormal;                  // +0x214: bonusImgNormal
	engine::Component* mFoodBankRankHeader;                // +0x21c: foodBankRankHeader
	engine::TextItem* mFoodBankCount;                      // +0x220: foodBankCount
	engine::TextItem* mFoodBankRank;                       // +0x224: foodBankRank
	engine::TextItem* mTimeAttackTimeLeft;                 // +0x228: timeAttackTimeLeft
	engine::Component* mTimeAttackGroup;                   // +0x22c: timeAttackGroup
	engine::Component* mFunFactGroup;                      // +0x230: funFactGrp
	engine::TextItem* mFunFactText;                        // +0x234: funFactText
	engine::Component* mFinalBossGroup;                    // +0x238: finalBossGrp
	engine::Component* mAlienUnlockText;                   // +0x23c: alienUnlockText
	engine::Component* mDebugFunFactNext;                  // +0x240: debugFunFactNext
	engine::Component* mDebugFunFactPrevious;              // +0x244: debugFunFactPrev
	engine::Container* mBossLayoutGroup;                   // +0x248: bossLayoutGrp
	engine::Component* mBossBackground;                    // +0x24c: bossBkg
	engine::Component* mLunchReportGroup;                  // +0x250: lunchReportGrp
	engine::TextItem* mFoodBankBonusCount;                 // +0x254: foodBankBonusCount
	engine::TextItem* mBarracudaText;                      // +0x258: barracudaText
	engine::Component* mBarracudaBonusGroup;               // +0x25c: barracudaBonusGrp
	engine::Component* mQuitButton;                        // +0x260: stageCompleteQuitBtn
	int mBossBonus;                                        // +0x264: 250, or 500 for the final boss
	int mBarracudaBonus;                                   // +0x268
	engine::StateMgr<CountState> mStateMgr;                // +0x26c
};

// The bonus stage's end (0x234 bytes, its constructor 0x4969a0 (feedingfrenzy2)): the fish eaten of the stage's fish
// and their share count up; when every fish was eaten, the time left, its bonus (a hundred a second, added to the score
// as the screen fills) and the final score count up, else a group saying there is no time bonus shows; then the food
// bank counts up to its total and the rank's header and the rank come in.
class StageCompleteScreenBonus : public StageCompleteScreen
{
public:
	StageCompleteScreenBonus();

	// 0x180 (0x4a1b10 (feedingfrenzy2)): the food bank kept as the total too, the components found, the screen filled
	// and its count begun (the stage's id is not read)
	void setup(const std::string& theStageId, Stage* theStage, int theFoodBank) override;
	// 0x184 (0x5019c8 (feedingfrenzy2))
	int getTotal() override { return mTotal; }
	// 0x188 (0x491620 (feedingfrenzy2))
	void updateCount(engine::UpdateContext* theContext) override { mStateMgr.update(theContext); }
	// 0x190 and 0x194 do nothing
	void nextFunFact() override {}
	void previousFunFact() override {}
	// 0x198 (0x4a1c30 (feedingfrenzy2)): the current state's 0x1c, when there is one
	void skip() override;

private:
	// The count's states (StageCompleteScreenBonus::StageCompleteStateBonus, an AbstractState of the screen, its
	// constructor 0x4a4b40 (feedingfrenzy2))
	class CountState : public engine::State
	{
	public:
		explicit CountState(StageCompleteScreenBonus* theScreen) : mScreen(theScreen) {}
		// 0x1c: the count finished at once (pure in the base)
		virtual void skip() = 0;

	protected:
		StageCompleteScreenBonus* mScreen;  // +0x0c
	};
	class CountFishRatioState;
	class TimeBonusState;
	class TimeLeftCountupState;
	class FoodBankState;

	// 0x48c530 (feedingfrenzy2): the screen's components found by name
	void findComponents();
	// 0x48a610 (feedingfrenzy2): the fish counted and the total reckoned, the food bank shown, the rank found and
	// hidden, the share and the fish eaten written at none, both time bonus groups hidden, the time bonus reckoned and
	// added to the score, and its texts hidden
	void fill();
	// 0x490a60 (feedingfrenzy2): over the stage's spawns that have a supply, their actors that died and their supply
	// summed, and the share of them eaten in hundredths
	void countFish();
	// 0x493710 (feedingfrenzy2): the share and the fish eaten written in full and shown
	void finishFish();
	// 0x53b3e4 (feedingfrenzy2): when a fish was left, the no-bonus group shown; else the bonus group, the time left,
	// the bonus and the final score shown in full
	void finishTimeBonus();
	// 0x4a0120 (feedingfrenzy2): the total in the food bank's text, it, the rank's header and the rank shown
	void finishFoodBank();
	// Whether a fish of the stage was not eaten (0x4a0170, 0x53ceb0 and 0x53b3e4 (feedingfrenzy2))
	bool isFishLeft() const { return mFishEaten < mFishSupply; }
	// The stage's time left in whole seconds, rounded up (0x48a610 and others)
	int getSecondsLeft() const;

	int mFoodBank;                                 // +0x1c0: before the stage
	int mTotal;                                    // +0x1c4: after it
	engine::TextItem* mEatRatioText;               // +0x1d0: eatRatioText
	engine::TextItem* mEatPercentText;             // +0x1d4: eatPctText
	engine::Component* mTimeBonusGroup;            // +0x1d8: timeBonusGrp
	engine::TextItem* mTimeLeftText;               // +0x1dc: timeLeftText
	engine::TextItem* mTimeBonusText;              // +0x1e0: timeBonusText
	engine::TextItem* mFinalScoreText;             // +0x1e4: finalScoreText
	engine::Component* mFoodBankRankHeader;        // +0x1e8: foodBankRankHeader
	engine::TextItem* mFoodBankRank;               // +0x1ec: foodBankRank
	engine::TextItem* mFoodBankCount;              // +0x1f0: foodBankCount
	engine::Component* mNoTimeBonusGroup;          // +0x1f4: noTimeBonusGrp
	int mFishEaten;                                // +0x1f8: each one food
	int mFishSupply;                               // +0x1fc
	int mEatPercent;                               // +0x200
	int mScoreBefore;                              // +0x204: the score before the time bonus
	int mFinalScore;                               // +0x208
	int mTimeBonus;                                // +0x20c
	engine::StateMgr<CountState> mStateMgr;        // +0x210
};

#endif
