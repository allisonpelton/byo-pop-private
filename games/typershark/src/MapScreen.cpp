// MapScreen.cpp: the map screen (class Sexy::MapScreen, constructor 0x458d10): the table of the 36 dive sites, construction
// and destruction, the Widget overrides that are not Draw or Update, and the choice of a level. Draw is in
// MapScreenDraw.cpp, Update in MapScreenUpdate.cpp (three files because the whole class is over 1000 lines).
//
// Left out (the brief): nothing of the class is registration code or an outbound button. The screen has no such calls.
//
// Quirks of the original, kept (for the game notes):
//  - ResetScroll (0x441030) sets both scroll targets from the width difference (app width - map picture width), where
//    Draw's final-panel test (0x447cd3) compares mScrollY with the height difference; Update drives mScrollY to the width
//    difference too. The Gumbo ship of the final panel shows only if the two differences are equal.
//  - The site with number 35 has the link -1. Update (0x45af60) and SetVisible (0x450ec0) would look up site -1 (making
//    a null entry in the map and dereferencing it) if they ran for it; both are skipped by the final-level flag, which
//    the app sets (SetFinalLevel) when site 35 is chosen.
//  - OnLevelChosen (0x4538e0) reads mVector1d4[size - 2], which is the site before the chosen one; it needs two entries.
//  - A dead end (OnEnter, 0x4533b0) takes the link out of the site before it and moves that site behind the dead end,
//    so the ship sails back.

#include "MapScreen.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "LessonManager.h"
#include "XWidget.h"
#include "TooltipWidget.h"
#include "WSTimer.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "GameText.h"
#include "Common.h"
#include "Font.h"
#include "ImageFont.h"
#include "Image.h"
#include "Res.h"
#include <math.h>

using namespace Sexy;

// 0x450e80 (the MapNode constructor) with the fields that the map's constructor sets after it: this helper stands for the
// block "new MapNode, set the number, the two flags, x, y and the text" that the original repeats 36 times in 0x458d10. The
// text is the game's own, named by its address.
static MapNode* MakeNode(int theId, bool theVisited, bool theDeadEnd, int theX, int theY, unsigned int theText)
{
	MapNode* aNode = new MapNode();
	aNode->mId = theId;
	aNode->mDeadEnd = theDeadEnd;
	aNode->mVisited = theVisited;
	aNode->mX = theX;
	aNode->mY = theY;
	aNode->mText = GT(theText);
	return aNode;
}

// 0x4537b0: a function with three arguments (it does not use a this and pops them itself). The dots of the route between two
// sites: one every 8 pixels of the line, without the ones next to the sites (the loop runs from the third dot to the one
// before the last but one). Each dot is a new pair that the caller's list owns (the original allocates it with malloc).
void Sexy::MapAddRoutePoints(MapNode* theFrom, MapNode* theTo, std::vector<ProfilePair*>& theVector)
{
	int aDx = (theTo->mX + 8) - (theFrom->mX + 8);
	int aDy = (theTo->mY + 8) - (theFrom->mY + 8);
	int aCount = (int)(sqrt((double)(aDx * aDx + aDy * aDy)) * 0.125);
	int aStep = 2;
	if (2 < aCount - 1)
	{
		do
		{
			ProfilePair* aPair = new ProfilePair;
			aPair->mFirst = 0;
			aPair->mSecond = 0;
			aPair->mFirst = ((theTo->mX - theFrom->mX) * aStep) / aCount + theFrom->mX;
			aPair->mSecond = ((theTo->mY - theFrom->mY) * aStep) / aCount + theFrom->mY;
			theVector.push_back(aPair);
			aStep++;
		} while (aStep < aCount - 1);
	}
}

// 0x4506a0: an XWidget method of the original (thiscall; the text goes in by value on the stack): the chest's tooltip gets
// the text and the app's dialog font (+0x8bc, Rockwell 13). XWidget keeps its app as a SexyAppBase.
void Sexy::MapSetChestTip(XWidget* theChest, std::string theText)
{
	WinWSApp* anApp = static_cast<WinWSApp*>(theChest->mApp);
	theChest->mTooltip->SetTextAndFont(theText, anApp->mFontRockwell13);
}

