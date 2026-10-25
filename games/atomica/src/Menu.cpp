// The main menu (see Menu.h).

#include "Menu.h"
#include "AtomicaApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	// the buttons: the games, the buttons that switch the expert mode, the saved game, the exit (0x4198e0)
	struct ButtonSpec { int mId; int mX; int mY; int mW; int mH; };
	const ButtonSpec kButtons[] = {
		{0, 0x5a, 0x4b, 0x1c5, 0x3b},		// strategy
		{1, 0x1de, 0x4b, 0x41, 0x3c},
		{2, 0x5a, 0xb1, 0x1c5, 0x3b},		// action
		{3, 0x1de, 0xb1, 0x41, 0x3c},
		{4, 0x5a, 0x119, 0x1c5, 0x3b},		// the saved game
		{5, 0xd9, 0x178, 0xce, 0x25}		// exit
	};
}

// 0x4198e0
MainMenu::MainMenu(AtomicaApp* theApp, bool theGameOver) :
	mApp(theApp),
	mDoor(0),
	mDoorTick(0),
	mGameOver(theGameOver),
	mOpening(true),
	mPanelShown(false),
	mSpin(0)
{
	Sprite* aDoors = mApp->mSprites->Load("menudoors");
	mDoorLeft = mApp->mSprites->SubSprite(aDoors, 0, 0, 0x12f, 0x1e0);
	mDoorRight = mApp->mSprites->SubSprite(aDoors, 0x12f, 0, 0x151, 0x1e0);
	mPanel = mApp->mSprites->Load("menupanel");
	mApp->mB3eac = false;
	mPanelY = mPanel->mHeight + 10;
	mExpert[0] = mExpert[1] = 0;
	mHasSave = mApp->HasSavedGame();
	mApp->mMusicInterface->PlayMusic(0, 0, false);
	for (size_t i = 0; i < sizeof(kButtons) / sizeof(kButtons[0]); i++)
	{
		const ButtonSpec& aSpec = kButtons[i];
		if ((aSpec.mId == 4) && !mHasSave)
			continue;
		ButtonWidget* aButton = new ButtonWidget(aSpec.mId, this);
		aButton->mBtnNoDraw = true;
		aButton->mDoFinger = true;
		aButton->Resize(aSpec.mX, aSpec.mY, aSpec.mW, aSpec.mH);
		mButtons.Add(aButton);
	}
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

// 0x4191d0: the doors slide open
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
				mPanelShown = true;
			}
		}
	}
	MarkDirty();
}

// 0x41a010: port: the doors, the panel and the captions of the buttons
void MainMenu::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 255));
	g->FillRect(0, 0, mWidth, mHeight);
	if (mDoor > 0)
		mPanel->Draw(g, (mWidth - mPanel->mWidth) / 2, 10);
	if (mPanelShown)
	{
		ImageFont* aFont = mApp->mAnnounceFont;
		g->SetFont(aFont);
		static const unsigned int kCaptions[] = {0x4b0000, 0x4affd8, 0x4affa8, 0x4aff4c};
		static const int kIds[] = {0, 2, 4, 5};
		for (int i = 0; i < 4; i++)
		{
			const ButtonSpec* aSpec = NULL;
			for (size_t j = 0; j < sizeof(kButtons) / sizeof(kButtons[0]); j++)
				if (kButtons[j].mId == kIds[i])
					aSpec = &kButtons[j];
			if ((kIds[i] == 4) && !mHasSave)
				continue;
			bool aOver = false;
			mButtons.Reset();
			Widget* aButton = mButtons.Next();
			while (aButton != NULL)
			{
				if (((ButtonWidget*) aButton)->mId == kIds[i])
					aOver = aButton->mIsOver;
				aButton = mButtons.Next();
			}
			const std::string& aText = GT(kCaptions[i]);
			g->SetColor(aOver ? Color(255, 255, 160, 255) : Color(150, 191, 150, 255));
			g->DrawString(aText, aSpec->mX + (aSpec->mW - aFont->StringWidth(aText)) / 2,
				aSpec->mY + (aSpec->mH + aFont->GetAscent()) / 2 - 2);
		}
		// the expert switches
		for (int i = 0; i < 2; i++)
		{
			Sprite* aSprite = mApp->mExpert[i];
			const ButtonSpec& aSpec = kButtons[1 + 2 * i];
			g->SetColorizeImages(true);
			g->SetColor(mExpert[i] ? Color(255, 255, 255, 255) : Color(255, 255, 255, 140));
			aSprite->Draw(g, aSpec.mX + (aSpec.mW - aSprite->mWidth) / 2, aSpec.mY + (aSpec.mH - aSprite->mHeight) / 2);
			g->SetColorizeImages(false);
		}
	}
	if (mDoor < 0x14a + 1)
	{
		mDoorLeft->Draw(g, -mDoor, 0);
		mDoorRight->Draw(g, 0x12f + mDoor, 0);
	}
}

// 0x4195a0
void MainMenu::ButtonDepress(int theId)
{
	if (!mPanelShown)
		return;
	switch (theId)
	{
	case 0:
	case 2:
		mApp->StartNewGame(theId == 2 ? 1 : 0, mExpert[theId == 2 ? 1 : 0]);
		break;
	case 1:
		mExpert[0] = 1 - mExpert[0];
		break;
	case 3:
		mExpert[1] = 1 - mExpert[1];
		break;
	case 4:
		mApp->ResumeGame();
		break;
	case 5:
		mApp->Shutdown();
		break;
	}
}
