#include "GameOverDialog.h"
#include "WinWSApp.h"
#include "Player.h"
#include "UserProfile.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"
#include "DialogButton.h"
#include "Common.h"

using namespace Sexy;

// 0x43adb0: the base is Dialog with the header "GAME OVER", no lines or footer, the yes and no buttons (mode 1).
// A normal game that is not past its first level, and an abyss game under 400 points or 25000 milliseconds, show no speed or
// accuracy (-1): the speed is then not worth a figure.
GameOverDialog::GameOverDialog(WinWSApp* theApp, Player* thePlayer, bool isAbyss, int theTime, int theCharsTyped,
	int theMistakes, int theScore) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_GAME_OVER, true, GT(0x0052e6ec), "", "",
		Dialog::BUTTONS_YES_NO),
	mPlayer(thePlayer),
	mApp(theApp),
	mAbyss(isAbyss),
	mField11c(0),		// port: the original leaves it unset until the board fills it in
	mWPM(0),
	mAccuracy(0),
	mRankTitle(),
	mField144(theScore),
	mCharsTyped(theCharsTyped),
	mMistakes(theMistakes),
	mTime(theTime)
{
	mYesButton->mLabel = GT(0x0052e5ec);
	mNoButton->mLabel = GT(0x0052b4dc);
	mContentInsets.mLeft = 0x28;
	mContentInsets.mRight = 0x28;
	mContentInsets.mTop = 0x14;
	mContentInsets.mBottom = 0x28;
	Dialog::SetColor(COLOR_HEADER, Color(0, 0, 0));
	Dialog::SetColor(COLOR_LINES, Color(0, 0, 0));
	Dialog::SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	SetButtonFont(mApp->mFontRockwell13);
	SetHeaderFont(mApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);

	if (((mPlayer->mField114 == 0) && (!mAbyss)) || ((mAbyss) && ((mField144 < 400) || (mTime < 25000))))
	{
		mWPM = -1;
		mAccuracy = -1;
	}
	else
	{
		// words (5 characters) per minute of the time, the minutes at least 1 for the mistakes: the speed is cut to 0.75, less the
		// mistakes per minute; the accuracy is the right keystrokes of all of them
		int aCharsTyped = mCharsTyped;
		double aMinutes = (double)mTime * 0.001 * 0.016666666666666666;
		if (1.0 <= aMinutes)
			mWPM = (int)(((double)aCharsTyped * 0.2) / aMinutes);
		else
		{
			double aInverse = 1.0 / aMinutes;
			mWPM = (int)((((double)aCharsTyped * aInverse) * 0.2) / (aInverse * aMinutes));
			aMinutes = 1.0;
		}

		int aMistakes = mMistakes;
		int aReducedWPM = (int)((double)mWPM * 0.75);
		mWPM = (int)((double)aReducedWPM - ((double)aMistakes * 0.2) / aMinutes);
		int aTotal = aMistakes + aCharsTyped;
		mAccuracy = (int)(((double)aCharsTyped / (double)aTotal) * 100.0);
	}

	if (!mAbyss)
		Dialog::Resize(mApp->mWidth / 2 - 200, mApp->mHeight / 2 - 0x96, 400, 300);
	else
		Dialog::Resize(mApp->mWidth / 2 - 0xb9, mApp->mHeight / 2 - 0xbe, 0x172, 0x17c);

	if (mWPM < -1)
		mWPM = -1;
}

// 0x43b200 (body; the scalar deleting destructor is 0x43b1d0): the rank title is freed with the object (std::string), then
// Dialog's destructor body
GameOverDialog::~GameOverDialog()
{
}

