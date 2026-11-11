// The main menu (see Menu.h).

#include "Menu.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Dialogs.h"
#include "GameText.h"
#include "Graphics.h"
#include "Grid.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "WidgetManager.h"

#include <cstdio>

using namespace Sexy;

namespace
{
	// the buttons (0x4198e0): the games, the expert games beside them, the saved game, the exit, and after a game
	// "main menu" and "play again"
	struct ButtonSpec { int mId; int mX; int mY; int mW; int mH; };
	const ButtonSpec kButtons[] = {
		{0, 0x5a, 0x4b, 0x1c5, 0x3b},		// strategy
		{1, 0x1de, 0x4b, 0x41, 0x3c},		// expert strategy
		{2, 0x5a, 0xb1, 0x1c5, 0x3b},		// action
		{3, 0x1de, 0xb1, 0x41, 0x3c},		// expert action
		{4, 0x5a, 0x119, 0x1c5, 0x3b},		// the saved game
		{5, 0xd9, 0x178, 0xce, 0x25},		// exit
		{6, 0x68, 0x178, 0xce, 0x25},		// main menu
		{7, 0x14a, 0x178, 0xce, 0x25}		// play again
	};

	std::string Format(const std::string& theFormat, int theA = 0, int theB = 0)
	{
		char aBuffer[256];
		snprintf(aBuffer, sizeof(aBuffer), theFormat.c_str(), theA, theB);
		return aBuffer;
	}
}

// 0x4198e0 / 0x419760
MainMenu::MainMenu(AtomicaApp* theApp, bool theGameOver) :
	mApp(theApp),
	mDoor(0x12f),
	mAlpha(0),
	mTimer(0),
	mSlammed(false),
	mDoorOpenPlayed(false),
	mSwoopPlayed(false),
	mClosing(false),
	mLeaving(false),
	mGameOver(theGameOver),
	mSpin(0),
	mInsertTimer(100),
	mRank(0),
	mEntryShown(false)
{
	mDoorSheet = mApp->mSprites->Load("menudoors");
	mDoorLeft = mApp->mSprites->SubSprite(mDoorSheet, 0, 0, 0x12f, 0x1e0);
	mDoorRight = mApp->mSprites->SubSprite(mDoorSheet, 0x12f, 0, 0x151, 0x1e0);
	mPanel = mApp->mSprites->Load("menupanel");
	mApp->mMenuDoorsOpen = false;
	mPanelY = mPanel->mHeight + 10;
	mHasSave = mApp->HasSavedGame();

	// 0x419760: what the game that just ended came to
	Board* aBoard = mApp->mBoard;
	if ((aBoard == NULL) || (aBoard->mGrid == NULL))
	{
		mScore = 999999;
		mLevelReached = 5;
		mTime = 0x1c84;
		mAtoms = 0x32;
		mMolecules = 4;
		mCombo = 4;
		mBonusMolecules = 5;
		mBonusUpgrades = 5;
	}
	else
	{
		mScore = aBoard->mScore;
		mLevelReached = aBoard->mMeter.mMoleculesMade;
		mTime = aBoard->mStatUpdates;
		mAtoms = aBoard->mStatAtoms;
		mMolecules = aBoard->mStatMolecules;
		mCombo = aBoard->mStatBestCombo;
		mBonusMolecules = aBoard->mStatBonusMolecules;
		mBonusUpgrades = aBoard->mStatBonusUpgrades;
	}
	mRank = RankOfScore(mScore);
	mApp->mMusicInterface->PlayMusic(1, theGameOver ? 0x29 : 0x1f, false);
	mHasTransparencies = true;

	for (int i = 0; i < 8; i++)
		mBtn[i] = NULL;
	for (size_t i = 0; i < sizeof(kButtons) / sizeof(kButtons[0]); i++)
	{
		const ButtonSpec& aSpec = kButtons[i];
		if ((aSpec.mId == 4) && !mHasSave)
			continue;
		ButtonWidget* aButton = new ButtonWidget(aSpec.mId, this);
		aButton->mBtnNoDraw = true;
		aButton->mDoFinger = true;
		aButton->Resize(aSpec.mX, aSpec.mY, aSpec.mW, aSpec.mH);
		mBtn[aSpec.mId] = aButton;
		mButtons.Add(aButton);
	}
	ShowPage(mGameOver);
}

