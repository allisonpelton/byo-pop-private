// TypingTutor.cpp: the typing tutor's life cycle, its buttons, its clock and the keys it takes (the drawing is in
// TypingTutorDraw.cpp, the keyboard highlight in TypingTutorKeys.cpp, the lesson list and its XML reader in
// TypingTutorLessons.cpp). See TypingTutor.h for what the widget is.
//
// Left out (registration, removed): the trial limit in ButtonDepress's next-lesson case (the original's MaxUnregTutorialLessons
// test and its register prompt, 0x41c1c0) and the call to 0x41d9d0 at the top of Update (the trial's play-time count).
//
// Quirks of the original that are kept: Update's creature loop has a branch for a creature above y = 160 in state 1 that the
// loop (which only runs outside state 1) never takes; KeyChar takes a CR, LF or Space typed at the first or the last character
// of a line as neither right nor wrong (only the typing sound plays and the character is not accepted), so a line that starts
// or ends with one of them cannot be finished (a line read from a text file ends with its newline; whether the shipped
// lessons have such lines is not checked); Update resets the seconds timer at each second, so the time a slow tick overshot
// is lost; a lesson that ends within its first second divides by zero in Finish (both words per minute come out 0).
// Deviations (port:): the three timers are freed with the tutor (the original leaks them); the fields the original leaves for
// ResetLesson to set start at zero; a missing data file is not guarded (the original crashes there too).

#include "TypingTutor.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "WSTimer.h"
#include "DialogButton.h"
#include "GameText.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "Common.h"
#include "Res.h"
#include "Enemy.h"
#include "BlackShark.h"
#include "BonusCreature.h"
#include "Piranha.h"
#include "RedPiranha.h"
#include "RedShark.h"
#include "ToxicShark.h"
#include "WhitePiranha.h"

using namespace Sexy;

// _ftol: the double truncated to an int; 0x80000000 when it does not fit (and for a NaN), as the x87 does
static int FToL(double theValue)
{
	if (!((theValue > -2147483649.0) && (theValue < 2147483648.0)))
		return (int) 0x80000000;
	return (int) theValue;
}

// 0x460b30: the four buttons (next, previous, options, menu; the listener is the tutor's ButtonListener part) with a dimmed copy
// of the button picture each as its disabled picture, the lesson list, three timers (seconds, cursor blink, flash after an
// error), then the lesson the user left off at: the tutor steps to it from the one before the first (a user who has only taken
// the typing test starts at lesson 1)
TypingTutor::TypingTutor(WinWSApp* theApp)
{
	// port: the original leaves these to ResetLesson, which the steps below reach; the rest it sets itself
	mTimeLeft = 0;
	mTask = 0;
	mErrors = 0;
	mCleanKeys = 0;
	mShift = 0;
	mRawWPM = 0;
	mAdjustedWPM = 0;
	mSeconds = 0;
	mCleanChar = true;
	mAccuracy = 0;
	mExpectedChar = ' ';
	mFirstLine = 0;

	// the nine finger sprites run their constructor (0x4433c0, destructor 0x4433e0); the three vectors start empty
	mApp = theApp;
	mPauseCount = 0;
	mViewStatsAsked = false;

	mNextButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], NEXT_BUTTON_ID, this);
	mPrevButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], PREV_BUTTON_ID, this);
	mOptionsButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], OPTIONS_BUTTON_ID, this);
	mMenuButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], MENU_BUTTON_ID, this);

	// 0x4abd40: a copy of the picture drawn at half opacity; the original stores it at +0x8c of each button, the disabled
	// picture of ButtonWidget (DialogButton's own picture is at +0xe0)
	mNextButton->mDisabledImage = mApp->CreateColorizedImage(mApp->mImages[IMAGE_DBUTTON], Color(255, 255, 255, 0x80));
	mPrevButton->mDisabledImage = mApp->CreateColorizedImage(mApp->mImages[IMAGE_DBUTTON], Color(255, 255, 255, 0x80));
	mOptionsButton->mDisabledImage = mApp->CreateColorizedImage(mApp->mImages[IMAGE_DBUTTON], Color(255, 255, 255, 0x80));
	mMenuButton->mDisabledImage = mApp->CreateColorizedImage(mApp->mImages[IMAGE_DBUTTON], Color(255, 255, 255, 0x80));

	mNextButton->SetFont(mApp->mFontRockwell13);
	mPrevButton->SetFont(mApp->mFontRockwell13);
	mOptionsButton->SetFont(mApp->mFontRockwell13);
	mMenuButton->SetFont(mApp->mFontRockwell13);

	mLessons = new TutorLessons(mApp);
	mLesson = NULL;

	mSecondTimer = new WSTimer();
	mBlinkTimer = new WSTimer();
	mBlinkTimer->SetDisabled(false);
	mCursorOn = true;
	mState = 0;
	mLeaveAsked = false;
	mSpareDialogFlag = false;
	mCorrectKeys = 0;
	mFlashOn = true;
	mErrorTimer = new WSTimer();
	mErrorTimer->SetDisabled(true);

	int aLesson = mApp->mUser->mLastLesson;
	if ((aLesson == 0) && (mApp->mUser->mLessonOutcome[0] == 1))
		aLesson = 1;
	if (-1 < aLesson)
	{
		int aCount = aLesson + 1;
		do
		{
			GoToNextLesson();
			aCount = aCount - 1;
		} while (aCount != 0);
	}
	if (aLesson == 0)
		mPrevButton->mVisible = false;
}

