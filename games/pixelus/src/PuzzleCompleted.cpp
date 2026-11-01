#include "PuzzleCompleted.h"
#include "PixelusApp.h"
#include "Temples.h"
#include "GameDialog.h"
#include "GameText.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include <math.h>
#include <stdio.h>
#include <algorithm>

using namespace Sexy;

namespace
{

const double PI = 3.14159265358979;

// the player has solved every puzzle of a temple
bool IsTempleSolved(PixelusApp* theApp, UserProfile* theUser, const std::string& theName)
{
	return theUser != NULL && theUser->CountSolved(theName) == theApp->mLevelData->GetPuzzleCount(theName);
}

// ... and every one of the nine temples that come after the cave
bool IsGameSolved(PixelusApp* theApp, UserProfile* theUser)
{
	for (int i = 1; i < NUM_TEMPLES; i++)
		if (!IsTempleSolved(theApp, theUser, gTemples[i].mName))
			return false;
	return true;
}

// 0x433530: the puzzle just solved is the one that opens temple theTemple
bool OpensTemple(PixelusApp* theApp, int theSolvedTotal, int theTemple)
{
	UserProfile* aUser = theApp->mUserDirectory->GetCurrent();
	int aTutorial = aUser != NULL ? aUser->CountSolved(gTemples[0].mName) : 0;
	return theSolvedTotal - aTutorial == gTemples[theTemple].mUnlock;
}

}

// 0x41b3c0
PuzzleCompletedScreen::PuzzleCompletedScreen(PixelusApp* theApp, MemoryImage* thePicture, int theStartX, int theStartY,
	int theTime, int theMoves, const std::string& theSolution, const std::string& theTitle, const std::string& theUuid)
{
	mApp = theApp;
	mPicture = thePicture;
	mTitle = theTitle;
	mTime = theTime;
	mMoves = theMoves;
	mTicks = 0;
	mArrivedAt = 0;
	mProgress = 0;
	mMedal = 0;
	mStartX = mX = (float) theStartX;
	mStartY = mY = (float) theStartY;
	mSize = 320;
	mWorldRecord = mWasMaxed = mOpenedTemple = mShortest = mBronze = mSilver = false;
	mTempleDone = mGameDone = mTutorialDone = false;
	mPreviousMoves = mPreviousTime = 0;
	mOpenedIndex = -1;

	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	const std::string aLevel = mApp->mLevelData->mCurrentLevel;
	mTutorial = aLevel == gTemples[0].mName;

	// what the player had done with this puzzle before, and what the best known solution is
	const PuzzleRecord* aPrevious = aUser != NULL ? aUser->GetRecord(theUuid) : NULL;
	int aBest = mApp->mProgress->GetBestMoves(theUuid);
	bool aTempleDoneBefore = IsTempleSolved(mApp, aUser, aLevel);
	bool aGameDoneBefore = IsGameSolved(mApp, aUser);
	if (aPrevious != NULL)
	{
		mPreviousMoves = aPrevious->mMoves;
		mPreviousTime = aPrevious->mMoves2;
		mWasMaxed = aBest >= aPrevious->mMoves;
		mShortest = theMoves < aPrevious->mMoves;
	}

	mApp->mProgress->UpdateBest(theUuid, theMoves, theSolution, &mWorldRecord);
	mApp->mProgress->SaveBest();

	if ((!mWorldRecord && mShortest) || aPrevious == NULL)
	{
		if (aBest + 5 < theMoves)
		{
			if (theMoves <= aBest + 10)
				mBronze = true;
		}
		else
			mSilver = true;
	}

	if (aUser != NULL)
	{
		aUser->SetRecord(theUuid, mWorldRecord ? 1 : 0, theMoves, theTime);
		mApp->mUserDirectory->Save();
	}

	// the puzzle that opens a temple, or the last one of the tutorial, which opens the first temple
	bool aTutorialDone = mTutorial && IsTempleSolved(mApp, aUser, aLevel);
	if (aPrevious == NULL && aUser != NULL)
	{
		if (!mTutorial)
		{
			int aTotal = aUser->CountSolved();
			for (int i = 1; i < NUM_TEMPLES; i++)
				if (OpensTemple(mApp, aTotal, i))
				{
					mOpenedTemple = true;
					mOpenedIndex = i;
				}
		}
		else if (aTutorialDone && !aUser->mFlag79)
		{
			mOpenedTemple = true;
			mOpenedIndex = 1;
		}
	}
	mTempleDone = !aTempleDoneBefore && IsTempleSolved(mApp, aUser, aLevel);
	mGameDone = !aGameDoneBefore && IsGameSolved(mApp, aUser);
	// mode 5 of 0x41b3c0: only the solve that finishes the tutorial says so (the same test before and after the record)
	mTutorialDone = mTutorial && !aTempleDoneBefore && aTutorialDone;
	if (mTutorialDone && aUser != NULL)
		aUser->mFlag79 = true;

	mApp->mLastSolvedUuid = theUuid;

	// the button at the bottom of the tablet's picture
	mContinueButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_CONTINUE, this);
	mContinueButton->mLabel = GT(0x50c9a0);
	StyleButton(mApp, mContinueButton, 3);
}