// 0x4190b0
MainMenu::~MainMenu()
{
	// 0x4190b0 / 0x41e690: the sheet, the doors and the panel are freed (the original's four frees; the sprite manager is gone when the
	// app's destructor deletes a menu that is still up)
	if (mApp->mSprites != NULL)
	{
		mApp->mSprites->Free(mDoorSheet);
		mApp->mSprites->Free(mDoorLeft);
		mApp->mSprites->Free(mDoorRight);
		mApp->mSprites->Free(mPanel);
	}
	mApp->mMusicInterface->StopMusic(1);
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		mApp->SafeDeleteWidget(aButton);
		aButton = mButtons.Next();
	}
	mButtons.Clear();
}

// port: the buttons of the main page or those of the end of a game
void MainMenu::ShowPage(bool theGameOver)
{
	mGameOver = theGameOver;
	for (int i = 0; i < 8; i++)
		if (mBtn[i] != NULL)
			mBtn[i]->SetVisible((i >= 6) ? theGameOver : !theGameOver);
}

// 0x431d20
void MainMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		theWidgetManager->AddWidget(aButton);
		aButton = mButtons.Next();
	}
}

// 0x4194e0
void MainMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		theWidgetManager->RemoveWidget(aButton);
		aButton = mButtons.Next();
	}
}

// 0x4191d0: the doors close over what is behind the menu and slam (0x405e20 deletes the title screen or the game that ended),
// 75 updates later the panel comes down and its contents fade in; after a game the score goes to the high score table 100
// updates after the menu began. "Main menu" after a game makes the panel drop out of sight and come back with the main page;
// a game that starts makes the panel go up, and the doors open wide on the new board (the board closes the menu then).
// Each step eases: a tenth to a twentieth of what is left, or a tenth or a quarter more of what is gone.
void MainMenu::Update()
{
	MarkDirty();	// the original calls the widget's vtable slot 42 here, not Widget::Update
	mSpin += 0.04f;
	if (mSpin >= 256.0f)
		mSpin -= 256.0f;
	if (mGameOver)
	{
		mInsertTimer--;
		if (mInsertTimer == 0)
		{
			mEntryShown = true;
			ScoreTable& aTable = mApp->mScoreTables[mApp->mMode];
			if (aTable.Qualifies(mScore))
				mApp->ShowHighScoreEntry(mScore);
		}
	}
	if (mLeaving)
	{
		mAlpha -= 3;
		if (mAlpha < 0)
			mAlpha = 0;
		mPanelY = (int) ((double) (mPanelY - 1) * 1.1);
		if (mPanelY >= -0x1e0)
			return;
		mLeaving = false;
		mApp->mSndSwoop->Play();
		mPanelY = mPanel->mHeight + 10;
		mAlpha = 0;
		ShowPage(false);
		return;
	}
	if (!mClosing)
	{
		mDoor = -1 - (int) ((double) mDoor * (double) -0.95f);
		if (mDoor < 0)
		{
			mDoor = 0;
			if (!mSlammed)
			{
				mApp->CloseBoard();
				mApp->mSndDoorSlam->Play();
				mTimer = 0x4b;
				mSlammed = true;
				mButtons.Reset();
				Widget* aButton = mButtons.Next();
				while (aButton != NULL)
				{
					mWidgetManager->BringToFront(aButton);
					aButton = mButtons.Next();
				}
			}
			mTimer--;
			if (mTimer < 0)
				mTimer = 0;
		}
		if ((mDoor == 0) && (mTimer < 1))
		{
			mPanelY = -1 - (int) ((double) mPanelY * (double) -0.9f);
			if (mPanelY < 0)
			{
				if (!mSwoopPlayed)
				{
					mApp->mSndSwoop->Play();
					mSwoopPlayed = true;
				}
				mPanelY = 0;
			}
			mAlpha += 3;
			if (mAlpha > 0xff)
			{
				mAlpha = 0xff;
				mHasTransparencies = (mDoor != 0);
				return;
			}
		}
	}
	else
	{
		mAlpha -= 3;
		if (mAlpha < 0)
			mAlpha = 0;
		int aGone = mPanel->mHeight + 10;
		mPanelY = (int) ((double) (mPanelY + 1) * (double) 1.1f);
		if (mPanelY > aGone)
			mPanelY = aGone;
		if (mPanelY == aGone)
		{
			if (!mDoorOpenPlayed)
			{
				mApp->mSndDoorOpen->Play();
				mDoorOpenPlayed = true;
			}
			mDoor = (int) ((double) (mDoor + 1) * (double) 1.25f);
			if (mDoor >= 0x14a)
			{
				mDoor = 0x14a;
				mApp->mMenuDoorsOpen = true;
			}
		}
	}
	mHasTransparencies = (mDoor != 0);
}

