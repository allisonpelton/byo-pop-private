// HallOfFame: the screen of the best results (see HallOfFame.h).

#include "HallOfFame.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Common.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Particle.h"
#include "Res.h"
#include "UserProfile.h"
#include "WidgetManager.h"

using namespace Sexy;

// the ids of the pictures that are drawn by the pilot (the pilot's number is added)
static_assert(IMAGE_HOF_VECTOR_ID == 0x26b, "the pilot pictures of the hall of fame");
static_assert(IMAGE_PERSONAL_VECTOR_GAUGE_ID == 0x26f, "the gauges");
static_assert(IMAGE_CS_VECTOR_STATFILL_ID == 0xff, "the bars of the gauges");

namespace
{

// a number from the game's random numbers, below theRange (the original takes the remainder of 500000000 first)
int RandomBelow(int theRange)
{
	return ((int) AstroRand() % 500000000) % theRange;
}

// a time in minutes and seconds, with a nought in front of the seconds when they are under ten
std::string MinutesSeconds(int theSeconds)
{
	int aRest = theSeconds % 60;
	return StrFormat((aRest < 10) ? "%d:0%d" : "%d:%d", theSeconds / 60, aRest);
}

}

// 0x427e30
HallOfFame::HallOfFame(AstroPopApp* theApp) :
	mApp(theApp),
	mCloseButton(NULL),
	mPageButton(NULL),
	mPage(0),
	mInactive(false)
{
}

// 0x45c2c0: the two buttons go with it (the application takes the screen off the manager first)
HallOfFame::~HallOfFame()
{
	delete mCloseButton;
	delete mPageButton;
}

// 0x455c90: the buttons come, and the stars are scattered over the screen (the drawing of the random numbers for each star
// is in the order of the original: its kind, its speed, then the place)
void HallOfFame::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);

	mCloseButton = new AstroButton(BUTTON_ID_CLOSE, this);
	mCloseButton->mButtonImage = NULL;
	mCloseButton->mOverImage = IMAGE_HOF_CONTINUE_OVER;
	mCloseButton->mDownImage = IMAGE_HOF_CONTINUE_DOWN;
	mCloseButton->Resize(0x1d0, 0x1b7, IMAGE_HOF_CONTINUE_OVER->mWidth, IMAGE_HOF_CONTINUE_OVER->mHeight);
	theManager->AddWidget(mCloseButton);
	theManager->BringToFront(mCloseButton);

	mPage = 0;
	mPageButton = new AstroButton(BUTTON_ID_PAGE, this);
	mPageButton->mButtonImage = IMAGE_HOF_PERSONAL_UP;
	mPageButton->mOverImage = IMAGE_HOF_PERSONAL_OVER;
	mPageButton->mDownImage = IMAGE_HOF_PERSONAL_DOWN;
	mPageButton->Resize(0x19, 0x1b7, IMAGE_HOF_ALLTIME_UP->mWidth, IMAGE_HOF_ALLTIME_UP->mHeight);
	theManager->AddWidget(mPageButton);
	theManager->BringToFront(mPageButton);

	mStars.resize(NUM_STARS);
	for (int i = 0; i < NUM_STARS; i++)
	{
		Star& aStar = mStars[i];
		int aKind = RandomBelow(3);
		if (aKind == 0)
		{
			aStar.mColor = Color(0x50, 0x50, 0x50);
			aStar.mSpeed = (float) ((double) (float) RandomBelow(10) * (double) 0.01f + (double) 0.25f);
		}
		else if (aKind == 1)
		{
			aStar.mColor = Color(0x96, 0x96, 0x96);
			aStar.mSpeed = (float) ((double) (float) RandomBelow(10) * (double) 0.01f + (double) 0.5f);
		}
		else
		{
			aStar.mColor = Color(0xfa, 0xfa, 0xfa);
			aStar.mSpeed = (float) ((double) (float) RandomBelow(10) * (double) 0.01f + (double) 1.0f);
		}
		aStar.mX = (float) RandomBelow(0x280);
		aStar.mY = (float) (RandomBelow(0x1dc) + 4);
	}
}

// 0x4350e0
void HallOfFame::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mCloseButton);
	theManager->RemoveWidget(mPageButton);
}