// 0x43b250 slot 16: the lines are drawn one under the other from the top (the font of a header, the rest in the lines font);
// every line is a text with colour codes, so the numbers are in their own colours. A normal game's score and rank lines are
// centred; an abyss game's lines are right-justified (50 pixels in from the right) and followed by the note on saving. The
// speed and the level are on one row, the accuracy and the time on the next.
void GameOverDialog::Draw(Graphics* g)
{
	std::string aString;
	Dialog2004::Draw(g);

	int aWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	g->SetFont(mApp->mFontRockwell13Bold);
	int aY = mContentInsets.mTop + 0x3c;
	aString = StrFormat(GTC(0x0052b19c), mPlayer->mScore);		// not used: every path below makes its own text
	g->SetColor(Color(0, 0, 0));
	if (!mAbyss)
	{
		aString = StrFormat(GTC(0x0052e854), mPlayer->mScore);
		WriteWordWrapped2004(g, Rect(mContentInsets.mLeft, aY, aWidth, mApp->mFontRockwell13Bold->GetHeight()), aString, -1, 0);
		aY = (aY + g->GetFont()->GetHeight()) - 5;

		// the original tests the flag again here and goes on to the abyss lines when it is set (it never is at this point)
		aString = StrFormat(GTC(0x0052e840), mRankTitle.c_str());
		WriteWordWrapped2004(g, Rect(mContentInsets.mLeft, aY, aWidth, mApp->mFontRockwell13Bold->GetHeight()), aString, -1, 0);
		aY = aY + g->GetFont()->GetHeight();
	}
	else
	{
		aString = StrFormat(GTC(0x0052e828), mPlayer->mScore);
		int aLineHeight = mApp->mFontRockwell13Bold->GetHeight();
		aWidth = aWidth - 0x32;
		WriteWordWrapped2004(g, Rect(0, aY, aWidth, aLineHeight), aString, -1, 1);
		g->SetColor(Color(100, 100, 100));
		aY = aY + g->GetFont()->GetHeight();

		aString = StrFormat(GTC(0x0052e80c), mApp->mUser->mGroups[5].mBestScore);
		WriteWordWrapped2004(g, Rect(0, aY, aWidth, mApp->mFontRockwell13Bold->GetHeight()), aString, -1, 1);
		aY = (aY + g->GetFont()->GetHeight()) + 10;

		aString = StrFormat(GTC(0x0052e7ec), mField144);
		WriteWordWrapped2004(g, Rect(0, aY, aWidth, mApp->mFontRockwell13Bold->GetHeight()), aString, -1, 1);
		aY = aY + g->GetFont()->GetHeight();

		// the best depth shown is the greater of the user's and this game's
		int aBestDepth = mApp->GetUserBestDepth();
		g->SetColor(Color(100, 100, 100));
		int aShownDepth = mField144;
		if (aBestDepth >= mField144)
			aShownDepth = aBestDepth;
		aString = StrFormat(GTC(0x0052e7d4), aShownDepth);
		WriteWordWrapped2004(g, Rect(0, aY, aWidth, mApp->mFontRockwell13Bold->GetHeight()), aString, -1, 1);
		aY = aY + 0xf;
	}

	g->SetFont(mApp->mFontRockwell13);
	if (mWPM < 0)
		aString = GTC(0x0052e7d0);
	else
		aString = StrFormat(GTC(0x0052b19c), mWPM);
	g->SetColor(Color(0, 0, 0));
	aY = (aY + g->GetFont()->GetHeight() * 2) - 10;
	WriteWordWrapped2004(g, Rect(mContentInsets.mLeft + 10, aY, 0x80, 0x14), StrFormat(GTC(0x0052e7b8), aString.c_str()),
		-1, 1);

	g->SetColor(Color(0, 0, 0));
	if (mAbyss)
		aString = StrFormat(GTC(0x0052e7a8));
	else
		aString = StrFormat(GTC(0x0052e79c), mPlayer->mField114 + 1);
	WriteWordWrapped2004(g, Rect(0xd2, aY, 0x80, 0x14), StrFormat(GTC(0x0052e788), aString.c_str()), -1, -1);
	aY = aY + g->GetFont()->GetHeight();

	if (mAccuracy == -1)
		aString = GTC(0x0052e77c);
	else
		aString = StrFormat(GTC(0x0052e76c), mAccuracy);
	WriteWordWrapped2004(g, Rect(mContentInsets.mLeft - 10, aY, 0x94, 0x14), StrFormat(GTC(0x0052e754), aString.c_str()),
		-1, 1);

	// the time of the game as minutes:seconds (a 0 before a single digit of seconds)
	if ((mField11c / 1000) % 0x3c < 10)
		aString = StrFormat(GTC(0x0052e744), (mField11c / 1000) / 0x3c, (mField11c / 1000) % 0x3c);
	else
		aString = StrFormat(GTC(0x0052e73c), (mField11c / 1000) / 0x3c, (mField11c / 1000) % 0x3c);
	WriteWordWrapped2004(g, Rect(0xd2, aY, 0x80, 0x14), StrFormat(GTC(0x0052e720), aString.c_str()), -1, -1);

	if (mAbyss)
	{
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(100, 100, 100));
		aY = (aY + g->GetFont()->GetHeight()) + 0x14;
		aString = StrFormat(GTC(0x0052e6f8), 400);
		WriteWordWrapped2004(g, Rect(0x4b, aY, 0xc8, g->GetFont()->GetHeight() * 3), aString, -1, -1);
	}
}