// 0x458d10. Widget first; the numbers and the strings; the prompt's end position from the width of its text; the ship's
// timer (a new WSTimer, disabled); then the 36 sites, with node 0 (the harbour, already visited) as the ship's first stop;
// last OnEnter. The original leaves some members unset until OnEnter or Update (the port zeroes them: the ones marked here).
MapScreen::MapScreen(WinWSApp* theApp)
{
	// port: these are unset in the original until OnEnter / Update / the app writes them
	mDifficultyPercent = 0.0;	// OnEnter's switch leaves it alone for a difficulty it does not know (the abyss, -1)
	mGumboPeriod = 0;
	mHintPending = false;
	mShipArrived = false;
	mFlag1e4 = false;
	mDotIndex = 0;
	mFlag1ec = false;
	mArrivals = 0;
	mHeadingOn = false;
	mTipX = 0;
	mTipY = 0;

	mGumboFrame = 0;
	mShipFrame = 0;
	mContinueLink = NULL;
	mDotsDone = 0;

	mApp = theApp;
	mFlag234 = false;
	mPromptX = theApp->mWidth;
	mPromptEndX = theApp->mFontRockwell13->StringWidth(GT(0x0052f988)) / 2 - 0x78;
	mFinal = false;
	mTimer = new WSTimer();
	mLevel = 0;
	mScrollX = 0;
	mScrollY = 0;
	mScrolling = false;
	mField1f8 = 0;
	mTimer->SetDisabled(true);
	mFlag224 = true;

	// the 36 sites: the number, the visited and dead-end flags, the position, the text; then the links; then the map entry
	MapNode* aNode;
	aNode = MakeNode(0, true, false, 115, 42, 0x005313a0);
	aNode->mLinks.push_back(1);
	mVector1d4.push_back(aNode);
	mMap1a8[0] = aNode;
	aNode = MakeNode(1, false, false, 178, 82, 0x00531384);
	aNode->mLinks.push_back(2); aNode->mLinks.push_back(6); aNode->mLinks.push_back(7);
	mMap1a8[1] = aNode;
	aNode = MakeNode(2, false, false, 277, 81, 0x00531360);
	aNode->mLinks.push_back(3);
	mMap1a8[2] = aNode;
	aNode = MakeNode(3, false, false, 336, 86, 0x00531334);
	aNode->mLinks.push_back(8); aNode->mLinks.push_back(4);
	mMap1a8[3] = aNode;
	aNode = MakeNode(4, false, false, 464, 64, 0x00531300);
	aNode->mLinks.push_back(5); aNode->mLinks.push_back(14);
	mMap1a8[4] = aNode;
	aNode = MakeNode(5, false, true, 574, 81, 0x005312dc);
	aNode->mLinks.push_back(4);
	mMap1a8[5] = aNode;
	aNode = MakeNode(6, false, false, 91, 158, 0x005312b8);
	aNode->mLinks.push_back(9); aNode->mLinks.push_back(10);
	mMap1a8[6] = aNode;
	aNode = MakeNode(7, false, false, 259, 164, 0x00531294);
	aNode->mLinks.push_back(12);
	mMap1a8[7] = aNode;
	aNode = MakeNode(8, false, false, 401, 139, 0x00531334);
	aNode->mLinks.push_back(14); aNode->mLinks.push_back(13);
	mMap1a8[8] = aNode;
	aNode = MakeNode(9, false, false, 67, 246, 0x00531260);
	aNode->mLinks.push_back(21);
	mMap1a8[9] = aNode;
	aNode = MakeNode(10, false, false, 168, 215, 0x00531238);
	aNode->mLinks.push_back(21); aNode->mLinks.push_back(11);
	mMap1a8[10] = aNode;
	aNode = MakeNode(11, false, false, 248, 227, 0x00531214);
	aNode->mLinks.push_back(15); aNode->mLinks.push_back(16);
	mMap1a8[11] = aNode;
	aNode = MakeNode(12, false, false, 297, 209, 0x005311ec);
	aNode->mLinks.push_back(13); aNode->mLinks.push_back(11);
	mMap1a8[12] = aNode;
	aNode = MakeNode(13, false, false, 383, 180, 0x005311c8);
	aNode->mLinks.push_back(17);
	mMap1a8[13] = aNode;
	aNode = MakeNode(14, false, false, 471, 180, 0x00531198);
	aNode->mLinks.push_back(19);
	mMap1a8[14] = aNode;
	aNode = MakeNode(15, false, false, 230, 263, 0x0053116c);
	aNode->mLinks.push_back(23);
	mMap1a8[15] = aNode;
	aNode = MakeNode(16, false, false, 286, 278, 0x00531148);
	aNode->mLinks.push_back(23);
	mMap1a8[16] = aNode;
	aNode = MakeNode(17, false, false, 399, 244, 0x0053111c);
	aNode->mLinks.push_back(24); aNode->mLinks.push_back(18);
	mMap1a8[17] = aNode;
	aNode = MakeNode(18, false, true, 444, 252, 0x005310f8);
	aNode->mLinks.push_back(17);
	mMap1a8[18] = aNode;
	aNode = MakeNode(19, false, false, 529, 238, 0x005310d0);
	aNode->mLinks.push_back(25);
	mMap1a8[19] = aNode;
	aNode = MakeNode(20, false, true, 46, 365, 0x005310b4);
	aNode->mLinks.push_back(21);
	mMap1a8[20] = aNode;
	aNode = MakeNode(21, false, false, 86, 340, 0x00531238);
	aNode->mLinks.push_back(26); aNode->mLinks.push_back(20);
	mMap1a8[21] = aNode;
	aNode = MakeNode(22, false, true, 196, 328, 0x00531090);
	aNode->mLinks.push_back(23);
	mMap1a8[22] = aNode;
	aNode = MakeNode(23, false, false, 249, 320, 0x00531068);
	aNode->mLinks.push_back(22); aNode->mLinks.push_back(28);
	mMap1a8[23] = aNode;
	aNode = MakeNode(24, false, false, 363, 308, 0x00531040);
	aNode->mLinks.push_back(28);
	mMap1a8[24] = aNode;
	aNode = MakeNode(25, false, false, 542, 284, 0x00531018);
	aNode->mLinks.push_back(30);
	mMap1a8[25] = aNode;
	aNode = MakeNode(26, false, false, 168, 381, 0x00530ffc);
	aNode->mLinks.push_back(27);
	mMap1a8[26] = aNode;
	aNode = MakeNode(27, false, false, 236, 406, 0x00530fd4);
	aNode->mLinks.push_back(31); aNode->mLinks.push_back(32);
	mMap1a8[27] = aNode;
	aNode = MakeNode(28, false, false, 294, 342, 0x00530fbc);
	aNode->mLinks.push_back(29);
	mMap1a8[28] = aNode;
	aNode = MakeNode(29, false, false, 387, 347, 0x00530fa4);
	aNode->mLinks.push_back(34);
	mMap1a8[29] = aNode;
	aNode = MakeNode(30, false, false, 526, 338, 0x00530f7c);
	aNode->mLinks.push_back(34);
	mMap1a8[30] = aNode;
	aNode = MakeNode(31, false, false, 303, 375, 0x00530f58);
	aNode->mLinks.push_back(33);
	mMap1a8[31] = aNode;
	aNode = MakeNode(32, false, false, 292, 425, 0x00530f30);
	aNode->mLinks.push_back(33);
	mMap1a8[32] = aNode;
	aNode = MakeNode(33, false, false, 350, 420, 0x00530f20);
	aNode->mLinks.push_back(34);
	mMap1a8[33] = aNode;
	aNode = MakeNode(34, false, false, 448, 372, 0x00530f10);
	aNode->mLinks.push_back(35);
	mMap1a8[34] = aNode;
	aNode = MakeNode(35, false, false, 496, 425, 0x00530ee8);
	aNode->mLinks.push_back(-1);
	mMap1a8[35] = aNode;

	OnEnter();	// 0x4533b0
}