// 0x45bd30 (deleting 0x45e770): the lesson the user is at is stored in the profile (lesson 0, the typing test, counts as 1);
// the dimmed button pictures, the lesson list, the buttons and the creatures are freed
TypingTutor::~TypingTutor()
{
	int aLastLesson = mLessons->mCurrent;
	if (aLastLesson == 0)
		aLastLesson = 1;
	mApp->mUser->mLastLesson = aLastLesson;
	mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, false);

	if (mNextButton->mDisabledImage != NULL)
		delete mNextButton->mDisabledImage;
	if (mMenuButton->mDisabledImage != NULL)
		delete mMenuButton->mDisabledImage;
	if (mOptionsButton->mDisabledImage != NULL)
		delete mOptionsButton->mDisabledImage;
	if (mPrevButton->mDisabledImage != NULL)
		delete mPrevButton->mDisabledImage;

	delete mLessons;
	delete mNextButton;
	delete mOptionsButton;
	delete mMenuButton;
	delete mPrevButton;

	unsigned int aCreatureIdx = 0;
	while (aCreatureIdx < mCreatures.size())
	{
		if (mCreatures[aCreatureIdx] != NULL)
			delete mCreatures[aCreatureIdx];
		aCreatureIdx++;
	}
	mCreatures.clear();

	// 0x4433e0 (the sprites' destructor) runs for the glows, the labels and the nine fingers as the members go

	// port: the original never frees the three timers
	delete mSecondTimer;
	delete mBlinkTimer;
	delete mErrorTimer;
}

// 0x44b260 slot 1: the buttons get their places and labels, then the timers and the buttons join the manager (the buttons in
// front of everything)
void TypingTutor::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	mNextButton->Resize(0x24b, 0x17c, 0x32, 0x1e);
	mNextButton->mDoFinger = true;
	mNextButton->mLabel = GT(0x00530220);

	mPrevButton->Resize(0x1d4, 0x17c, 0x32, 0x1e);
	mPrevButton->mDoFinger = true;
	mPrevButton->mLabel = GT(0x00530218);

	mOptionsButton->Resize(0x1d2, 0x1a9, 0xaa, 0x19);
	mOptionsButton->mDoFinger = true;
	mOptionsButton->mLabel = GT(0x00530210);

	mMenuButton->Resize(0x1d2, 0x1bf, 0xaa, 0x19);
	mMenuButton->mDoFinger = true;
	mMenuButton->mLabel = GT(0x00530200);

	theWidgetManager->AddWidget(mSecondTimer);
	theWidgetManager->AddWidget(mBlinkTimer);
	theWidgetManager->AddWidget(mErrorTimer);
	theWidgetManager->AddWidget(mNextButton);
	theWidgetManager->BringToFront(mNextButton);
	theWidgetManager->AddWidget(mOptionsButton);
	theWidgetManager->BringToFront(mOptionsButton);
	theWidgetManager->AddWidget(mPrevButton);
	theWidgetManager->BringToFront(mPrevButton);
	theWidgetManager->AddWidget(mMenuButton);
	theWidgetManager->BringToFront(mMenuButton);
}

