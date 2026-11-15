#include "StoryScreen.h"
#include "App.h"
#include "FrenzyName.h"
#include "PizzaFrenzy.h"
#include "Players.h"
#include "Random.h"
#include "ScreenNodes.h"
#include "TileManifest.h"

using namespace pizzafrenzy;

namespace
{
// The actions the screen takes for its own or sends on
const char* const ACTION_QUIT = "quitStoryScreen";
const char* const ACTION_CONFIRM_QUIT = "confirmQuit";
const char* const ACTION_CONTINUE = "continue";
const char* const ACTION_SKIP = "skip";
const char* const ACTION_REPLAY = "replay";
const char* const ACTION_FAST_FORWARD = "fastForwardStep";
const char* const ACTION_STORY_DONE = "storyDone";
const char* const ACTION_END_GAME = "endGame";

const char* const SOUND_GRADE_APPEARS = "menu_gradeAppears";
const char* const SOUND_ROLLUP = "menu_rollup";
const char* const SOUND_CASH = "tip_cash";
const char* const SOUND_FLIP_OVER = "popup_flipover";
const char* const SOUND_COMBO_ICON = "bonus_toppingCombo";
const char* const SOUND_FRENZY_FORMAT = "frenzy%d";
const char* const SOUND_UPGRADE = "bonus_upgrade";
const char* const SOUND_FUN_FACT = "tip_couponConvert";

// The events the screen's own stories tell it
const char* const EVENT_SHOW_SPEAKER = "showSpeaker";
const char* const EVENT_HIDE_SPEAKER = "hideSpeaker";

// The speakers' nodes (the portrait, its bubble and the text in it), and the width their text breaks at
const char* const NAME_PAULA = "Paula";
const char* const NAME_LISA = "Lisa";
const char* const NAME_LORENZO = "Lorenzo";
const char* const NAME_NICCOLO = "Niccolo";
const float TEXT_WIDTH = 360.0f;          // 0x43b40000
const float TEXT_WIDTH_LORENZO = 395.0f;  // 0x43c58000
const float TEXT_RATE = 120.0f;           // the letters each second

// The speaker's portrait fades in and out in this long
const float SPEAKER_FADE_SECONDS = 0.5f;
const float SPEAKER_FADE_TARGET = 1.0f;

// A sound plays at its own volume and pitch
const float NORMAL_VOLUME = 1.0f;
const float NORMAL_PITCH = 1.0f;

// The pulldown screen and the blackboard come in over this long, and the places they go through: the first key is
// outside the screen (the pulldown 400 above its place, the blackboard 670 to the right of it), the second a little
// past the place (20 under it, 20 to the left of it) at 0.75 of the way, the third its place
const float MOTION_SECONDS = 0.4f;
const float MOTION_TOP = 1.0f;
const float PULLDOWN_START_ABOVE = 400.0f;
const float PULLDOWN_OVERSHOOT = 20.0f;
const float BLACKBOARD_START_RIGHT = 670.0f;
const float BLACKBOARD_OVERSHOOT = 20.0f;
const float MOTION_OVERSHOOT_TIME = 0.75f;
const float MOTION_END_TIME = 1.0f;
const float KEY_IN_SPEED = 1.0f;

// The keys: Escape skips a story that can be skipped; F, with the debug flag on, shows the coupons' fun fact
const int KEY_ESCAPE = 0x1b;
const int KEY_F_UPPER = 0x46;
const int KEY_F_LOWER = 0x66;

// The player's parameter of the orders served, and what a money amount starts with
const char* const PARAM_FOOD_BANK = "foodBank";
const char* const MONEY_PREFIX = "$";

// The level's end: how long it waits between two of its steps (0x3e99999a, 0x3f000000, 0x3e6147ae, the combo's
// pictures), and what the first summary sets for a gap that nothing reads (0x3e19999a)
const float ROW_DELAY = 0.3f;
const float PART_DELAY = 0.5f;
const float COMBO_DELAY = 0.22f;
const float COUNT_INTERVAL = 0.15f;

// The numbers a second counts up: the sales bonus, the orders served and the bonus round's total
const float SALES_COUNT_RATE = 2000.0f;
const float SERVED_COUNT_RATE = 100.0f;
const float BONUS_COUNT_RATE = 1000.0f;

// The orders delivered as delivered/ordered
const char* const FORMAT_ORDERS = "%d/%d";

// The rating's stars: five for each hundred of the happiness (one for each 20), a half star for a rest of 10 or more,
// each worth points in the score, in a row going left with a gap between them
const char* const STAR_FULL_IMAGE = "res\\hud\\star-full.jpg";
const char* const STAR_HALF_IMAGE = "res\\hud\\star-half.jpg";
const int PIVOT_CENTRE = 1;
const int STARS_PER_HUNDRED = 5;
const int HAPPINESS_PER_STAR = 20;
const int HALF_STAR_HAPPINESS = 10;
const int STAR_POINTS = 500;
const int HALF_STAR_POINTS = 250;
const float STAR_GAP = 5.0f;

// The longest combo's pictures: each is worth points, and the gap to the next is a random number from 4 to 7. Each
// shown picture raises the sound's pitch, and every fifth is a frenzy, with a sound of its own (the fifth and above
// share one) and more points
const int COMBO_ICON_POINTS = 100;
const int COMBO_GAP_LEAST = 4;
const int COMBO_GAP_MOST = 7;
const float COMBO_PITCH_STEP = 0.05f;
const int COMBO_PER_FRENZY = 5;
const int MAX_FRENZY_SOUND = 5;
const int FRENZY_POINTS = 300;

// The bonus round's report: what each of the toppings' counts is worth, and each second left
const int PERFECT_VALUE = 100;
const int GOOD_VALUE = 20;
const int OKAY_VALUE = 5;
const int MISSED_VALUE = 50;
const int SECOND_VALUE = 100;

// The upgrade's text breaks at this width, its group fades in in this long, and the first upgrade of the game waits
// this long before Lisa tells it
const float DESCRIPTION_WIDTH = 290.0f;
const float UPGRADE_FADE_SECONDS = 0.3f;
const float UPGRADE_FADE_TARGET = 1.0f;
const float UPGRADE_PAUSE_SECONDS = 1.0f;

// The texts of the string table: the title the player has now (with the title), and the first upgrade of the game
// (with the topping's name; the concentration mode has its own)
const int STRING_NEW_TITLE = 0xe7;
const int STRING_FIRST_UPGRADE = 0xea;
const int STRING_FIRST_UPGRADE_CONCENTRATION = 0xeb;

void playSound(const std::string& theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, NORMAL_VOLUME, NORMAL_PITCH);
}

