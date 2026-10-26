// The game selector (see Selector.h).

#include "Selector.h"
#include "AlchemyApp.h"
#include "Wizard.h"
#include "ButtonWidget.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "WidgetManager.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x40c817
SelectorScreen::SelectorScreen(AlchemyApp* theApp)
{
	mApp = theApp;

	// the three 89 x 67 skill pictures: the green tile with the level's name
	for (int i = 0; i < 3; i++)
	{
		mSkillImages[i] = new MemoryImage(mApp);
		mSkillImages[i]->Create(0x59, 0x43);
		mSkillImages[i]->SetImageMode(false, false);
		Graphics g(mSkillImages[i]);
		g.DrawImage(mApp->mImages[IMAGE_GAMESELECTOR_GREENON], 0, 0);

		SexyString aText;
		if (i == 0)
			aText = mApp->GetString("EASY");
		else if (i == 1)
			aText = mApp->GetString("AVERAGE");
		else
			aText = mApp->GetString("HARD");
		g.SetFont(mApp->mFont1);
		g.SetColor(Color(0x39, 0xff, 0x4a));
		g.DrawString(aText, 0x2c - g.GetFont()->StringWidth(aText) / 2, 0x32);
	}

	// the buttons in the order the original made them: ids 0xb..0xd, then 4, 0 and 1
	for (int i = 0; i < 3; i++)
	{
		mSkillButtons[i] = new ButtonWidget(0xb + i, this);
		mSkillButtons[i]->mDoFinger = true;
		mSkillButtons[i]->mDownImage = mSkillImages[i];
	}

	mTutorialButton = new ButtonWidget(4, this);
	mTutorialButton->mBtnNoDraw = true;
	mTutorialButton->mDoFinger = true;
	mTutorialButton->SetVisible(mApp->mTimesPlayed < 5);

	mStartButton = new ButtonWidget(0, this);
	mStartButton->mOverImage = mApp->mImages[IMAGE_GAMESELECTOR_NEWGAMEON];
	mStartButton->mDownImage = mApp->mImages[IMAGE_GAMESELECTOR_NEWGAMEON];
	mStartButton->mDoFinger = true;

	mTutorialPicButton = new ButtonWidget(1, this);
	mTutorialPicButton->mOverImage = mApp->mImages[IMAGE_GAMESELECTOR_TUTON];
	mTutorialPicButton->mDownImage = mApp->mImages[IMAGE_GAMESELECTOR_TUTON];
	mTutorialPicButton->mDoFinger = true;

	// the two 132 x 48 mode pictures: the red tile with its caption
	mStrategicImage = new MemoryImage(mApp);
	mStrategicImage->Create(0x84, 0x30);
	mStrategicImage->SetImageMode(false, false);
	{
		Graphics g(mStrategicImage);
		g.DrawImage(mApp->mImages[IMAGE_GAMESELECTOR_REDON], 0, 0);
		g.SetFont(mApp->mFont1);
		g.SetColor(Color(0xf7, 0x86, 0x31));
		g.DrawString(mApp->GetString("STRATEGIC"), 0x2b, 0x1b);
	}

	mTimeTrialImage = new MemoryImage(mApp);
	mTimeTrialImage->Create(0x84, 0x30);
	mTimeTrialImage->SetImageMode(false, false);
	{
		Graphics g(mTimeTrialImage);
		g.DrawImage(mApp->mImages[IMAGE_GAMESELECTOR_REDON], 0, 0);
		g.SetFont(mApp->mFont1);
		g.SetColor(Color(0xf7, 0x86, 0x31));
		g.DrawString(mApp->GetString("INTROSCREEN_TIME_TRIAL"), 0x2b, 0x1b);
	}

	mStrategicButton = new ButtonWidget(2, this);
	mStrategicButton->mDoFinger = true;
	mStrategicButton->mDownImage = mStrategicImage;

	mTimeTrialButton = new ButtonWidget(3, this);
	mTimeTrialButton->mDoFinger = true;
	mTimeTrialButton->mDownImage = mTimeTrialImage;

	mWizard = new WizardBox(mApp);

	mSkill = 1;
	mTimeTrial = false;
	SetSkill(mApp->mSkillLevel);
	SetTimeTrial(mApp->mTimeTrial);
}

// 0x40c7fb (-> 0x40c3ca)
SelectorScreen::~SelectorScreen()
{
	delete mStartButton;
	delete mTutorialPicButton;
	delete mTutorialButton;
	for (int i = 0; i < 3; i++)
	{
		delete mSkillButtons[i];
		delete mSkillImages[i];
	}
	delete mStrategicButton;
	delete mTimeTrialButton;
	delete mStrategicImage;
	delete mTimeTrialImage;
	delete mWizard;
}

// 0x40c488: the chosen skill's button shows its picture, the other two are blank until pressed
void SelectorScreen::SetSkill(int theSkill)
{
	mSkill = theSkill;
	for (int i = 0; i < 3; i++)
	{
		mSkillButtons[i]->mButtonImage = (i == theSkill - 1) ? mSkillImages[i] : NULL;
		mSkillButtons[i]->MarkDirty();
	}
}