// 0x45acf0 (deleting destructor; body 0x4530f0). The 36 sites are freed by number (the original frees the strings and
// link lists by hand), then the two lists of dots, the chests (taken off the manager and queued for deletion), the link
// and the timer; the map's entries are erased last.
MapScreen::~MapScreen()
{
	int aNodeId = 0;
	do
	{
		MapNode* aNode = mMap1a8[aNodeId];
		if (aNode != NULL)
			delete aNode;
		aNodeId++;
	} while (aNodeId < 0x24);

	unsigned int i = 0;
	while (i < mVector1c4.size())
	{
		delete mVector1c4[i];
		mVector1c4[i] = NULL;
		i++;
	}

	i = 0;
	while (i < mVector198.size())
	{
		delete mVector198[i];
		i++;
	}

	i = 0;
	while (i < mVector1b4.size())
	{
		mApp->mWidgetManager->RemoveWidget(mVector1b4[i]);
		mApp->SafeDeleteWidget(mVector1b4[i]);
		i++;
	}

	if (mContinueLink != NULL)
		delete mContinueLink;
	if (mTimer != NULL)
		delete mTimer;

	mMap1a8.clear();	// 0x44cfd0 (erase of the whole map; the vectors and strings free themselves)
}

// 0x4480d0 slot 1: the "Press space bar to continue" link (id 0x1cf3, the app is its listener) is made hidden, with an
// underline of 1, the dialog font, white when the pointer is over it and green otherwise; it sits centred above the bottom
// edge (10 pixels above it); then the link and the ship's timer join the manager.
void MapScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	mContinueLink = new HyperlinkWidget(0x1cf3, mApp);
	mContinueLink->SetVisible(false);
	mContinueLink->mUnderlineSize = 1;
	mContinueLink->SetFont(mApp->mFontRockwell13);
	mContinueLink->mOverColor = Color(255, 255, 255);
	mContinueLink->mColor = Color(0, 255, 0);
	mContinueLink->mLabel = GT(0x0052f8d8);
	mContinueLink->SetDisabled(true);
	int aWidth = mContinueLink->mFont->StringWidth(mContinueLink->mLabel);
	mContinueLink->Resize(mApp->mWidth / 2 - aWidth / 2, (mApp->mHeight - mContinueLink->mFont->GetHeight()) - 10,
		aWidth + 10, mContinueLink->mFont->GetHeight() + 10);
	theWidgetManager->AddWidget(mContinueLink);
	mContinueLink->mDoFinger = true;
	mContinueLink->SetDisabled(false);
	theWidgetManager->AddWidget(mTimer);
}