// port: the panel is up and complete: the bot waits for it, and a click before the doors have shut is ignored
bool MainMenu::PanelShown() const
{
	return mSlammed && !mClosing && !mLeaving && (mAlpha >= 0xff);
}

// port: whether the mouse is over a button of the menu
bool MainMenu::Over(int theId) const
{
	return (mBtn[theId] != NULL) && mBtn[theId]->mIsOver;
}

// port: a button's field, brighter when the mouse is over it
void MainMenu::FillButton(Graphics* g, int theId)
{
	ButtonWidget* aButton = mBtn[theId];
	g->SetColor(Color(Over(theId) ? 0x80 : 0, 0x80, 0, mAlpha / 2));
	g->FillRect(aButton->mX, aButton->mY - mPanelY, aButton->mWidth, aButton->mHeight);
	g->SetColor(Color(Over(theId) ? 255 : 0, 255, 0, mAlpha));
	g->SetFont(mApp->mAtomicFont);
	// the caption is drawn by the caller
}

// 0x41a010: the 256 colours the high score rows cycle through: red, green and blue run up and down between their ends at
// their own steps (-25 from 255, 5 from 185 and 25 from -255, the negative ends being black), the alpha is the page's
void BuildScoreColors(Color* theColors, int theAlpha)
{
	int aRed = 255, aRedStep = -25;
	int aGreen = 0xb9, aGreenStep = 5;
	int aBlue = -255, aBlueStep = 25;
	for (int i = 0; i < 256; i++)
	{
		theColors[i] = Color(aRed < 1 ? 0 : aRed, aGreen < 1 ? 0 : aGreen, aBlue < 1 ? 0 : aBlue, theAlpha);
		aRed += aRedStep;
		if (aRed < -255)
		{
			aRed = -255;
			aRedStep = (aRedStep < 0) ? -aRedStep : aRedStep;
		}
		else if (aRed > 255)
		{
			aRed = 255;
			aRedStep = (aRedStep < 0) ? aRedStep : -aRedStep;
		}
		aGreen += aGreenStep;
		if (aGreen < 0xb9)
		{
			aGreen = 0xb9;
			aGreenStep = (aGreenStep < 0) ? -aGreenStep : aGreenStep;
		}
		else if (aGreen > 255)
		{
			aGreen = 255;
			aGreenStep = (aGreenStep < 0) ? aGreenStep : -aGreenStep;
		}
		aBlue += aBlueStep;
		if (aBlue < -255)
		{
			aBlue = -255;
			aBlueStep = (aBlueStep < 0) ? -aBlueStep : aBlueStep;
		}
		else if (aBlue > 255)
		{
			aBlue = 255;
			aBlueStep = (aBlueStep < 0) ? aBlueStep : -aBlueStep;
		}
	}
}