// 0x441920 slot 2
void TypingTutor::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNextButton);
	theWidgetManager->RemoveWidget(mOptionsButton);
	theWidgetManager->RemoveWidget(mPrevButton);
	theWidgetManager->RemoveWidget(mMenuButton);
	theWidgetManager->RemoveWidget(mSecondTimer);
	theWidgetManager->RemoveWidget(mBlinkTimer);
	theWidgetManager->RemoveWidget(mErrorTimer);
}

// 0x441a40 slot 25: the pointer is the arrow over the whole tutor (0x47b200 is the base class's MouseMove)
void TypingTutor::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->SetCursor(0);
}

// 0x45bf50 listener slot 3: the click sound
void TypingTutor::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x441a00: the tutor stops while the pause count is above 0 (a lost focus pauses it once; the count never goes below 0)
void TypingTutor::SetPaused(bool thePause)
{
	if (thePause)
	{
		mPauseCount = mPauseCount + 1;
		MarkDirty();
		return;
	}

	int aCount = mPauseCount - 1;
	mPauseCount = aCount;
	if (aCount < 1)
		mPauseCount = 0;
}

// 0x442020 slot 19: pause, and the four buttons cannot be pressed
void TypingTutor::LostFocus()
{
	SetPaused(true);
	mNextButton->SetDisabled(true);
	mPrevButton->SetDisabled(true);
	mMenuButton->SetDisabled(true);
	mNextButton->MarkDirty();
	mPrevButton->MarkDirty();
	mMenuButton->MarkDirty();
	mOptionsButton->SetDisabled(true);
	mOptionsButton->MarkDirty();
}

// 0x442080 slot 18
void TypingTutor::GotFocus()
{
	SetPaused(false);
	mNextButton->SetDisabled(false);
	mPrevButton->SetDisabled(false);
	mMenuButton->SetDisabled(false);
	mOptionsButton->SetDisabled(false);
}

// 0x454010: a creature that swims across the water behind the panel while the lesson has not started: one of the seven kinds
// (a bonus creature rises from the bottom of the window, any other comes in from the left or the right edge at a height of
// 155 up to the window's height less 40, at a speed of 0.25 to 0.499 pixels a tick), with five of its flags set (0xd0, 0x1a,
// 0x1d, 0x1c, 0x1b of the Enemy part)
void TypingTutor::AddCreature()
{
	Enemy* aCreature;
	unsigned int aType = (unsigned int) gGameRand->NextNoAssert();
	switch (aType % 7)
	{
	case 0:
		aCreature = new BlackShark(mApp);
		break;
	case 1:
		{
			BonusCreature* aBonus = new BonusCreature(mApp);
			aBonus->SetState(0);
			aBonus->SetX((double) (mWidth / 2));
			aBonus->SetY((double) mHeight);
			aBonus->SetVelY(0.3);
			aCreature = aBonus;
		}
		break;
	case 2:
		aCreature = new Piranha(mApp, 0);
		break;
	case 3:
		aCreature = new RedPiranha(mApp, 0);
		break;
	case 4:
		aCreature = new RedShark(mApp);
		break;
	case 5:
		aCreature = new ToxicShark(mApp);
		break;
	default:
		aCreature = new WhitePiranha(mApp, 0);
		break;
	}

	aCreature->mBiteEventSent = true;
	aCreature->mPlayerDiesSoundPlayed = true;
	aCreature->mAlwaysTrueC = true;
	aCreature->mAlwaysTrueB = true;
	aCreature->mAlwaysTrueA = true;

	unsigned int aSpeedRand = (unsigned int) gGameRand->NextNoAssert();
	if (aType % 7 != 1)
	{
		unsigned int aSide = (unsigned int) gGameRand->NextNoAssert();
		if ((aSide & 1) == 0)
		{
			aCreature->SetState(0);
			aCreature->SetX(470.0);
		}
		else
		{
			aCreature->SetState(1);
			aCreature->SetX((double) -aCreature->GetFrameWidth());
		}

		unsigned int aHeightRand = (unsigned int) gGameRand->NextNoAssert();
		int aY = aHeightRand % (unsigned int) (mHeight - 0xc3) + 0x9b;
		double aYDouble = (double) aY;
		if (aY < 0)
			aYDouble = aYDouble + 4294967296.0;
		aCreature->SetY(aYDouble);

		aCreature->SetVelX((double) (aSpeedRand % 0xfa) * 0.001 + 0.25);
	}
	aCreature->SetVisible(true);
	mCreatures.push_back(aCreature);
}

