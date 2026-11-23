#include "StageCompleteScreen.h"

#include "ActorInfo.h"
#include "Engine.h"
#include "FeedingFrenzyTwo.h"
#include "FunFacts.h"
#include "Image.h"
#include "NumberText.h"
#include "ParticleSystem.h"
#include "PlayerRanks.h"
#include "Spawn.h"
#include "Stage.h"
#include "StringTable.h"
#include "Theme.h"
#include "UserManager.h"
#include "VisualData.h"

#include <cstdio>

using namespace engine;

namespace
{

// The slots' groups are this wide, side by side about the centre (0x53ad86 (feedingfrenzy2))
const double SLOT_WIDTH = 75.0;
const double SLOT_CENTRE = 37.5;
const double HALF = 0.5;
// The chomp's splat lands this far below the number (0x540a9f (feedingfrenzy2))
const double CHOMP_DROP = 30.0;
// The boss stages' bonus (0x53c106 (feedingfrenzy2))
const int BOSS_BONUS = 250;
const int FINAL_BOSS_BONUS = 500;

// The counts' waits, in seconds
const float FIRST_FISH_WAIT = 1.3f;         // before the first slot (0x53c0ec (feedingfrenzy2))
const float NEXT_WAIT = 0.5f;               // before each next step (0x5019b3, 0x501996 and others)
const float FOOD_BANK_STEP_WAIT = 0.05f;    // between the food bank's steps (0x53e459 (feedingfrenzy2))
const float RANK_WAIT = 0.97f;              // from the rank's header to the rank
const float BARRACUDA_WAIT = 0.5f;          // before the barracudas' bonus (0x516572 (feedingfrenzy2))
const float BARRACUDA_GROW_SECONDS = 0.2f;  // its growing in, from twice its size
const double BARRACUDA_GROW_DIVISOR = (double)BARRACUDA_GROW_SECONDS;
const float BARRACUDA_START_SCALE = 2.0f;
const float FUN_FACT_FADE_SECONDS = 0.3f;   // the fun fact's fading in (0x53e2b1 (feedingfrenzy2))
const double FUN_FACT_SOUND_AT = 0.5;       // its sound, halfway in

// The food bank counts by one when this close to its total, else by ten (0x53db1c (feedingfrenzy2))
const int FOOD_BANK_FINE_STEPS = 11;
const int FOOD_BANK_COARSE_STEP = 10;

// A key of the float interpolators (0x484300 (feedingfrenzy2)): a straight line
FloatKey floatKey(float theTime, float theValue)
{
	FloatKey aKey;
	aKey.mTime = theTime;
	aKey.mValue = theValue;
	return aKey;
}

// 0x49b880 (feedingfrenzy2): the players have one of the alien's name (the strings' alienUser) at a place after the
// first
bool hasAlienUser()
{
	return 0 < UserManager::instance()->findUser(getApp()->mStrings->getString("alienUser"));
}

// 0x53de33 (feedingfrenzy2): a player of the alien's name added (which makes it the current one), then the player that
// was current made so again
void addAlienUser()
{
	Ref<UserInfo> anAlien = new UserInfo();
	anAlien->setName(getApp()->mStrings->getString("alienUser"));
	UserManager* aUsers = UserManager::instance();
	std::string aCurrentName = getApp()->mUserData->getName();
	aUsers->addUser(anAlien.get());
	aUsers->setCurrentUser(aUsers->findUser(aCurrentName));
}

} // namespace

// The fun fact (or, after the final boss, its group) fading in (StageCompleteScreenNormal::ShowFunFactState, 0x98
// bytes, its constructor 0x52d834 (feedingfrenzy2)); the count is over when it is in
class StageCompleteScreenNormal::ShowFunFactState : public CountState
{
public:
	explicit ShowFunFactState(StageCompleteScreenNormal* theScreen)
	    : CountState(theScreen), mWait(0), mGroup(nullptr), isSoundPlayed(false)
	{
	}

