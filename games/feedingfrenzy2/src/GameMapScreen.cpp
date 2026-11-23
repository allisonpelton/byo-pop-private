#include "GameMapScreen.h"

#include "DrawContext.h"
#include "FeedingFrenzyTwo.h"
#include "Modifiers.h"

#include <cstdio>

using namespace engine;

namespace
{

// The large pin's scale (0x423ac0 (feedingfrenzy2))
const float LARGE_PIN_SCALE = 0.75f;
// The hit box's button's place in the checkpoint, the small pin's and the large one's
const Vector2f HIT_BOX_POSITION(-10.0f, -5.0f);
const Vector2f LARGE_HIT_BOX_POSITION(-20.0f, -12.0f);
// The high image's pulse: a second from clear to opaque and back
const float HIGH_PULSE_SECONDS = 1.0f;
// The seconds a stage's name stays once the mouse leaves its checkpoint, and the cancel button's caption
const float STAGE_TEXT_SECONDS = 0.75f;
const float CANCEL_TEXT_SECONDS = 0.05f;
// While the mouse is on a button, the name stays
const float STAGE_TEXT_HELD = -1.0f;

const char* const CANCEL_ACTION = "gameMapCancel";
const char* const LEVEL_START_ACTION = "levelStart:%s";

// A theme's image as a component that takes no input, added to the checkpoint
Image* makePinImage(Container* theCheckpoint, Theme* theTheme, const char* theName, const char* theLargeName,
                    bool isLarge)
{
	Image* anImage = new Image(theTheme->getImage(isLarge ? theLargeName : theName));
	anImage->setTakesInput(false);
	theCheckpoint->addChild(anImage);
	return anImage;
}

// "%d. %s": the stage's place counted from 1, and its intro's title
std::string numberedTitle(int theStage, const std::string& theTitle)
{
	char aNumber[16];
	snprintf(aNumber, sizeof(aNumber), "%d. ", theStage + 1);
	return aNumber + theTitle;
}

} // namespace

void GameMapCheckpoint::setup(GameMapScreen* theMap, const std::string& theAction, bool isLarge, const Vector2f& thePin)
{
	Theme* aTheme = getApp()->mShellTheme.get();
	mMap = theMap;
	mOffImage = makePinImage(this, aTheme, "checkpointOff", "checkpointOffLg", isLarge);
	mOnImage = makePinImage(this, aTheme, "checkpointOn", "checkpointOnLg", isLarge);
	mCheckedImage = makePinImage(this, aTheme, "checkpointChecked", "checkpointCheckedLg", isLarge);
	mHighImage = makePinImage(this, aTheme, "checkpointHigh", "checkpointHighLg", isLarge);
	mHighImage->addModifier(new OpacityModifier(HIGH_PULSE_SECONDS, 0.0f, 1.0f, 0.0f));
	mOffset = aTheme->getVector("checkpointOffset");
	Surface* aHitBox = aTheme->getImage(isLarge ? "checkpointHitBoxLg" : "checkpointHitBox");
	mButton = new ImageButton();
	mButton->setImages(aHitBox, aHitBox, aHitBox);
	mButton->setPivot(PIVOT_CENTER);
	mButton->setPosition(isLarge ? LARGE_HIT_BOX_POSITION : HIT_BOX_POSITION);
	mButton->setVisible(false);
	mMap->addButton(mButton.get(), theAction, theAction);
	mButton->addListener(this, theAction);
	setPosition(Vector2f(mOffset.x + thePin.x, mOffset.y + thePin.y));
}

void GameMapCheckpoint::setState(int theState)
{
	mState = theState;
	switch (theState)
	{
	case CHECKPOINT_AHEAD:
	case CHECKPOINT_PASSED:
		mOffImage->setVisible(theState == CHECKPOINT_AHEAD);
		mOnImage->setVisible(theState == CHECKPOINT_PASSED);
		mCheckedImage->setVisible(false);
		mHighImage->setVisible(false);
		mHighImage->addOffState(STATE_ANIMATED);
		removeChild(mButton.get());
		break;
	case CHECKPOINT_CURRENT:
		mOffImage->setVisible(true);
		mOnImage->setVisible(false);
		mCheckedImage->setVisible(false);
		mHighImage->setVisible(true);
		mHighImage->removeOffState(STATE_ANIMATED);
		removeChild(mButton.get());
		break;
	case CHECKPOINT_SELECTABLE:
		addChild(mButton.get());
		mButton->setVisible(false);
		mButton->enable();
		// (the original clears the button's switched-off input bit itself too)
		mButton->removeOffState(STATE_INPUT);
		mOffImage->setVisible(false);
		mOnImage->setVisible(true);
		mHighImage->setVisible(false);
		mHighImage->addOffState(STATE_ANIMATED);
		break;
	}
}

void GameMapCheckpoint::buttonEnter(const std::string& theAction)
{
	if (mState != CHECKPOINT_SELECTABLE)
		return;
	mCheckedImage->setVisible(true);
	mOnImage->setVisible(false);
}

void GameMapCheckpoint::buttonLeave(const std::string& theAction)
{
	if (mState != CHECKPOINT_SELECTABLE)
		return;
	mCheckedImage->setVisible(false);
	mOnImage->setVisible(true);
}