// 0x4601a0 slot 17: the clocks (lesson seconds, flash after an error, cursor blink), then in the states before and after the
// lesson the creatures behind the panel (never more than three; one is added every fifth tick while there are fewer)
void TypingTutor::Update()
{
	Widget::Update();
	if (mPauseCount == 0)
	{
		if (mState == 1)
		{
			if (mErrorTimer->Elapsed() >= 400)
			{
				mFlashOn = !mFlashOn;
				mErrorTimer->Reset();
				MarkDirty();
			}

			if (mSecondTimer->Elapsed() >= 1000)
			{
				mSeconds = mSeconds + 1;
				if (mLesson->mLessonTime != -1)
				{
					mTimeLeft = mTimeLeft - 1;
					if (mTimeLeft < 1)
					{
						Finish();
						mSecondTimer->SetDisabled(true);
						return;
					}
				}
				mSecondTimer->Reset();
			}
		}

		if ((mState == 1) || (mState == 0))
		{
			if (mBlinkTimer->Elapsed() >= 500)
			{
				MarkDirty();
				mBlinkTimer->Reset();
				mCursorOn = !mCursorOn;
			}
		}

		if (mState != 1)
		{
			if ((mUpdateCnt % 5 == 0) && (mCreatures.empty() || (mCreatures.size() < 3)))
				AddCreature();

			unsigned int aCreatureIdx = 0;
			do
			{
				while (true)
				{
					if (mCreatures.empty() || (mCreatures.size() <= aCreatureIdx))
					{
						MarkDirty();
						return;
					}

					Enemy* aCreature = mCreatures[aCreatureIdx];
					aCreature->Update();
					aCreature = mCreatures[aCreatureIdx];
					if ((160.0 <= aCreature->mY) || (mState != 1))
						break;

					// a creature that is above y = 160 goes (never reached: the loop only runs outside state 1)
					if (aCreature != NULL)
						delete aCreature;
					mCreatures.erase(mCreatures.begin() + aCreatureIdx);
					aCreatureIdx = aCreatureIdx - 1;
					aCreatureIdx = aCreatureIdx + 1;
				}

				// the creature's events: 4 and 5 (it left the window, or it was done with) end it; the others are only taken off
				bool aErased = false;
				int anEvent = mCreatures[aCreatureIdx]->PopEvent();
				if (anEvent != 0)
				{
					do
					{
						if ((anEvent == 5) || (anEvent == 4))
						{
							Enemy* aDead = mCreatures[aCreatureIdx];
							if (aDead != NULL)
								delete aDead;
							mCreatures.erase(mCreatures.begin() + aCreatureIdx);
							aErased = true;			// 0x460412: the same index is the next creature
							break;
						}
						anEvent = mCreatures[aCreatureIdx]->PopEvent();
					} while (anEvent != 0);
				}
				if (!aErased)
					aCreatureIdx = aCreatureIdx + 1;
			} while (true);
		}
	}
}

