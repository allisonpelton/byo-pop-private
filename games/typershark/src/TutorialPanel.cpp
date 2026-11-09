#include "TutorialPanel.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "WSTimer.h"
#include "ButtonWidget.h"
#include "DialogButton.h"
#include "GameText.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "Common.h"
#include "compat2004/Compat2004.h"
#include "Res.h"

using namespace Sexy;

// 0x453ba0: the five buttons (all with alpha) get the app's Rockwell 13 font and join the panel's children; the radio
// buttons draw the inactive image and, when disabled (the one whose graph shows), the active one; the words-per-minute
// graph is the one shown at first; the page starts hidden by nobody (the StatsScreen hides it); the timer stays paused
// until the page is shown
TutorialPanel::TutorialPanel(WinWSApp* theApp) :
	TabPanel(theApp)
{
	mUser = NULL;	// port: the original leaves +0x170 unset until StatsScreen::SetUser
	mNextButton = new DialogButton(mApp->mImages[IMAGE_TAB_BUTTON], NEXT_BUTTON_ID, this);
	mNextButton->mHasAlpha = true;
	mPrevButton = new DialogButton(mApp->mImages[IMAGE_TAB_BUTTON], PREV_BUTTON_ID, this);
	mPrevButton->mHasAlpha = true;
	mWPMButton = new ButtonWidget(WPM_BUTTON_ID, this);
	mWPMButton->mHasAlpha = true;
	mAccuracyButton = new ButtonWidget(ACCURACY_BUTTON_ID, this);
	mAccuracyButton->mHasAlpha = true;
	mAdjustedButton = new ButtonWidget(ADJUSTED_BUTTON_ID, this);
	mAdjustedButton->mHasAlpha = true;

	mNextButton->SetFont(mApp->mFontRockwell13);
	mPrevButton->SetFont(mApp->mFontRockwell13);
	mWPMButton->SetFont(mApp->mFontRockwell13);
	mAccuracyButton->SetFont(mApp->mFontRockwell13);
	mAdjustedButton->SetFont(mApp->mFontRockwell13);

	mWPMButton->mButtonImage = mApp->mImages[IMAGE_TAB_RADIOBTN_INACTIVE];
	mWPMButton->mDisabledImage = mApp->mImages[IMAGE_TAB_RADIOBTN_ACTIVE];
	mWPMButton->SetDisabled(true);
	mAdjustedButton->mButtonImage = mApp->mImages[IMAGE_TAB_RADIOBTN_INACTIVE];
	mAdjustedButton->mDisabledImage = mApp->mImages[IMAGE_TAB_RADIOBTN_ACTIVE];
	mAccuracyButton->mButtonImage = mApp->mImages[IMAGE_TAB_RADIOBTN_INACTIVE];
	mAccuracyButton->mDisabledImage = mApp->mImages[IMAGE_TAB_RADIOBTN_ACTIVE];

	Widget* aChild = mNextButton;
	mChildren.push_back(aChild);
	aChild = mPrevButton;
	mChildren.push_back(aChild);
	aChild = mWPMButton;
	mChildren.push_back(aChild);
	aChild = mAccuracyButton;
	mChildren.push_back(aChild);
	aChild = mAdjustedButton;
	mChildren.push_back(aChild);

	mLesson = -1;
	mGraphType = GRAPH_WPM;
	mAxisMax = 0;
	mStep = 0;
	mTickCount = 0;
	mUnitHeight = 0;
	mBar1 = 0;	// port: the original leaves the bars and values (+0x10c to +0x12c) unset until SetGraph
	mBar1Target = 0;
	mValue1 = 0;
	mBar2 = 0;
	mBar2Target = 0;
	mValue2 = 0;
	mBar3 = 0;
	mBar3Target = 0;
	mValue3 = 0;

	mTimer = new WSTimer();
	mTimer->SetDisabled(true);

	mTitle = GT(0x00530b7c);
	mHeader = GT(0x00530b60);
	mCaption = "";
}