// A text node of the screen's layout
engine::TextItem* getText(engine::ScreenLayout* theScreen, const char* theName)
{
	return static_cast<engine::TextItem*>(theScreen->getNode(theName));
}

// A text typer that takes the place of a text node of the layout (its position, font and colour), which is emptied;
// the original repeats these lines for each typer of 0x43cd40
engine::TextTyper* makeTyper(engine::TextItem* theText, float theWidth)
{
	engine::TextTyper* aTyper = new engine::TextTyper();
	aTyper->setPosition(*theText->getPosition());
	aTyper->setFont(theText->mFont);
	aTyper->setWrapAndTiming(theWidth, TEXT_RATE, 0);
	aTyper->setColor(*theText->getColor());
	aTyper->setColorMode(theText->mColorMode);
	aTyper->setLineOffsets(0, 0);
	theText->setText("");
	return aTyper;
}

// A key of a motion: a time, a place, and the speed the curve meets it with
void addMotionKey(engine::PositionInterpolator& theMotion, float theTime, float theX, float theY)
{
	engine::PositionKey aKey;
	aKey.mTime = theTime;
	aKey.mValue = engine::Point(theX, theY);
	aKey.mInTime = 0;
	aKey.mInSpeed = KEY_IN_SPEED;
	aKey.mOutTime = 0;
	aKey.mOutSpeed = 0;
	theMotion.addKey(aKey);
}
}

// 0x442f50
StoryScreen::StoryScreen() :
	mRollupSound(nullptr), mGameStats(nullptr), mTimer(0), mCountInterval(0), mStarsShown(0), mBonusLeft(0),
	mTimeBonus(0), mBonusAmount(0), mScoreShown(0), mServedLeft(0), mServedTotal(0), mLongestComboCount(0),
	mComboShown(0), mFoodBankCount(0), mLevel(nullptr), mMode(MODE_RESET)
{
}

// 0x441bd0, 0x4408a0
StoryScreen::~StoryScreen()
{
}

// 0x43c9e0
void StoryScreen::resetTransitions()
{
	reset();
	hideUpgrades();
	GenericScreen::resetTransitions();
}

// 0x43c490
void StoryScreen::show()
{
	GenericScreen::show();
	mMode = MODE_SHOWN;
}

// 0x43c4b0
void StoryScreen::hide()
{
	GenericScreen::hide();
	mMode = MODE_HIDDEN;
}

// 0x43cd10
void StoryScreen::setStory(const std::string& theStoryFile)
{
	mStoryFile = theStoryFile;
	buildStory();
}

// port: one character (the original repeats this for each in 0x43cd40): the layout's text node of the character is
// copied into a text typer in the portrait
void StoryScreen::setUpSpeaker(Speaker& theSpeaker, const char* theName, const char* theBubble, const char* theText,
                               float theWidth)
{
	theSpeaker.mPortrait = static_cast<engine::FadeContainer*>(getNode(theName));
	theSpeaker.mBubble = getNode(theBubble);
	theSpeaker.mText = makeTyper(getText(this, theText), theWidth);
	theSpeaker.mPortrait->addChild(theSpeaker.mText);
}

// 0x43cd40
void StoryScreen::setup()
{
	mPulldownGroup = getNode("pulldownScreenGroup");
	mBlackboardGroup = getNode("blackboardGroup");
	mPulldown = static_cast<engine::Container*>(getNode("pulldownScreen"));
	mBlackboard = static_cast<engine::Container*>(getNode("blackboard"));
	mQuitButtonGroup = getNode("quitButtonGroup");
	mQuitButton = getNode("quitButton");
	mContinueButtonGroup = getNode("continueButtonGroup");
	mContinueButton = getNode("continueButton");
	setUpSpeaker(mSpeakers[SPEAKER_PAULA], NAME_PAULA, "PaulaBubble", "paulaText", TEXT_WIDTH);
	setUpSpeaker(mSpeakers[SPEAKER_LISA], NAME_LISA, "LisaBubble", "lisaText", TEXT_WIDTH);
	setUpSpeaker(mSpeakers[SPEAKER_LORENZO], NAME_LORENZO, "LorenzoBubble", "lorenzoText", TEXT_WIDTH_LORENZO);
	setUpSpeaker(mSpeakers[SPEAKER_NICCOLO], NAME_NICCOLO, "NiccoloBubble", "niccoloText", TEXT_WIDTH);
	mSkipString = getNode("skipString");
	setUpMotion();
	mMode = MODE_RESET;
	findUpgradeNodes();
	mRollupSound = PizzaFrenzy::instance()->mSounds->getVoice(SOUND_ROLLUP);
	if (mRollupSound != nullptr)
		mRollupSound->setLoop(true);
	addOwnFlags(engine::NODE_MOUSE);
}

