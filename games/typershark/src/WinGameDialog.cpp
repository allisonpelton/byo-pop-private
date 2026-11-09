#include "WinGameDialog.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "DialogButton.h"
#include "Common.h"

using namespace Sexy;

// Not translated (forwarders to Dialog's own functions; nothing to override): AddedToManager 0x4379c0 (slot 1),
// RemovedFromManager 0x4379d0 (slot 2) and Resize 0x4379e0 (slot 13)

// 0x43a4f0: the two buttons are "yes" and "no" (mode 1), no header lines or footer. The dialog is made by the map screen's update,
// which sets the three counters afterwards.
WinGameDialog::WinGameDialog(WinWSApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_WIN_GAME, true, GT(0x0052e554), "", "",
		Dialog::BUTTONS_YES_NO),
	mApp(theApp),
	mCharsTyped(0),
	mTime(0),
	mMistakes(0)
{
	SetButtonFont(theApp->mFontRockwell13);
	SetHeaderFont(mApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	Dialog::SetColor(COLOR_HEADER, Color(0, 0, 0));
	Dialog::SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets.mLeft = 0x28;
	mContentInsets.mRight = 0x28;
	mContentInsets.mBottom = 0x28;
	mContentInsets.mTop = 0x14;
	mYesButton->mLabel = GT(0x0052e548);
	mNoButton->mLabel = GT(0x0052e53c);
	mTime = 0;
	mCharsTyped = 0;
	mMistakes = 0;
}

// 0x437990 (the destructor body, which the export names ~CreditsDialog: it sets the vftables and calls Dialog's destructor
// body, and nothing else); 0x438e70 is the scalar deleting destructor
WinGameDialog::~WinGameDialog()
{
}

// 0x4379b0 slot 50: a fixed height (the box is taller than its text)
int WinGameDialog::GetPreferredHeight(int theWidth)
{
	return 0x1ea;
}

// One of the ten pictures of the bottom rows: the gem or treasure in a 30 by 30 square, and the count the user has under it
// (the text is grey while there is none). The original has this block ten times in Draw; the picture is read from the app each
// time, and the colour is made before the text.
static void DrawCollectedItem(Graphics* g, Image* theImage, int theX, int theY, int theCount, int theTextX, int theTextY)
{
	g->DrawImage(theImage, Rect(theX, theY, 0x1e, 0x1e), Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
	if (theCount == 0)
		g->SetColor(Color(0x80, 0x80, 0x80));
	else
		g->SetColor(Color(0, 0, 0));
	g->DrawString(StrFormat(GTC(0x0052e144), theCount), theTextX, theTextY);
}

// 0x439390 slot 16: after the box the text of the game's totals, the medal (for the number of finished games) and the pictures
// of what the user has collected. The plain speed is words (5 characters) per minute of the game's time; the mistakes per minute
// use at least one minute; the adjusted speed is 0.75 of the plain speed less the mistakes per minute.
void WinGameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);

	int aCharsTyped = mCharsTyped;
	double aMinutes = (double)mTime * 0.001 * 0.016666666666666666;
	double aWPM;
	if (1.0 <= aMinutes)
		aWPM = ((double)aCharsTyped * 0.2) / aMinutes;
	else
	{
		aWPM = (((double)aCharsTyped * (1.0 / aMinutes)) * 0.2) / ((1.0 / aMinutes) * aMinutes);
		aMinutes = 1.0;
	}

	double aMistakesPerMinute = ((double)mMistakes * 0.2) / aMinutes;
	int aTotal = mMistakes + aCharsTyped;
	UserProfile* aUser = mApp->mUser;
	int aAccuracy = (int)(((double)aCharsTyped / (double)aTotal) * 100.0);
	int anAdjustedWPM = (int)(aWPM * 0.75 - aMistakesPerMinute);
	std::string aText = StrFormat(GTC(0x0052e148), mApp->GetPointsSoFar(), aUser->mField1f4, aUser->mField1f8,
		anAdjustedWPM, aAccuracy);

	int aTextY = (((mContentInsets.mTop - mHeaderFont->GetAscentPadding()) + mHeaderFont->GetAscent()) +
		(mHeaderFont->GetHeight() - mHeaderFont->GetAscent())) + mSpaceAfterHeader + 0x14;
	Rect aTextRect(mContentInsets.mLeft + 2, aTextY, ((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - 4, 0);
	WriteWordWrapped2004(g, aTextRect, aText, mLinesFont->GetLineSpacing() + mLineSpacingOffset, -1);

	// the medal: bronze for 1 or 2 finished games, silver for 3 or 4, gold for more; none for 0
	int aGamesWon = mApp->mUser->mGamesCompleted;
	if (aGamesWon != 0)
	{
		Image* aMedal;
		if (aGamesWon < 3)
			aMedal = mApp->mImages[IMAGE_MEDAL_BRONZE];
		else if (aGamesWon < 5)
			aMedal = mApp->mImages[IMAGE_MEDAL_SILVER];
		else
			aMedal = mApp->mImages[IMAGE_MEDAL_GOLD];

		if (aMedal != NULL)
			g->DrawImage(aMedal, Rect(0x4c, 0x96, 0x42, 0x5a), Rect(0, 0, aMedal->GetWidth(), aMedal->GetHeight()));
	}

	// the six gems, then the four treasures: two rows, five columns 0x50 apart
	DrawCollectedItem(g, mApp->mImages[IMAGE_GEM_GREEN], mContentInsets.mLeft, 0x10e, mApp->mUser->mField234[0], 0x4b, 0x124);
	DrawCollectedItem(g, mApp->mImages[IMAGE_GEM_ORANGE], mContentInsets.mLeft, 0x131, mApp->mUser->mField234[1], 0x4b, 0x147);
	DrawCollectedItem(g, mApp->mImages[IMAGE_GEM_PURPLE], mContentInsets.mLeft + 0x50, 0x10e, mApp->mUser->mField234[2], 0x9b, 0x124);
	DrawCollectedItem(g, mApp->mImages[IMAGE_GEM_RED], mContentInsets.mLeft + 0x50, 0x131, mApp->mUser->mField234[3], 0x9b, 0x147);
	DrawCollectedItem(g, mApp->mImages[IMAGE_GEM_WHITE], mContentInsets.mLeft + 0xa0, 0x10e, mApp->mUser->mField234[4], 0xeb, 0x124);
	DrawCollectedItem(g, mApp->mImages[IMAGE_GEM_YELLOW], mContentInsets.mLeft + 0xa0, 0x131, mApp->mUser->mField234[5], 0xeb, 0x147);
	DrawCollectedItem(g, mApp->mImages[IMAGE_SECRET_CROWN], mContentInsets.mLeft + 0xf0, 0x10e, mApp->mUser->mField24c[0], 0x13b, 0x124);
	DrawCollectedItem(g, mApp->mImages[IMAGE_SECRET_FIGURINE], mContentInsets.mLeft + 0xf0, 0x131, mApp->mUser->mField24c[1], 0x13b, 0x147);
	DrawCollectedItem(g, mApp->mImages[IMAGE_SECRET_NECKLACE], mContentInsets.mLeft + 0x140, 0x10e, mApp->mUser->mField24c[2], 0x18b, 0x124);
	DrawCollectedItem(g, mApp->mImages[IMAGE_SECRET_SCEPTER], mContentInsets.mLeft + 0x140, 0x131, mApp->mUser->mField24c[3], 0x18b, 0x147);
}