// 0x449c40: the five buttons are deleted (the strings and the TabPanel part go with the object). The original never
// deletes the timer
TutorialPanel::~TutorialPanel()
{
	unsigned int i;
	for (i = 0; i < mChildren.size(); i++)
	{
		if (mChildren[i] != NULL)
			delete mChildren[i];
	}
	delete mTimer;	// port: the original leaks the timer
}

// 0x449df0 slot 1: the timer joins the manager (so it counts), the two lesson buttons and the three radio buttons are
// placed, labelled and given the hand cursor; Next is disabled on the last lesson (index 18), Previous on lesson 0; the
// radio buttons stand in a column under each other (the image's height apart); then the buttons join the manager in
// front
void TutorialPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mTimer);

	mNextButton->Resize(0x3c, 0x4b, 0x96, 0x1b);
	mNextButton->mDoFinger = true;
	mNextButton->mLabel = GT(0x0052fedc);
	mNextButton->mDoFinger = true;
	mNextButton->SetDisabled(0x11 < mLesson);

	mPrevButton->Resize(0x3c, 0x69, 0x96, 0x1b);
	mPrevButton->mDoFinger = true;
	mPrevButton->mLabel = GT(0x0052fecc);
	mPrevButton->SetDisabled(mLesson == 0);
	mPrevButton->mDoFinger = true;

	Image* aRadioImage = mApp->mImages[IMAGE_TAB_RADIOBTN_ACTIVE];
	mWPMButton->Resize(0x3c, 0x131, aRadioImage->mWidth, aRadioImage->mHeight);
	mWPMButton->mDoFinger = true;
	mWPMButton->mLabel = GT(0x0052feb8);
	mWPMButton->mDoFinger = true;

	aRadioImage = mApp->mImages[IMAGE_TAB_RADIOBTN_ACTIVE];
	int aRadioHeight = aRadioImage->mHeight;
	mAccuracyButton->Resize(0x3c, aRadioHeight + 0x131, aRadioImage->mWidth, aRadioHeight);
	mAccuracyButton->mDoFinger = true;
	mAccuracyButton->mLabel = GT(0x0052feac);
	mAccuracyButton->SetDisabled(false);
	mAccuracyButton->mDoFinger = true;

	aRadioImage = mApp->mImages[IMAGE_TAB_RADIOBTN_ACTIVE];
	aRadioHeight = aRadioImage->mHeight;
	mAdjustedButton->Resize(0x3c, aRadioHeight * 2 + 0x131, aRadioImage->mWidth, aRadioHeight);
	mAdjustedButton->mDoFinger = true;
	mAdjustedButton->mLabel = GT(0x0052fe9c);
	mAdjustedButton->SetDisabled(false);
	mAdjustedButton->mDoFinger = true;

	unsigned int i;
	for (i = 0; i < mChildren.size(); i++)
	{
		theWidgetManager->AddWidget(mChildren[i]);
		theWidgetManager->BringToFront(mChildren[i]);
	}
}

// 0x4432e0 slot 2
void TutorialPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTimer);

	unsigned int i;
	for (i = 0; i < mChildren.size(); i++)
		theWidgetManager->RemoveWidget(mChildren[i]);
}

// 0x443330 slot 47: when the page is shown, "Previous Lesson" is there unless the lesson is 0, and "Next Lesson" unless
// the lesson is the last of the profile's records
void TutorialPanel::SetShown(bool theShow)
{
	TabPanel::SetShown(theShow);
	if (theShow)
	{
		if (mLesson == 0)
			mPrevButton->SetVisible(false);
		else
			mPrevButton->SetVisible(true);

		int aLessonCount = (int)mApp->mUser->mRecordsE0.size();
		if (aLessonCount <= mLesson + 1)
		{
			mNextButton->SetVisible(false);
			return;
		}
		mNextButton->SetVisible(true);
	}
}