// 0x43e090
void StoryScreen::setGameStats(GameStats* theStats)
{
	mGameStats = theStats;
	findTotalNodes();
}

// 0x43ed20
void StoryScreen::reset()
{
	mPulldownRamp.reset();
	mPulldownRamp.stop();
	mBlackboardRamp.reset();
	mBlackboardRamp.stop();
	for (int i = 0; i < SPEAKER_COUNT; i++)
	{
		mSpeakers[i].mText->reset();
		mSpeakers[i].mText->stop();
	}
	for (int i = 0; i < SPEAKER_COUNT; i++)
		mSpeakers[i].mPortrait->setVisible(false);
	mPulldown->removeAllChildren();
	mBlackboard->removeAllChildren();
	mSkipString->setVisible(true);
	mQuitButtonGroup->setVisible(false);
	mQuitButton->setEnabled(false);
	setCancelAction(ACTION_STORY_DONE);
	applyMotion();
	mMode = MODE_RESET;
}

// 0x43c8d0
void StoryScreen::startStory()
{
	mSequence->start();
	mMode = MODE_STORY;
}

// 0x43ee60
void StoryScreen::buildStory()
{
	if (mSequence != nullptr)
	{
		mSequence->stop();
		mSequence = nullptr;
	}
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<StorySequenceDocHandler> aHandler = new StorySequenceDocHandler("story", aStack);
	aStack->addHandler(aHandler);
	engine::Ref<engine::InputStream> aStream = engine::App::instance()->openStream(mStoryFile.c_str());
	engine::parseXml(*aStack, aStream);
	mSequence = aHandler->mSequence;
	mSequence->setScreen(this);
}

// 0x441bf0: the story's mode (2), and the level's end's modes 4 to 0x15 (updateLevelEnd)
void StoryScreen::updateScreen(engine::UpdateInfo& theInfo)
{
	if (mMode == MODE_STORY)
	{
		mPulldownRamp.update(theInfo.mSeconds);
		mBlackboardRamp.update(theInfo.mSeconds);
		applyMotion();
		if (mSequence != nullptr)
		{
			mSequence->update(theInfo);
			if (mSequence->isDone())
				screenAction(mCancelAction);
		}
	}
	else if (MODE_SHOW_REVENUE <= mMode && mMode <= MODE_LAST_END)
	{
		updateLevelEnd(theInfo);
	}
	GenericScreen::update(theInfo);
}

// 0x43c550
void StoryScreen::setUpMotion()
{
	mPulldownRamp.set(MOTION_SECONDS, MOTION_TOP);
	mBlackboardRamp.set(MOTION_SECONDS, MOTION_TOP);

	engine::Point* aPulldown = mPulldownGroup->getPosition();
	float aX = aPulldown->x;
	float aY = aPulldown->y;
	mPulldownMotion.clear();
	addMotionKey(mPulldownMotion, 0, aX, aY - PULLDOWN_START_ABOVE);
	addMotionKey(mPulldownMotion, MOTION_OVERSHOOT_TIME, aX, aY + PULLDOWN_OVERSHOOT);
	addMotionKey(mPulldownMotion, MOTION_END_TIME, aX, aY);

	engine::Point* aBlackboard = mBlackboardGroup->getPosition();
	aX = aBlackboard->x;
	aY = aBlackboard->y;
	mBlackboardMotion.clear();
	addMotionKey(mBlackboardMotion, 0, aX + BLACKBOARD_START_RIGHT, aY);
	addMotionKey(mBlackboardMotion, MOTION_OVERSHOOT_TIME, aX - BLACKBOARD_OVERSHOOT, aY);
	addMotionKey(mBlackboardMotion, MOTION_END_TIME, aX, aY);
}

// 0x43c870
void StoryScreen::applyMotion()
{
	mPulldownMotion.modify(mPulldownRamp.getProgress(), mPulldownGroup);
	mBlackboardMotion.modify(mBlackboardRamp.getProgress(), mBlackboardGroup);
}

// 0x43ca70
void StoryScreen::endStory()
{
	mPulldown->removeAllChildren();
	mBlackboard->removeAllChildren();
	if (mRollupSound != nullptr && mRollupSound->isPlaying())
		mRollupSound->stop();
	if (mSequence != nullptr)
		mSequence->clear();
	reset();
	hideUpgrades();
}

// 0x442ca0: the story's buttons are met here, the sequence is told what the player pressed, and the action goes on to
// the application's state
void StoryScreen::screenAction(std::string theAction)
{
	if (theAction.compare(ACTION_QUIT) == 0)
	{
		if (mRollupSound != nullptr && mRollupSound->isPlaying())
			mRollupSound->stop();
		PizzaFrenzy::instance()->screenAction(ACTION_CONFIRM_QUIT);
	}
	else if (theAction.compare(ACTION_CONTINUE) == 0)
	{
		if (mSequence != nullptr)
		{
			mSequence->action(ACTION_FAST_FORWARD);
			if (MODE_SHOW_REVENUE <= mMode && mMode <= MODE_SHOW_COMBO)
			{
				continueScores();
				playSound(SOUND_GRADE_APPEARS);
			}
			else if (MODE_BONUS <= mMode && mMode <= MODE_LAST_END)
			{
				continueBonus();
				playSound(SOUND_GRADE_APPEARS);
			}
		}
	}
	else if (theAction.compare(ACTION_SKIP) == 0)
	{
		reset();
		GenericScreen::screenAction(ACTION_STORY_DONE);
	}
	else if (theAction == ACTION_REPLAY)
	{
		reset();
		mSequence->start();
	}
	if (mSequence != nullptr)
		mSequence->action(theAction);
	engine::App::instance()->log(("layout action: " + theAction + "\n").c_str());
	GenericScreen::screenAction(theAction);
}