// 0x460680 slot 20: the first key starts the lesson (the creatures go, the second timer runs). A key is checked against the
// character that is next: the right one counts (and counts as clean when nothing wrong was typed for it) and moves on; a wrong
// one counts one error per character (the flash starts) and plays the wrong-key sound
void TypingTutor::KeyChar(SexyChar theChar)
{
	MarkDirty();
	if (mState != 2)
	{
		if (mState == 0)
		{
			mSecondTimer->SetDisabled(false);
			mSecondTimer->Reset();
			mState = 1;

			unsigned int aCreatureIdx = 0;
			while (aCreatureIdx < mCreatures.size())
			{
				if (mCreatures[aCreatureIdx] != NULL)
					delete mCreatures[aCreatureIdx];
				aCreatureIdx++;
			}
			mCreatures.clear();
		}

		mApp->SetCursor(CURSOR_NONE);

		TutorExercise* anExercise = mLesson->mTasks[mTask];
		std::string aLine = anExercise->mTexts[anExercise->mCurText];
		if (((anExercise->mCurChar == (int) (aLine.length() - 1)) || (anExercise->mCurChar == 0)) &&
			((theChar == '\r') || (theChar == '\n') || (theChar == ' ')))
		{
			mApp->PlaySample(SOUND_TYPE + (unsigned int) gGameRand->NextNoAssert() % 3);
		}
		else if (theChar == mExpectedChar)
		{
			mErrorTimer->SetDisabled(true);
			mFlashOn = true;
			mCursorOn = true;
			mBlinkTimer->Reset();
			mApp->PlaySample(SOUND_TYPE + (unsigned int) gGameRand->NextNoAssert() % 3);
			mCorrectKeys = mCorrectKeys + 1;
			if (mCleanChar)
				mCleanKeys = mCleanKeys + 1;
			AdvanceChar();
			if (mState == 1)
				SetupHighlight();
		}
		else
		{
			if (mCleanChar)
			{
				mErrorTimer->SetDisabled(false);
				mErrorTimer->Reset();
				mFlashOn = true;
				mCleanChar = false;
				mErrors = mErrors + 1;
			}
			mApp->PlaySample(SOUND_WRONG);
		}
	}
}

// 0x4605a0: one character on; at the end of a line the next line, at the end of the last line of a task the next task (the
// lines shown start again at 0), after the last task the lesson is finished. The four lines shown are those from the one that
// is a multiple of 4 and not past the current line
void TypingTutor::AdvanceChar()
{
	TutorExercise* anExercise = mLesson->mTasks[mTask];
	anExercise->mCurChar = anExercise->mCurChar + 1;
	if ((unsigned int) anExercise->mTexts[anExercise->mCurText].length() <= (unsigned int) anExercise->mCurChar)
	{
		anExercise->mCurChar = 0;
		anExercise->mCurText = anExercise->mCurText + 1;
		if (anExercise->mTexts.empty() || ((unsigned int) anExercise->mTexts.size() <= (unsigned int) anExercise->mCurText))
		{
			mTask = mTask + 1;
			if (mLesson->mTasks.empty() || ((unsigned int) mLesson->mTasks.size() <= (unsigned int) mTask))
			{
				Finish();
			}
			else
			{
				mLesson->mTasks[mTask]->mCurChar = 0;
				mLesson->mTasks[mTask]->mCurText = 0;
				mFirstLine = 0;
				anExercise = mLesson->mTasks[mTask];
			}
		}
		else
		{
			anExercise->mCurChar = 0;
		}
	}

	if (anExercise->mCurText % 4 == 0)
		mFirstLine = anExercise->mCurText;
}

// 0x460430: the next lesson (up to the 19th), its tasks start from their first character; the next button goes after the last
void TypingTutor::GoToNextLesson()
{
	if (mLessons->mCurrent + 1 < 0x13)
	{
		if (mLesson != NULL)
		{
			unsigned int aTaskIdx = 0;
			while (true)
			{
				if (mLesson->mTasks.empty() || ((unsigned int) mLesson->mTasks.size() <= aTaskIdx))
					break;
				mLesson->mTasks[aTaskIdx]->mCurChar = 0;
				mLesson->mTasks[aTaskIdx]->mCurText = 0;
				aTaskIdx++;
			}
		}

		mLesson = mLessons->NextLesson();
		unsigned int aTaskIdx = 0;
		while (true)
		{
			if (mLesson->mTasks.empty() || ((unsigned int) mLesson->mTasks.size() <= aTaskIdx))
				break;
			mLesson->mTasks[aTaskIdx]->mCurChar = 0;
			mLesson->mTasks[aTaskIdx]->mCurText = 0;
			aTaskIdx++;
		}

		ResetLesson();
		SetupHighlight();
	}

	if (0x12 < mLessons->mCurrent + 1)
		mNextButton->mVisible = false;
	MarkDirty();
}