	// 0x04 (0x53e2b1 (feedingfrenzy2)): a fact drawn into the text; after the final boss its group in the fact's
	// place and the alien added to the players unless it is there, the unlocking's text shown only then; the group
	// shown at its size, unseen, and waited for
	void enter() override
	{
		FeedingFrenzyTwo* anApp = getApp();
		mScreen->mFunFactText->setText(anApp->mFunFacts->drawFact());
		mFade.clear();
		mFade.addKey(floatKey(0.0f, 0.0f));
		mFade.addKey(floatKey(1.0f, 1.0f));
		mFadeTime.start(FUN_FACT_FADE_SECONDS, 1.0f);
		mFadeTime.setTime(0.0f);
		isSoundPlayed = false;
		if (!mScreen->mStage->isFinalBoss())
		{
			mGroup = mScreen->mFunFactGroup;
			mScreen->mFinalBossGroup->setVisible(false);
		}
		else
		{
			mGroup = mScreen->mFinalBossGroup;
			bool hasAlien = hasAlienUser();
			if (!hasAlien)
				addAlienUser();
			mScreen->mAlienUnlockText->setVisible(!hasAlien);
			mScreen->mFunFactGroup->setVisible(false);
		}
		mGroup->setScale(1.0f);
		mGroup->setOpacity(0.0f);
		mGroup->setVisible(true);
		mWait = NEXT_WAIT;
	}
	// 0x0c (0x53cca8 (feedingfrenzy2)): after the wait the group fades in, with the shell theme's sound
	// funFactDisplaySound once halfway (not yet translated, as the sounds are not); faded in, the count's machine is
	// left with no state
	void update(UpdateContext* theContext) override
	{
		if (!(mWait < 0.0f))
		{
			mWait = (float)((double)mWait - theContext->mElapsed);
			return;
		}
		mFadeTime.advance(theContext->mElapsed);
		float anOpacity = (float)mFade.getValue((float)mFadeTime.getValue());
		mGroup->setOpacity(anOpacity);
		if (!isSoundPlayed && FUN_FACT_SOUND_AT < mFadeTime.getValue())
			isSoundPlayed = true;
		if (!mFadeTime.isRunning())
			mScreen->mStateMgr.changeState(nullptr);
	}
	// 0x1c: nothing (the original's slot is an empty one)
	void skip() override {}

private:
	float mWait;                   // +0x10
	FloatKeyInterpolator mFade;    // +0x14: the opacity over the fade's time
	TimeDriver mFadeTime;          // +0x60
	Component* mGroup;             // +0x88: the fact's group, or the final boss's
	bool isSoundPlayed;            // +0x8c
};

// The food bank counted up to its total, then the rank (StageCompleteScreenNormal::FoodBankState, 0x24 bytes, its
// constructor 0x51d18a (feedingfrenzy2))
class StageCompleteScreenNormal::FoodBankState : public CountState
{
public:
	explicit FoodBankState(StageCompleteScreenNormal* theScreen)
	    : CountState(theScreen), mWait(0), mPhase(FOOD_BANK_COUNTING), mShown(0)
	{
	}

	// 0x04 (0x501996 (feedingfrenzy2)): counting from the food bank before the stage
	void enter() override
	{
		mWait = NEXT_WAIT;
		mPhase = FOOD_BANK_COUNTING;
		mShown = mScreen->mFoodBank;
	}
	// 0x0c (0x53e459 (feedingfrenzy2)): a step at each short wait's end until the total is shown; then the rank's
	// header, the rank, and the fun fact
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (mPhase == FOOD_BANK_COUNTING)
		{
			if (mWait < 0.0f)
			{
				step();
				mWait = FOOD_BANK_STEP_WAIT;
			}
			if (mShown != mScreen->mTotal)
				return;
			mPhase = FOOD_BANK_HEADER;
			mWait = NEXT_WAIT;
			return;
		}
		if (mPhase == FOOD_BANK_HEADER)
		{
			if (!(mWait < 0.0f))
				return;
			mScreen->mFoodBankRankHeader->setVisible(true);
			// The shell theme's sound drumRoll at full volume and pitch: not yet translated, as the sounds are not
			mPhase = FOOD_BANK_RANK;
			mWait = RANK_WAIT;
			return;
		}
		if (!(mWait < 0.0f))
			return;
		mScreen->mFoodBankRank->setVisible(true);
		// The shell theme's sound frenzyRankBest at full volume and pitch: not yet translated, as the sounds are not
		mScreen->mStateMgr.changeState(new ShowFunFactState(mScreen));
	}
	// 0x1c (0x53a3e3 (feedingfrenzy2)): the food bank shown, and the fun fact
	void skip() override
	{
		mScreen->finishFoodBank();
		mScreen->mStateMgr.changeState(new ShowFunFactState(mScreen));
	}