// 0x4414f0
void StoryScreen::keyAction(int theKey)
{
	if (mSequence == nullptr)
		return;
	if ((theKey == KEY_F_UPPER || theKey == KEY_F_LOWER) && PizzaFrenzy::sDesignerUnlockOverride)
	{
		showFunFact();
		return;
	}
	if (theKey == KEY_ESCAPE && mSkipString->isVisible())
	{
		screenAction(mCancelAction);
		return;
	}
	mSequence->action(ACTION_FAST_FORWARD);
	mSequence->action(ACTION_CONTINUE);
}

// 0x43cae0
engine::TextTyper* StoryScreen::showSpeaker(const std::string& theSpeaker)
{
	for (int i = 0; i < SPEAKER_COUNT; i++)
	{
		mSpeakers[i].mBubble->setVisible(false);
		mSpeakers[i].mText->setVisible(false);
	}
	int aSpeaker = -1;
	if (theSpeaker.compare(NAME_LORENZO) == 0)
		aSpeaker = SPEAKER_LORENZO;
	else if (theSpeaker.compare(NAME_LISA) == 0)
		aSpeaker = SPEAKER_LISA;
	else if (theSpeaker.compare(NAME_PAULA) == 0)
		aSpeaker = SPEAKER_PAULA;
	else if (theSpeaker == NAME_NICCOLO)
		aSpeaker = SPEAKER_NICCOLO;
	if (aSpeaker < 0)
		return nullptr;
	mSpeakers[aSpeaker].mBubble->setVisible(true);
	mSpeakers[aSpeaker].mText->setVisible(true);
	return mSpeakers[aSpeaker].mText;
}

// 0x441170
void StoryScreen::handleEvent(const std::string& theEvent, const std::string& theParam)
{
	if (theEvent.compare("showSpeaker") == 0)
	{
		engine::FadeContainer* aPortrait = static_cast<engine::FadeContainer*>(getNode(theParam));
		if (aPortrait != nullptr)
			aPortrait->fade(true, SPEAKER_FADE_SECONDS, SPEAKER_FADE_TARGET, false);
		engine::Node* aBubble = getNode(theParam + "Bubble");
		if (aBubble != nullptr)
			aBubble->setVisible(false);
		engine::Node* aText = getNode(theParam + "Text");
		if (aText != nullptr)
			aText->setVisible(false);
	}
	else if (theEvent.compare("hideSpeaker") == 0)
	{
		engine::FadeContainer* aPortrait = static_cast<engine::FadeContainer*>(getNode(theParam));
		if (aPortrait != nullptr)
			aPortrait->fade(false, SPEAKER_FADE_SECONDS, SPEAKER_FADE_TARGET, false);
	}
	else if (theEvent.compare("showScreen") == 0)
	{
		mPulldownRamp.setForward(true);
		mPulldownRamp.start();
	}
	else if (theEvent == "hideScreen")
	{
		mPulldown->removeAllChildren();
		mPulldownRamp.setForward(false);
	}
	else if (theEvent == "showBlackboard")
	{
		mBlackboardRamp.setForward(true);
		mBlackboardRamp.start();
	}
	else if (theEvent == "hideBlackboard")
	{
		mBlackboardRamp.setForward(false);
	}
	else if (theEvent == "showScores")
	{
		showScores();
	}
	else if (theEvent == "showFunFact")
	{
		showFunFact();
	}
	else if (theEvent == "showBonus")
	{
		showBonus();
	}
	else if (theEvent == "hideUpgrades")
	{
		hideUpgrades();
	}
	else if (theEvent == "showClickToContinue")
	{
		// nothing: the original's event does nothing
	}
	else if (theEvent == "hideClickToContinue")
	{
		// nothing
	}
	else if (theEvent == "showQuitButton")
	{
		showQuitButton(true);
	}
	else if (theEvent == "hideQuitButton")
	{
		showQuitButton(false);
	}
	else if (theEvent == "gameWon")
	{
		showGameWon();
	}
	else if (theEvent == "endGame")
	{
		screenAction(ACTION_END_GAME);
	}
	else if (theEvent == "setCancelAction")
	{
		mCancelAction = theParam;
	}
}

// 0x43c920
void StoryScreen::showQuitButton(bool theShow)
{
	mQuitButtonGroup->setVisible(theShow);
	mQuitButton->setEnabled(theShow);
}

// 0x43e090, after the stats are kept
void StoryScreen::findTotalNodes()
{
	mStatsGroup = getNode("stats");
	mActualRevenue = getText(this, "actualRevenue");
	mSalesTarget = getText(this, "salesTarget");
	mBonusSales = getText(this, "bonusSales");
	mTotalScore = getText(this, "totalScore");
	mOrdersDelivered = getText(this, "ordersDelivered");
	mFoodBank = getText(this, "foodBank");
	mServiceRating = static_cast<engine::Container*>(getNode("serviceRating"));
	mComboGroup = static_cast<engine::Container*>(getNode("comboGroup"));
	mLongestCombo = getText(this, "longestCombo");
	mBonusStats = getNode("bonusStats");
	mPerfectToppings = getText(this, "perfectToppings");
	mToppingsFormat = mPerfectToppings->mText;
	mGoodToppings = getText(this, "goodToppings");
	mOkayToppings = getText(this, "okayToppings");
	mMissedToppings = getText(this, "missedToppings");
	mToppingBonus = getText(this, "toppingBonus");
	mTimeLeft = getText(this, "timeLeft");
	mTimeLeftBonus = getText(this, "timeLeftBonus");
	mBonusTotal = getText(this, "bonusTotal");
	mBonusTotalScore = getText(this, "bonusTotalScore");
	mGameWon = getNode("gameWon");
	mTotalSales = getText(this, "totalSales");
	mElapsedTime = getText(this, "elapsedTime");
	mFinalFoodBank = getText(this, "finalFoodBank");
}

