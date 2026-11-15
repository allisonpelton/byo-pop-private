#include "HighScoresScreen.h"
#include "Board.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "YesNoDialog.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

namespace
{
	const int SCREEN_WIDTH = 0x280;
	const int SCREEN_HEIGHT = 0x1e0;
	const int BUBBLE_EVERY = 10;				// a row of bubbles starts every this many updates
	const int BUBBLE_COLUMN_STEP = 0x28;
	const int BUBBLE_LAST_X = 0x280;
	const double BUBBLE_START_Y = 520.0;
	const double BUBBLE_FADE = 0.015;
	const int BUBBLE_SPEED_RANGE = 3000;
	const double BUBBLE_SPEED_STEP = 0.001;
	const double BUBBLE_SPEED_BASE = 1.0;
	const double RED_SCALE = 1024.0;			// the bubble's colour: its life at these scales, 255 at most
	const double GREEN_SCALE = 512.0;
	const double BLUE_SCALE = 256.0;
	const int BLINK_PERIOD = 0x28;
	const double BLINK_STEP = 0.15707957651346172;
	const double BLINK_HALF = 127.0;
	const int FIRST_ROW_Y = 100;
	const int ROW_STEP = 0x23;
	const int ROWS_END_Y = 0x1c2;
	const int LAST_LEVEL_PLUS_ONE = 0x13;		// a record's level at this is a won campaign: it shows WIN
	const int LONG_TIME = 720000;				// survival times (updates, 200 a second) of an hour or more show hours
	const int UPDATES_PER_SECOND = 200;
	const int SECONDS_PER_MINUTE = 60;
	const int SURVIVAL_RESET_BOX_YES = HighScoresScreen::BUTTON_RESET_SURVIVAL_YES;
	const int RESET_BOX_WIDTH = 0x168;
	const int RESET_BOX_HEIGHT = 0xfa;

	// The first scores of a reset list: the names are game texts (GT's keys)
	struct DefaultMissionScore
	{
		unsigned int	mNameText;
		int				mScore;
		int				mLevel;
	};
	const DefaultMissionScore DEFAULT_MISSION_SCORES[HeavyWeaponApp::NUM_HIGH_SCORES] = {
		{GTA(0x52d600), 60000, 3}, {GTA(0x52d620), 50000, 2}, {GTA(0x52d640), 40000, 2}, {GTA(0x52d660), 30000, 2},
		{GTA(0x52d680), 20000, 1}, {GTA(0x52d6a0), 15000, 1}, {GTA(0x52d6c0), 10000, 1}, {GTA(0x52d6e0), 5000, 0},
		{GTA(0x52d700), 2500, 0}, {GTA(0x52d720), 1000, 0}};
	struct DefaultSurvivalScore
	{
		unsigned int	mNameText;
		int				mTime;
	};
	const DefaultSurvivalScore DEFAULT_SURVIVAL_SCORES[HeavyWeaponApp::NUM_HIGH_SCORES] = {
		{GTA(0x52d740), 30000}, {GTA(0x52d75c), 27000}, {GTA(0x52d778), 24000}, {GTA(0x52d794), 21000},
		{GTA(0x52d7b0), 18000}, {GTA(0x52d7cc), 15000}, {GTA(0x52d7e8), 12000}, {GTA(0x52d804), 9000},
		{GTA(0x52d820), 6000}, {GTA(0x52d83c), 3000}};

	std::string ToUpper(const std::string& theString)
	{
		std::string aResult = theString;
		for (size_t i = 0; i < aResult.size(); i++)
			aResult[i] = (char) toupper((unsigned char) aResult[i]);
		return aResult;
	}
}