// 0x40c4c6
void SelectorScreen::SetTimeTrial(bool isTimeTrial)
{
	mTimeTrial = isTimeTrial;
	mStrategicButton->mButtonImage = isTimeTrial ? NULL : mStrategicImage;
	mStrategicButton->MarkDirty();
	mTimeTrialButton->mButtonImage = isTimeTrial ? mTimeTrialImage : NULL;
	mTimeTrialButton->MarkDirty();
}

// 0x40c526: the first time the selector is up, the wizard greets the player a second in
void SelectorScreen::Update()
{
	Widget::Update();

	if (mApp->mSelectorFlag && mUpdateCnt == 0x3c)
	{
		mApp->PlaySample(SOUND_WELCOME);
		mWizard->Talk(0);
	}
}

// 0x40c559
void SelectorScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mStartButton);
	theWidgetManager->AddWidget(mTutorialPicButton);
	theWidgetManager->AddWidget(mStrategicButton);
	theWidgetManager->AddWidget(mTimeTrialButton);
	theWidgetManager->AddWidget(mWizard);
	theWidgetManager->AddWidget(mTutorialButton);
	for (int i = 0; i < 3; i++)
		theWidgetManager->AddWidget(mSkillButtons[i]);
}

// 0x40c5c8
void SelectorScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStartButton);
	theWidgetManager->RemoveWidget(mTutorialPicButton);
	theWidgetManager->RemoveWidget(mStrategicButton);
	theWidgetManager->RemoveWidget(mTimeTrialButton);
	theWidgetManager->RemoveWidget(mWizard);
	theWidgetManager->RemoveWidget(mTutorialButton);
	for (int i = 0; i < 3; i++)
		theWidgetManager->RemoveWidget(mSkillButtons[i]);
}

// 0x40c637
void SelectorScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	mSkillButtons[0]->Resize(mX + 0x67, 0x100, 0x59, 0x43);
	mSkillButtons[1]->Resize(mX + 0xc9, 0x100, 0x59, 0x43);
	mSkillButtons[2]->Resize(mX + 0x12d, 0x100, 0x59, 0x43);
	mStrategicButton->Resize(mX + 0x66, mY + 0x15c, 0x84, 0x30);
	mTimeTrialButton->Resize(mX + 0x101, mY + 0x15c, 0x84, 0x30);
	mStartButton->Resize(mX + 0x11f, mY + 0x1a1, 0xb4, 0x33);
	mTutorialPicButton->Resize(mX + 0xf, mY + 0x1a1, 0xb4, 0x33);
	mWizard->Resize(mX + 8, mY + 0x14, 0xae, 0xbc);
	mTutorialButton->Resize(mX + 200, mY + 0x9d, 0x56, 0xf);
}

// 0x40cefe
void SelectorScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0x96, 0, 0x1ea, 0x1e0));
	g->DrawImage(mApp->mImages[IMAGE_GAMESELECTOR_BASE], 10, 0x23);

	g->SetFont(mApp->mTitleFont);
	g->SetColor(Color(255, 255, 255));
	if (!mApp->mSelectorFlag)
	{
		SexyString aText = mApp->GetString("NEW_GAME");
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 100);
	}
	else
	{
		SexyString aText = mApp->GetString("WELCOME_TO_ALCHEMY1");
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x59);
		aText = mApp->GetString("WELCOME_TO_ALCHEMY2");
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x75);
	}

	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0xc6, 0xb6, 0x94));
	if (!mTutorialButton->mVisible)
	{
		SexyString aText = mApp->GetString("SELECT_GAME_OPTIONS");
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0xa3);
	}
	else
	{
		SexyString aText = mApp->GetString("TUTORIAL_SUGGESTION1");
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x93);
		aText = mApp->GetString("TUTORIAL_SUGGESTION2");
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0xa7);
		g->FillRect(200, 0xab, 0x56, 1);
	}

	// the three skill names under the buttons' pictures
	for (int i = 0; i < 3; i++)
	{
		SexyString aText;
		int aX;
		if (i == 0)
		{
			aText = mApp->GetString("EASY");
			aX = 0x93;
		}
		else if (i == 1)
		{
			aText = mApp->GetString("AVERAGE");
			aX = 0xf5;
		}
		else
		{
			aText = mApp->GetString("HARD");
			aX = 0x159;
		}
		g->SetFont(mApp->mFont1);
		g->SetColor(Color(0x8c, 0x82, 0x73));
		g->DrawString(aText, aX - g->GetFont()->StringWidth(aText) / 2, 0x132);
	}

	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0x8c, 0x82, 0x73));
	g->DrawString(mApp->GetString("STRATEGIC"), 0x91, 0x177);
	g->DrawString(mApp->GetString("INTROSCREEN_TIME_TRIAL"), 300, 0x177);
}

// 0x40c760: the click
void SelectorScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK);
}

// 0x40c770
void SelectorScreen::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mApp->mTutorialMode = false;
		mApp->SetTimeTrial(mTimeTrial);
		mApp->SetSkillLevel(mSkill);
		mApp->StartGame();
		break;
	case 1:
	case 4:
		mApp->mTutorialMode = true;
		mApp->StartGame();
		break;
	case 2:
		SetTimeTrial(false);
		break;
	case 3:
		SetTimeTrial(true);
		break;
	default:
		if (theId > 10 && theId <= 13)
			SetSkill(theId - 10);
		break;
	}
}