// 0x43cd40, after the speakers and the motion
void StoryScreen::findUpgradeNodes()
{
	mLevelUpgradeGroup = static_cast<engine::FadeContainer*>(getNode("levelUpgradeGroup"));
	mToppingImage = static_cast<engine::ImageNode*>(getNode("toppingImage"));
	mToppingName = getText(this, "toppingName");
	mLevelText = getText(this, "levelText");
	mToppingClass = getText(this, "toppingClass");
	mNextLevelText = getText(this, "nextLevelText");
	mLevelFormat = mLevelText->mText;
	mNextLevelFormat = mNextLevelText->mText;
	mUpgradeDescription = makeTyper(getText(this, "toppingDescription"), DESCRIPTION_WIDTH);
	mLevelUpgradeGroup->addChild(mUpgradeDescription);
}

// 0x443830
void StoryScreen::setUpSummary()
{
	mGameWon->setVisible(false);
	mBonusStats->setVisible(false);
	mStatsGroup->setVisible(true);
	mSalesTarget->setVisible(false);
	mActualRevenue->setVisible(false);
	mBonusSales->setVisible(false);
	mTotalScore->setVisible(true);
	mOrdersDelivered->setVisible(false);
	mFoodBank->setVisible(true);
	mActualRevenue->setNumber(mGameStats->mTipMoney, MONEY_PREFIX);
	mSalesTarget->setNumber(mGameStats->mLevelGoal, MONEY_PREFIX);
	mTotalScore->setNumber(mGameStats->mScore, MONEY_PREFIX);
	mScoreShown = mGameStats->mScore;
	mBonusLeft = mGameStats->mTipMoney - mGameStats->mLevelGoal;
	mBonusAmount = mBonusLeft;
	mBonusSales->setNumber(0, MONEY_PREFIX);
	mBonusSales->setColor(1.0f, 1.0f, 1.0f);
	if (0 < mBonusLeft)
		mGameStats->mScore += mBonusLeft;
	mServedLeft = mGameStats->mCustomersServed;
	mServedTotal = mServedLeft;
	engine::UserInfo* aUser = engine::Players::instance()->current();
	if (aUser != nullptr)
	{
		mFoodBankCount = (int)aUser->getParams().getInt(PARAM_FOOD_BANK, 0);
		aUser->getParams().setInt(PARAM_FOOD_BANK, mFoodBankCount + mServedLeft);
		engine::Players::instance()->saveUser(aUser);
	}
	showOrderCounts();
	setUpStars();
	setUpComboIcons();
	mTimer = 0;
	mCountInterval = COUNT_INTERVAL;
	PizzaFrenzy::instance()->saveGame();
}

// 0x4406c0
void StoryScreen::showOrderCounts()
{
	mOrdersDelivered->setText(formatText(FORMAT_ORDERS, mServedTotal - mServedLeft, mGameStats->mLevelOrders));
	mFoodBank->setNumber(mFoodBankCount, "");
}

// 0x443510
void StoryScreen::setUpStars()
{
	int aStarCount = (mGameStats->mHappiness * STARS_PER_HUNDRED) / 100;
	mServiceRating->removeAllChildren();
	mStarsShown = 0;
	mStars.clear();
	engine::Image* aStarImage = engine::App::instance()->createImage(STAR_FULL_IMAGE, false);
	aStarImage->setPivot(PIVOT_CENTRE);
	float aStarX = (float)(-aStarImage->getWidth()) * 0.5f;
	for (int i = 0; i < aStarCount; i++)
	{
		engine::Ref<engine::ImageNode> aStar = new engine::ImageNode(aStarImage);
		aStar->setPosition(aStarX, 0);
		aStar->setVisible(false);
		aStarX = aStarX - ((float)aStarImage->getWidth() + STAR_GAP);
		mServiceRating->addChild(aStar);
		mStars.push_back(aStar);
		mGameStats->mScore += STAR_POINTS;
	}
	if (mGameStats->mHappiness % HAPPINESS_PER_STAR >= HALF_STAR_HAPPINESS)
	{
		engine::Image* aHalfImage = engine::App::instance()->createImage(STAR_HALF_IMAGE, false);
		aHalfImage->setPivot(PIVOT_CENTRE);
		engine::Ref<engine::ImageNode> aHalfStar = new engine::ImageNode(aHalfImage);
		aHalfStar->setPosition(aStarX, 0);
		aHalfStar->setVisible(false);
		mServiceRating->addChild(aHalfStar);
		mStars.push_back(aHalfStar);
		mGameStats->mScore += HALF_STAR_POINTS;
	}
}

// 0x443290
void StoryScreen::setUpComboIcons()
{
	mLongestCombo->setVisible(false);
	mLongestCombo->setNumber(0, "");
	mComboGroup->removeAllChildren();
	mComboIcons.clear();
	mComboShown = 0;
	Topping* aBestTopping = mGameStats->getBestComboTopping();
	mLongestComboTopping = aBestTopping;
	mLongestComboCount = mGameStats->mBestCombos[aBestTopping];
	float anIconX = 0;
	for (int i = 0; i < mLongestComboCount; i++)
	{
		engine::Ref<engine::ImageNode> anIcon = new engine::ImageNode(mLongestComboTopping->mPreviewImage);
		anIcon->setPosition(anIconX, 0);
		anIcon->setVisible(false);
		anIconX = anIconX - (float)engine::randomRange(COMBO_GAP_LEAST, COMBO_GAP_MOST);
		mComboIcons.push_back(anIcon);
		mComboGroup->addChild(anIcon);
		mGameStats->mScore += COMBO_ICON_POINTS;
	}
}

