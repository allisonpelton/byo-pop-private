#include "StageIntroScreen.h"

#include "Engine.h"
#include "FeedingFrenzyTwo.h"
#include "NumberText.h"

#include <cstdio>

using namespace engine;

namespace
{

// The text of the name in the container, null for none
TextItem* findText(ScreenLayout* theLayout, const char* theName)
{
	return static_cast<TextItem*>(theLayout->findComponent(theName));
}

// A component found in the layout taking the field's place, when the layout has it
template <class T>
void takeFrom(ScreenLayout* theLayout, const char* theName, Ref<T>& theField)
{
	T* aFound = static_cast<T*>(theLayout->findComponent(theName));
	if (aFound != nullptr)
		theField = aFound;
}

} // namespace

void StageIntroScreen::showFor(const std::string& theStageId, Stage* theStage)
{
	FeedingFrenzyTwo* anApp = getApp();
	if (mTextLayout)
	{
		removeChild(mTextLayout.get());
		mTextLayout = nullptr;
	}
	if (mImageLayout)
	{
		removeChild(mImageLayout.get());
		mImageLayout = nullptr;
	}
	ScreenDataEntry* aData = anApp->getScreenData(theStageId);
	if (aData != nullptr)
	{
		loadLayout(mTextLayout, aData->mTextLayout, "text");
		loadLayout(mImageLayout, aData->mImageLayout, "img");
	}
	findComponents();

	if (aData == nullptr)
	{
		mTitle->setText("no screen data");
		mCaption->setText("no screen data");
		mPara1->setText("no screen data");
		mPara2->setText("no screen data");
	}
	else
	{
		int aStageIndex = anApp->mGameData->getStages().findStage(theStageId);
		char aTitle[256];
		snprintf(aTitle, sizeof(aTitle), "%d. %s", aStageIndex + 1, aData->mTitle.c_str());
		mTitle->setText(aTitle);
		mCaption->setText(aData->mCaption);
		mPara1->setText(aData->mPara1);
		mPara2->setText(aData->mPara2);
		if (mPara1Header)
			mPara1Header->setText(aData->mPara1Header);
		if (mPara2Header)
			mPara2Header->setText(aData->mPara2Header);
		mObjective->setText(aData->mObjective);
	}

	if (anApp->mGameData->getMode() == GAME_MODE_NORMAL || theStage->isBonus())
	{
		mTimeObjectiveGroup->setVisible(false);
		mObjective->setVisible(true);
		return;
	}
	mTimeObjectiveGroup->setVisible(true);
	mObjective->setVisible(false);
	const StageManifest::Stage* anEntry = anApp->mGameData->getStageEntry();
	if (anEntry != nullptr)
	{
		mNewTimeText->setText(stageClock(anEntry->mStageTime));
		mTotalTimeText->setText(stageClock(anApp->mGameData->getTimeRollover() + anEntry->mStageTime));
		mStartTotalTimeText->setText(stageClock(anApp->mGameData->getTimeRollover() + anEntry->mStageTime));
	}
	if (anApp->mFreshGame)
	{
		mStartingTimeObjectiveGroup->setVisible(true);
		mTimeObjectiveGroup->setVisible(false);
	}
	else
	{
		mStartingTimeObjectiveGroup->setVisible(false);
		mTimeObjectiveGroup->setVisible(true);
	}
}

void StageIntroScreen::findComponents()
{
	mTitle = findText(this, "title");
	mCaption = findText(this, "caption");
	mPara1 = findText(this, "para1");
	mPara2 = findText(this, "para2");
	mNewTimeText = findText(this, "newTimeText");
	mTotalTimeText = findText(this, "totalTimeText");
	mObjective = findText(this, "objectiveText");
	mTimeObjectiveGroup = findComponent("timeObjectiveGrp");
	mTimeObjectiveGroup->setVisible(false);
	mStartingTimeObjectiveGroup = findComponent("startingTimeObjectiveGrp");
	mStartTotalTimeText = findText(this, "startTotalTimeText");
	mStartingTimeObjectiveGroup->setVisible(false);
	mTitle->setText("");
	mCaption->setText("");
	mPara1->setText("");
	mPara2->setText("");
	mObjective->setText("");
	mNewTimeText->setText(stageClock(0));
	mTotalTimeText->setText(stageClock(0));
	mPara1Header = nullptr;
	mPara2Header = nullptr;
	if (!mTextLayout)
		return;
	ScreenLayout* aLayout = mTextLayout.get();
	takeFrom(aLayout, "title", mTitle);
	takeFrom(aLayout, "caption", mCaption);
	takeFrom(aLayout, "para1", mPara1);
	takeFrom(aLayout, "para2", mPara2);
	takeFrom(aLayout, "newTimeText", mNewTimeText);
	takeFrom(aLayout, "totalTimeText", mTotalTimeText);
	takeFrom(aLayout, "objectiveText", mObjective);
	takeFrom(aLayout, "para1Hdr", mPara1Header);
	takeFrom(aLayout, "para2Hdr", mPara2Header);
	takeFrom(aLayout, "timeObjectiveGrp", mTimeObjectiveGroup);
	takeFrom(aLayout, "startingTimeObjectiveGrp", mStartingTimeObjectiveGroup);
	takeFrom(aLayout, "startTotalTimeText", mStartTotalTimeText);
}

void StageIntroScreen::loadLayout(Ref<ScreenLayout>& theLayout, const std::string& thePath, const char* theKind)
{
	theLayout = new ScreenLayout();
	if (!theLayout->load(thePath, getApp()->mShellTheme.get()))
	{
		getEngine()->log("Failed to load %s layout: %s\n", theKind, thePath.c_str());
		return;
	}
	addChild(theLayout.get());
}