// 0x43c340: the stars fall; one that has left at the bottom comes in again at the top
void HallOfFame::Update()
{
	Widget::Update();
	if (!mInactive)
	{
		for (size_t i = 0; i < mStars.size(); i++)
		{
			Star& aStar = mStars[i];
			aStar.mY = aStar.mSpeed + aStar.mY;
			if (aStar.mY > 480.0f)
			{
				aStar.mX = (float) RandomBelow(0x280);
				aStar.mY = (float) (RandomBelow(0x32) - 100);
			}
		}
		MarkDirty();
	}
}

// 0x435140: the right button closes the screen, the left one turns the page and shows the picture of the other one
void HallOfFame::ButtonDepress(int theId)
{
	if (theId == BUTTON_ID_CLOSE)
	{
		mApp->CloseHallOfFame();
		return;
	}
	if (theId == BUTTON_ID_PAGE)
	{
		mPage++;
		if (mPage > 1)
		{
			mPage = 0;
			mPageButton->mButtonImage = IMAGE_HOF_PERSONAL_UP;
			mPageButton->mOverImage = IMAGE_HOF_PERSONAL_OVER;
			mPageButton->mDownImage = IMAGE_HOF_PERSONAL_DOWN;
			return;
		}
		mPageButton->mButtonImage = IMAGE_HOF_ALLTIME_UP;
		mPageButton->mOverImage = IMAGE_HOF_ALLTIME_OVER;
		mPageButton->mDownImage = IMAGE_HOF_ALLTIME_DOWN;
	}
}

// 0x446840: a name that is wider than eight capital Ws is cut where it stops fitting and ends in three dots
std::string HallOfFame::FitName(Font* theFont, const std::string& theName)
{
	int aMaxWidth = theFont->StringWidth("WWWWWWWW");
	if (aMaxWidth >= theFont->StringWidth(theName))
		return theName;

	std::string aResult;
	int aWidth = theFont->StringWidth(aResult);
	for (unsigned int i = 0; aWidth < aMaxWidth && i < theName.size(); i++)
	{
		aResult.append(1, theName[i]);
		aWidth = theFont->StringWidth(aResult);
	}
	aResult.erase(aResult.size() - 1, 1);
	aResult += "...";
	return aResult;
}

