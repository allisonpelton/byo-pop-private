#include "GameOverDialog.h"
#include "Board.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"

#include <sstream>
#include <stdio.h>

using namespace Sexy;

namespace
{
	const int DIALOG_WIDTH = 360;
	const int CAMPAIGN_HEIGHT = 220;
	const int SURVIVAL_HEIGHT = 240;
	const int SCREEN_CENTER_X = 320;
	const int SCREEN_CENTER_Y = 240;
	const int CONTENT_INSET = 20;
	const int BUTTON_HEIGHT = 30;
	const int BUTTON_BOTTOM = 55;			// a button's top, up from the box's foot
	const int TRY_AGAIN_WIDTH = 150;
	const int QUIT_WIDTH = 110;
	const int BLINK_PERIOD = 60;			// the "new high score" heading shows for half of every 60 updates
	const int TICKS_PER_SECOND = 200;		// survival's clock counts 2 an update
	const int TICKS_PER_HOUR = TICKS_PER_SECOND * 3600;

	// "h:mm:ss" from an hour on (theThreshold decides, not the time itself), else "mm:ss"
	std::string FormatTime(int theTime, int theThreshold)
	{
		char aText[32];
		if (theThreshold >= TICKS_PER_HOUR)
			sprintf(aText, "%i:%02i:%02i", theTime / TICKS_PER_HOUR, theTime / (TICKS_PER_SECOND * 60) % 60,
				theTime / TICKS_PER_SECOND % 60);
		else
			sprintf(aText, "%02i:%02i", theTime / (TICKS_PER_SECOND * 60) % 60, theTime / TICKS_PER_SECOND % 60);
		return aText;
	}
}

// 0x41fe40: "GAME OVER" over two buttons that answer to the board, which is told to stop
GameOverDialog::GameOverDialog(HeavyWeaponApp* theApp) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, Board::DIALOG_GAME_OVER, true, GT(0x52c978), "", "",
		BUTTONS_NONE)
{
	mApp = theApp;
	mBoard = theApp->mBoard;
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0x80, 0xc0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);
	mTryAgainButton = new BracketButton(mApp, mBoard, Board::BUTTON_GAME_OVER_CONTINUE);
	mTryAgainButton->SetTint(Color(0x80, 0xc0, 0x40));
	mTryAgainButton->mLabel = GT(0x52d2f8);
	mQuitButton = new BracketButton(mApp, mBoard, Board::BUTTON_GAME_OVER_SCORES);
	mQuitButton->SetTint(Color(0x80, 0, 0));
	mQuitButton->mLabel = GT(0x52a320);
	mWidth = DIALOG_WIDTH;
	mHeight = mBoard->mIsSurvival ? SURVIVAL_HEIGHT : CAMPAIGN_HEIGHT;
	mBoard->Suspend();
	Resize(SCREEN_CENTER_X - mWidth / 2, SCREEN_CENTER_Y - mHeight / 2, mWidth, mHeight);
}

// 0x41fca0 (body 0x41fc20)
GameOverDialog::~GameOverDialog()
{
	delete mTryAgainButton;
	delete mQuitButton;
}

// 0x41d160 slot 1 (the same code as YesNoDialog's)
void GameOverDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mTryAgainButton);
	theManager->AddWidget(mQuitButton);
}

// 0x41d2b0 slot 2 (the same code as YesNoDialog's)
void GameOverDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mTryAgainButton);
	theManager->RemoveWidget(mQuitButton);
}

// 0x41fb80 slot 13: the two buttons side by side at the foot of the box
void GameOverDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	Rect aRect = GetInsetRect();
	mTryAgainButton->Resize(aRect.mWidth / 2 - 140 + mX, mHeight - BUTTON_BOTTOM + mY, TRY_AGAIN_WIDTH, BUTTON_HEIGHT);
	mQuitButton->Resize(aRect.mWidth / 2 + 30 + mX, mHeight - BUTTON_BOTTOM + mY, QUIT_WIDTH, BUTTON_HEIGHT);
}

// 0x41fc90 slot 17: redrawn every update, for the blinking heading
void GameOverDialog::Update()
{
	MarkDirty();
}

// 0x4201c0 slot 16: a black panel with the labels down its left and the values right-aligned down its right
void GameOverDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetColor(Color(0x80, 0x80, 0x80));
	g->DrawRect(29, 54, 302, mHeight - 118);
	g->SetColor(Color(0, 0, 0));
	g->FillRect(30, 55, 300, mHeight - 120);
	g->SetColor(Color(0x40, 0xc0, 0));
	Font* aFont = mApp->mFonts[FONT_NORMAL];
	g->SetFont(aFont);

	std::ostringstream aValues;
	int* aStats = mApp->mStats;
	if (!mBoard->mIsSurvival)
	{
		if (mBoard->mRank == Board::NO_RANK)
			WriteString2004(g, GT(0x52d3d4), 40, 77, -1, -1, true, 0, -1);
		else
		{
			g->SetColor(Color(0xff, 0x40, 0));
			if (mApp->mUpdateCount % BLINK_PERIOD < BLINK_PERIOD / 2)
				WriteString2004(g, GT(0x52d3c4), 40, 77, -1, -1, true, 0, -1);
			g->SetColor(Color(0x40, 0xc0, 0));
		}
		g->SetColor(Color(0x40, 0xc0, 0));
		WriteWordWrapped2004(g, Rect(40, 82, mWidth - 60, mHeight - 90), GT(0x52d38c), aFont->mHeight, -1);
		g->SetColor(Color(0xff, 0xc0, 0));
		aValues << mApp->mScore << "\n" << aStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED] << "\n"
			<< KillPercentage(aStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED], aStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED])
			<< "%\n" << aStats[HeavyWeaponApp::STAT_FRIENDLIES_DESTROYED];
	}
	else
	{
		if (mBoard->mRank == Board::NO_RANK)
			WriteString2004(g, GT(0x52d374), 40, 77, -1, -1, true, 0, -1);
		else
		{
			g->SetColor(Color(0xff, 0x40, 0));
			if (mApp->mUpdateCount % BLINK_PERIOD < BLINK_PERIOD / 2)
				WriteString2004(g, GT(0x52d368), 40, 77, -1, -1, true, 0, -1);
			g->SetColor(Color(0x40, 0xc0, 0));
		}
		g->SetColor(Color(0x40, 0xc0, 0));
		WriteWordWrapped2004(g, Rect(40, 82, mWidth - 60, mHeight - 90), GT(0x52d320), aFont->mHeight, -1);
		g->SetColor(Color(0xff, 0xc0, 0));
		std::string aTime = FormatTime(mBoard->mLevelTime - mBoard->mTimePenalty, mBoard->mLevelTime);
		int aBestTime = mApp->CurrentUser().mBestSurvivalTime;
		std::string aBest = FormatTime(aBestTime, aBestTime);
		aValues << aTime << "\n" << aBest << "\n" << aStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED] << "\n"
			<< KillPercentage(aStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED], aStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED])
			<< "%\n" << aStats[HeavyWeaponApp::STAT_FRIENDLIES_DESTROYED];
	}
	WriteWordWrapped2004(g, Rect(230, 60, 90, mHeight - 90), aValues.str(), aFont->mHeight, 1);
}