// 0x4410b0 slot 2
void MapScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mContinueLink != NULL)
		theWidgetManager->RemoveWidget(mContinueLink);
	theWidgetManager->RemoveWidget(mTimer);
}

// 0x450ec0 slot 4: the link and the chests follow the screen's visibility (the link is disabled while hidden and comes to
// the front when shown). Shown, for a player who has not seen the hint (profile flag 0x284) and when the map is not on the
// final panel: if one of the sites that can be sailed to next is unvisited but has an open chest in the profile, the hint
// ("An open treasure chest means ...") is to be shown by Update once.
void MapScreen::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);
	if (mContinueLink != NULL)
	{
		mContinueLink->SetVisible(isVisible);
		mContinueLink->SetDisabled(isVisible == false);
		if (isVisible)
			mApp->mWidgetManager->BringToFront(mContinueLink);
	}

	unsigned int i = 0;
	while (i < mVector1b4.size())
	{
		mVector1b4[i]->SetVisible(isVisible);
		i++;
	}

	if (isVisible == false || mApp->mUser->mFlag284 != false || mFinal != false)
		return;

	unsigned int aLink = 0;
	while (true)
	{
		MapNode* aLast = mVector1d4.back();
		if (aLink >= aLast->mLinks.size())
			return;
		if (mMap1a8[aLast->mLinks[aLink]]->mVisited == false)
		{
			if (mApp->mUser->mField25c[mVector1d4.back()->mLinks[aLink]] != 0)
			{
				mHintPending = true;
				return;
			}
		}
		aLink++;
	}
}

// 0x442880 slot 20: the letter (a lower case one is made upper case) of the chosen chest or the space bar presses the
// continue link (the app's ButtonDepress 0x1cf3); while no level is chosen (and the final panel is not up) a letter that
// belongs to a chest clicks it.
void MapScreen::KeyChar(SexyChar theChar)
{
	Widget::KeyChar(theChar);
	char aChar = (char)theChar;
	if (aChar >= 'a' && aChar <= 'z')
		aChar = aChar - 0x20;

	if (aChar == ' ' || aChar == mChosenLetter)
		mApp->ButtonDepress(0x1cf3);

	if (mFlag234 == false && mFlag224 == false)
	{
		unsigned int i = 0;
		while (true)
		{
			if (i >= mVector1b4.size())
				return;
			if (aChar == mVector1b4[i]->mLetter)
				break;
			i++;
		}
		mVector1b4[i]->MouseDown(0, 0, 1);
	}
}