// 0x43f3f0
void StoryScreen::setUpBonusSummary()
{
	mGameWon->setVisible(false);
	mStatsGroup->setVisible(false);
	mBonusStats->setVisible(true);
	mPerfectToppings->setVisible(false);
	mGoodToppings->setVisible(false);
	mOkayToppings->setVisible(false);
	mMissedToppings->setVisible(false);
	mToppingBonus->setVisible(false);
	mTimeLeft->setVisible(false);
	mTimeLeftBonus->setVisible(false);
	mBonusTotal->setVisible(false);
	mBonusTotalScore->setVisible(true);
	int aPerfect = mGameStats->mPerfectToppings;
	int aGood = mGameStats->mGoodToppings;
	int anOkay = mGameStats->mOkayToppings;
	int aMissed = mGameStats->mMissedToppings;
	mPerfectToppings->setText(formatText(mToppingsFormat, aPerfect, PERFECT_VALUE, aPerfect * PERFECT_VALUE));
	mGoodToppings->setText(formatText(mToppingsFormat, aGood, GOOD_VALUE, aGood * GOOD_VALUE));
	mOkayToppings->setText(formatText(mToppingsFormat, anOkay, OKAY_VALUE, anOkay * OKAY_VALUE));
	mMissedToppings->setText(formatText(mToppingsFormat, -aMissed, MISSED_VALUE, aMissed * MISSED_VALUE));
	mBonusAmount = (aGood * 4 + (aPerfect * 2 - aMissed) * 10 + anOkay) * 5;
	mToppingBonus->setNumber(mBonusAmount, MONEY_PREFIX);
	mTimeBonus = mGameStats->mBonusSecondsLeft * SECOND_VALUE;
	mTimeLeft->setTime(mGameStats->mBonusSecondsLeft, true);
	mTimeLeftBonus->setNumber(mTimeBonus, MONEY_PREFIX);
	mBonusAmount = mBonusAmount + mTimeBonus;
	if (mBonusAmount < 0)
		mBonusAmount = 0;
	mScoreShown = mGameStats->mScore;
	mBonusTotal->setNumber(0, MONEY_PREFIX);
	mBonusTotalScore->setNumber(mGameStats->mScore, MONEY_PREFIX);
	if (0 < mBonusAmount)
		mGameStats->mScore += mBonusAmount;
	mBonusLeft = mBonusAmount;
}

// 0x43f980
void StoryScreen::showUpgrade(Topping* theTopping, Upgrade* theUpgrade, int theIndex)
{
	endStory();
	mSequence = new StorySequence();
	mSequence->setScreen(this);
	mUpgradeDescription->setTypedText(theUpgrade->mDescription);
	mToppingName->setText(theUpgrade->mName);
	mToppingImage->setImage(theTopping->mImage);
	mLevelText->setText(formatText(mLevelFormat, theIndex + 1));
	mToppingClass->setText(theTopping->mDisplayName);
	if (theIndex + 1 < (int)theTopping->mUpgrades.size())
	{
		std::string aFrenzy = getFrenzyName(theTopping->mUpgrades[theIndex + 1]->mCombo);
		mNextLevelText->setText(formatText(mNextLevelFormat, aFrenzy));
		mNextLevelText->setVisible(true);
	}
	else
		mNextLevelText->setVisible(false);
	mLevelUpgradeGroup->fade(true, UPGRADE_FADE_SECONDS, UPGRADE_FADE_TARGET, false);
	mUpgradeDescription->start();
	playSound(SOUND_UPGRADE);

	// Lisa tells the first upgrade of the game: when no topping has a level over 0 yet
	bool aFirstUpgrade = true;
	for (ToppingMap<int>::iterator i = mGameStats->mToppingLevels.begin(); i != mGameStats->mToppingLevels.end(); ++i)
	{
		if (0 < i->second)
		{
			aFirstUpgrade = false;
			break;
		}
	}
	if (aFirstUpgrade)
	{
		mSequence->addPause(UPGRADE_PAUSE_SECONDS);
		mSequence->addEvent(EVENT_SHOW_SPEAKER, NAME_LISA);
		int aTextId = PizzaFrenzy::instance()->mGameMode == PizzaFrenzy::MODE_CONCENTRATION ?
			STRING_FIRST_UPGRADE_CONCENTRATION : STRING_FIRST_UPGRADE;
		mSequence->addDialog(formatMessage(engine::App::instance()->loadString(aTextId), theTopping->mDisplayName),
		                     NAME_LISA);
	}
	mSequence->addWaitForPress();
	if (aFirstUpgrade)
		mSequence->addEvent(EVENT_HIDE_SPEAKER, NAME_LISA);
}

// 0x440240
void StoryScreen::startUpgradeStory()
{
	mSequence = new StorySequence();
	mSequence->setScreen(this);
	mSequence->addEvent(EVENT_SHOW_SPEAKER, NAME_LORENZO);
	ScoreTitle* aTitle = PizzaFrenzy::instance()->mTileManifest->getTitle(mGameStats->getToppingPoints());
	mSequence->addDialog(formatMessage(engine::App::instance()->loadString(STRING_NEW_TITLE), aTitle->mText),
	                     NAME_LORENZO);
	mSequence->addWaitForPress();
}