// 0x44bf80
void HallOfFame::Draw(Graphics* g)
{
	int aRowHeight = IMAGE_HOF_VIXX->mHeight + 1;

	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, 0x280, 0x1e0);
	for (size_t i = 0; i < mStars.size(); i++)
	{
		g->SetColor(mStars[i].mColor);
		g->FillRect((int) mStars[i].mX, (int) mStars[i].mY, 1, 1);
	}
	g->DrawImage(IMAGE_HOF_BG, 0, 0);
	g->SetFont(FONT_HIGH_SCORE);

	if (mPage == 0)
	{
		// the hall of fame: the results of the classic game are in the top list, the survival results (by time) under it
		g->DrawImage(IMAGE_HOF_TITLE_ALLTIME, 0x3c, 7);
		int aClassicRows = 0;
		int aSurvivalRows = 0;
		int aClassicY = 0x4b;
		int aSurvivalY = 0x119;
		for (size_t i = 0; i < mApp->mHall.size(); i++)
		{
			const HallEntry& anEntry = mApp->mHall[i];
			if (anEntry.mKind == 1 && aClassicRows < 5)
			{
				g->DrawImage(GetImageById(anEntry.mMode + IMAGE_HOF_VECTOR_ID), 0x63, aClassicY - 0x15);
				g->SetColor(Color(0xff, 0xff, 0xff));
				g->DrawString(FitName(FONT_HIGH_SCORE, anEntry.mName), 0x90, aClassicY);
				g->SetColor(Color(0xd1, 0xf2, 0x1f));
				WriteString(g, CommaSeperate(anEntry.mScore), 0x174, aClassicY, 0x94, 1, true, 0, -1);
				g->SetColor(Color(0, 0xff, 0x1e));
				WriteString(g, StrFormat("%d", anEntry.mSecond), 0x207, aClassicY, 0x45, 0, true, 0, -1);
				aClassicRows++;
				aClassicY += aRowHeight;
			}
			else if (anEntry.mKind == 2 && aSurvivalRows < 5)
			{
				g->DrawImage(GetImageById(anEntry.mMode + IMAGE_HOF_VECTOR_ID), 0x63, aSurvivalY - 0x15);
				g->SetColor(Color(0xff, 0xff, 0xff));
				g->DrawString(FitName(FONT_HIGH_SCORE, anEntry.mName), 0x90, aSurvivalY);
				g->SetColor(Color(0xd1, 0xf2, 0x1f));
				WriteString(g, CommaSeperate(anEntry.mScore), 0x157, aSurvivalY, 0x94, 1, true, 0, -1);
				g->SetColor(Color(0, 0xff, 0x1e));
				WriteString(g, MinutesSeconds(anEntry.mSecond), 0x1f2, aSurvivalY, 0x45, 0, true, 0, -1);
				aSurvivalRows++;
				aSurvivalY += aRowHeight;
			}
		}
		return;
	}

	// the player's own results: the five best of each mode, each with its pilot and a gauge of how far the pilot has got
	g->DrawImage(IMAGE_HOF_TITLE_PERSONAL, 0x3c, 7);
	ProfileRecord aRecord;
	for (int i = 0; i < 6; i++)
		aRecord.mField[i] = -1;
	int aRows[2] = {0, 0};
	for (int aList = 1; aList <= 2; aList++)
	{
		int aY = (aList == 1) ? 0x35 : 0x103;
		for (int aRow = 0; aRow < 5; aRow++)
		{
			if (mApp->mUser != NULL && mApp->mUser->GetRecord(aList, aRow, &aRecord))
			{
				aRows[aList - 1]++;
				int aPilot = aRecord.mField[1];
				int aScore = aRecord.mField[2];
				int aLevel = aRecord.mField[3];
				int aTime = aRecord.mField[4];
				int aProgress = aRecord.mField[5];
				g->DrawImage(GetImageById(aPilot + IMAGE_HOF_VECTOR_ID), 0x63, aY + 1);

				// the gauge: a picture of the pilot's bar, filled in eighths as far as the pilot has got (the second half of
				// the picture is the full bar)
				int aBarWidth = IMAGE_CS_VECTOR_STATFILL->mWidth;
				int aBarX = 100;
				switch (aPilot)
				{
				case 0:
				case 2:
					aBarX = 100;
					break;
				case 1:
					aBarX = 0x65;
					break;
				case 3:
					aBarX = 0x62;
					break;
				}
				bool aFull = aProgress == 8;
				int aFilled = aBarWidth;
				if (aProgress < 8)
					aFilled = aProgress * (aBarWidth / 8);
				int aBarHeight = IMAGE_CS_VECTOR_STATFILL->mHeight / 2;
				g->DrawImage(GetImageById(aPilot + IMAGE_PERSONAL_VECTOR_GAUGE_ID), 0x97, aY);
				g->DrawImage(GetImageById(aPilot + IMAGE_CS_VECTOR_STATFILL_ID), aBarX + 0x97, aY + 2,
					Rect(0, (aFull ? 1 : 0) * aBarHeight, aFilled, aBarHeight));

				if (aList == 1)
				{
					g->SetColor(Color(0xff, 0xff, 0));
					WriteString(g, CommaSeperate(aScore), 0x174, aY + 0x16, 0x94, 1, true, 0, -1);
					g->SetColor(Color(0, 0xff, 0x1e));
					WriteString(g, StrFormat("%d", aLevel), 0x207, aY + 0x16, 0x45, 0, true, 0, -1);
				}
				else
				{
					g->SetColor(Color(0xff, 0xff, 0x1e));
					WriteString(g, CommaSeperate(aScore), 0x157, aY + 0x16, 0x94, 1, true, 0, -1);
					g->SetColor(Color(0, 0xff, 0x1e));
					WriteString(g, MinutesSeconds(aTime), 0x1f2, aY + 0x16, 0x45, 0, true, 0, -1);
				}
			}
			aY += aRowHeight;
		}
	}

	// a list that has nothing yet says when it gets its first result
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (aRows[0] == 0)
		WriteWordWrapped(g, Rect(0x90, 0x73, 0x178, 200), GT(0x56507c), -1, 0);
	if (aRows[1] == 0)
		WriteWordWrapped(g, Rect(0x90, 0x141, 0x178, 200), GT(0x565044), -1, 0);
}