// 0x4224e0: the four buttons; the list of the highlighted game's mode is shown first when asked for, and a game in
// progress on the board says which kind it is
HighScoresScreen::HighScoresScreen(HeavyWeaponApp* theApp, bool theShowSurvival, int theHighlightRank)
{
	mApp = theApp;
	mMainButton = new BracketButton(mApp, this, BUTTON_MAIN);
	mMainButton->SetTint(Color(0x80, 0x40, 0x20, 0xff));
	mMainButton->mLabel = GT(0x52b900);
	mMissionButton = new BracketButton(mApp, this, BUTTON_MISSION);
	mMissionButton->SetTint(Color(0x40, 0x80, 0xff, 0xff));
	mMissionButton->mLabel = GT(0x52d95c);
	mSurvivalButton = new BracketButton(mApp, this, BUTTON_SURVIVAL);
	mSurvivalButton->SetTint(Color(0x40, 0x80, 0xff, 0xff));
	mSurvivalButton->mLabel = GT(0x52d950);
	mResetButton = new BracketButton(mApp, this, BUTTON_RESET);
	mResetButton->SetTint(Color(0, 0x80, 0, 0xff));
	mResetButton->mLabel = GT(0x52d948);
	mShowSurvival = theShowSurvival;
	mHighlightRank = theHighlightRank;
	if (mApp->mBoard == NULL)
		mHighlightSurvival = false;
	else
		mHighlightSurvival = mApp->mBoard->mIsSurvival;
	Resize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// 0x4224b0 (body 0x422410)
HighScoresScreen::~HighScoresScreen()
{
	delete mMainButton;
	delete mMissionButton;
	delete mSurvivalButton;
	delete mResetButton;
}

// 0x422190 slot 1
void HighScoresScreen::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mMainButton);
	theManager->AddWidget(mMissionButton);
	theManager->AddWidget(mSurvivalButton);
	theManager->AddWidget(mResetButton);
}

// 0x4221e0 slot 2
void HighScoresScreen::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mMainButton);
	theManager->RemoveWidget(mMissionButton);
	theManager->RemoveWidget(mSurvivalButton);
	theManager->RemoveWidget(mResetButton);
}

// 0x422230 slot 13: the buttons along the foot
void HighScoresScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMainButton->Resize(0x1e0, 0x1b8, 0x6e, 0x1e);
	mMissionButton->Resize(0x32, 0x1b8, 0x8c, 0x1e);
	mSurvivalButton->Resize(200, 0x1b8, 0x96, 0x1e);
	mResetButton->Resize(0x168, 0x1b8, 0x6e, 0x1e);
}

// 0x4222b0 slot 17: every tenth update a row of bubbles starts under the screen; all of them rise and fade
void HighScoresScreen::Update()
{
	Widget::Update();
	if (mApp->mUpdateCount % BUBBLE_EVERY == 0)
	{
		int anX = 0;
		do
		{
			HighScoreBubble aBubble;
			aBubble.mX = (double) anX;
			aBubble.mY = BUBBLE_START_Y;
			aBubble.mLife = 1.0;
			mBubbles.push_back(aBubble);
			mBubbles.back().mSpeed = (double) (Rand() % BUBBLE_SPEED_RANGE) * BUBBLE_SPEED_STEP + BUBBLE_SPEED_BASE;
			anX += BUBBLE_COLUMN_STEP;
		} while (anX <= BUBBLE_LAST_X);
	}
	std::list<HighScoreBubble>::iterator anItr = mBubbles.begin();
	while (anItr != mBubbles.end())
	{
		anItr->mY = anItr->mY - anItr->mSpeed;
		anItr->mLife = anItr->mLife - BUBBLE_FADE;
		if (anItr->mLife <= 0.0)
			anItr = mBubbles.erase(anItr);
		else
			++anItr;
	}
	MarkDirty();
}