// 0x41a010
void MainMenu::Draw(Graphics* g)
{
	int aDoor = (mDoor < 0) ? 0 : mDoor;
	mDoorLeft->Draw(g, -aDoor, 0);
	mDoorRight->Draw(g, 0x12f + aDoor, 0);
	mPanel->Draw(g, 0x140 - mPanel->mWidth / 2, 10 - mPanelY);
	int aPY = mPanelY;
	ImageFont* aAtomic = mApp->mAtomicFont;
	ImageFont* aSmall = mApp->mMonitorFontSmall;
	{
		if (mGameOver)
		{
			// the buttons
			static const unsigned int kCaptions[2] = {0x4aff40, 0x4aff34};
			for (int i = 0; i < 2; i++)
			{
				FillButton(g, 6 + i);
				ButtonWidget* aButton = mBtn[6 + i];
				const std::string& aText = GT(kCaptions[i]);
				g->DrawString(aText, aButton->mX + aButton->mWidth / 2 - aAtomic->StringWidth(aText) / 2,
					aButton->mY + aButton->mHeight - aPY - 0xc);
			}
			// the score
			g->SetFont(mApp->mMonitorFontMedium);
			g->SetColor(Color(0, 255, 100, mAlpha));
			const std::string& aFinal = GT(0x4aff24);
			g->DrawString(aFinal, 0xde - mApp->mMonitorFontMedium->StringWidth(aFinal), 0x55 - aPY);
			g->SetFont(mApp->mMonitorFont);
			g->SetColor(Color(255, 255, 0, mAlpha));
			g->DrawString(Format(GT(0x4aff20), mScore), 0xe3, 0x55 - aPY);
			// what the game came to
			g->SetFont(aSmall);
			int aTime = mTime / 100;
			int aCombo = mCombo - 1 < 0 ? 0 : mCombo - 1;
			struct Row { unsigned int mLabel; std::string mValue; int mY; };
			Row aRows[7] = {
				{0x4aff10, Format(GT(0x4aff20), mLevelReached), 0x68},
				{0x4aff04, Format(GT(0x4afefc), aTime / 60, aTime % 60), 0x77},
				{0x4afee8, Format(GT(0x4aff20), mAtoms), 0x86},
				{0x4afed8, Format(GT(0x4aff20), mMolecules), 0x95},
				{0x4afec8, Format(GT(0x4afec4), aCombo), 0xa4},
				{0x4afeb0, Format(GT(0x4aff20), mBonusMolecules), 0xb3},
				{0x4afea0, Format(GT(0x4aff20), mBonusUpgrades), 0xc2}
			};
			for (int i = 0; i < 7; i++)
			{
				g->SetColor(Color(0, 255, 0, mAlpha));
				const std::string& aLabel = GT(aRows[i].mLabel);
				g->DrawString(aLabel, 0xde - aSmall->StringWidth(aLabel), aRows[i].mY - aPY);
				g->SetColor(Color(200, 255, 0, mAlpha));
				g->DrawString(aRows[i].mValue, 0xe3, aRows[i].mY - aPY);
			}
			// the rank
			g->SetColor(Color(0, 255, 0, mAlpha));
			const std::string& aRankLabel = GT(0x4afe94);
			g->DrawString(aRankLabel, 0xaf - aSmall->StringWidth(aRankLabel) / 2, 0xe0 - aPY);
			g->SetFont(mApp->mMonitorFontMedium);
			g->SetColor(Color(0xaf, 255, 100, mAlpha));
			const std::string& aRankName = GT(kRankNames[mRank]);
			g->DrawString(aRankName, 0xaf - mApp->mMonitorFontMedium->StringWidth(aRankName) / 2, 0xf1 - aPY);
			mApp->mRank[mRank]->Draw(g, 0x78, 0xf9 - aPY);
			mApp->mGreenAtomicus->Draw(g, 0x1d5, 0x3a - aPY);
			// the high scores
			ScoreTable& aTable = mApp->mScoreTables[mApp->mMode == 0 ? 0 : 1];
			g->SetFont(mApp->mMonitorFont);
			g->SetColor(Color(200, 255, 100, mAlpha));
			const std::string& aTitle = GT(0x4afe88);
			g->DrawString(aTitle, 0x186 - mApp->mMonitorFont->StringWidth(aTitle) / 2, 0xb4 - aPY);
			const std::string& aDayTitle = GT(0x4afe70);
			g->DrawString(aDayTitle, 0x186 - mApp->mMonitorFont->StringWidth(aDayTitle) / 2, 0x140 - aPY);
			// what the monitor says of the score, in the box at the top right
			g->SetColor(Color(0, 255, 0, mAlpha));
			g->SetFont(mApp->mCommentFont);
			static const unsigned int kComments[22] = {0x4afd4c, 0x4afd38, 0x4afd20, 0x4afd08, 0x4afcfc, 0x4afce0, 0x4afccc,
				0x4afcb8, 0x4afca8, 0x4afc98, 0x4afc7c, 0x4afc70, 0x4afc60, 0x4afc54, 0x4afc44, 0x4afc30, 0x4afc18, 0x4afbf4,
				0x4afbe0, 0x4afbc4, 0x4afba4, 0x4afb88};
			WriteWordWrapped(g, Rect(0x125, 0x3c - aPY, 0xa7, 0x50), std::string("\"") + GT(kComments[mRank]) + "\"", 0x10, 1);
			// the rows run through the cycle of colours, each one on from the one above
			Color aColors[256];
			BuildScoreColors(aColors, mAlpha);
			int aColor = (int) mSpin;
			g->SetFont(mApp->mMonitorFontMedium);
			for (int i = 0; i < ScoreTable::NUM_SCORES; i++)
			{
				g->SetColor(aColors[aColor]);
				g->DrawString(aTable.mNames[i], 0x10e, 0xd2 + 0x12 * i - aPY);
				std::string aScore = Format(GT(0x4aff20), aTable.mScores[i]);
				g->DrawString(aScore, 0x1fe - mApp->mMonitorFontMedium->StringWidth(aScore), 0xd2 + 0x12 * i - aPY);
				aColor++;
				if (aColor > 0xff)
					aColor -= 0xff;
			}
			g->SetColor(aColors[aColor]);
			if ((aTable.mDate == ScoreTable::DateCode()) && (aTable.mBestToday != -1))
			{
				g->DrawString(aTable.mBestName, 0x10e, 0x159 - aPY);
				std::string aScore = Format(GT(0x4aff20), aTable.mBestToday);
				g->DrawString(aScore, 0x1fe - mApp->mMonitorFontMedium->StringWidth(aScore), 0x159 - aPY);
			}
			else
			{
				const std::string& aNone = GT(0x4afe5c);
				g->DrawString(aNone, 0x186 - mApp->mMonitorFontMedium->StringWidth(aNone) / 2, 0x159 - aPY);
			}
		}
		else
		{
			g->SetColorizeImages(true);
			// strategy and action: a field with the picture, the name and what it is
			for (int aGame = 0; aGame < 2; aGame++)
			{
				int aButtonId = aGame * 2;
				int aY = 0x4b + aGame * 0x66;
				bool aOver = Over(aButtonId);
				bool aExpertOver = Over(aButtonId + 1);
				g->SetFont(aAtomic);
				g->SetColor(Color((aOver || aExpertOver) ? 0x80 : 0, 0x80, 0, mAlpha / 2));
				g->FillRect(0x5a, aY - aPY, 0x17f, 0x3c);
				if (aExpertOver)
					g->SetColor(Color(200, 200, 0, mAlpha / 2));
				g->FillRect(0x1de, aY - aPY, 0x41, 0x3c);
				g->SetColor(Color((aOver || aExpertOver) ? 255 : 0, 255, 0, mAlpha));
				mApp->mGameType[aGame]->Draw(g, 100, aY - 10 - 2 * aGame - aPY);	// 0x41 and 0xa5 in the original
				g->DrawString(GT(aGame == 0 ? 0x4b0000 : 0x4affd8), 0xbe, aY + 0x14 - aPY);
				g->SetFont(aSmall);
				g->DrawString(GT(aGame == 0 ? 0x4affe0 : 0x4affbc), 0xbe, aY + 0x28 - aPY);
				g->SetColor(aExpertOver ? Color(255, 255, 255, mAlpha) : Color(0, 255, 0, mAlpha));
				Sprite* anExpert = mApp->mExpert[aGame];
				anExpert->Draw(g, 0x200 - anExpert->mWidth / 2, aY + 0x1e - aPY - anExpert->mHeight / 2);
			}
			// the saved game
			g->SetFont(aAtomic);
			if (!mHasSave)
				g->SetColor(Color(0x40, 0x40, 0x40, mAlpha / 2));
			else
				g->SetColor(Over(4) ? Color(0x80, 0x80, 0, mAlpha / 2) : Color(0, 0x80, 0, mAlpha / 2));
			g->FillRect(0x5a, 0x119 - aPY, 0x1c5, 0x3c);
			if (!mHasSave)
				g->SetColor(Color(0x80, 0x80, 0x80, mAlpha));
			else
				g->SetColor(Over(4) ? Color(255, 255, 0, mAlpha) : Color(0, 255, 0, mAlpha));
			mApp->mGameType[2]->Draw(g, 100, 0x109 - aPY);
			g->DrawString(GT(0x4affa8), 0xbe, 0x12f - aPY);
			g->SetFont(aSmall);
			g->DrawString(GT(0x4aff80), 0xbe, 0x13e - aPY);
			g->DrawString(GT(0x4aff5c), 0xbe, 0x14d - aPY);
			// the exit
			FillButton(g, 5);
			ButtonWidget* aQuit = mBtn[5];
			const std::string& aQuitText = GT(0x4aff4c);
			g->DrawString(aQuitText, aQuit->mX + aQuit->mWidth / 2 - aAtomic->StringWidth(aQuitText) / 2,
				aQuit->mY + aQuit->mHeight - aPY - 0xc);
			g->SetColorizeImages(false);
		}
	}
}