PuzzleCompletedScreen::~PuzzleCompletedScreen()
{
	delete mContinueButton;
	delete mPicture;
}

// 0x419b60
void PuzzleCompletedScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinueButton);
}

void PuzzleCompletedScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinueButton);
}

// 0x419bc0: the button sits on the lower edge, in the middle
void PuzzleCompletedScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aHeight = mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS]->GetHeight();
	mContinueButton->Resize(theWidth / 2 - 0x4b + theX, theY + theHeight - aHeight + 1, 0x96, aHeight);
}

// 0x419a00: the picture flies in, then the medal grows
void PuzzleCompletedScreen::Update()
{
	Widget::Update();
	mTicks++;
	if (mProgress < 1.0f)
	{
		mProgress = std::min(1.0f, mProgress + 0.0125f);
		float t = mProgress * mProgress * (3.0f - 2.0f * mProgress);
		if (mProgress >= 1.0f)
		{
			mSize = END_SIZE;
			mX = (float) END_X;
			mY = (float) END_Y;
			mArrivedAt = mTicks;
		}
		else
		{
			mSize = (int) (320 + (END_SIZE - 320) * t);
			mX = mStartX + (END_X - mStartX) * t;
			mY = mStartY + (END_Y - mStartY) * t;
		}
	}
	else if (mMedal < 1.0f)
		mMedal = std::min(1.0f, mMedal + 0.03f);
	MarkDirty();
}

void PuzzleCompletedScreen::DrawCentered(Graphics* g, const std::string& theText, int theCenterX, int theY)
{
	g->DrawString(theText, theCenterX - g->GetFont()->StringWidth(theText) / 2, theY);
}

void PuzzleCompletedScreen::DrawOutlined(Graphics* g, const std::string& theText, int theCenterX, int theY,
	const Color& theColor, const Color& theOutline)
{
	int aX = theCenterX - g->GetFont()->StringWidth(theText) / 2;
	g->SetColor(theOutline);
	g->DrawString(theText, aX - 1, theY);
	g->DrawString(theText, aX + 1, theY);
	g->DrawString(theText, aX, theY - 1);
	g->DrawString(theText, aX, theY + 1);
	g->SetColor(theColor);
	g->DrawString(theText, aX, theY);
}

// 0x431df0
std::string PuzzleCompletedScreen::FormatTime(int theMilliseconds, int theMode)
{
	char aBuffer[64];
	int aMinutes = theMilliseconds / 60000;
	int aSeconds = (theMilliseconds % 60000) / 1000;
	if (theMode == 0)
	{
		snprintf(aBuffer, sizeof(aBuffer), "%d'%02d\"%d", aMinutes, aSeconds, (theMilliseconds % 1000) / 100);
		return aBuffer;
	}
	if (theMode == 1)
	{
		snprintf(aBuffer, sizeof(aBuffer), "%d'%02d\"%02d", aMinutes, aSeconds, (theMilliseconds % 1000) / 10);
		return aBuffer;
	}
	if (theMode == 2)
	{
		snprintf(aBuffer, sizeof(aBuffer), "%dm %02ds", aMinutes, aSeconds);
		return aBuffer;
	}
	if (theMode == 3)
	{
		if (aMinutes < 1)
			snprintf(aBuffer, sizeof(aBuffer), "%2ds", aSeconds);
		else
			snprintf(aBuffer, sizeof(aBuffer), "%dm %2ds", aMinutes, aSeconds);
		return aBuffer;
	}

	std::string aResult;
	if (aMinutes != 0)
	{
		if (aMinutes != 1)
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x510f6c), aMinutes);
			aResult += aBuffer;
		}
		else
			aResult += GT(0x510f4c);
	}
	if (aMinutes > 0 && aSeconds > 0)
		aResult += "and ";
	if (aSeconds == 0)
		aResult += GT(0x510f38);
	else if (aSeconds == 1)
		aResult += GT(0x510f40);
	else
	{
		snprintf(aBuffer, sizeof(aBuffer), GTC(0x510f58), aSeconds);
		aResult += aBuffer;
	}
	return aResult;
}

