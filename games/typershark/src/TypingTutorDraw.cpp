// TypingTutorDraw.cpp: how the typing tutor looks (the water and the creatures behind, the panel, the lesson's title and
// result, the hands with the finger to use, the lit key, and the lesson's text on the left; the clock, the counts and the
// statistics on the right). The layout is in window pixels of the 640 x 480 window.
//
// Quirk of the original that is kept: in the typing state the lines before and after the line being typed are placed by their
// index in the task, not by their place among the four shown (only the line being typed is), so from the fifth line of a task
// on, when mFirstLine is above 0, they are drawn that many lines too low.
// Deviation (port:): the key labels around the fingers are drawn without the "^rrggbb^" colour codes of WriteString2004, as
// the original does (it switches the framework's colour codes off around them); the port's WriteString2004 has no such switch.

#include "TypingTutor.h"
#include "WinWSApp.h"
#include "Board.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "Enemy.h"
#include "Common.h"
#include "Res.h"
#include "compat2004/Compat2004.h"

using namespace Sexy;

// 0x441990: the cursor under the next character: a white line of the character's width, five pixels under theY
void TypingTutor::DrawCursor(Graphics* g, int theY, int theX, char theChar)
{
	if (mCursorOn)
	{
		int aWidth = g->GetFont()->CharWidth(theChar);
		g->SetColor(Color(255, 255, 255));
		g->DrawLine(theX, theY + 5, aWidth + theX, theY + 5);
	}
}

