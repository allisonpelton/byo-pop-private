#include "StoryScreen.h"
#include "App.h"
#include "PizzaFrenzy.h"

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

const char* const SOUND_CONTINUE = "menu_gradeAppears";
const char* const SOUND_ROLLUP = "menu_rollup";

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

// The modes of the level's end that the "continue" button goes through: the totals (4 to 0xb), the bonus (0xd to 0x15)
const int MODE_TOTALS_FIRST = 4;
const int MODE_TOTALS_LAST = 0xb;
const int MODE_BONUS_FIRST = 0xd;
const int MODE_BONUS_LAST = 0x15;

void playSound(const char* theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, NORMAL_VOLUME, NORMAL_PITCH);
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
StoryScreen::StoryScreen() : mGameStats(nullptr), mRollupSound(nullptr), mLevel(nullptr), mMode(MODE_RESET)
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

// port: one character (the original repeats this for each in 0x43cd40): the layout's text node of the character is copied into a text typer in the portrait, and emptied
void StoryScreen::setUpSpeaker(Speaker& theSpeaker, const char* theName, const char* theBubble, const char* theText,
                               float theWidth)
{
	theSpeaker.mPortrait = static_cast<engine::FadeContainer*>(getNode(theName));
	theSpeaker.mBubble = getNode(theBubble);
	engine::TextItem* aText = static_cast<engine::TextItem*>(getNode(theText));
	theSpeaker.mText = new engine::TextTyper();
	theSpeaker.mText->setPosition(*aText->getPosition());
	theSpeaker.mText->setFont(aText->mFont);
	theSpeaker.mText->setWrapAndTiming(theWidth, TEXT_RATE, 0);
	theSpeaker.mText->setColor(*aText->getColor());
	theSpeaker.mText->setColorMode(aText->mColorMode);
	theSpeaker.mText->setLineOffsets(0, 0);
	aText->setText("");
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

// 0x441bf0: the story's mode (2) is the one this translates; the level's end has the modes 4 to 0x15
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
	else if (MODE_SCORES <= mMode && mMode <= MODE_LAST_END)
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
			if (MODE_TOTALS_FIRST <= mMode && mMode <= MODE_TOTALS_LAST)
			{
				continueScores();
				playSound(SOUND_CONTINUE);
			}
			else if (MODE_BONUS_FIRST <= mMode && mMode <= MODE_BONUS_LAST)
			{
				continueBonus();
				playSound(SOUND_CONTINUE);
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

// The places the story's part of the screen reaches the level's end: each is empty until the level's end is translated

// 0x43e090, after the stats are kept: translated with the level's end
void StoryScreen::findTotalNodes()
{
}

// 0x43cd40, after the speakers and the motion: the upgrades' group; the nodes inside it are translated with the
// level's end
void StoryScreen::findUpgradeNodes()
{
	mLevelUpgradeGroup = getNode("levelUpgradeGroup");
}

// 0x441bf0, cases 4 to 0x15: translated with the level's end
void StoryScreen::updateLevelEnd(engine::UpdateInfo& theInfo)
{
}

// 0x441170, "showScores": translated with the level's end
void StoryScreen::showScores()
{
}

// 0x43f0f0: translated with the level's end
void StoryScreen::showFunFact()
{
}

// 0x441170, "showBonus": translated with the level's end
void StoryScreen::showBonus()
{
}

// 0x43f240, 0x43c8f0: translated with the level's end
void StoryScreen::showGameWon()
{
}

// 0x441650: translated with the level's end
void StoryScreen::continueScores()
{
}

// 0x43f7f0: translated with the level's end
void StoryScreen::continueBonus()
{
}