// the head the game's buttons share: the panel will go and the doors open on the board that is made next (0x405b10, or
// 0x406060 for the saved game; the menu goes to the front over it)
void MainMenu::BeginClosing()
{
	mClosing = true;
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		aButton->SetVisible(false);
		aButton = mButtons.Next();
	}
}

// 0x4195a0: the games (0 and 1 the strategy mode, normal and expert; 2 and 3 the action mode), the saved game, "main menu" (6)
// and "play again" (7) after a game; the buttons act when they are pressed
void MainMenu::ButtonPress(int theId)
{
	if (!mSlammed)
		return;
	mApp->mSndCapClick->Play();
	switch (theId)
	{
	case 0:
	case 1:
		mApp->mMode = 0;
		mApp->mExpertMode = (theId == 1) ? 1 : 0;
		BeginClosing();
		mApp->StartGame();
		break;
	case 2:
	case 3:
		mApp->mMode = 1;
		mApp->mExpertMode = (theId == 3) ? 1 : 0;
		BeginClosing();
		mApp->StartGame();
		break;
	case 4:
		BeginClosing();
		mApp->ResumeGame();
		break;
	case 6:
		mLeaving = true;
		mBtn[6]->SetVisible(false);
		mBtn[7]->SetVisible(false);
		mApp->mMusicInterface->PlayMusic(1, 0x1f, false);
		return;
	case 7:
		BeginClosing();
		mApp->StartGame();
		break;
	default:
		return;
	}
	mWidgetManager->BringToFront(this);
	mApp->mSndSwoop->Play();
}