// 0x44b400 slot 16
void TypingTutor::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_WATER1], 0, 0);
	if (mState != 1)
	{
		unsigned int aCreatureIdx = 0;
		while (aCreatureIdx < mCreatures.size())
		{
			mCreatures[aCreatureIdx]->Draw(g);
			aCreatureIdx++;
		}
	}
	g->DrawImage(mApp->mImages[IMAGE_TUTOR_BG], 0, 0);
	g->SetFont(mApp->mFontMyriadCondensed14Bold);
	g->SetColor(Color(0, 0, 0));

	// the title; at the right of it, "passed" or "failed" for a lesson from the 8th on that has been taken
	std::string aText;
	if (mLessons->mCurrent == 0)
		aText = GT(0x0053037c);
	else
		aText = StrFormat(GTC(0x00530370), mLessons->mCurrent);
	g->DrawString(aText, 0x1d8, 0x4b);

	if ((mLesson->mUserState == 1) && (7 < mLessons->mCurrent))
	{
		aText = GT(0x00530368);
		g->SetColor(Color(0, 0xdc, 0));
	}
	else if ((mLesson->mUserState != 0) || (mLessons->mCurrent < 8))
	{
		aText = "";
	}
	else
	{
		aText = GT(0x00530360);
		g->SetColor(Color(0xff, 0, 0));
	}
	WriteString2004(g, aText, 0x1d8, 0x4b, 0x91, 1, true, 0, -1);

	g->SetColor(Color(0, 0, 0));
	g->DrawLine(0x1d8, 0x56, 0x269, 0x56);

	// the hands: the fingers 0 to 3, then 8 down to 4
	int aFingerIdx = 0;
	do
	{
		g->DrawImage(mFingers[aFingerIdx].mImage, mFingers[aFingerIdx].mX, mFingers[aFingerIdx].mY);
		aFingerIdx++;
	} while (aFingerIdx < 4);
	aFingerIdx = 8;
	do
	{
		g->DrawImage(mFingers[aFingerIdx].mImage, mFingers[aFingerIdx].mX, mFingers[aFingerIdx].mY);
		aFingerIdx--;
	} while (aFingerIdx > 3);
	g->DrawImage(mApp->mImages[IMAGE_HANDS], 0xd, 0x17b);

	// the rings around the finger to use, each with the key's name centred in it (0x561208, the framework's colour-code switch,
	// is off while they are drawn)
	unsigned int aLabelIdx = 0;
	while (aLabelIdx < mLabels.size())
	{
		TutorSprite& aLabel = mLabels[aLabelIdx];
		g->DrawImage(aLabel.mImage, aLabel.mX, aLabel.mY);
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		g->SetColor(Color(0, 0, 0));
		Image* aRing = mApp->mImages[IMAGE_GLOWRING];
		// port: WriteString2004(g, aLabel.mText, aLabel.mX, aLabel.mY + aRing->mHeight / 2 + 5, aRing->mWidth, 0, true, 0, -1)
		// without the colour codes
		int aLabelX = aLabel.mX + (aRing->mWidth - g->GetFont()->StringWidth(aLabel.mText)) / 2;
		g->DrawString(aLabel.mText, aLabelX, aRing->mHeight / 2 + aLabel.mY + 5);
		aLabelIdx++;
	}

	// the key that is lit (flashes after a wrong key)
	unsigned int aGlowIdx = 0;
	while (aGlowIdx < mGlows.size())
	{
		if (mFlashOn)
			g->DrawImage(mGlows[aGlowIdx].mImage, mGlows[aGlowIdx].mX, mGlows[aGlowIdx].mY);
		aGlowIdx++;
	}

	if (mState == 0)
	{
		g->SetFont(mApp->mFontGillSansMT10Bold);
		WriteWordWrapped2004(g, Rect(0x1d8, 0x5f, 0x98, 0x7f), mLesson->mDescription, -1, -1);
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		g->SetColor(Color(0, 200, 0xe6));
		WriteWordWrapped2004(g, Rect(10, 5, 0x1cc, g->GetFont()->GetAscent() * 2), mLesson->mTasks[mTask]->mHeading,
			g->GetFont()->GetAscent() - 5, 0);

		TutorExercise* anExercise = mLesson->mTasks[mTask];
		unsigned int aLineCount = anExercise->mTexts.size();
		unsigned int aEnd = mFirstLine + 4;
		if (aLineCount < aEnd)
			aEnd = aLineCount;

		g->SetColor(Color(255, 255, 255));
		int aLine = 0;
		if (0 < (int) aEnd)
		{
			do
			{
				DrawTwoColorString(g, 0x10, g->GetFont()->GetAscent() * aLine + 0x41, mLesson->mTasks[mTask]->mTexts[aLine],
					Color(255, 255, 255), mApp->mFontMyriadCondensed14Bold, "", Color(255, 255, 255),
					mApp->mFontMyriadCondensed14Bold, 0x1c9, false);
				aLine++;
			} while (aLine < (int) aEnd);
		}
		DrawCursor(g, 0x41, 0x10, mExpectedChar);

		g->SetColor(Color(0, 0, 0));
		g->DrawLine(0x1d8, 0xf7, 0x270, 0xf7);
		if (mTimeLeft % 0x3c < 10)
			aText = StrFormat(GTC(0x00530354), mTimeLeft / 0x3c, mTimeLeft % 0x3c);
		else
			aText = StrFormat(GTC(0x00530348), mTimeLeft / 0x3c, mTimeLeft % 0x3c);
		if (mLesson->mLessonTime != -1)
		{
			g->SetColor(Color(0xff, 0, 0));
			g->DrawString(aText, 0x1fc, 0x10e);
		}
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(0, 0, 0));
		g->DrawLine(0x1d8, 0x118, 0x270, 0x118);
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		aText = GT(0x00530330);
		g->DrawString(aText, 0x1d8, 300);
	}
	else if (mState == 1)
	{
		g->SetFont(mApp->mFontGillSansMT10Bold);
		WriteWordWrapped2004(g, Rect(0x1d8, 0x5f, 0x98, 0x7f), mLesson->mDescription, -1, -1);
		g->SetColor(Color(0, 200, 0xe6));
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		WriteWordWrapped2004(g, Rect(10, 5, 0x1cc, g->GetFont()->GetAscent() * 2), mLesson->mTasks[mTask]->mHeading,
			g->GetFont()->GetAscent() - 5, 0);

		if (mPauseCount == 0)
		{
			TutorExercise* anExercise = mLesson->mTasks[mTask];
			unsigned int aLineCount = anExercise->mTexts.size();
			unsigned int aEnd = mFirstLine + 4;
			if (aLineCount < aEnd)
				aEnd = aLineCount;

			g->SetColor(Color(255, 255, 255));
			int aLine = mFirstLine;
			if (aLine < (int) aEnd)
			{
				do
				{
					anExercise = mLesson->mTasks[mTask];
					if (aLine < anExercise->mCurText)
					{
						// a line that is done, all green
						g->SetColor(Color(0, 0xff, 0));
						DrawTwoColorString(g, 0x10, g->GetFont()->GetAscent() * aLine + 0x41, anExercise->mTexts[aLine],
							Color(0, 0xff, 0), mApp->mFontMyriadCondensed14Bold, "", Color(255, 255, 255),
							mApp->mFontMyriadCondensed14Bold, 0x1c9, false);
					}
					else if (aLine != anExercise->mCurText)
					{
						// a line to come, all white
						g->SetColor(Color(255, 255, 255));
						DrawTwoColorString(g, 0x10, g->GetFont()->GetAscent() * aLine + 0x41, anExercise->mTexts[aLine],
							Color(255, 255, 255), mApp->mFontMyriadCondensed14Bold, "", Color(255, 255, 255),
							mApp->mFontMyriadCondensed14Bold, 0x1c9, false);
					}
					else
					{
						// the line being typed: green up to the next character, white after it, the cursor under it
						std::string aLineText = anExercise->mTexts[aLine];
						int aTyped = anExercise->mCurChar;
						std::string aDone = aLineText.substr(0, aTyped);
						int aDoneWidth = g->GetFont()->StringWidth(aDone);
						DrawTwoColorString(g, 0x10, g->GetFont()->GetAscent() * (aLine - mFirstLine) + 0x41, aLineText.substr(0, aTyped),
							Color(0, 0xff, 0), mApp->mFontMyriadCondensed14Bold, aLineText.substr(aTyped), Color(255, 255, 255),
							mApp->mFontMyriadCondensed14Bold, 0x1c9, false);
						DrawCursor(g, g->GetFont()->GetAscent() * (aLine - mFirstLine) + 0x41, aDoneWidth + 0x10, mExpectedChar);
					}
					aLine++;
				} while (aLine < (int) aEnd);
			}
		}

		g->SetColor(Color(0, 0, 0));
		g->DrawLine(0x1d8, 0xf7, 0x270, 0xf7);
		if (mTimeLeft % 0x3c < 10)
			aText = StrFormat(GTC(0x00530354), mTimeLeft / 0x3c, mTimeLeft % 0x3c);
		else
			aText = StrFormat(GTC(0x00530348), mTimeLeft / 0x3c, mTimeLeft % 0x3c);
		if (mLesson->mLessonTime != -1)
		{
			g->SetColor(Color(0xff, 0, 0));
			g->DrawString(aText, 0x1fc, 0x10e);
		}
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(0, 0, 0));
		g->DrawLine(0x1d8, 0x118, 0x270, 0x118);
		g->SetFont(mApp->mFontMyriadCondensed14Bold);

		int aPercent = (int) ((double) mTask / (double) (unsigned int) mLesson->mTasks.size() * 100.0);
		aText = StrFormat(GTC(0x00530320), aPercent);
		g->DrawString(aText, 0x1d8, 300);
		aText = StrFormat(GTC(0x00530314), mErrors);
		g->DrawString(aText, 0x1d8, g->GetFont()->GetHeight() + 0x136);
	}
	else
	{
		g->SetFont(mApp->mFontGillSansMT10Bold);
		aText = GT(0x00530268);
		WriteWordWrapped2004(g, Rect(0x1d8, 0x5f, 0x98, 0x7f), aText, -1, -1);
		g->SetColor(Color(0, 0, 0));
		g->DrawLine(0x1d8, 0x101, 0x270, 0x101);
		aText = GT(0x00530258);
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		g->SetColor(Color(0xff, 0, 0));
		g->DrawString(aText, 0x1fc, 0x11a);
		g->SetColor(Color(0, 0, 0));
		g->DrawLine(0x1d8, 0x127, 0x270, 0x127);
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		aText = StrFormat(GTC(0x00530250), mRawWPM);
		g->DrawString(aText, 0x1d8, 0x13b);
		aText = StrFormat(GTC(0x00530240), mAdjustedWPM);
		g->DrawString(aText, 0x1d8, 0x14f);
		aText = StrFormat(GTC(0x00530230), mAccuracy);
		g->DrawString(aText, 0x1d8, 0x163);
	}

	// while paused (the window has lost the focus) the panel is dimmed and says so
	if (mPauseCount != 0)
	{
		g->SetColor(Color(0, 0, 0, 0x80));
		g->FillRect(Rect(0, 0, 0x1d0, mApp->mHeight));
		g->SetFont(mApp->mFontSwissCheesed24);
		g->SetColor(Color(200, 200, 200));
		std::string aPaused = GT(0x00530228);
		Font* aFont = mApp->mFontSwissCheesed24;
		int aPausedY = mApp->mHeight / 2 - aFont->GetHeight() / 2;
		g->DrawString(aPaused, 0xe7 - aFont->StringWidth(aPaused) / 2, aPausedY);
	}
}