// 0x422780 slot 16: the picture, the bubbles added over it, a dim panel with its lines, the headings of the shown list and
// its ten lines (the one that is highlighted blinks)
void HighScoresScreen::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_NUKEBG]->Draw(g, 0, 0, false, 0, 0, false);
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (std::list<HighScoreBubble>::iterator anItr = mBubbles.begin(); anItr != mBubbles.end(); ++anItr)
	{
		int aRed = (int) (anItr->mLife * RED_SCALE);
		int aGreen = (int) (anItr->mLife * GREEN_SCALE);
		int aBlue = (int) (anItr->mLife * BLUE_SCALE);
		if (aRed > 0xff)
			aRed = 0xff;
		if (aGreen > 0xff)
			aGreen = 0xff;
		if (aBlue > 0xff)
			aBlue = 0xff;
		g->SetColor(Color(aRed, aGreen, aBlue));
		mApp->mSprites[IMAGE_FIRE]->Draw(g, (int) anItr->mX, (int) anItr->mY, true, 0, 0, false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
	g->SetColor(Color(0, 0, 0, 0x40));
	g->FillRect(0x32, 0x32, 0x21c, 0x17c);
	g->FillRect(0x32, 0x32, 0x21c, 0x14);
	g->SetColor(Color(0, 0, 0));
	g->DrawRect(0x32, 0x32, 0x21c, 0x17c);
	g->DrawLine(0x32, 0x46, 0x24e, 0x46);
	if (!mShowSurvival)
	{
		g->DrawLine(0x136, 0x32, 0x136, 0x1ae);
		g->DrawLine(0x1cc, 0x32, 0x1cc, 0x1ae);
		g->DrawLine(0x212, 0x32, 0x212, 0x1ae);
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->SetFont(mApp->mFonts[FONT_OUTLINED]);
		g->Translate(-0x8c, 0);
		WriteCenteredLine(g, 0x43, GT(0x52d9c8));
		g->Translate(0xd2, 0);
		WriteCenteredLine(g, 0x43, GT(0x52d99c));
		g->Translate(0x6c, 0);
		WriteCenteredLine(g, 0x43, GT(0x52d998));
		g->Translate(0x3e, 0);
		WriteCenteredLine(g, 0x43, GT(0x52d990));
		g->Translate(-0xfa, 0);
		g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
		WriteCenteredLine(g, 0x23, GT(0x52d978));
	}
	else
	{
		g->DrawLine(0x1b8, 0x32, 0x1b8, 0x1ae);
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->SetFont(mApp->mFonts[FONT_OUTLINED]);
		g->Translate(-0x50, 0);
		WriteCenteredLine(g, 0x43, GT(0x52d9c8));
		g->Translate(0x113, 0);
		WriteCenteredLine(g, 0x43, GT(0x52d9c0));
		g->Translate(-0xc3, 0);
		g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
		WriteCenteredLine(g, 0x23, GT(0x52d9a4));
	}
	g->SetColorizeImages(true);
	int aRank = 0;
	for (int aY = FIRST_ROW_Y; aY < ROWS_END_Y; aY += ROW_STEP)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		if (!mShowSurvival)
		{
			const HighScoreRecord& aRecord = mApp->mHighScores[aRank];
			if ((aRank == mHighlightRank) && !mHighlightSurvival)
			{
				int anAlpha = (int) ((cos((double) (mApp->mUpdateCount % BLINK_PERIOD) * BLINK_STEP) + 1.0) * BLINK_HALF);
				g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			}
			WriteString2004(g, ToUpper(aRecord.mName), 0x46, aY, -1, -1, true, 0, -1);
			char aBuffer[0x40];
			sprintf(aBuffer, "%i", aRecord.mScore);
			WriteString2004(g, aBuffer, 0x1cc, aY, -1, 1, true, 0, -1);
			if (aRecord.mLevel < LAST_LEVEL_PLUS_ONE)
				sprintf(aBuffer, "%i", aRecord.mLevel + 1);
			else
				sprintf(aBuffer, "%s", GTC(0x52d964));
			g->Translate(0xb9, 0);
			WriteCenteredLine(g, aY, aBuffer);
			g->Translate(-0xb9, 0);
			if (aRecord.mMaxLevel > 0)
				mApp->mSprites[IMAGE_SMALLINSIGNIA]->Draw(g, 0x22e, aY - 0x18, false, aRecord.mMaxLevel - 1, 0, false);
		}
		else
		{
			const SurvivalScoreRecord& aRecord = mApp->mSurvivalScores[aRank];
			if ((aRank == mHighlightRank) && mHighlightSurvival)
			{
				int anAlpha = (int) ((cos((double) (mApp->mUpdateCount % BLINK_PERIOD) * BLINK_STEP) + 1.0) * BLINK_HALF);
				g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			}
			WriteString2004(g, ToUpper(aRecord.mName), 0x46, aY, -1, -1, true, 0, -1);
			char aBuffer[0x40];
			int aSeconds = (aRecord.mTime / UPDATES_PER_SECOND) % SECONDS_PER_MINUTE;
			int aMinutes = (aRecord.mTime / (UPDATES_PER_SECOND * SECONDS_PER_MINUTE)) % SECONDS_PER_MINUTE;
			if (aRecord.mTime >= LONG_TIME)
				sprintf(aBuffer, "%i:%02i:%02i", aRecord.mTime / LONG_TIME, aMinutes, aSeconds);
			else
				sprintf(aBuffer, "%02i:%02i", aMinutes, aSeconds);
			WriteString2004(g, aBuffer, 0x244, aY, -1, 1, true, 0, -1);
		}
		aRank++;
	}
	g->SetColorizeImages(false);
}