private:
	enum
	{
		FOOD_BANK_COUNTING = 0,
		FOOD_BANK_HEADER = 1,
		FOOD_BANK_RANK = 2
	};

	// 0x53db1c (feedingfrenzy2): one more, or ten more while more than ten are left (the total at once when past it),
	// with the shell theme's sound countUpClick (not yet translated, as the sounds are not), and the food bank's text
	void step()
	{
		int aLeft = mScreen->mTotal - mShown;
		if (aLeft < 0)
			mShown = mScreen->mTotal;
		else if (aLeft < FOOD_BANK_FINE_STEPS)
			mShown = mShown + 1;
		else
			mShown = mShown + FOOD_BANK_COARSE_STEP;
		mScreen->mFoodBankCount->setText(groupedNumber(mShown, true));
	}

	float mWait;   // +0x10
	int mPhase;    // +0x14
	int mShown;    // +0x18: the food bank shown
};

// The barracudas' bonus growing in (StageCompleteScreenNormal::CountBarracudaState, 0x70 bytes, its constructor
// 0x52d8a3 (feedingfrenzy2))
class StageCompleteScreenNormal::CountBarracudaState : public CountState
{
public:
	explicit CountBarracudaState(StageCompleteScreenNormal* theScreen)
	    : CountState(theScreen), mWait(0), mGrowLeft(0), isShown(false)
	{
	}

	// 0x04 (0x516572 (feedingfrenzy2)): the size from twice to once, easing in to its end
	void enter() override
	{
		mWait = BARRACUDA_WAIT;
		mGrowLeft = BARRACUDA_GROW_SECONDS;
		isShown = false;
		mScale.clear();
		mScale.addKey(floatKey(0.0f, BARRACUDA_START_SCALE));
		mScale.addKey(floatKey(1.0f, 1.0f));
		mScale.easeStretch(0, false, true, 0.0f);
	}
	// 0x0c (0x53cd72 (feedingfrenzy2)): at the wait's end the group shown unseen, then each frame more of it seen and
	// smaller, then the food bank
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (!(mWait < 0.0f))
			return;
		Component* aGroup = mScreen->mBarracudaBonusGroup;
		if (!isShown)
		{
			aGroup->setVisible(true);
			aGroup->setOpacity(0.0f);
			isShown = true;
			// The shell theme's sound fishChomp1 at full volume and a pitch of 0.8: not yet translated, as the sounds
			// are not
			return;
		}
		mGrowLeft = (float)((double)mGrowLeft - theContext->mElapsed);
		float aDone = (float)(1.0 - (double)mGrowLeft / BARRACUDA_GROW_DIVISOR);
		// (0x46d360 (feedingfrenzy2): the less of the two)
		if (1.0f <= aDone)
			aDone = 1.0f;
		aGroup->setOpacity(aDone);
		aGroup->setScale((float)mScale.getValue(aDone));
		if (1.0f <= aDone)
			mScreen->mStateMgr.changeState(new FoodBankState(mScreen));
	}
	// 0x1c (0x53a3e3 (feedingfrenzy2), the food bank's): the food bank shown, and the fun fact
	void skip() override
	{
		mScreen->finishFoodBank();
		mScreen->mStateMgr.changeState(new ShowFunFactState(mScreen));
	}

private:
	float mWait;                   // +0x10
	float mGrowLeft;               // +0x14
	bool isShown;                  // +0x18
	FloatKeyInterpolator mScale;   // +0x1c
};

// The bonuses' number (StageCompleteScreenNormal::CountBubbleState, 0x1c bytes, its constructor 0x51d1cd
// (feedingfrenzy2))
class StageCompleteScreenNormal::CountBubbleState : public CountState
{
public:
	explicit CountBubbleState(StageCompleteScreenNormal* theScreen) : CountState(theScreen), mWait(0) {}