// 0x43f0f0
void StoryScreen::showFunFact()
{
	std::string aFact = PizzaFrenzy::instance()->mTileManifest->getRandomFunFact();
	engine::TextTyper* aTyper = showSpeaker(NAME_LORENZO);
	aTyper->setTypedText(aFact);
	aTyper->reset();
	aTyper->start();
	playSound(SOUND_FUN_FACT);
}

// 0x43f240, then 0x43c8f0
void StoryScreen::showGameWon()
{
	mBonusStats->setVisible(false);
	mStatsGroup->setVisible(false);
	mGameWon->setVisible(true);
	mTotalSales->setVisible(false);
	mElapsedTime->setVisible(false);
	mFinalFoodBank->setVisible(false);
	mTotalSales->setNumber(mGameStats->mScore, MONEY_PREFIX);
	mElapsedTime->setTime((int)mGameStats->mElapsedTime, true);
	engine::UserInfo* aUser = engine::Players::instance()->current();
	if (aUser != nullptr)
		mFinalFoodBank->setNumber((int)aUser->getParams().getInt(PARAM_FOOD_BANK, 0), "");
	mTotalSales->setVisible(true);
	mElapsedTime->setVisible(true);
	mFinalFoodBank->setVisible(true);
}

// 0x441650
void StoryScreen::continueScores()
{
	mSalesTarget->setVisible(true);
	mActualRevenue->setVisible(true);
	mBonusSales->setVisible(true);
	mOrdersDelivered->setVisible(true);
	mTotalScore->setNumber(mGameStats->mScore, MONEY_PREFIX);
	if (0 < mBonusAmount)
		mBonusSales->setNumber(mBonusAmount, MONEY_PREFIX);
	engine::UserInfo* aUser = engine::Players::instance()->current();
	if (aUser != nullptr)
		mFoodBankCount = (int)aUser->getParams().getInt(PARAM_FOOD_BANK, 0);
	mServedLeft = 0;
	showOrderCounts();
	for (size_t i = 0; i < mStars.size(); i++)
		mStars[i]->setVisible(true);
	mLongestCombo->setVisible(true);
	mLongestCombo->setNumber(mLongestComboCount, "");
	for (size_t i = 0; i < mComboIcons.size(); i++)
		mComboIcons[i]->setVisible(true);
	stopRollupSound();
	mMode = MODE_STORY;
}

// 0x43f7f0
void StoryScreen::continueBonus()
{
	mPerfectToppings->setVisible(true);
	mGoodToppings->setVisible(true);
	mOkayToppings->setVisible(true);
	mMissedToppings->setVisible(true);
	mToppingBonus->setVisible(true);
	mTimeLeft->setVisible(true);
	mTimeLeftBonus->setVisible(true);
	mBonusTotal->setVisible(true);
	mBonusTotalScore->setNumber(mGameStats->mScore, MONEY_PREFIX);
	if (0 < mBonusAmount)
		mBonusTotal->setNumber(mBonusAmount, MONEY_PREFIX);
	stopRollupSound();
	mMode = MODE_STORY;
}

// 0x441bf0 (cases 7, 9 and 0x15)
void StoryScreen::countUp(int& theRest, int& theCounted, float theSeconds, float theRate)
{
	int aStep = (int)((double)theSeconds * (double)theRate);
	theRest = theRest - aStep;
	theCounted = theCounted + aStep;
	if (theRest < 0)
	{
		theCounted = theCounted + theRest;
		theRest = 0;
	}
}

// 0x441bf0: the sound of the count plays, from the start when it is not playing
void StoryScreen::keepRollupSoundPlaying()
{
	if (mRollupSound != nullptr && !mRollupSound->isPlaying())
		mRollupSound->play();
}

// 0x441bf0, 0x441650, 0x43f7f0
void StoryScreen::stopRollupSound()
{
	if (mRollupSound != nullptr && mRollupSound->isPlaying())
		mRollupSound->stop();
}