// 0x4417f0 slot 17: only while the page is shown. A paused timer is started again and the update ends; otherwise, when it
// reads more than 4 milliseconds and a bar has not reached its height, all three bars grow by 20 pixels (not beyond
// their heights), the page is repainted and the timer restarts
void TutorialPanel::Update()
{
	Widget::Update();
	if (mShown)
	{
		if (mTimer->mPaused)
		{
			mTimer->SetDisabled(false);
			mTimer->Reset();
			return;
		}
		int anElapsed = mTimer->Elapsed();
		if (4 < anElapsed)
		{
			if (mBar1 < mBar1Target || mBar2 < mBar2Target || mBar3 < mBar3Target)
			{
				mBar1 = mBar1 + 0x14;
				if (mBar1Target <= mBar1)
					mBar1 = mBar1Target;
				int aBar = mBar2;
				mBar2 = aBar + 0x14;
				if (mBar2Target <= aBar + 0x14)
					mBar2 = mBar2Target;
				aBar = mBar3;
				mBar3 = aBar + 0x14;
				if (mBar3Target <= aBar + 0x14)
					mBar3 = mBar3Target;
				MarkDirtyFull();
				mTimer->Reset();
				return;
			}
		}
	}
}

// 0x44a060 slot 16: the white panel and the header; below it the caption (two lines of Rockwell 13). When all three bars
// are 0 that is all. Otherwise the axes (gold, 2 pixels), a tick line and its value for every tick, the three bars each
// with a shadow (the average blue, the best red, the most recent green, half transparent), their labels under them, and
// their values (with a percent sign for the accuracy graph) above them, at half the bar's height
void TutorialPanel::Draw(Graphics* g)
{
	if (mShown)
	{
		g->DrawImage(mApp->mImages[IMAGE_TAB_PANEL_WHITE], 0xf2, 0x3b);
		g->SetFont(mApp->mFontRockwell13Bold);
		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, mHeader, 0xf5, 0x37, 0x15e, 0, true, 0, -1);

		if (mBar1Target == 0 && mBar2Target == 0 && mBar3Target == 0)
		{
			g->SetFont(mApp->mFontRockwell13);
			Rect aRect(0xfa, 0x4f, mWidth - 0x113, g->GetFont()->GetHeight() * 2);
			WriteWordWrapped2004(g, aRect, mCaption, -1, -1);
			return;
		}

		g->SetFont(mApp->mFontRockwell13);
		Rect aRect(0xfa, 0x4f, mWidth - 0x113, g->GetFont()->GetHeight() * 2);
		WriteWordWrapped2004(g, aRect, mCaption, -1, -1);

		g->SetColor(Color(0xff, 0xcc, 0));
		g->FillRect(0x111, 0x154, 0x135, 2);
		g->FillRect(0x111, 0x154, 2, -0x10a);
		g->SetFont(mApp->mFontMyriadCondensed12Bold);

		int i = 0;
		if (0 < mTickCount)
		{
			do
			{
				g->SetColor(Color(0, 0, 0));
				int aTickY = 0x154 - mUnitHeight * i;
				g->FillRect(0x111, aTickY, 0x135, 2);
				g->SetColor(Color(0, 0, 0));
				SexyString aTickText = StrFormat(GTC(0x0052b19c), mStep * i);
				int aTextY = aTickY + (g->GetFont()->GetAscent() >> 1);
				g->DrawString(aTickText, 0x10f - g->GetFont()->StringWidth(aTickText), aTextY);
				i++;
			} while (i < mTickCount);
		}

		if (0 < mBar1)
		{
			g->SetColor(Color(0x14, 0x14, 0x14, 0x80));
			g->FillRect(0x12c, 0x159 - mBar1, 0x3c, mBar1 - 5);
			g->SetColor(Color(0, 0, 0xff, 0x80));
			g->FillRect(0x127, 0x154 - mBar1, 0x3c, mBar1);
		}
		if (0 < mBar2)
		{
			g->SetColor(Color(0x14, 0x14, 0x14, 0x80));
			g->FillRect(0x19a, 0x159 - mBar2, 0x3c, mBar2 - 5);
			g->SetColor(Color(0xff, 0, 0, 0x80));
			g->FillRect(0x195, 0x154 - mBar2, 0x3c, mBar2);
		}
		if (0 < mBar3)
		{
			g->SetColor(Color(0x14, 0x14, 0x14, 0x80));
			g->FillRect(0x208, 0x159 - mBar3, 0x3c, mBar3 - 5);
			g->SetColor(Color(0, 0xff, 0, 0x80));
			g->FillRect(0x203, 0x154 - mBar3, 0x3c, mBar3);
		}

		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, mAverageLabel, 0x127, 0x168, 0x3c, 0, true, 0, -1);
		WriteString2004(g, mBestLabel, 0x195, 0x168, 0x3c, 0, true, 0, -1);
		WriteString2004(g, mRecentLabel, 0x206, 0x168, 0x3c, 0, true, 0, -1);

		g->SetFont(mApp->mFontRockwell13Bold);
		int aHalfAscent = g->GetFont()->GetAscent() / 2;
		g->SetColor(Color(0xff, 0xff, 0xff));

		SexyString aValueText;
		if (mGraphType == GRAPH_ACCURACY)
			aValueText = StrFormat(GTC(0x0052fee8), mValue1);
		else
			aValueText = StrFormat(GTC(0x0052b19c), mValue1);
		int aValueY = aHalfAscent + 0x154 - (mBar1 >> 1);
		if (0 < aValueY)
			WriteString2004(g, aValueText, 0x127, aValueY, 0x3c, 0, true, 0, -1);

		if (mGraphType == GRAPH_ACCURACY)
			aValueText = StrFormat(GTC(0x0052fee8), mValue2);
		else
			aValueText = StrFormat(GTC(0x0052b19c), mValue2);
		aValueY = aHalfAscent + 0x154 - (mBar2 >> 1);
		if (0 < aValueY)
			WriteString2004(g, aValueText, 0x195, aValueY, 0x3c, 0, true, 0, -1);

		if (mGraphType == GRAPH_ACCURACY)
			aValueText = StrFormat(GTC(0x0052fee8), mValue3);
		else
			aValueText = StrFormat(GTC(0x0052b19c), mValue3);
		aValueY = aHalfAscent - (mBar3 >> 1) + 0x154;
		if (0 < aValueY)
			WriteString2004(g, aValueText, 0x203, aValueY, 0x3c, 0, true, 0, -1);
	}
}