// 0x419e60
void PuzzleCompletedScreen::Draw(Graphics* g)
{
	const int CENTER = 320;			// the tablet's middle
	const int LEFT_SIGN = 105;		// the signs the characters hold
	const int RIGHT_SIGN = 525;
	const Color aWhite(255, 255, 255);
	const Color aBrown(0x66, 0x24, 0);
	bool aArrived = mProgress >= 1.0f;
	int aSincePulse = mTicks - mArrivedAt;

	g->DrawImage(mApp->mImages[IMAGE_PUZZLECOMPLETED], 0, 0);

	// the puzzle's title, in the colours of a flame
	{
		double t = (cos(mTicks * 0.02) + 1.0) * 0.5;
		Color aColor(255, (int) (190 + 52 * t), 0);
		std::string aTitle = mTitle.empty() ? GT(0x50e8b0) : "<" + mTitle + ">";
		g->SetFont(mApp->mFonts[FONT_GARAMOND12BOLD]);
		DrawOutlined(g, aTitle, CENTER, 240, aColor, aBrown);
	}
	if (aArrived)
	{
		// the sparkle of the picture settling in the tablet
	}

	// the sign on the left: the temple that has opened, or the time
	if (!mTutorial && (mWasMaxed || mOpenedTemple))
	{
		g->DrawImage(mApp->mImages[IMAGE_PUZZLECOMPLETEDPANEL1], 27, 305);
		if (mOpenedTemple)
		{
			if (aArrived)
			{
				double aGlow = (2.0 - (cos((aSincePulse % 200) * (1.0 / 199.0) * 2.0 * PI) + 1.0)) * 32.0;
				g->SetColorizeImages(true);
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColor(Color(255, 255, 255, (int) aGlow));
				g->DrawImage(mApp->mImages[IMAGE_MERMAIDPANELGLOW], 5, 276);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
			g->SetColor(aWhite);
			g->SetFont(mApp->mFonts[FONT_GARAMOND12]);
			DrawCentered(g, GT(0x50e888), LEFT_SIGN, 352);
			DrawCentered(g, GT(0x50e878), LEFT_SIGN, 372);
		}
	}
	if (!mWasMaxed && mWorldRecord && !mTutorial)
		g->DrawImage(mApp->mImages[IMAGE_PUZZLECOMPLETEDPANEL2], 449, 226);

	// "Well Done!", with a shine that breathes
	{
		g->SetColor(aWhite);
		g->SetFont(mApp->mFonts[FONT_GARAMOND24GOLD]);
		DrawCentered(g, GT(0x50e868), CENTER, 95);
		int anAlpha = 0x80;
		if (aArrived)
			anAlpha += (int) ((sin(((aSincePulse + 0xe1) % 300) * (1.0 / 299.0) * 2.0 * PI) + 1.0) * 32.0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawCentered(g, GT(0x50e868), CENTER, 95);
		g->SetColor(aWhite);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// the moves
	char aNumber[16];
	snprintf(aNumber, sizeof(aNumber), "%d", mMoves);
	g->SetFont(mApp->mFonts[FONT_GARAMOND24GOLD]);
	DrawOutlined(g, aNumber, CENTER, 278, aWhite, Color(0, 0, 0, 64));
	if (aArrived)
	{
		int anAlpha = 0x80 + (int) ((sin(((aSincePulse + 0xe1) % 300) * (1.0 / 299.0) * 2.0 * PI) + 1.0) * 32.0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, std::min(255, anAlpha)));
		DrawCentered(g, aNumber, CENTER, 278);
		g->SetColor(aWhite);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
	g->SetColor(aWhite);
	DrawCentered(g, GT(0x50e7f8), CENTER, 298);

	// the notes under the tablet's number; the tutorial has encouragement instead
	g->SetFont(mApp->mFonts[FONT_GARAMOND12]);
	char aBuffer[160];
	if (mTutorial)
	{
		g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
		if (mTutorialDone)
		{
			DrawCentered(g, GT(0x50e858), CENTER, 324);
			DrawCentered(g, GT(0x50e844), CENTER, 341);
		}
		else
		{
			DrawCentered(g, GT(0x50e830), CENTER, 318);
			DrawCentered(g, GT(0x50e818), CENTER, 334);
			DrawCentered(g, GT(0x50e800), CENTER, 350);
		}
	}
	else if (!mWasMaxed)
	{
		if (mWorldRecord)
		{
			DrawCentered(g, GT(0x50e7dc), CENTER, 324);
			DrawCentered(g, GT(0x50e7d0), RIGHT_SIGN, 278);
			DrawCentered(g, GT(0x50e7bc), RIGHT_SIGN, 298);
			DrawCentered(g, GT(0x50e7ac), RIGHT_SIGN, 318);
		}
		else if (mSilver)
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x50e730), 5);
			DrawCentered(g, aBuffer, CENTER, 324);
		}
		else if (mBronze)
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x50e730), 10);
			DrawCentered(g, aBuffer, CENTER, 324);
		}
		else if (mShortest)
			DrawCentered(g, GT(0x50e784), CENTER, 324);
		else if (mPreviousMoves > 0)
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x50e758), mPreviousMoves);
			DrawCentered(g, aBuffer, CENTER, 324);
		}
	}
	else
	{
		// the player has matched the best solution already: now it is a matter of speed
		if (mPreviousTime < 1 || mPreviousTime / 1000 <= mTime / 1000)
		{
			if (mTime < 1000)
				DrawCentered(g, GT(0x50e6d4), CENTER, 326);
			else
				DrawCentered(g, GT(0x50e6c4) + FormatTime(mTime, 4) + ".", CENTER, 326);
			if (mPreviousTime >= 1 && !mTutorial)
			{
				DrawCentered(g, GT(0x50e6b4), LEFT_SIGN, 348);
				DrawCentered(g, FormatTime(mPreviousTime, 3), LEFT_SIGN, 378);
			}
		}
		else
		{
			DrawCentered(g, GT(0x50e704), CENTER, 324);
			if (!mTutorial)
			{
				DrawCentered(g, GT(0x50e6f4), LEFT_SIGN, 348);
				DrawCentered(g, FormatTime(mTime, 3), LEFT_SIGN, 378);
			}
		}
	}

	// the picture on its way to the tablet, and the shine over it once it is there
	if (mPicture != NULL)
	{
		Rect aDest((int) mX, (int) mY, mSize, mSize);
		g->DrawImage(mPicture, aDest, Rect(0, 0, mPicture->GetWidth(), mPicture->GetHeight()));
		if (aArrived)
		{
			double aShine = (sin(((aSincePulse + 0x60) % 300) * (1.0 / 299.0) * 2.0 * PI) + 1.0) * 24.0;
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int) aShine));
			g->DrawImage(mPicture, aDest, Rect(0, 0, mPicture->GetWidth(), mPicture->GetHeight()));
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	// the medal for how close the solution is to the best known
	if (!mTutorial && (mWorldRecord || mBronze || mSilver) && mMedal > 0)
	{
		Image* aMedal = mApp->mImages[mWorldRecord ? IMAGE_MEDAL : (mBronze ? IMAGE_MEDAL_BRONZE : IMAGE_MEDAL_SILVER)];
		int aWidth = (int) (aMedal->GetWidth() * mMedal);
		int aHeight = (int) (aMedal->GetHeight() * mMedal);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (255 * mMedal)));
		g->DrawImage(aMedal, Rect(END_X + END_SIZE - aWidth / 2 - 6, END_Y + END_SIZE - aHeight / 2 - 6, aWidth, aHeight),
			Rect(0, 0, aMedal->GetWidth(), aMedal->GetHeight()));
		g->SetColorizeImages(false);
	}
}

// what the button does: the way on is to the island when something opened (the tablet's news is the temple's), else the
// temple the puzzle belongs to
void PuzzleCompletedScreen::ButtonDepress(int theId)
{
	if (theId == ID_CONTINUE)
		mApp->ContinueAfterPuzzle(mOpenedTemple ? mOpenedIndex : -1, mTempleDone, mGameDone);
}