// 0x419520: the exit (5), when it is released
void MainMenu::ButtonDepress(int theId)
{
	if (!mSlammed || (theId != 5))
		return;
	if (mGameOver)
	{
		BeginClosing();
		mApp->StartGame();
		mWidgetManager->BringToFront(this);
		mApp->mSndSwoop->Play();
	}
	else
		mApp->Shutdown();
}

// ---------------------------------------------------------------------------------------------------------
// HighScoresBox

// 0x41ea40
HighScoresBox::HighScoresBox(AtomicaApp* theApp) :
	mApp(theApp),
	mDoor(0x12f),
	mAlpha(0),
	mTimer(0),
	mSlammed(false),
	mDoorOpenPlayed(false),
	mSwoopPlayed(false),
	mClosing(false),
	mSpin(0)
{
	mDoorSheet = mApp->mSprites->Load("menudoors");
	mDoorLeft = mApp->mSprites->SubSprite(mDoorSheet, 0, 0, 0x12f, 0x1e0);
	mDoorRight = mApp->mSprites->SubSprite(mDoorSheet, 0x12f, 0, 0x151, 0x1e0);
	mPanel = mApp->mSprites->Load("menupanel");
	mPanelY = mPanel->mHeight + 10;
	mHasTransparencies = true;
	mReturn = new ButtonWidget(0, this);
	mReturn->mBtnNoDraw = true;
	mReturn->mDoFinger = true;
	mReturn->Resize(0xd9, 0x17c, 0xce, 0x25);
}

