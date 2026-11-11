// CharSelector: the pilot choice screen. See CharSelector.h.

#include "CharSelector.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "UserProfile.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// where the buttons stand (0x59db08): the four pilots, then the back button
const int kButtonRect[5][4] =
{
	{ 0x001, 0x24, 0xa0, 0x1a3 },   // Vector
	{ 0x140, 0x24, 0xa0, 0x1a3 },   // Sprocket
	{ 0x1e0, 0x24, 0xa0, 0x1a3 },   // Turbot
	{ 0x0a0, 0x24, 0xa0, 0x1a3 },   // Vixx
	{ 0x0db, 0x1c5, 0xd4, 0x1b }    // back
};

// where the pictures of each pilot are drawn (0x59db58 ..): the pilot, the list of what it can do and the bar under it
const int kPilotPos[4][2] = { { 0x000, 0x65 }, { 0x140, 0x65 }, { 0x1e0, 0x65 }, { 0x0a0, 0x65 } };
const int kStatsPos[4][2] = { { 0x000, 0x148 }, { 0x140, 0x148 }, { 0x1e0, 0x148 }, { 0x0a0, 0x148 } };
const int kFillPos[4][2] = { { 0x069, 0x14a }, { 0x1a8, 0x14a }, { 0x245, 0x14a }, { 0x105, 0x14a } };

// what the pilot's upgrades come to (0 .. 8): three settings of the pilot, each counted from its first level (the
// original reads them in this order, the first two with the default 1 and the third with 0)
int UpgradeLevel(UserProfile* theUser, int theFirstId, int theSecondId, int theThirdId)
{
	int aFirst = 1;
	theUser->GetInt(theFirstId, &aFirst);
	aFirst = aFirst - 1;
	if (aFirst < 0)
		aFirst = 0;
	int aSecond = 1;
	theUser->GetInt(theSecondId, &aSecond);
	aSecond = aSecond - 1;
	if (aSecond < 0)
		aSecond = 0;
	int aSum = aFirst + aSecond;
	if (theThirdId != -1)
	{
		int aThird = 0;
		theUser->GetInt(theThirdId, &aThird);
		aThird = aThird - 1;
		if (aThird < 0)
			aThird = 0;
		aSum = aSum + aThird;
	}
	return aSum;
}

// a result of the second mode: the setting is a level of the forty, shown as a percent (the original converts the
// product of the setting and two constants, 0.025 and 100.0, with _ftol)
int ReadPercent(UserProfile* theUser, int theId)
{
	int aValue = 0;
	theUser->GetInt(theId, &aValue);
	return (int) ((long double) aValue * (long double) 0.025f * (long double) 100.0f);
}

// rounded down to a multiple of four
int RoundDown4(int theValue)
{
	if (theValue < 4)
		return 0;
	return ((theValue + ((theValue >> 31) & 3)) >> 2) << 2;
}

}

// 0x4556c0
CharSelector::CharSelector(AstroPopApp* theApp, int theMode)
{
	mApp = theApp;
	mMode = theMode;
	mTurbotLocked = !theApp->mUser->HasModeUnlocked(2);
	mVixxLocked = !theApp->mUser->HasModeUnlocked(3);

	mButtons.resize(5, (AstroButton*) NULL);
	for (int i = 0; i < 5; i++)
	{
		AstroButton* aButton = new AstroButton(i, this);
		aButton->mButtonImage = NULL;
		if (i < 4)
		{
			aButton->mUseTintNormal = true;
			aButton->mUseTintOver = true;
			aButton->mTintNormal = Color(0xa0, 0xa0, 0xa0, 0xff);
			aButton->mTintOver = Color(0xff, 0xff, 0xff, 0xff);

			// the lit picture of the pilot, or the one with the lock over it while the pilot is not open
			// (the original shows Sprocket's lock while the copy is not registered; the port's copy is)
			Image* anImage = NULL;
			if (i == PILOT_TURBOT && mTurbotLocked)
				anImage = IMAGE_CS_TURBOT_HILIGHT_LOCK;
			else if (i == PILOT_VIXX && mVixxLocked)
				anImage = IMAGE_CS_VIXX_HILIGHT_LOCK;
			if (anImage == NULL)
				anImage = GetImageById(i + 0xe9);
			aButton->mOverImage = anImage;
			aButton->mDownImage = anImage;
		}
		else
		{
			aButton->mOverImage = IMAGE_CS_BOTTOM_BTN;
			aButton->mDownImage = IMAGE_CS_BOTTOM_BTN_DOWN;
		}
		aButton->Resize(Rect(kButtonRect[i][0], kButtonRect[i][1], kButtonRect[i][2], kButtonRect[i][3]));
		mButtons[i] = aButton;
	}

	// how far each pilot's upgrades go
	UserProfile* aUser = mApp->mUser;
	mStats.resize(4, 0);
	mStats[PILOT_VECTOR] = UpgradeLevel(aUser, 0x33, 0x37, 0x3b);
	mStats[PILOT_SPROCKET] = UpgradeLevel(aUser, 0x34, 0x3c, -1);
	mStats[PILOT_TURBOT] = UpgradeLevel(aUser, 0x35, 0x3d, -1);
	mStats[PILOT_VIXX] = UpgradeLevel(aUser, 0x36, 0x3e, 0x3a);

	// the second mode's percents (Vector has none) and the first mode's best levels
	mPercent[PILOT_VECTOR] = 0;
	mPercent[PILOT_SPROCKET] = ReadPercent(aUser, 0x40);
	mPercent[PILOT_TURBOT] = ReadPercent(aUser, 0x41);
	mPercent[PILOT_VIXX] = ReadPercent(aUser, 0x42);
	for (int i = 0; i < NUM_PILOTS; i++)
	{
		if (mPercent[i] > 100)
			mPercent[i] = 100;
	}
	for (int i = 0; i < NUM_PILOTS; i++)
		mBest[i] = 0;
	aUser->GetInt(8, &mBest[PILOT_VECTOR]);
	aUser->GetInt(9, &mBest[PILOT_SPROCKET]);
	aUser->GetInt(10, &mBest[PILOT_TURBOT]);
	aUser->GetInt(11, &mBest[PILOT_VIXX]);
	for (int i = 0; i < NUM_PILOTS; i++)
		mBest[i] = RoundDown4(mBest[i]);
}