	// 0x04 (0x5019b3 (feedingfrenzy2))
	void enter() override { mWait = NEXT_WAIT; }
	// 0x0c (0x540881 (feedingfrenzy2)): at the wait's end the bonuses' number pops in, when the stage has bonuses;
	// then the barracudas, when any were eaten, else the food bank
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (!(mWait < 0.0f))
			return;
		if (0 < mScreen->mBonusSpawns)
		{
			mScreen->popIn(mScreen->mBonusCountText, mScreen->mBonusesTaken);
			// The shell theme's sound fishChomp1 at full volume and pitch: not yet translated, as the sounds are not
		}
		if (mScreen->mBarracudaBonus < 1)
			mScreen->mStateMgr.changeState(new FoodBankState(mScreen));
		else
			mScreen->mStateMgr.changeState(new CountBarracudaState(mScreen));
	}
	// 0x1c (0x53acba (feedingfrenzy2)): the numbers left shown, and the fun fact
	void skip() override
	{
		mScreen->finishBonuses();
		mScreen->finishFoodBank();
		mScreen->finishBarracudas();
		mScreen->mStateMgr.changeState(new ShowFunFactState(mScreen));
	}

private:
	float mWait;  // +0x10
};

// The fish of each slot counted, one slot at a time (StageCompleteScreenNormal::CountFishState, 0x20 bytes, its
// constructor 0x51d210 (feedingfrenzy2))
class StageCompleteScreenNormal::CountFishState : public CountState
{
public:
	explicit CountFishState(StageCompleteScreenNormal* theScreen) : CountState(theScreen), mSlot(0), mWait(0) {}

	// 0x04 (0x53c0ec (feedingfrenzy2)): the slots filled, and the first waited for
	void enter() override
	{
		mScreen->fill();
		mSlot = 0;
		mWait = FIRST_FISH_WAIT;
	}
	// 0x0c (0x540a9f (feedingfrenzy2)): at each wait's end the next filled slot's number pops in; after the last, the
	// bonuses
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (mWait < 0.0f && mScreen->mSlotSpawns[mSlot] != nullptr)
		{
			mScreen->popIn(mScreen->mFishCounts[mSlot], mScreen->mSlotCounts[mSlot]);
			mSlot++;
			// The shell theme's sound fishChomp1 at full volume and pitch: not yet translated, as the sounds are not
			mWait = NEXT_WAIT;
		}
		if (SLOT_COUNT - 1 < mSlot || mScreen->mSlotSpawns[mSlot] == nullptr)
			mScreen->mStateMgr.changeState(new CountBubbleState(mScreen));
	}
	// 0x1c (0x53ad1c (feedingfrenzy2)): every number shown, and the fun fact
	void skip() override
	{
		mScreen->finishFish();
		mScreen->finishBonuses();
		mScreen->finishFoodBank();
		mScreen->finishBarracudas();
		mScreen->mStateMgr.changeState(new ShowFunFactState(mScreen));
	}

private:
	int mSlot;    // +0x10: the next slot to show
	float mWait;  // +0x14
};

void StageCompleteScreen::loadScreen(const std::string& thePath)
{
	load(thePath, getApp()->mShellTheme.get());
	mSplats = new SplatFactory();
	mSplats->load("resource/shell/stageCompleteSplats.xml");
	mFxGroup = new Container();
	mFxGroup->setName("FxGroup");
	addChild(mFxGroup.get());
	addChild(mSplats.get());
}

void StageCompleteScreen::enable()
{
	ScreenLayout::enable();
	getEngine()->mMouseDownSignal.connect(this, &StageCompleteScreen::mouseDownSlot);
}

void StageCompleteScreen::disable()
{
	ScreenLayout::disable();
	getEngine()->mMouseDownSignal.disconnect(this);
}

StageCompleteScreenNormal::StageCompleteScreenNormal()
    : mFoodBank(0), mTotal(0), mBonusesTaken(0), mBonusSpawns(0), mBonusCountText(nullptr),
      mBonusImageTimed(nullptr), mBonusImageNormal(nullptr), mFoodBankRankHeader(nullptr), mFoodBankCount(nullptr),
      mFoodBankRank(nullptr), mTimeAttackTimeLeft(nullptr), mTimeAttackGroup(nullptr), mFunFactGroup(nullptr),
      mFunFactText(nullptr), mFinalBossGroup(nullptr), mAlienUnlockText(nullptr), mDebugFunFactNext(nullptr),
      mDebugFunFactPrevious(nullptr), mBossLayoutGroup(nullptr), mBossBackground(nullptr), mLunchReportGroup(nullptr),
      mFoodBankBonusCount(nullptr), mBarracudaText(nullptr), mBarracudaBonusGroup(nullptr), mQuitButton(nullptr),
      mBossBonus(0), mBarracudaBonus(0)
{
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		mFishCounts[i] = nullptr;
		mFishImages[i] = nullptr;
		mSlotCounts[i] = 0;
	}
}