// 0x4604f0: the lesson before; the previous button goes at the first
void TypingTutor::GoToPrevLesson()
{
	if (0 < mLessons->mCurrent)
	{
		if (mLesson != NULL)
		{
			unsigned int aTaskIdx = 0;
			while (true)
			{
				if (mLesson->mTasks.empty() || ((unsigned int) mLesson->mTasks.size() <= aTaskIdx))
					break;
				mLesson->mTasks[aTaskIdx]->mCurChar = 0;
				mLesson->mTasks[aTaskIdx]->mCurText = 0;
				aTaskIdx++;
			}
		}

		mLesson = mLessons->PrevLesson();
		unsigned int aTaskIdx = 0;
		while (true)
		{
			if (mLesson->mTasks.empty() || ((unsigned int) mLesson->mTasks.size() <= aTaskIdx))
				break;
			mLesson->mTasks[aTaskIdx]->mCurChar = 0;
			mLesson->mTasks[aTaskIdx]->mCurText = 0;
			aTaskIdx++;
		}

		ResetLesson();
		SetupHighlight();
	}

	if (mLessons->mCurrent == 0)
		mPrevButton->mVisible = false;
	MarkDirty();
}

// 0x45e7a0: the lesson starts again: the clock and the counts are cleared, the fingers have their plain pictures, no key is
// lit, the seconds timer stops
void TypingTutor::ResetLesson()
{
	mTimeLeft = mLesson->mLessonTime;
	mTask = 0;
	mErrors = 0;
	mCleanKeys = 0;
	mCorrectKeys = 0;
	mState = 0;
	mCleanChar = true;
	mShift = 0;
	mSeconds = 0;
	mExpectedChar = ' ';
	ResetFingers();
	// 0x4512a0: the lit keys go (ResetFingers cleared the labels)
	mGlows.clear();
	mFirstLine = 0;
	mSecondTimer->SetDisabled(true);
}