// 0x446720: the buttons go with the screen
CharSelector::~CharSelector()
{
	for (unsigned int i = 0; i < mButtons.size(); i++)
		delete mButtons[i];
}

// 0x43c2d0
void CharSelector::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
	{
		theWidgetManager->AddWidget(mButtons[i]);
		theWidgetManager->BringToFront(mButtons[i]);
	}
}

// 0x43c310
void CharSelector::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(mButtons[i]);
}

// 0x434fd0
void CharSelector::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_BUTTON_DOWN);
}

// 0x434ff0 (the back button's two calls are 0x4226a0): a pilot is chosen and the game starts (the original refuses Sprocket until the copy is registered; the
// port's copy is)
void CharSelector::ButtonDepress(int theId)
{
	if (theId == BUTTON_BACK)
	{
		mApp->CloseSelectorScreen();
		mApp->ShowMainMenu();
		return;
	}
	if (theId == PILOT_TURBOT && mTurbotLocked)
		return;
	if (theId == PILOT_VIXX && mVixxLocked)
		return;
	if (theId < PILOT_VECTOR || theId > PILOT_VIXX)
		return;

	mApp->mClassicPilot = theId;
	if (mMode != 0)
		mApp->StartSurvivalGame(theId);
	else
		mApp->StartClassic(false, false);
}

// the best level, the percent or the best time under a pilot, white in the small font. The places of the time and of
// "N/A" in the second mode are a little left of the number's.
void CharSelector::DrawPilotLabel(Graphics* g, int thePilot, int theX)
{
	static const int kTimeX[4] = { 0x5f, 0x1a0, 0x23f, 0xff };
	static const int kNoTimeX[4] = { 0x73, 0x1a6, 0x246, 0x106 };
	static const int kTimeSetting[4] = { 0x67, 0x68, 0x69, 0x6a };

	g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (mMode == 1)
		g->DrawString(StrFormat("%d%%", mPercent[thePilot]), theX, 0x181);
	else if (mMode == 0 && mBest[thePilot] > 0)
		g->DrawString(StrFormat("%d", mBest[thePilot]), theX, 0x181);
	else if (mMode == 2)
	{
		int aTime = 0;
		mApp->mUser->GetInt(kTimeSetting[thePilot], &aTime);
		if (aTime < 1)
			g->DrawString("N/A", kNoTimeX[thePilot], 0x181);
		else
		{
			std::string aText;
			if (aTime % 0x3c < 10)
				aText = StrFormat("%d:0%d", aTime / 0x3c, aTime % 0x3c);
			else
				aText = StrFormat("%d:%d", aTime / 0x3c, aTime % 0x3c);
			WriteString(g, aText, kTimeX[thePilot], 0x181, 0x37, 0, true, 0, -1);
		}
	}
	else
		g->DrawString("N/A", theX, 0x181);
}