// 0x41e690
HighScoresBox::~HighScoresBox()
{
	// 0x4190b0 / 0x41e690: the sheet, the doors and the panel are freed (the original's four frees; the sprite manager is gone when the
	// app's destructor deletes a menu that is still up)
	if (mApp->mSprites != NULL)
	{
		mApp->mSprites->Free(mDoorSheet);
		mApp->mSprites->Free(mDoorLeft);
		mApp->mSprites->Free(mDoorRight);
		mApp->mSprites->Free(mPanel);
	}
	mApp->mHighScoresBox = NULL;
	mApp->SafeDeleteWidget(mReturn);
}

// 0x41e960
void HighScoresBox::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mReturn);
	theWidgetManager->AddBaseModal(this);
}

// 0x41e9a0
void HighScoresBox::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveBaseModal(this);
	theWidgetManager->RemoveWidget(mReturn);
}

// 0x41e770: the menu's doors and panel (see MainMenu::Update), the doors shut over what is under the box and the panel waits
// 15 updates after the slam; "return" makes the panel go, the doors open and the box delete itself
void HighScoresBox::Update()
{
	MarkDirty();	// the original calls the widget's vtable slot 42 here, not Widget::Update
	mSpin += 0.04f;
	if (mSpin >= 256.0f)
		mSpin -= 256.0f;
	if (!mClosing)
	{
		mDoor = -1 - (int) ((double) mDoor * (double) -0.95f);
		if (mDoor < 0)
		{
			mDoor = 0;
			if (!mSlammed)
			{
				mApp->mSndDoorSlam->Play();
				mTimer = 0xf;
				mSlammed = true;
				mApp->mWidgetManager->BringToFront(mReturn);
			}
			mTimer--;
			if (mTimer < 0)
				mTimer = 0;
		}
		if ((mDoor == 0) && (mTimer < 1))
		{
			mPanelY = -1 - (int) ((double) mPanelY * (double) -0.9f);
			if (mPanelY < 0)
			{
				if (!mSwoopPlayed)
				{
					mApp->mSndSwoop->Play();
					mSwoopPlayed = true;
				}
				mPanelY = 0;
			}
			mAlpha += 3;
			if (mAlpha > 0xff)
			{
				mAlpha = 0xff;
				mHasTransparencies = (mDoor != 0);
				return;
			}
		}
	}
	else
	{
		mAlpha -= 3;
		if (mAlpha < 0)
			mAlpha = 0;
		int aGone = mPanel->mHeight + 10;
		mPanelY = (int) ((double) (mPanelY + 1) * (double) 1.1f);
		if (mPanelY > aGone)
			mPanelY = aGone;
		if (mPanelY == aGone)
		{
			if (!mDoorOpenPlayed)
			{
				mApp->mSndDoorOpen->Play();
				mDoorOpenPlayed = true;
			}
			mDoor = (int) ((double) (mDoor + 1) * (double) 1.25f);
			if (mDoor > 0x149)
			{
				mDoor = 0x14a;
				mApp->mWidgetManager->RemoveWidget(this);
				mApp->SafeDeleteWidget(this);
				return;		// the original goes on to write the widget's flag after it was deleted
			}
		}
	}
	mHasTransparencies = (mDoor != 0);
}