// 0x4533b0: the map shows again (after a level, or at construction). The prompt text starts off the right edge, the level
// counter goes up, no level is chosen, the ship's timer stops, the dots behind the ship are all the dots there are, the
// Gumbo frame and the hint start over; the dots of the last fork are freed; the last site is marked visited (a dead end
// is left again: see the file's note); then the difficulty scales the level number into the percentage shown on the final
// panel and sets how fast Gumbo blinks.
void MapScreen::OnEnter()
{
	mPromptX = mApp->mWidth;
	mField1f8 = mField1f8 + 1;
	mDotIndex = mVector1c4.size();
	mArrivals = 0;
	mLevel = -1;
	mFlag1e4 = true;
	mTimer->SetDisabled(true);
	mFlag1ec = true;
	mGumboFrame = 0;
	mHintPending = false;
	mChosenLetter = 0x20;
	mFlag234 = false;
	mApp->SetCursor(0);

	unsigned int i = 0;
	while (i < mVector198.size())
	{
		delete mVector198[i];		// port: the original frees it with free(), as the pair came from malloc
		i++;
	}
	mVector198.clear();

	mVector1d4.back()->mVisited = true;
	if (mVector1d4.back()->mDeadEnd != false)
	{
		MapNode* aPrevious = mVector1d4[mVector1d4.size() - 2];
		MapNode* aCurrent = mVector1d4[mVector1d4.size() - 1];
		unsigned int aLink = 0;
		while (true)
		{
			if (aLink >= aPrevious->mLinks.size())
				break;
			if (aPrevious->mLinks[aLink] == aCurrent->mId)
			{
				aPrevious->mLinks.erase(aPrevious->mLinks.begin() + aLink);
				break;
			}
			aLink++;
		}
		mVector1d4.erase(mVector1d4.begin() + (mVector1d4.size() - 2));
		mVector1d4.push_back(aPrevious);
	}

	int aLevelNumber = 0;
	if (mApp->mLessonManager != NULL)
		aLevelNumber = mApp->mLessonManager->GetEndlessCount() - 1;		// 0x412370
	aLevelNumber = aLevelNumber + 1;
	double aLevels = (double)aLevelNumber;
	switch (mApp->mDifficulty)
	{
	case 0:
		mDifficultyPercent = aLevels * 0.1 * 2.5;
		mDifficultyName = GT(0x0052d280);
		break;
	case 1:
		mDifficultyPercent = aLevels * 0.1 * 2.2222222222222223;
		mDifficultyName = GT(0x0052d278);
		break;
	case 2:
		mDifficultyPercent = aLevels * 0.1;
		mDifficultyPercent = mDifficultyPercent + mDifficultyPercent;
		mDifficultyName = GT(0x0052d270);
		break;
	case 3:
		mDifficultyPercent = aLevels * 0.1 * 1.7857142857142856;
		mDifficultyName = GT(0x0052d268);
		break;
	case 4:
		mDifficultyPercent = aLevels * 0.1 * 1.4285714285714286;
		mDifficultyName = GT(0x0052d260);
		break;
	default:
		break;
	}

	mDifficultyPercent = mDifficultyPercent * 100.0;
	mGumboPeriod = (5 - (int)(mDifficultyPercent * 0.02)) * 10;
	if (mGumboPeriod < 5)
		mGumboPeriod = 5;
}

// 0x4538e0 (the chest is clicked: XWidget::MouseDown has set mLevel and mChosenLetter): every other chest is taken away,
// the dots of the other ways are freed, the chosen site joins the route, the dots of the line to it are added to the
// route, the ship's timer runs and the screen waits for no more choices.
void MapScreen::OnLevelChosen()
{
	unsigned int i = 0;
	while (i < mVector1b4.size())
	{
		if (mVector1b4[i]->mLevel != mLevel)
		{
			mApp->mWidgetManager->RemoveWidget(mVector1b4[i]);
			mApp->SafeDeleteWidget(mVector1b4[i]);
			mVector1b4.erase(mVector1b4.begin() + i);
			i = i - 1;
		}
		i++;
	}

	i = 0;
	while (i < mVector198.size())
	{
		delete mVector198[i];		// port: free() in the original
		i++;
	}
	mVector198.clear();

	MapNode* aNode = mMap1a8[mLevel];
	mVector1d4.push_back(aNode);
	MapAddRoutePoints(mVector1d4[mVector1d4.size() - 2], aNode, mVector1c4);		// 0x4537b0
	mTimer->SetDisabled(false);
	mTimer->Reset();
	mHeadingOn = true;
	mFlag1ec = false;
}

// 0x441030: the scroll targets are the width of the window minus the width of the map picture (the original does the same
// subtraction for both, see the file's note), and the scroll is off.
void MapScreen::ResetScroll()
{
	mScrolling = false;
	mScrollX = mApp->mWidth - mApp->mImages[IMAGE_MAPBG]->mWidth;
	mScrollY = mApp->mWidth - mApp->mImages[IMAGE_MAPBG]->mWidth;
}

// 0x441080: switching to the final panel (the whirlpool site) starts the scroll; switching it off does not stop one
void MapScreen::SetFinalLevel(bool theFinal)
{
	bool aWasFinal = mFinal;
	mFinal = theFinal;
	if (aWasFinal == false && theFinal != false)
		mScrolling = true;
}

// 0x443e20: all the chests go (the app calls it when the level starts)
void MapScreen::ClearNodeWidgets()
{
	unsigned int i = 0;
	while (i < mVector1b4.size())
	{
		mApp->mWidgetManager->RemoveWidget(mVector1b4[i]);
		mApp->SafeDeleteWidget(mVector1b4[i]);
		i++;
	}
	mVector1b4.clear();
}