void StageCompleteScreenNormal::setup(const std::string& theStageId, Stage* theStage, int theFoodBank)
{
	FeedingFrenzyTwo* anApp = getApp();
	mStage = theStage;
	mFoodBank = theFoodBank;
	mTotal = theFoodBank;
	findComponents();
	// (0x5220eb (feedingfrenzy2): the stage's place at least the last's)
	StageManifest& aStages = anApp->mGameData->getStages();
	bool isLastStage = (int)aStages.getStages().size() - 1 <= aStages.findStage(theStageId);
	if (!isLastStage)
	{
		mQuitButton->addOwnState(STATE_INPUT);
		mQuitButton->setVisible(true);
	}
	else
	{
		mQuitButton->removeOwnState(STATE_INPUT);
		mQuitButton->setVisible(false);
	}
	mBossLayoutGroup->removeAllChildren();
	if (!theStage->isBoss())
	{
		mFoodBankBonusCount->setVisible(false);
		mBossBonus = 0;
		mLunchReportGroup->setVisible(true);
		mBossLayoutGroup->setVisible(false);
		mBossBackground->setVisible(false);
		mStateMgr.changeState(new CountFishState(this));
		return;
	}
	mLunchReportGroup->setVisible(false);
	mBossLayoutGroup->setVisible(true);
	mBossBackground->setVisible(true);
	// The boss's layout (a ScreenLayout, 0x17c bytes) read with the shell's theme. port: one that cannot be read is
	// let go of, where the original leaks it
	Ref<ScreenLayout> aBossLayout = new ScreenLayout();
	if (!aBossLayout->load(theStage->getBossLayout(), anApp->mShellTheme.get()))
		getEngine()->log("Boss Stage Complete: Failed to load img layout: %s\n", theStage->getBossLayout().c_str());
	else
		mBossLayoutGroup->addChild(aBossLayout.get());
	mBossBonus = theStage->isFinalBoss() ? FINAL_BOSS_BONUS : BOSS_BONUS;
	mFoodBankBonusCount->setText(localeNumber(mBossBonus));
	mFoodBankBonusCount->setVisible(true);
	fill();
	mStateMgr.changeState(new FoodBankState(this));
}

void StageCompleteScreenNormal::skip()
{
	CountState* aState = mStateMgr.getState();
	if (aState != nullptr)
		aState->skip();
}

void StageCompleteScreenNormal::findComponents()
{
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		char aName[32];
		snprintf(aName, sizeof(aName), "fishCount%d", i);
		mFishCounts[i] = static_cast<TextItem*>(findComponent(aName));
		snprintf(aName, sizeof(aName), "fishImg%d", i);
		mFishImages[i] = static_cast<Container*>(findComponent(aName));
	}
	mBonusCountText = static_cast<TextItem*>(findComponent("bonusCount"));
	mBonusImageTimed = findComponent("bonusImgTimed");
	mBonusImageNormal = findComponent("bonusImgNormal");
	mFoodBankCount = static_cast<TextItem*>(findComponent("foodBankCount"));
	mFoodBankRank = static_cast<TextItem*>(findComponent("foodBankRank"));
	mFoodBankRankHeader = findComponent("foodBankRankHeader");
	mFunFactGroup = findComponent("funFactGrp");
	mFunFactText = static_cast<TextItem*>(findComponent("funFactText"));
	mTimeAttackTimeLeft = static_cast<TextItem*>(findComponent("timeAttackTimeLeft"));
	mTimeAttackGroup = findComponent("timeAttackGroup");
	mQuitButton = findComponent("stageCompleteQuitBtn");
	mBossLayoutGroup = static_cast<Container*>(findComponent("bossLayoutGrp"));
	mLunchReportGroup = findComponent("lunchReportGrp");
	mBossBackground = findComponent("bossBkg");
	mFoodBankBonusCount = static_cast<TextItem*>(findComponent("foodBankBonusCount"));
	mBarracudaText = static_cast<TextItem*>(findComponent("barracudaText"));
	mBarracudaBonusGroup = findComponent("barracudaBonusGrp");
	mDebugFunFactNext = findComponent("debugFunFactNext");
	mDebugFunFactPrevious = findComponent("debugFunFactPrev");
	mFinalBossGroup = findComponent("finalBossGrp");
	mAlienUnlockText = findComponent("alienUnlockText");
}