// 0x410670 listener slot 4
void HighScoresScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x423450 listener slot 5: MAIN leaves for the main menu; MISSION and SURVIVAL pick the list; RESET asks first, and its
// YES writes the first scores of the list back (a game that has just ended loses its blinking line then)
void HighScoresScreen::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	switch (theId)
	{
	case BUTTON_MAIN:
		mApp->ShowMainMenu();
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
		return;
	case BUTTON_MISSION:
		mShowSurvival = false;
		return;
	case BUTTON_SURVIVAL:
		mShowSurvival = true;
		return;
	case BUTTON_RESET:
		{
			YesNoDialog* aDialog;
			if (!mShowSurvival)
				aDialog = new YesNoDialog(mApp, this, GT(0x52da70), GT(0x52d9f0), RESET_BOX_WIDTH, RESET_BOX_HEIGHT,
					BUTTON_RESET_MISSION_YES, 0);
			else
				aDialog = new YesNoDialog(mApp, this, GT(0x52da70), GT(0x52da80), RESET_BOX_WIDTH, RESET_BOX_HEIGHT,
					SURVIVAL_RESET_BOX_YES, 0);
			mApp->AddDialog(YesNoDialog::DIALOG_ID, aDialog);
			aDialog->WaitForResult(false);
		}
		return;
	case BUTTON_RESET_SURVIVAL_YES:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		for (int i = 0; i < HeavyWeaponApp::NUM_HIGH_SCORES; i++)
		{
			strcpy(mApp->mSurvivalScores[i].mName, GTC(DEFAULT_SURVIVAL_SCORES[i].mNameText));
			mApp->mSurvivalScores[i].mTime = DEFAULT_SURVIVAL_SCORES[i].mTime;
		}
		if (mHighlightSurvival)
			mHighlightRank = NO_HIGHLIGHT;
		mApp->SaveHighScores();
		return;
	case BUTTON_RESET_MISSION_YES:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		for (int i = 0; i < HeavyWeaponApp::NUM_HIGH_SCORES; i++)
		{
			strcpy(mApp->mHighScores[i].mName, GTC(DEFAULT_MISSION_SCORES[i].mNameText));
			mApp->mHighScores[i].mScore = DEFAULT_MISSION_SCORES[i].mScore;
			mApp->mHighScores[i].mLevel = DEFAULT_MISSION_SCORES[i].mLevel;
			mApp->mHighScores[i].mMaxLevel = DEFAULT_MISSION_SCORES[i].mLevel;
		}
		if (!mHighlightSurvival)
			mHighlightRank = NO_HIGHLIGHT;
		mApp->SaveHighScores();
		return;
	case BUTTON_RESET_NO:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		return;
	}
}
