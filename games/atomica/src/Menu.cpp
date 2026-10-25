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

// 0x4198e0
MainMenu::MainMenu(AtomicaApp* theApp, bool theGameOver) :
	mApp(theApp),
	mDoor(0),
	mAlpha(0),
	mDoorTick(0),
	mGameOver(theGameOver),
	mOpening(true),
	mPanelShown(false),
	mSpin(0),
	mInsertTimer(100),
	mRank(0),
	mEntryShown(false)
{
	Sprite* aDoors = mApp->mSprites->Load("menudoors");
	mDoorLeft = mApp->mSprites->SubSprite(aDoors, 0, 0, 0x12f, 0x1e0);
	mDoorRight = mApp->mSprites->SubSprite(aDoors, 0x12f, 0, 0x151, 0x1e0);
	mPanel = mApp->mSprites->Load("menupanel");
	mApp->mB3eac = false;
	mPanelY = mPanel->mHeight + 10;
	mHasSave = mApp->HasSavedGame();
	mApp->mMusicInterface->PlayMusic(0, 0, false);

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
		mTime = aBoard->mStat1c4;
		mAtoms = aBoard->mStat1cc;
		mMolecules = aBoard->mStat1d0;
		mCombo = aBoard->mStat1d4;
		mBonusMolecules = aBoard->mStat1d8;
		mBonusUpgrades = aBoard->mStat1dc;
	}
	mRank = RankOfScore(mScore);

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

MainMenu::~MainMenu()
{
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		mApp->SafeDeleteWidget(aButton);
		aButton = mButtons.Next();
	}
	mButtons.Clear();
}

// the buttons of the main page or those of the end of a game
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

// 0x4191d0: the doors slide open, the panel comes up and its contents fade in; after a game the score goes to the high
// score table 100 updates later
void MainMenu::Update()
{
	Widget::Update();
	mSpin += 0.04f;
	if (mSpin >= 256.0f)
		mSpin -= 256.0f;
	if (mOpening)
	{
		if (mDoorTick == 0)
			mApp->mSndDoorOpen->Play();
		mDoorTick++;
		if (mDoorTick > 20)
		{
			mDoor += 3;
			if (mDoor > 0x14a)
			{
				mDoor = 0x14a;
				mOpening = false;
			}
		}
	}
	else if (mPanelY > 0)
	{
		mPanelY -= 12;
		if (mPanelY < 0)
			mPanelY = 0;
	}
	else if (mAlpha < 255)
	{
		mAlpha += 3;
		if (mAlpha > 255)
			mAlpha = 255;
		mPanelShown = (mAlpha >= 255);
	}
	if (mGameOver && mPanelShown && !mEntryShown)
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
	MarkDirty();
}

bool MainMenu::Over(int theId) const
{
	return (mBtn[theId] != NULL) && mBtn[theId]->mIsOver;
}

// a button's field, brighter when the mouse is over it
void MainMenu::FillButton(Graphics* g, int theId)
{
	ButtonWidget* aButton = mBtn[theId];
	g->SetColor(Color(Over(theId) ? 0x80 : 0, 0x80, 0, mAlpha / 2));
	g->FillRect(aButton->mX, aButton->mY - mPanelY, aButton->mWidth, aButton->mHeight);
	g->SetColor(Color(Over(theId) ? 255 : 0, 255, 0, mAlpha));
	g->SetFont(mApp->mAtomicFont);
	// the caption is drawn by the caller
}

// 0x41a010
void MainMenu::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 255));
	g->FillRect(0, 0, mWidth, mHeight);
	if (mDoor > 0)
		mPanel->Draw(g, 0x140 - mPanel->mWidth / 2, 10 - mPanelY);
	int aPY = mPanelY;
	ImageFont* aAtomic = mApp->mAtomicFont;
	ImageFont* aSmall = mApp->mMonitorFontSmall;
	if (!mOpening || mDoor >= 0x14a)
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
			g->SetFont(mApp->mMonitorFontMedium);
			g->SetColor(Color(0, 255, 0, mAlpha));
			for (int i = 0; i < ScoreTable::NUM_SCORES; i++)
			{
				g->DrawString(aTable.mNames[i], 0x10e, 0xd2 + 0x12 * i - aPY);
				std::string aScore = Format(GT(0x4aff20), aTable.mScores[i]);
				g->DrawString(aScore, 0x1fe - mApp->mMonitorFontMedium->StringWidth(aScore), 0xd2 + 0x12 * i - aPY);
			}
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
				{
					g->SetColor(Color(200, 200, 0, mAlpha / 2));
					g->FillRect(0x1de, aY - aPY, 0x41, 0x3c);
				}
				g->SetColor(Color((aOver || aExpertOver) ? 255 : 0, 255, 0, mAlpha));
				mApp->mGameType[aGame]->Draw(g, 100, aY - 10 - aPY);
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
	if (mDoor < 0x14a + 1)
	{
		mDoorLeft->Draw(g, -mDoor, 0);
		mDoorRight->Draw(g, 0x12f + mDoor, 0);
	}
}

// 0x4195a0: the games (0 and 1 the strategy mode, normal and expert; 2 and 3 the action mode), the saved game, the exit;
// after a game "main menu" (6) and "play again" (7)
void MainMenu::ButtonDepress(int theId)
{
	if (!mPanelShown)
		return;
	mApp->mSndCapClick->Play();
	switch (theId)
	{
	case 0:
	case 1:
		mApp->StartNewGame(0, theId == 1 ? 1 : 0);
		break;
	case 2:
	case 3:
		mApp->StartNewGame(1, theId == 3 ? 1 : 0);
		break;
	case 4:
		mApp->ResumeGame();
		break;
	case 5:
		mApp->Shutdown();
		break;
	case 6:
		ShowPage(false);
		break;
	case 7:
		mApp->StartNewGame(mApp->mMode, mApp->mExpertMode);
		break;
	}
}