void GameMapScreen::setup(StageManifest* theStages)
{
	mStages = theStages;
	if (!mCheckpoints.empty())
		clearCheckpoints();
	mCheckpointGroup = static_cast<Container*>(findComponent("checkpointGrp"));
	makeCheckpoints();
	mLoadText = static_cast<TextItem*>(findComponent("loadText"));
	mNewGameText = static_cast<TextItem*>(findComponent("newGameText"));
	mInstructText = static_cast<TextItem*>(findComponent("instructText"));
	mStageText = static_cast<TextItem*>(findComponent("stageText"));
	mCancelButton = findComponent("gameMapCancelBtn");
	mLoadBar = dynamic_cast<Image*>(findComponent("loadBar"));
	setName("GameMapScreen");
}

void GameMapScreen::makeCheckpoints()
{
	for (const StageManifest::Stage& aStage : mStages->getStages())
	{
		GameMapCheckpoint* aCheckpoint = new GameMapCheckpoint();
		char anAction[256];
		snprintf(anAction, sizeof(anAction), LEVEL_START_ACTION, aStage.mId.c_str());
		aCheckpoint->setup(this, anAction, aStage.mLargePin, aStage.mMapPos);
		aCheckpoint->setState(CHECKPOINT_AHEAD);
		mCheckpoints.push_back(aCheckpoint);
		if (aStage.mLargePin)
		{
			aCheckpoint->setScale(LARGE_PIN_SCALE);
			aCheckpoint->setFilterMode(FILTER_MODE_SMOOTH);
		}
		mCheckpointGroup->addChild(aCheckpoint);
	}
}

void GameMapScreen::clearCheckpoints()
{
	mCheckpoints.clear();
	mCheckpointGroup->removeAllChildren();
}

void GameMapScreen::showFor(bool isChoosing, int theMode)
{
	FeedingFrenzyTwo* anApp = getApp();
	int aStage = anApp->mGameData->getStage();
	int aLastStage = (int)anApp->mGameData->getStages().getStages().size() - 1;
	int aReached = anApp->mUserData->getMode(theMode).mMaxStage;
	if (aReached > aLastStage)
		aReached = aLastStage;
	ScreenDataEntry* aData = anApp->getScreenData(anApp->mGameData->getStageEntry()->mId);
	std::string aName = aData != nullptr ? numberedTitle(aStage, aData->mTitle) : "no name";
	if (!isChoosing)
	{
		mStageText->setVisible(false);
		mInstructText->setText(aName);
		mInstructText->setVisible(true);
		mNewGameText->setVisible(false);
		mLoadText->setVisible(true);
		mLoadText->setText(anApp->mStrings->getString("mapNowLoadingPrompt"));
		mCancelButton->setVisible(false);
		mCancelButton->setTakesInput(false);
		for (int i = 0; i < (int)mStages->getStages().size(); i++)
		{
			if (i == aStage)
				mCheckpoints[i]->setState(CHECKPOINT_CURRENT);
			else
				mCheckpoints[i]->setState(i < aStage ? CHECKPOINT_PASSED : CHECKPOINT_AHEAD);
		}
	}
	else
	{
		mStageText->setVisible(true);
		mStageText->setText("");
		mInstructText->setVisible(true);
		mInstructText->setText(anApp->mStrings->getString("mapSelectInstruct"));
		mNewGameText->setVisible(true);
		mLoadText->setVisible(false);
		mCancelButton->setVisible(true);
		mCancelButton->setTakesInput(true);
		for (int i = 0; i < (int)mStages->getStages().size(); i++)
		{
			if (i <= aReached && aReached > 0)
				mCheckpoints[i]->setState(CHECKPOINT_SELECTABLE);
			else
				mCheckpoints[i]->setState(CHECKPOINT_AHEAD);
		}
	}
	mLoadBar->setSource(RectI(0, 0, 0, mLoadBar->getHeight()));
}

void GameMapScreen::update(UpdateContext& theContext)
{
	GenericScreen::update(theContext);
	if (!(0.0f < mStageTextTime))
		return;
	mStageTextTime = mStageTextTime - theContext.mElapsed;
	if (mStageTextTime < 0.0f)
	{
		mInstructText->setVisible(true);
		mStageText->setText("");
	}
}

void GameMapScreen::buttonEnter(const std::string& theAction)
{
	FeedingFrenzyTwo* anApp = getApp();
	if (theAction == CANCEL_ACTION)
	{
		mStageText->setText(anApp->mStrings->getString("gameMapCancelCaption"));
		mInstructText->setVisible(false);
		mStageTextTime = STAGE_TEXT_HELD;
		return;
	}
	std::string anId = theAction.substr(theAction.find(':') + 1);
	std::string aName;
	if (anApp->getScreenData(anId) == nullptr)
		aName = anId;
	else
	{
		int aStage = anApp->mGameData->getStages().findStage(anId);
		aName = numberedTitle(aStage, anApp->getScreenData(anId)->mTitle);
	}
	mStageTextTime = STAGE_TEXT_HELD;
	mStageText->setText(aName);
	mInstructText->setVisible(false);
}

void GameMapScreen::buttonLeave(const std::string& theAction)
{
	if (theAction != CANCEL_ACTION)
	{
		mStageTextTime = STAGE_TEXT_SECONDS;
		addOwnState(STATE_ANIMATED);
	}
	else
		mStageTextTime = CANCEL_TEXT_SECONDS;
}