// 0x450a80 (the listener's slot 4): Next and Previous change the lesson; a radio button selects its measure, shows the
// others as enabled and itself as the down one (disabled); the page is repainted for every id
void TutorialPanel::ButtonDepress(int theId)
{
	if (theId == NEXT_BUTTON_ID)
	{
		NextLesson();
		MarkDirty();
		return;
	}
	if (theId == PREV_BUTTON_ID)
	{
		PrevLesson();
		MarkDirty();
		return;
	}

	if (theId == WPM_BUTTON_ID)
	{
		mGraphType = GRAPH_WPM;
		SetGraph(GRAPH_WPM);
		mWPMButton->SetDisabled(true);
		mAccuracyButton->SetDisabled(false);
		mAdjustedButton->SetDisabled(false);
	}
	else
	{
		if (theId == ACCURACY_BUTTON_ID)
		{
			mGraphType = GRAPH_ACCURACY;
			SetGraph(GRAPH_ACCURACY);
			mWPMButton->SetDisabled(false);
			mAccuracyButton->SetDisabled(true);
			mAdjustedButton->SetDisabled(false);
			MarkDirty();
			return;
		}
		if (theId != ADJUSTED_BUTTON_ID)
		{
			MarkDirty();
			return;
		}
		mGraphType = GRAPH_ADJUSTED;
		SetGraph(GRAPH_ADJUSTED);
		mWPMButton->SetDisabled(false);
		mAccuracyButton->SetDisabled(false);
		mAdjustedButton->SetDisabled(true);
	}
	MarkDirty();
}