// the bar under a pilot: an eighth of the picture for each upgrade level, from the picture's second half when all eight are there
void CharSelector::DrawStatBar(Graphics* g, int thePilot, Image* theFill)
{
	int aLevel = mStats[thePilot];
	if (aLevel <= 0)
		return;
	int aWidth = aLevel * (theFill->mWidth / 8);
	if (aWidth <= 0)
		return;
	int aHalf = theFill->mHeight / 2;
	int aSecondHalf = (aLevel == 8) ? 1 : 0;
	g->DrawImage(theFill, kFillPos[thePilot][0], kFillPos[thePilot][1], Rect(0, aHalf * aSecondHalf, aWidth, aHalf));
}

// 0x43e300: the pilots in the order Vector, Vixx, Sprocket, Turbot, and what the player has done with each under it
// (the original also draws a list of four pictures that nothing ever fills, and the locked covers of an unregistered copy)
void CharSelector::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, 0x1e0, 0x280);
	g->DrawImage((mMode != 0) ? IMAGE_CS_TITLE_SURVIVAL : IMAGE_CS_TITLE_ARCADE, 0, 0);
	g->DrawImage(IMAGE_CS_BOTTOM, 0, 0x1e0 - IMAGE_CS_BOTTOM->mHeight);

	// Vector is always open
	g->DrawImage(IMAGE_CS_VECTOR, kPilotPos[PILOT_VECTOR][0], kPilotPos[PILOT_VECTOR][1]);
	g->DrawImage((mMode != 0) ? IMAGE_CS_VECTOR_STATS_SURVIVAL : IMAGE_CS_VECTOR_STATS_ARCADE,
		kStatsPos[PILOT_VECTOR][0], kStatsPos[PILOT_VECTOR][1]);
	DrawStatBar(g, PILOT_VECTOR, IMAGE_CS_VECTOR_STATFILL);

	if (!mVixxLocked)
	{
		g->DrawImage(IMAGE_CS_VIXX_OPEN, kPilotPos[PILOT_VIXX][0], kPilotPos[PILOT_VIXX][1]);
		g->DrawImage((mMode != 0) ? IMAGE_CS_VIXX_STATS_SURVIVAL : IMAGE_CS_VIXX_STATS_ARCADE,
			kStatsPos[PILOT_VIXX][0], kStatsPos[PILOT_VIXX][1]);
		DrawStatBar(g, PILOT_VIXX, IMAGE_CS_VIXX_STATFILL);
		DrawPilotLabel(g, PILOT_VIXX, 0x10e);
	}
	else
	{
		g->DrawImage(IMAGE_CS_VIXX_LOCK, kPilotPos[PILOT_VIXX][0], kPilotPos[PILOT_VIXX][1]);
		g->DrawImage(IMAGE_CS_VIXX_STATLOCK, kStatsPos[PILOT_VIXX][0], kStatsPos[PILOT_VIXX][1]);
	}

	g->DrawImage(IMAGE_CS_SPROCKET_OPEN, kPilotPos[PILOT_SPROCKET][0], kPilotPos[PILOT_SPROCKET][1]);
	g->DrawImage((mMode != 0) ? IMAGE_CS_SPROCKET_STATS_SURVIVAL : IMAGE_CS_SPROCKET_STATS_ARCADE,
		kStatsPos[PILOT_SPROCKET][0], kStatsPos[PILOT_SPROCKET][1]);
	DrawStatBar(g, PILOT_SPROCKET, IMAGE_CS_SPROCKET_STATFILL);

	if (!mTurbotLocked)
	{
		g->DrawImage(IMAGE_CS_TURBOT_OPEN, kPilotPos[PILOT_TURBOT][0], kPilotPos[PILOT_TURBOT][1]);
		g->DrawImage((mMode != 0) ? IMAGE_CS_TURBOT_STATS_SURVIVAL : IMAGE_CS_TURBOT_STATS_ARCADE,
			kStatsPos[PILOT_TURBOT][0], kStatsPos[PILOT_TURBOT][1]);
		DrawStatBar(g, PILOT_TURBOT, IMAGE_CS_TURBOT_STATFILL);
		DrawPilotLabel(g, PILOT_TURBOT, 0x24f);
	}
	else
	{
		g->DrawImage(IMAGE_CS_TURBOT_LOCK, kPilotPos[PILOT_TURBOT][0], kPilotPos[PILOT_TURBOT][1]);
		g->DrawImage(IMAGE_CS_TURBOT_STATLOCK, kStatsPos[PILOT_TURBOT][0], kStatsPos[PILOT_TURBOT][1]);
	}

	DrawPilotLabel(g, PILOT_SPROCKET, 0x1b0);
	DrawPilotLabel(g, PILOT_VECTOR, 0x73);
}