// 0x45fb10: the lesson is over (the last task is done or the time is up). The words per minute are the keys typed in five
// per word over the seconds taken (the raw one from all the right keys, the adjusted one from those typed clean); the
// accuracy is the clean keys' share of the clean keys and the errors. The lesson passes when the errors are not above its
// maximum, the adjusted words per minute, the tasks done and the accuracy are not below its minimums (a minimum of -1 means
// none, except that -1 tasks means all of them and 0 means none). The three results are added to the user's records, a
// passed lesson is marked (a failed one only when it has not been taken before), and the profile is saved. Then a dialog:
// the typing test shows the speed and the typing level, any other lesson asks whether to see the statistics
void TypingTutor::Finish()
{
	int aMaxErrors = 0x7fffffff;
	if (mLesson->mMaxErrors != -1)
		aMaxErrors = mLesson->mMaxErrors;

	int aMinWPM = -50;
	if (mLesson->mMinWPM != -1)
		aMinWPM = mLesson->mMinWPM;

	int aMinTasks;
	if (mLesson->mMinTasks == -1)
		aMinTasks = (int) mLesson->mTasks.size();
	else if (mLesson->mMinTasks == 0)
		aMinTasks = -5;
	else
		aMinTasks = mLesson->mMinTasks;

	int aMinAccuracy = -10;
	if (mLesson->mMinAccuracy != -1)
		aMinAccuracy = mLesson->mMinAccuracy;

	mApp->SetCursor(0);
	mState = 2;

	// (the original walks the tasks and their lines here and does nothing with them)

	int aCleanKeys = mCleanKeys;
	double aRate = 1.0 / ((double) mSeconds * 0.016666666666666666);
	mAdjustedWPM = FToL(aRate * (0.2 * (double) aCleanKeys));
	mRawWPM = FToL(aRate * ((double) mCorrectKeys * 0.2));
	if (mAdjustedWPM < 0)
		mAdjustedWPM = 0;
	if (mRawWPM < 0)
		mRawWPM = 0;
	mAccuracy = FToL((double) aCleanKeys / (double) (mErrors + aCleanKeys) * 100.0);

	mApp->mUser->RecordTutorWPM(mRawWPM, mLessons->mCurrent);
	mApp->mUser->RecordTutorAdjustedWPM(mAdjustedWPM, mLessons->mCurrent);
	mApp->mUser->RecordTutorAccuracy(mAccuracy, mLessons->mCurrent);

	if ((aMaxErrors < mErrors) || (mAdjustedWPM < aMinWPM) || (mTask < aMinTasks) || (mAccuracy < aMinAccuracy))
	{
		if (mApp->mUser->mLessonOutcome[mLessons->mCurrent] == -1)
		{
			mApp->mUser->mLessonOutcome[mLessons->mCurrent] = 0;
			mLesson->mUserState = 0;
		}
	}
	else
	{
		mApp->mUser->mLessonOutcome[mLessons->mCurrent] = 1;
		mLesson->mUserState = 1;
	}

	mApp->mUser->AddWPM(mAdjustedWPM);
	mApp->mUser->mLastLesson = mLessons->mCurrent;
	mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, false);

	ResetFingers();
	// 0x4433e0 / free: the lit keys and the labels go
	mGlows.clear();
	mLabels.clear();
	mSecondTimer->SetDisabled(true);

	if (mLessons->mCurrent < 1)
	{
		mLeaveAsked = true;
		// the typing level by speed: 80, 60, 40 and 30 words per minute are the limits
		std::string aLevel;
		if (mAdjustedWPM < 0x50)
		{
			if (mAdjustedWPM < 0x3c)
			{
				if (mAdjustedWPM < 0x28)
				{
					if (mAdjustedWPM < 0x1e)
						aLevel = GT(0x00531860);
					else
						aLevel = GT(0x00531870);
				}
				else
				{
					aLevel = GT(0x00531880);
				}
			}
			else
			{
				aLevel = GT(0x00531890);
			}
		}
		else
		{
			aLevel = GT(0x005318a0);
		}

		std::string aLines = StrFormat(GTC(0x005317a8), mRawWPM, mAdjustedWPM, aLevel.c_str());
		mApp->ShowMessageDialog(GT(0x00531780), aLines, GT(0x00531790), false);		// 0x41bd80: "Test Completed!", "Return to Main Menu"
	}
	else
	{
		mViewStatsAsked = true;
		mApp->ShowConfirmDialog(GT(0x005318b0), GT(0x005318bc), "", true, GT(0x0052d1f8), GT(0x0052d1fc));		// 0x41c4f0: "VIEW STATS?", "YES", "NO"
	}
}

// 0x460900 listener slot 4: next lesson, previous lesson, options, back to the menu (a confirmation first; the app does the
// rest when it is answered)
void TypingTutor::ButtonDepress(int theId)
{
	if (theId == NEXT_BUTTON_ID)
	{
		mPrevButton->mVisible = true;
		GoToNextLesson();
		return;
	}
	if (theId == PREV_BUTTON_ID)
	{
		mNextButton->mVisible = true;
		GoToPrevLesson();
		return;
	}
	if (theId != MENU_BUTTON_ID)
	{
		if (theId == OPTIONS_BUTTON_ID)
		{
			MarkDirty();
			mApp->ShowOptions();		// 0x414150
		}
		return;
	}

	MarkDirty();
	mLeaveAsked = true;
	mApp->ShowConfirmDialog(GT(0x005318e8), GT(0x005318f0), "", true, GT(0x0052d1f8), GT(0x0052d1fc));		// 0x41c4f0: "QUIT?", the question, "YES", "NO"
}