// 0x441bf0, cases 4 to 0x15. Each step shows a row or counts one, and waits before the next; the sounds are the
// game's own
void StoryScreen::updateLevelEnd(engine::UpdateInfo& theInfo)
{
	bool aWaited = false;
	if (mMode != MODE_SHOW_REVENUE && mMode != MODE_SCORES_DONE && mMode != MODE_BONUS)
	{
		// every other step waits for the time its last step set
		mTimer = mTimer - theInfo.mSeconds;
		aWaited = mTimer <= 0;
	}
	switch (mMode)
	{
	case MODE_SHOW_REVENUE:
		mActualRevenue->setVisible(true);
		playSound(SOUND_CASH);
		mTimer = ROW_DELAY;
		mMode = MODE_SHOW_TARGET;
		break;
	case MODE_SHOW_TARGET:
		if (aWaited)
		{
			mSalesTarget->setVisible(true);
			playSound(SOUND_CASH);
			mTimer = ROW_DELAY;
			mMode = MODE_SHOW_BONUS_SALES;
		}
		break;
	case MODE_SHOW_BONUS_SALES:
		if (!aWaited)
			break;
		mBonusSales->setVisible(true);
		playSound(SOUND_CASH);
		mTimer = ROW_DELAY;
		mMode = 0 < mBonusLeft ? MODE_COUNT_BONUS_SALES : MODE_SHOW_ORDERS;
		break;
	case MODE_COUNT_BONUS_SALES:
		if (!aWaited)
			break;
		if (0 < mBonusLeft)
		{
			countUp(mBonusLeft, mScoreShown, theInfo.mSeconds, SALES_COUNT_RATE);
			mBonusSales->setNumber(mBonusAmount - mBonusLeft, MONEY_PREFIX);
			mTotalScore->setNumber(mScoreShown, MONEY_PREFIX);
			keepRollupSoundPlaying();
			break;
		}
		stopRollupSound();
		playSound(SOUND_CASH);
		mTimer = ROW_DELAY;
		mMode = MODE_SHOW_ORDERS;
		break;
	case MODE_SHOW_ORDERS:
		if (aWaited)
		{
			mOrdersDelivered->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mTimer = ROW_DELAY;
			mMode = MODE_COUNT_SERVED;
		}
		break;
	case MODE_COUNT_SERVED:
		if (!aWaited)
			break;
		if (mServedLeft < 1)
		{
			stopRollupSound();
			mTimer = ROW_DELAY;
			mMode = MODE_SHOW_STARS;
			break;
		}
		countUp(mServedLeft, mFoodBankCount, theInfo.mSeconds, SERVED_COUNT_RATE);
		showOrderCounts();
		keepRollupSoundPlaying();
		break;
	case MODE_SHOW_STARS:
		if (!aWaited)
			break;
		if (mStarsShown < (int)mStars.size())
		{
			int aStar = mStarsShown;
			mStarsShown = mStarsShown + 1;
			mStars[aStar]->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mTimer = ROW_DELAY;
			if (mStarsShown == (int)mStars.size() && mGameStats->mHappiness % HAPPINESS_PER_STAR >= HALF_STAR_HAPPINESS)
				mScoreShown = mScoreShown + HALF_STAR_POINTS;
			else
				mScoreShown = mScoreShown + STAR_POINTS;
			mTotalScore->setNumber(mScoreShown, MONEY_PREFIX);
		}
		else
		{
			mTimer = PART_DELAY;
			mMode = MODE_SHOW_COMBO;
			playSound(SOUND_FLIP_OVER);
		}
		break;
	case MODE_SHOW_COMBO:
		if (!aWaited)
			break;
		if (mComboShown < (int)mComboIcons.size())
		{
			mLongestCombo->setVisible(true);
			mComboIcons[mComboShown]->setVisible(true);
			float aPitch = (float)((double)mComboShown * (double)COMBO_PITCH_STEP + (double)NORMAL_PITCH);
			PizzaFrenzy::instance()->mSounds->play(SOUND_COMBO_ICON, NORMAL_VOLUME, aPitch);
			int aFrenzies = (mComboShown + 1) / COMBO_PER_FRENZY;
			if (0 < aFrenzies && (mComboShown + 1) % COMBO_PER_FRENZY == 0)
			{
				int aFrenzy = aFrenzies - 1;
				if (aFrenzy > MAX_FRENZY_SOUND)
					aFrenzy = MAX_FRENZY_SOUND;
				playSound(formatText(SOUND_FRENZY_FORMAT, aFrenzy).c_str());
				mScoreShown = mScoreShown + aFrenzies * FRENZY_POINTS;
				mTotalScore->setNumber(mScoreShown, MONEY_PREFIX);
				// the splat "cashTipSplat" of the frenzy's points over the picture (0x474a20): left out
			}
			mTimer = COMBO_DELAY;
			mLongestCombo->setNumber(mComboShown + 1, "");
			mComboShown = mComboShown + 1;
			break;
		}
		mMode = MODE_SCORES_DONE;
		mTimer = PART_DELAY;
		break;
	case MODE_SCORES_DONE:
		mMode = MODE_STORY;
		break;
	case MODE_BONUS:
		mPerfectToppings->setVisible(true);
		playSound(SOUND_GRADE_APPEARS);
		mTimer = ROW_DELAY;
		mMode = MODE_SHOW_GOOD;
		break;
	case MODE_SHOW_GOOD:
		if (aWaited)
		{
			mGoodToppings->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mTimer = ROW_DELAY;
			mMode = MODE_SHOW_OKAY;
		}
		break;
	case MODE_SHOW_OKAY:
		if (aWaited)
		{
			mOkayToppings->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mTimer = ROW_DELAY;
			mMode = MODE_SHOW_MISSED;
		}
		break;
	case MODE_SHOW_MISSED:
		if (aWaited)
		{
			mMissedToppings->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mMode = MODE_SHOW_TOPPING_BONUS;
			mTimer = PART_DELAY;
		}
		break;
	case MODE_SHOW_TOPPING_BONUS:
		if (aWaited)
		{
			mToppingBonus->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mMode = MODE_SHOW_TIME_LEFT;
			mTimer = PART_DELAY;
		}
		break;
	case MODE_SHOW_TIME_LEFT:
		if (aWaited)
		{
			mTimeLeft->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mTimer = ROW_DELAY;
			mMode = MODE_SHOW_TIME_BONUS;
		}
		break;
	case MODE_SHOW_TIME_BONUS:
		if (aWaited)
		{
			mTimeLeftBonus->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mMode = MODE_SHOW_BONUS_TOTAL;
			mTimer = PART_DELAY;
		}
		break;
	case MODE_SHOW_BONUS_TOTAL:
		if (aWaited)
		{
			mBonusTotal->setVisible(true);
			playSound(SOUND_GRADE_APPEARS);
			mTimer = ROW_DELAY;
			mMode = MODE_COUNT_BONUS_TOTAL;
		}
		break;
	case MODE_COUNT_BONUS_TOTAL:
		if (!aWaited)
			break;
		if (0 < mBonusLeft)
		{
			countUp(mBonusLeft, mScoreShown, theInfo.mSeconds, BONUS_COUNT_RATE);
			mBonusTotal->setNumber(mBonusAmount - mBonusLeft, MONEY_PREFIX);
			mBonusTotalScore->setNumber(mScoreShown, MONEY_PREFIX);
			keepRollupSoundPlaying();
			break;
		}
		stopRollupSound();
		mMode = MODE_STORY;
		mTimer = PART_DELAY;
		break;
	}
}