// 0x44a7b0: loads the graph of one measure for mLesson from the profile's record of the lesson: the axis (the best value;
// a tick of 10, or of 1 when the best is below 10; the accuracy graph is fixed at 100 with 11 ticks), the three values
// (the average of the lesson's runs, the best, the most recent) with the bars' final heights (a tick's pixels for every
// whole tick, one pixel for every unit left over), the labels, the caption (what the graph is, or that nothing was
// scored, or that the lesson is not completed) and the header. The bars restart from 0 and the page is repainted
void TutorialPanel::SetGraph(int theGraph)
{
	if (theGraph == 0)
	{
		mAxisMax = mUser->mRecordsE0[mLesson].mField[0];
		mStep = (9 < mAxisMax) ? 10 : 1;
		mTickCount = mAxisMax / mStep + 2;
		mUnitHeight = 0xd4 / mTickCount;
		int aValue = mUser->GetAverageWPM(mLesson);
		mValue1 = aValue;
		mBar1Target = mUnitHeight * (aValue / mStep) + aValue % mStep;
		aValue = mUser->mRecordsE0[mLesson].mField[0];
		mValue2 = aValue;
		mBar2Target = mUnitHeight * (aValue / mStep) + aValue % mStep;
		aValue = mUser->mRecordsE0[mLesson].mField[1];
		mValue3 = aValue;
		mBar3Target = mUnitHeight * (aValue / mStep) + aValue % mStep;

		mAverageLabel = GT(0x005301dc);
		mBestLabel = GT(0x005301d4);
		mRecentLabel = GT(0x005301c0);
		if (mBar1Target == 0 && mBar2Target == 0 && mBar3Target == 0)
			mCaption = GT(0x00530184);
		else
			mCaption = GT(0x00530118);

		if (mLesson < 1)
			mHeader = GT(0x005300dc);
		else
			mHeader = StrFormat(GTC(0x005300fc), mLesson);
	}
	else if (theGraph == 1)
	{
		mAxisMax = mUser->mRecordsE0[mLesson].mField[4];
		mStep = (9 < mAxisMax) ? 10 : 1;
		mTickCount = mAxisMax / mStep + 2;
		mUnitHeight = 0xd4 / mTickCount;
		int aValue = mUser->GetAverageAdjustedWPM(mLesson);
		mValue1 = aValue;
		mBar1Target = mUnitHeight * (aValue / mStep) + aValue % mStep;
		aValue = mUser->mRecordsE0[mLesson].mField[4];
		mValue2 = aValue;
		mBar2Target = mUnitHeight * (aValue / mStep) + aValue % mStep;
		aValue = mUser->mRecordsE0[mLesson].mField[5];
		mValue3 = aValue;
		mBar3Target = mUnitHeight * (aValue / mStep) + aValue % mStep;

		mAverageLabel = GT(0x005301dc);
		mBestLabel = GT(0x005301d4);
		mRecentLabel = GT(0x005301c0);
		if (mBar1Target == 0 && mBar2Target == 0 && mBar3Target == 0)
			mCaption = GT(0x00530098);
		else
			mCaption = GT(0x00530028);

		if (mLesson < 1)
			mHeader = GT(0x0052fff0);
		else
			mHeader = StrFormat(GTC(0x0053000c), mLesson);
	}
	else if (theGraph == 2)
	{
		mAxisMax = 100;
		mStep = 10;
		mTickCount = 0xb;
		mUnitHeight = 0x13;
		int aValue = mUser->GetAverageAccuracy(mLesson);
		mValue1 = aValue;
		mBar1Target = mUnitHeight * (aValue / mStep) + aValue % mStep;
		aValue = mUser->mRecordsE0[mLesson].mField[8];
		mValue2 = aValue;
		mBar2Target = mUnitHeight * (aValue / mStep) + aValue % mStep;
		aValue = mUser->mRecordsE0[mLesson].mField[9];
		mValue3 = aValue;
		mBar3Target = mUnitHeight * (aValue / mStep) + aValue % mStep;

		mAverageLabel = GT(0x005301dc);
		mBestLabel = GT(0x005301d4);
		mRecentLabel = GT(0x005301c0);
		if (mBar1Target == 0 && mBar2Target == 0 && mBar3Target == 0)
			mCaption = GT(0x0052ffbc);
		else
			mCaption = GT(0x0052ff48);

		if (mLesson < 1)
			mHeader = GT(0x0052ff18);
		else
			mHeader = StrFormat(GTC(0x0052ff30), mLesson);
	}

	// the app's current profile (the same one StatsScreen::SetUser gave this page)
	if (mApp->mUser->mField94[mLesson] == -1)
		mCaption = GT(0x0052fef0);

	mBar1 = 0;
	mBar2 = 0;
	mBar3 = 0;
	MarkDirtyFull();
}