// 0x41ec90: both tables side by side (strategy left, action right), each with the best of the day
void HighScoresBox::Draw(Graphics* g)
{
	int aDoor = (mDoor < 0) ? 0 : mDoor;
	mDoorLeft->Draw(g, -aDoor, 0);
	mDoorRight->Draw(g, 0x12f + aDoor, 0);
	mPanel->Draw(g, 0x140 - mPanel->mWidth / 2, 10 - mPanelY);
	int aPY = mPanelY;
	// the button
	g->SetFont(mApp->mAtomicFont);
	g->SetColor(Color(mReturn->mIsOver ? 0x80 : 0, 0x80, 0, mAlpha / 2));
	g->FillRect(mReturn->mX, mReturn->mY - aPY, mReturn->mWidth, mReturn->mHeight);
	g->SetColor(Color(mReturn->mIsOver ? 255 : 0, 255, 0, mAlpha));
	const std::string& aReturn = GT(0x4b03b4);
	g->DrawString(aReturn, mReturn->mX + mReturn->mWidth / 2 - mApp->mAtomicFont->StringWidth(aReturn) / 2,
		mReturn->mY + mReturn->mHeight - aPY - 0xc);
	// the title
	g->SetFont(mApp->mAnnounceFont);
	g->SetColor(Color(0, 255, 0, 200));
	std::string aAtomic = "ATOMIC";
	g->DrawString(aAtomic, 0x140 - mApp->mAnnounceFont->StringWidth(aAtomic) / 2, 0x64 - aPY);
	const std::string& aTitle = GT(0x4afe88);
	g->DrawString(aTitle, 0x140 - mApp->mAnnounceFont->StringWidth(aTitle) / 2, 0x87 - aPY);
	// the tables
	Font* aMedium = mApp->mMonitorFontMedium;
	Color aColors[256];
	BuildScoreColors(aColors, mAlpha);
	Color aScoreColor(0, 255, 0, 200);
	for (int aKind = 0; aKind < 2; aKind++)
	{
		ScoreTable& aTable = mApp->mScoreTables[aKind];
		int aNameX = (aKind == 0) ? 0x69 : 0x14a;
		int aScoreX = (aKind == 0) ? 300 : 0x20d;
		int aCenter = (aKind == 0) ? 0xca : 0x1ab;
		g->SetFont(mApp->mMonitorFont);
		g->SetColor(Color(0xaf, 255, 0x3c, 255));
		const std::string& aHeading = GT(aKind == 0 ? 0x4b038c : 0x4b039c);
		g->DrawString(aHeading, aCenter - mApp->mMonitorFont->StringWidth(aHeading) / 2, 0xb4 - aPY);
		const std::string& aToday = GT(0x4b037c);
		g->DrawString(aToday, aCenter - mApp->mMonitorFont->StringWidth(aToday) / 2, 0x13b - aPY);
		g->SetFont(aMedium);
		// the rows run through the cycle of colours, each one on from the one above (both tables from the same one)
		int aColor = (int) mSpin;
		for (int i = 0; i < ScoreTable::NUM_SCORES; i++)
		{
			g->SetColor(aColors[aColor]);
			g->DrawString(aTable.mNames[i], aNameX, 0xd2 + 0x12 * i - aPY);
			// the scores are drawn in the colour the page itself holds: the title's for the first table, then the one the
			// row of the best of the day set
			g->SetColor(aScoreColor);
			std::string aScore = Format(GT(0x4aff20), aTable.mScores[i]);
			g->DrawString(aScore, aScoreX - aMedium->StringWidth(aScore), 0xd2 + 0x12 * i - aPY);
			aColor++;
			if (aColor > 0xff)
				aColor -= 0xff;
		}
		g->SetColor(aColors[aColor]);
		aScoreColor = aColors[aColor];
		if ((aTable.mDate == ScoreTable::DateCode()) && (aTable.mBestToday != -1))
		{
			g->DrawString(aTable.mBestName, aNameX, 0x159 - aPY);
			std::string aScore = Format(GT(0x4aff20), aTable.mBestToday);
			g->DrawString(aScore, aScoreX - aMedium->StringWidth(aScore), 0x159 - aPY);
		}
		else
		{
			const std::string& aNone = GT(0x4afe5c);
			g->DrawString(aNone, aCenter - aMedium->StringWidth(aNone) / 2, 0x159 - aPY);
		}
	}
}

// 0x41e9f0
void HighScoresBox::ButtonPress(int theId)
{
	(void) theId;
	mApp->mSndCapClick->Play();
}

// 0x41e9e0
void HighScoresBox::ButtonDepress(int theId)
{
	(void) theId;
	mClosing = true;
}