void StageCompleteScreenNormal::fill()
{
	FeedingFrenzyTwo* anApp = getApp();
	int aFilled = 0;
	for (const Ref<FishSpawn>& aSpawn : mStage->getFishSpawns())
	{
		if (!aSpawn->showsOnStageComplete())
			continue;
		if (SLOT_COUNT <= aFilled)
		{
			getEngine()->log("StageCompleteScreenNormal - fish wants to be displayed in count-up screen, but all slots "
			                 "are already full.\n");
			continue;
		}
		mFishCounts[aFilled]->setVisible(true);
		mFishImages[aFilled]->removeAllChildren();
		mSlotSpawns[aFilled] = aSpawn.get();
		mSlotCounts[aFilled] = countSlot(aFilled);
		// The fish's menu picture, else the shell theme's debugMissingImg
		Surface* aPicture = aSpawn->getFishVisual()->getMenuPicture();
		if (aPicture == nullptr)
			aPicture = anApp->mShellTheme->getImage("debugMissingImg");
		mFishImages[aFilled]->addChild(new Image(aPicture));
		aFilled++;
	}
	for (int i = aFilled; i < SLOT_COUNT; i++)
	{
		mFishCounts[i]->setVisible(false);
		mFishImages[i]->removeAllChildren();
		mSlotSpawns[i] = nullptr;
		mSlotCounts[i] = 0;
	}
	countBonuses();
	mBarracudaBonus = countBarracudas();
	if (0 < mBonusSpawns)
	{
		// The normal mode's picture, else the timed mode's
		bool isNormalMode = anApp->mGameData->getMode() == GAME_MODE_NORMAL;
		mBonusImageTimed->setVisible(!isNormalMode);
		mBonusImageNormal->setVisible(isNormalMode);
	}
	else
	{
		mBonusImageTimed->setVisible(false);
		mBonusImageNormal->setVisible(false);
	}
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		mFishCounts[i]->setText(localeNumber(0));
		mFishCounts[i]->setVisible(false);
	}

	// The filled slots' groups, and the bonus' after them when the stage has bonuses, centred on the screen's middle
	// (each keeping its height)
	int aShown = aFilled + (0 < mBonusSpawns ? 1 : 0);
	float aWidth = (float)((double)aShown * SLOT_WIDTH);
	double aHalf = (double)aWidth * HALF;
	for (int i = 0; i < aFilled; i++)
	{
		char aName[32];
		snprintf(aName, sizeof(aName), "fishGrp%d", i);
		Component* aGroup = findComponent(aName);
		float aX = (float)((double)i * SLOT_WIDTH - aHalf + SLOT_CENTRE);
		if (aGroup != nullptr)
			aGroup->setPosition(aX, aGroup->getPosition().y);
	}
	if (0 < mBonusSpawns)
	{
		Component* aGroup = findComponent("bonusGrp");
		float aX = (float)((double)aFilled * SLOT_WIDTH - (double)aWidth * HALF + SLOT_CENTRE);
		if (aGroup != nullptr)
			aGroup->setPosition(aX, aGroup->getPosition().y);
	}

	mBonusCountText->setText(localeNumber(0));
	mBonusCountText->setVisible(false);
	mBarracudaText->setText(localeNumber(mBarracudaBonus));
	mBarracudaBonusGroup->setVisible(false);
	mFunFactGroup->setVisible(false);
	mFinalBossGroup->setVisible(false);

	int aFishFood = 0;
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		if (mSlotSpawns[i] != nullptr)
			aFishFood += mSlotCounts[i];
	}
	mTotal = mBossBonus + mFoodBank + aFishFood + mBonusesTaken + mBarracudaBonus;
	mFoodBankCount->setText(groupedNumber(mFoodBank, true));
	mFoodBankRank->setText(anApp->mPlayerRanks->getTitle(mTotal));
	mFoodBankRankHeader->setVisible(false);
	mFoodBankRank->setVisible(false);
	if (anApp->mGameData->getMode() == GAME_MODE_TIMED)
	{
		mTimeAttackGroup->setVisible(true);
		// (0x48c830 (feedingfrenzy2))
		mTimeAttackTimeLeft->setText(stageClock(anApp->mGameData->getTimeRollover()));
	}
	else
		mTimeAttackGroup->setVisible(false);
	mDebugFunFactNext->setVisible(false);
	mDebugFunFactNext->removeOwnState(STATE_INPUT);
	mDebugFunFactPrevious->setVisible(false);
	mDebugFunFactPrevious->removeOwnState(STATE_INPUT);
}