// 0x44ae30: on a lesson after the current one, Previous Lesson is enabled and shown again (when the page is visible), the
// lesson moves on and, on the last one, Next Lesson is disabled and hidden; the measure's graph is loaded for it. The
// header is rebuilt in any case: "TYPING TEST: " for the lesson 0, else "LESSON n: ", and the name of the measure
void TutorialPanel::NextLesson()
{
	int aLessonCount = (int)mUser->mRecordsE0.size();
	if (mLesson + 1 < aLessonCount)
	{
		mPrevButton->SetDisabled(false);
		mPrevButton->mDoFinger = true;
		if (mVisible)
			mPrevButton->SetVisible(true);
		int aLesson = mLesson;
		mLesson = aLesson + 1;
		aLessonCount = (int)mUser->mRecordsE0.size();
		if (aLessonCount <= aLesson + 2)
		{
			mNextButton->SetDisabled(true);
			mNextButton->mDoFinger = false;
			mNextButton->SetVisible(false);
		}
		SetGraph(mGraphType);
	}

	SexyString aGraphName;
	if (mGraphType == GRAPH_WPM)
		aGraphName = GT(0x0052feb8);
	else if (mGraphType == GRAPH_ADJUSTED)
		aGraphName = GT(0x0052fe9c);
	else
		aGraphName = GT(0x0052feac);

	if (mLesson < 1)
		mHeader = GT(0x005301e4);
	else
		mHeader = StrFormat(GTC(0x005301f4), mLesson);
	mHeader.append(aGraphName);
	MarkDirtyFull();
}

// 0x44b070: the same towards the lesson before the current one: Next Lesson is enabled and shown again, and on lesson 0
// Previous Lesson is hidden and disabled
void TutorialPanel::PrevLesson()
{
	if (-1 < mLesson - 1)
	{
		mNextButton->SetDisabled(false);
		mNextButton->mDoFinger = true;
		if (mVisible)
			mNextButton->SetVisible(true);
		int aLesson = mLesson;
		mLesson = aLesson - 1;
		if (aLesson - 2 < 0)
		{
			mPrevButton->SetVisible(false);
			mPrevButton->SetDisabled(true);
			mPrevButton->mDoFinger = false;
		}
		SetGraph(mGraphType);
	}

	SexyString aGraphName;
	if (mGraphType == GRAPH_WPM)
		aGraphName = GT(0x0052feb8);
	else if (mGraphType == GRAPH_ADJUSTED)
		aGraphName = GT(0x0052fe9c);
	else
		aGraphName = GT(0x0052feac);

	if (mLesson < 1)
		mHeader = GT(0x005301e4);
	else
		mHeader = StrFormat(GTC(0x005301f4), mLesson);
	mHeader.append(aGraphName);
	MarkDirtyFull();
}

// 0x450bc0: theLesson + 1 times NextLesson (from the constructor's -1 that reaches lesson theLesson)
void TutorialPanel::AdvanceToLesson(int theLesson)
{
	if (-1 < theLesson)
	{
		int aCount = theLesson + 1;
		do
		{
			NextLesson();
			aCount = aCount - 1;
		} while (aCount != 0);
	}
}