int StageCompleteScreenNormal::countSlot(int theSlot)
{
	FishSpawn* aSpawn = mSlotSpawns[theSlot].get();
	int aCount = aSpawn->getDeaths();
	if (aSpawn->getFoodValue() < 1)
		aCount = 0;
	int aRank = aSpawn->getRank();
	bool isInAir = aSpawn->getBehavior()->mInAir;
	for (int i = 0; i < theSlot; i++)
	{
		if (mSlotSpawns[i]->getRank() == aRank && mSlotSpawns[i]->getBehavior()->mInAir == isInAir)
			return aCount;
	}
	for (const Ref<FishSpawn>& anOther : mStage->getFishSpawns())
	{
		if (anOther->getRank() == aRank && !anOther->showsOnStageComplete() &&
		    anOther->getBehavior()->mInAir == isInAir && 0 < anOther->getFoodValue())
			aCount += anOther->getDeaths();
	}
	return aCount;
}

void StageCompleteScreenNormal::countBonuses()
{
	mBonusesTaken = 0;
	mBonusSpawns = 0;
	for (const Ref<NonPlayerSpawn>& aSpawn : mStage->getNonPlayerSpawns())
	{
		if (aSpawn->getClass()->getName().find("Bonus") != std::string::npos)
		{
			mBonusSpawns++;
			mBonusesTaken += aSpawn->getDeaths();
		}
	}
}

int StageCompleteScreenNormal::countBarracudas()
{
	int aFood = 0;
	for (const Ref<FishSpawn>& aSpawn : mStage->getFishSpawns())
	{
		if (aSpawn->getClass()->getName() == "Barracuda")
			aFood += aSpawn->getDeaths();
	}
	return aFood;
}

void StageCompleteScreenNormal::finishFish()
{
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		if (mSlotSpawns[i] == nullptr)
			return;
		mFishCounts[i]->setText(localeNumber(mSlotCounts[i]));
		mFishCounts[i]->setVisible(true);
	}
}

void StageCompleteScreenNormal::finishBonuses()
{
	if (mBonusSpawns < 1)
		return;
	mBonusCountText->setText(localeNumber(mBonusesTaken));
	mBonusCountText->setVisible(true);
}

void StageCompleteScreenNormal::finishFoodBank()
{
	mFoodBankCount->setText(groupedNumber(mTotal, true));
	mFoodBankCount->setVisible(true);
	mFoodBankRankHeader->setVisible(true);
	mFoodBankRank->setVisible(true);
}

void StageCompleteScreenNormal::finishBarracudas()
{
	if (mBarracudaBonus < 1)
		return;
	mBarracudaBonusGroup->setVisible(true);
	mBarracudaBonusGroup->setScale(1.0f);
}

void StageCompleteScreenNormal::popIn(TextItem* theText, int theNumber)
{
	Vector2f aPlace = theText->getAbsolutePosition();
	ParticleSystem* anEffect = getApp()->mShellTheme->getParticleSystem("fishCountPopIn");
	anEffect->setPosition(aPlace);
	mFxGroup->addChild(anEffect);
	anEffect->setDeleteWhenDone(true);
	anEffect->start();
	mSplats->spawn("stageCompleteChomp", aPlace.x, (float)((double)aPlace.y + CHOMP_DROP));
	theText->setText(localeNumber(theNumber));
	theText->setVisible(true);
}
