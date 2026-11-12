#include "IntroScreen.h"
#include "CellButton.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MenuTalker.h"
#include "MusicInterface.h"
#include "PuzzleListWidget.h"
#include "Sprite.h"
#include "ScrollbarWidget.h"
#include "WidgetManager.h"
#include "platform/VFS.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

// 0x52b320: true in the exe's data, so the first main menu after the title has the welcome sign and MenuTalker 1 and
// leaves the title's music playing; the screen's destructor clears it
bool Sexy::gIntroReturning = true;

// port: split out of the constructor 0x424e90, which makes each of its buttons this way
static CellButton* NewMenuButton(int theId, ButtonListener* theListener, bool noDraw)
{
	CellButton* aButton = new CellButton(theId, theListener);
	aButton->mBtnNoDraw = noDraw;
	aButton->mDoFinger = true;
	aButton->mHasTransparencies = true;
	aButton->SetVisible(true);
	return aButton;
}

// 0x424e90
IntroScreen::IntroScreen(FossilizedApp* theApp)
{
	mApp = theApp;
	if ((gIntroReturning != true) && !theApp->mInTutorial)
	{
		theApp->mMusicInterface->FadeOut(0, true, 0.004);
		mApp->mMusicInterface->FadeOut(1, true, 0.004);
		mApp->mMusicInterface->SetSongVolume(2, 0.0);
		mApp->mMusicInterface->FadeIn(2, 0x16, 0.002, false);
	}
	mPage = 0;
	mStartChoice = 0;
	mChosenSlot = -1;
	mSlotCount = 0;
	int i = 8;
	Pic** aPic = mSlotPics;
	do
	{
		aPic[0] = NULL;
		mSlotCells[aPic - mSlotPics] = NULL;
		mSlotButtons[aPic - mSlotPics] = NULL;
		aPic++;
		i--;
	} while (i != 0);
	if (gIntroReturning == true)
		mTalker = new MenuTalker(mApp, this, 1);
	else
		mTalker = new MenuTalker(mApp, this, 3);

	// the buttons draw nothing themselves: the page's picture shows them
	mStartButtons[0] = NewMenuButton(10, this, true);
	mStartButtons[1] = NewMenuButton(11, this, true);
	mStartButtons[2] = NewMenuButton(12, this, true);
	mSkillButtons[0] = NewMenuButton(0, this, true);
	mSkillButtons[1] = NewMenuButton(1, this, true);
	mSkillButtons[2] = NewMenuButton(2, this, true);
	mModeButtons[0] = NewMenuButton(3, this, true);
	mModeButtons[1] = NewMenuButton(4, this, true);
	mModeButtons[2] = NewMenuButton(5, this, true);
	mModeButtons[3] = NewMenuButton(6, this, true);
	mOkButton = NewMenuButton(7, this, false);
	mBackButton = NewMenuButton(9, this, false);
	mOkButton->mButtonCell = mApp->mPicNextButton;
	mOkButton->mOverCell = mApp->mPicNextButtonOver;
	mOkButton->mDownCell = mApp->mPicNextButtonOver;
	mBackButton->mButtonCell = mApp->mPicBackButton;
	mBackButton->mOverCell = mApp->mPicBackButtonOver;
	mBackButton->mDownCell = mApp->mPicBackButtonOver;
	mScrollbar = NULL;
	mPuzzleList = NULL;
	mPuzzleList = new PuzzleListWidget(mApp);
	if (mPuzzleList == NULL)
		mScrollbar = new ScrollbarWidget(1, NULL);
	else
		mScrollbar = new ScrollbarWidget(1, mPuzzleList);
	mScrollbar->mInvisIfNoScroll = true;
	mPuzzleList->mScrollbar = mScrollbar;
	mKeepMusic = false;
}

// 0x425570
IntroScreen::~IntroScreen()
{
	FreeSlots();
	if (!mKeepMusic)
		mApp->mMusicInterface->FadeOut(2, true, 0.004);
	if (mStartButtons[0] != NULL)
	{
		mApp->SafeDeleteWidget(mStartButtons[0]);
		mStartButtons[0] = NULL;
	}
	if (mScrollbar != NULL)
	{
		mApp->SafeDeleteWidget(mScrollbar);
		mScrollbar = NULL;
	}
	if (mPuzzleList != NULL)
	{
		mApp->SafeDeleteWidget(mPuzzleList);
		mPuzzleList = NULL;
	}
	if (mStartButtons[1] != NULL)
	{
		mApp->SafeDeleteWidget(mStartButtons[1]);
		mStartButtons[1] = NULL;
	}
	if (mStartButtons[2] != NULL)
	{
		mApp->SafeDeleteWidget(mStartButtons[2]);
		mStartButtons[2] = NULL;
	}
	if (mSkillButtons[0] != NULL)
	{
		mApp->SafeDeleteWidget(mSkillButtons[0]);
		mSkillButtons[0] = NULL;
	}
	if (mSkillButtons[1] != NULL)
	{
		mApp->SafeDeleteWidget(mSkillButtons[1]);
		mSkillButtons[1] = NULL;
	}
	if (mSkillButtons[2] != NULL)
	{
		mApp->SafeDeleteWidget(mSkillButtons[2]);
		mSkillButtons[2] = NULL;
	}
	if (mModeButtons[0] != NULL)
	{
		mApp->SafeDeleteWidget(mModeButtons[0]);
		mModeButtons[0] = NULL;
	}
	if (mModeButtons[1] != NULL)
	{
		mApp->SafeDeleteWidget(mModeButtons[1]);
		mModeButtons[1] = NULL;
	}
	if (mModeButtons[2] != NULL)
	{
		mApp->SafeDeleteWidget(mModeButtons[2]);
		mModeButtons[2] = NULL;
	}
	if (mModeButtons[3] != NULL)
	{
		mApp->SafeDeleteWidget(mModeButtons[3]);
		mModeButtons[3] = NULL;
	}
	if (mOkButton != NULL)
	{
		mApp->SafeDeleteWidget(mOkButton);
		mOkButton = NULL;
	}
	if (mBackButton != NULL)
	{
		mApp->SafeDeleteWidget(mBackButton);
		mBackButton = NULL;
	}
	delete mTalker;
	mTalker = NULL;
	gIntroReturning = false;
}

// 0x423d10
void IntroScreen::Update()
{
	Widget::Update();
	mTalker->Update();
}

// 0x423d30
void IntroScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mScrollbar);
	theWidgetManager->AddWidget(mPuzzleList);
	theWidgetManager->AddWidget(mStartButtons[0]);
	theWidgetManager->AddWidget(mStartButtons[1]);
	theWidgetManager->AddWidget(mStartButtons[2]);
	theWidgetManager->AddWidget(mSkillButtons[0]);
	theWidgetManager->AddWidget(mSkillButtons[1]);
	theWidgetManager->AddWidget(mSkillButtons[2]);
	theWidgetManager->AddWidget(mModeButtons[0]);
	theWidgetManager->AddWidget(mModeButtons[1]);
	theWidgetManager->AddWidget(mModeButtons[2]);
	theWidgetManager->AddWidget(mModeButtons[3]);
	theWidgetManager->AddWidget(mOkButton);
	theWidgetManager->AddWidget(mBackButton);
	int i = 0;
	if (0 < mSlotCount)
	{
		do
		{
			if (mSlotButtons[i] != NULL)
				theWidgetManager->AddWidget(mSlotButtons[i]);
			i++;
		} while (i < mSlotCount);
	}
}

// 0x423e40
void IntroScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStartButtons[0]);
	theWidgetManager->RemoveWidget(mStartButtons[1]);
	theWidgetManager->RemoveWidget(mStartButtons[2]);
	theWidgetManager->RemoveWidget(mSkillButtons[0]);
	theWidgetManager->RemoveWidget(mSkillButtons[1]);
	theWidgetManager->RemoveWidget(mSkillButtons[2]);
	theWidgetManager->RemoveWidget(mModeButtons[0]);
	theWidgetManager->RemoveWidget(mModeButtons[1]);
	theWidgetManager->RemoveWidget(mModeButtons[2]);
	theWidgetManager->RemoveWidget(mModeButtons[3]);
	theWidgetManager->RemoveWidget(mScrollbar);
	theWidgetManager->RemoveWidget(mPuzzleList);
	theWidgetManager->RemoveWidget(mOkButton);
	theWidgetManager->RemoveWidget(mBackButton);
	int i = 0;
	if (0 < mSlotCount)
	{
		do
		{
			if (mSlotButtons[i] != NULL)
				theWidgetManager->RemoveWidget(mSlotButtons[i]);
			i++;
		} while (i < mSlotCount);
	}
}

// 0x423f50: every button goes off the screen, then the page's own come back
void IntroScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mStartButtons[0]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mStartButtons[1]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mStartButtons[2]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mModeButtons[0]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mModeButtons[1]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mModeButtons[2]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mModeButtons[3]->Resize(mX - 1000, mY - 1000, 0x11f, 0x44);
	mBackButton->Resize(mX - 1000, mY - 1000, 0x69, 0x23);
	mSkillButtons[0]->SetDisabled(true);
	mSkillButtons[1]->SetDisabled(true);
	mSkillButtons[2]->SetDisabled(true);
	mSkillButtons[0]->Resize(mX - 1000, mY - 1000, 0x48, 0x43);
	mSkillButtons[1]->Resize(mX - 1000, mY - 1000, 0x48, 0x43);
	mSkillButtons[2]->Resize(mX - 1000, mY - 1000, 0x48, 0x43);
	mScrollbar->ResizeScrollbar(mX - 1000, mY - 1000, 100, 100);
	mPuzzleList->Resize(mX - 1000, mY - 1000, 100, 0x10e);
	if (mPage == 0)
	{
		mStartButtons[0]->Resize(mX + 0xda, mY + 0xb4, 0xec, 0x43);
		mStartButtons[1]->Resize(mX + 0xdb, mY + 0xfd, 0xec, 0x43);
		mStartButtons[2]->Resize(mX + 0xdb, mY + 0x146, 0xec, 0x43);
		mOkButton->mButtonCell = mApp->mPicNextButton;
		mOkButton->mOverCell = mApp->mPicNextButtonOver;
		mOkButton->mDownCell = mApp->mPicNextButtonOver;
	}
	if (mPage == 1)
	{
		mModeButtons[0]->Resize(mX + 0xaf, mY + 0xc4, 0x11f, 0x44);
		mModeButtons[1]->Resize(mX + 0xaf, mY + 0x80, 0x118, 0x44);
		mModeButtons[2]->Resize(mX + 0xaf, mY + 0x108, 0x11f, 0x44);
		mModeButtons[3]->Resize(mX + 0xaf, mY + 0x14c, 0x11f, 0x44);
		mOkButton->mButtonCell = mApp->mPicNextButton;
		mOkButton->mOverCell = mApp->mPicNextButtonOver;
		mOkButton->mDownCell = mApp->mPicNextButtonOver;
	}
	if (mPage == 2)
	{
		mSkillButtons[0]->Resize(mX + 0xa0, mY + 0xb0, 0x113, 0x43);
		mSkillButtons[1]->Resize(mX + 0xa0, mY + 0xf3, 0x113, 0x43);
		if (mPage != 6)
			mSkillButtons[2]->Resize(mX + 0xa0, mY + 0x138, 0x113, 0x43);
		if ((mApp->mPuzzleSkills & 1) != 0)
			mSkillButtons[0]->SetDisabled(false);
		if ((mApp->mPuzzleSkills & 2) != 0)
			mSkillButtons[1]->SetDisabled(false);
		if ((mApp->mPuzzleSkills & 4) != 0)
			mSkillButtons[2]->SetDisabled(false);
		mOkButton->mButtonCell = mApp->mPicPlayButton;
		mOkButton->mOverCell = mApp->mPicPlayButtonOver;
		mOkButton->mDownCell = mApp->mPicPlayButtonOver;
	}
	if (mPage == 6)
	{
		mSkillButtons[0]->Resize(mX + 0xa0, mY + 0xba, 0x113, 0x43);
		mSkillButtons[1]->Resize(mX + 0xa0, mY + 0xfd, 0x113, 0x43);
		mOkButton->mButtonCell = mApp->mPicPlayButton;
		mOkButton->mOverCell = mApp->mPicPlayButtonOver;
		mOkButton->mDownCell = mApp->mPicPlayButtonOver;
		mSkillButtons[0]->SetDisabled(false);
		mSkillButtons[1]->SetDisabled(false);
	}
	mOkButton->Resize(mX + 0x121, mY + 0x1a1, 0x69, 0x23);
	if (mPage == 5)
	{
		mPuzzleList->Resize(mX + 0xb4, mY + 0x82, 0xfa, 0x10e);
		if (mPuzzleList->mNeedsScrollbar == true)
			mScrollbar->ResizeScrollbar(mX + 0x1ae, mY + 0x82, 0x14, 0x10e);
		if (mPuzzleList->mNothingToPlay == true)
			mOkButton->Resize(mX - 1000, mY - 1000, 0x69, 0x23);
		mOkButton->mButtonCell = mApp->mPicNextButton;
		mOkButton->mOverCell = mApp->mPicNextButtonOver;
		mOkButton->mDownCell = mApp->mPicNextButtonOver;
	}
	if (mPage != 0)
		mBackButton->Resize(mX + 0xb3, mY + 0x1a1, 0x69, 0x23);
	if (mPage == 3)
	{
		int x = 200;
		if (mChosenSlot == -1)
			mOkButton->Resize(mX - 1000, mY - 1000, 0x69, 0x23);
		int i = 0;
		if (0 < mSlotCount)
		{
			do
			{
				if (2 < i)
					break;
				mSlotButtons[i]->Resize(mX + x, mY + 0x91, 0x3c, 0x3c);
				x += 0x5a;
				i++;
			} while (i < mSlotCount);
		}
		i = 3;
		x = 200;
		if (3 < mSlotCount)
		{
			do
			{
				if (5 < i)
					break;
				mSlotButtons[i]->Resize(mX + x, mY + 0xeb, 0x3c, 0x3c);
				x += 0x5a;
				i++;
			} while (i < mSlotCount);
		}
		i = 6;
		x = 200;
		if (6 < mSlotCount)
		{
			do
			{
				mSlotButtons[i]->Resize(mX + x, mY + 0x145, 0x3c, 0x3c);
				x += 0x5a;
				i++;
			} while (i < mSlotCount);
		}
		mOkButton->mButtonCell = mApp->mPicPlayButton;
		mOkButton->mOverCell = mApp->mPicPlayButtonOver;
		mOkButton->mDownCell = mApp->mPicPlayButtonOver;
	}
}

// 0x425790: the page's texts and pictures, then the welcome sign and the large Inchy
void IntroScreen::Draw(Graphics* g)
{
	SetDrawGraphics(g);
	mApp->mPicIntroscreen->Draw(0, 0);
	g->SetFont(mApp->mWhiteFontSmall);
	CellButton* aButton = NULL;
	int aChoice;
	switch (mPage)
	{
	case 0:
		g->SetColor(Color(0, 0, 0, 0xff));
		g->DrawString(mApp->GetString("INTRO_SCREEN_1"), 0x2f, 0xca);
		g->DrawString(mApp->GetString("INTRO_SCREEN_2"), 0x2f, 0xd9);
		g->DrawString(mApp->GetString("INTRO_SCREEN_3"), 0x2f, 0xe8);
		g->DrawString(mApp->GetString("INTRO_SCREEN_4"), 0x2f, 0xf7);
		g->DrawString(mApp->GetString("INTRO_SCREEN_5"), 0x2f, 0x115);
		g->DrawString(mApp->GetString("INTRO_SCREEN_6"), 0x2f, 0x124);
		g->DrawString(mApp->GetString("INTRO_SCREEN_7"), 0x2f, 0x133);
		g->DrawString(mApp->GetString("INTRO_SCREEN_8"), 0x2f, 0x142);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(mApp->GetString("INTRO_SCREEN_1"), 0x2d, 200);
		g->DrawString(mApp->GetString("INTRO_SCREEN_2"), 0x2d, 0xd7);
		g->DrawString(mApp->GetString("INTRO_SCREEN_3"), 0x2d, 0xe6);
		g->DrawString(mApp->GetString("INTRO_SCREEN_4"), 0x2d, 0xf5);
		g->DrawString(mApp->GetString("INTRO_SCREEN_5"), 0x2d, 0x113);
		g->DrawString(mApp->GetString("INTRO_SCREEN_6"), 0x2d, 0x122);
		g->DrawString(mApp->GetString("INTRO_SCREEN_7"), 0x2d, 0x131);
		g->DrawString(mApp->GetString("INTRO_SCREEN_8"), 0x2d, 0x140);
		g->SetColor(Color(0xad, 0x73, 0, 0x80));
		aChoice = mStartChoice;
		if (aChoice == 0)
		{
			aButton = mStartButtons[0];
		}
		if (aChoice == 1)
		{
			aButton = mStartButtons[1];
		}
		if (aChoice == 2)
		{
			aButton = mStartButtons[2];
		}
		g->FillRect(aButton->mX - 0x9b, aButton->mY - 5, aButton->mWidth + 10, aButton->mHeight + 10);
		g->FillRect(aButton->mX - 0x96, aButton->mY, aButton->mWidth, 0x1b);
		g->SetColorizeImages(true);
		g->SetColor(Color::Black);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_NEW_GAME").c_str(), mStartButtons[0]->mX - 0x4c, mStartButtons[0]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("CONTINUE_GAME").c_str(), mStartButtons[1]->mX - 0x4c, mStartButtons[1]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("TUTORIAL").c_str(), mStartButtons[2]->mX - 0x4c, mStartButtons[2]->mY + 4);
		g->SetColorizeImages(false);
		mApp->mPicNewGameIcon->Draw(mStartButtons[0]->mX - 0x94, mStartButtons[0]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_NEW_GAME").c_str(), mStartButtons[0]->mX - 0x4e, mStartButtons[0]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("NEW_GAME_BUTTON1").c_str(), mStartButtons[0]->mX - 0x4e, mStartButtons[0]->mY + 0x1c);
		mApp->mPicWizardgraphics_c8->Draw(mStartButtons[1]->mX - 0x94, mStartButtons[1]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("CONTINUE_GAME").c_str(), mStartButtons[1]->mX - 0x4e, mStartButtons[1]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("NEW_GAME_BUTTON1").c_str(), mStartButtons[1]->mX - 0x4e, mStartButtons[1]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("NEW_GAME_BUTTON2").c_str(), mStartButtons[1]->mX - 0x4e, mStartButtons[1]->mY + 0x25);
		mApp->mPicWizardgraphics_c7->Draw(mStartButtons[2]->mX - 0x94, mStartButtons[2]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("TUTORIAL").c_str(), mStartButtons[2]->mX - 0x4e, mStartButtons[2]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("CLICK_HERE_TO_LEARN_TO_PLAY_DYNOMITE").c_str(), mStartButtons[2]->mX - 0x4e, mStartButtons[2]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("THE_TUTORIAL_WILL_WALK_YOU_THROUGH").c_str(), mStartButtons[2]->mX - 0x4e, mStartButtons[2]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("DYNOMITE_BASICS_AND_GIVE_YOU_A_BRIEF").c_str(), mStartButtons[2]->mX - 0x4e, mStartButtons[2]->mY + 0x2e);
		mApp->mTinyFont->Draw(mApp->GetString("EXPLANATION_OF_ALL_THE_PUZZLE_VARIETIES").c_str(), mStartButtons[2]->mX - 0x4e, mStartButtons[2]->mY + 0x37);
		break;
	case 1:
		g->SetColor(Color(0, 0, 0, 0xff));
		g->DrawString(mApp->GetString("SELECT_GAME1"), 0x2f, 0xca);
		g->DrawString(mApp->GetString("SELECT_GAME2"), 0x2f, 0xd9);
		g->DrawString(mApp->GetString("SELECT_GAME3"), 0x2f, 0xf7);
		g->DrawString(mApp->GetString("SELECT_GAME4"), 0x2f, 0x106);
		g->DrawString(mApp->GetString("SELECT_GAME5"), 0x2f, 0x115);
		g->DrawString(mApp->GetString("SELECT_GAME6"), 0x2f, 0x124);
		g->DrawString(mApp->GetString("SELECT_GAME7"), 0x2f, 0x133);
		g->DrawString(mApp->GetString("SELECT_GAME8"), 0x2f, 0x142);
		g->DrawString(mApp->GetString("SELECT_GAME9"), 0x2f, 0x151);
		g->DrawString(mApp->GetString("SELECT_GAME10"), 0x2f, 0x160);
		g->DrawString(mApp->GetString("SELECT_GAME11"), 0x2f, 0x16f);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(mApp->GetString("SELECT_GAME1"), 0x2d, 200);
		g->DrawString(mApp->GetString("SELECT_GAME2"), 0x2d, 0xd7);
		g->DrawString(mApp->GetString("SELECT_GAME3"), 0x2d, 0xf5);
		g->DrawString(mApp->GetString("SELECT_GAME4"), 0x2d, 0x104);
		g->DrawString(mApp->GetString("SELECT_GAME5"), 0x2d, 0x113);
		g->DrawString(mApp->GetString("SELECT_GAME6"), 0x2d, 0x122);
		g->DrawString(mApp->GetString("SELECT_GAME7"), 0x2d, 0x131);
		g->DrawString(mApp->GetString("SELECT_GAME8"), 0x2d, 0x140);
		g->DrawString(mApp->GetString("SELECT_GAME9"), 0x2d, 0x14f);
		g->DrawString(mApp->GetString("SELECT_GAME10"), 0x2d, 0x15e);
		g->DrawString(mApp->GetString("SELECT_GAME11"), 0x2d, 0x16d);
		if (mApp->mGameMode == 0)
		{
			mApp->mGameMode = 1;
		}
		aChoice = mApp->mGameMode;
		if (aChoice == 1)
		{
			aButton = mModeButtons[0];
		}
		if (aChoice == 2)
		{
			aButton = mModeButtons[1];
		}
		if (aChoice == 3)
		{
			aButton = mModeButtons[2];
		}
		if (aChoice == 4)
		{
			aButton = mModeButtons[3];
		}
		g->SetColor(Color(0xad, 0x73, 0, 0x80));
		g->FillRect(aButton->mX - 0x9b, aButton->mY - 5, aButton->mWidth + 10, aButton->mHeight + 10);
		g->FillRect(aButton->mX - 0x96, aButton->mY, aButton->mWidth, 0x1b);
		g->SetColorizeImages(true);
		g->SetColor(Color::Black);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_STOMPED_PUZZLE").c_str(), mModeButtons[0]->mX - 0x4c, mModeButtons[0]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_ENDLESS_PUZZLE").c_str(), mModeButtons[1]->mX - 0x4c, mModeButtons[1]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_FOSSIL_CHALLENGE").c_str(), mModeButtons[2]->mX - 0x4c, mModeButtons[2]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_TIME_TRIAL").c_str(), mModeButtons[3]->mX - 0x4c, mModeButtons[3]->mY + 4);
		g->SetColorizeImages(false);
		mApp->mPicWizardgraphics_c0->Draw(mModeButtons[0]->mX - 0x94, mModeButtons[0]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_STOMPED_PUZZLE").c_str(), mModeButtons[0]->mX - 0x4e, mModeButtons[0]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("STOMPED_DESC1").c_str(), mModeButtons[0]->mX - 0x4e, mModeButtons[0]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("STOMPED_DESC2").c_str(), mModeButtons[0]->mX - 0x4e, mModeButtons[0]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("STOMPED_DESC3").c_str(), mModeButtons[0]->mX - 0x4e, mModeButtons[0]->mY + 0x2e);
		mApp->mPicWizardgraphics_c1->Draw(mModeButtons[1]->mX - 0x94, mModeButtons[1]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_ENDLESS_PUZZLE").c_str(), mModeButtons[1]->mX - 0x4e, mModeButtons[1]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("ENDLESS_DESC1").c_str(), mModeButtons[1]->mX - 0x4e, mModeButtons[1]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("ENDLESS_DESC2").c_str(), mModeButtons[1]->mX - 0x4e, mModeButtons[1]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("ENDLESS_DESC3").c_str(), mModeButtons[1]->mX - 0x4e, mModeButtons[1]->mY + 0x2e);
		mApp->mPicWizardgraphics_c2->Draw(mModeButtons[2]->mX - 0x94, mModeButtons[2]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_FOSSIL_CHALLENGE").c_str(), mModeButtons[2]->mX - 0x4e, mModeButtons[2]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("FOSSIL_DESC1").c_str(), mModeButtons[2]->mX - 0x4e, mModeButtons[2]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("FOSSIL_DESC2").c_str(), mModeButtons[2]->mX - 0x4e, mModeButtons[2]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("FOSSIL_DESC3").c_str(), mModeButtons[2]->mX - 0x4e, mModeButtons[2]->mY + 0x2e);
		mApp->mPicWizardgraphics_c3->Draw(mModeButtons[3]->mX - 0x94, mModeButtons[3]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("INTROSCREEN_TIME_TRIAL").c_str(), mModeButtons[3]->mX - 0x4e, mModeButtons[3]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("TIME_TRIAL_DESC1").c_str(), mModeButtons[3]->mX - 0x4e, mModeButtons[3]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("TIME_TRIAL_DESC2").c_str(), mModeButtons[3]->mX - 0x4e, mModeButtons[3]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("TIME_TRIAL_DESC3").c_str(), mModeButtons[3]->mX - 0x4e, mModeButtons[3]->mY + 0x2e);
		break;
	case 2:
		g->SetColor(Color(0, 0, 0, 0xff));
		g->DrawString(mApp->GetString("SKILL_SELECT1"), 0x2f, 0xca);
		g->DrawString(mApp->GetString("SKILL_SELECT2"), 0x2f, 0xd9);
		g->DrawString(mApp->GetString("SKILL_SELECT3"), 0x2f, 0xe8);
		g->DrawString(mApp->GetString("SKILL_SELECT4"), 0x2f, 0x106);
		g->DrawString(mApp->GetString("SKILL_SELECT5"), 0x2f, 0x115);
		g->DrawString(mApp->GetString("SKILL_SELECT6"), 0x2f, 0x124);
		g->DrawString(mApp->GetString("SKILL_SELECT7"), 0x2f, 0x133);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(mApp->GetString("SKILL_SELECT1"), 0x2d, 200);
		g->DrawString(mApp->GetString("SKILL_SELECT2"), 0x2d, 0xd7);
		g->DrawString(mApp->GetString("SKILL_SELECT3"), 0x2d, 0xe6);
		g->DrawString(mApp->GetString("SKILL_SELECT4"), 0x2d, 0x104);
		g->DrawString(mApp->GetString("SKILL_SELECT5"), 0x2d, 0x113);
		g->DrawString(mApp->GetString("SKILL_SELECT6"), 0x2d, 0x122);
		g->DrawString(mApp->GetString("SKILL_SELECT7"), 0x2d, 0x131);
		aChoice = mApp->mSkill;
		if (aChoice == 1)
			aButton = mSkillButtons[0];
		else if (aChoice == 2)
			aButton = mSkillButtons[1];
		else if (aChoice == 3)
			aButton = mSkillButtons[2];
		else
		{
			mApp->mSkill = 1;
			aButton = mSkillButtons[0];
		}
		g->SetColor(Color(0xad, 0x73, 0, 0x80));
		g->FillRect(aButton->mX - 0x9b, aButton->mY - 5, aButton->mWidth + 10, aButton->mHeight + 10);
		g->FillRect(aButton->mX - 0x96, aButton->mY, aButton->mWidth, 0x1b);
		g->SetColorizeImages(true);
		g->SetColor(Color::Black);
		mApp->mWizardFont->Draw(mApp->GetString("NORMAL").c_str(), mSkillButtons[0]->mX - 0x4c, mSkillButtons[0]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("HARD").c_str(), mSkillButtons[1]->mX - 0x4c, mSkillButtons[1]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("EXPERT").c_str(), mSkillButtons[2]->mX - 0x4c, mSkillButtons[2]->mY + 4);
		g->SetColorizeImages(false);
		if ((mApp->mPuzzleSkills & 1) == 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(100, 100, 100, 0xff));
		}
		mApp->mPicWizardgraphics_c4->Draw(mSkillButtons[0]->mX - 0x94, mSkillButtons[0]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("NORMAL").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("EASY_SKILL_DESC1").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("EASY_SKILL_DESC2").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("EASY_SKILL_DESC3").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 0x2e);
		g->SetColorizeImages(false);
		if ((mApp->mPuzzleSkills & 2) == 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(100, 100, 100, 0xff));
		}
		mApp->mPicWizardgraphics_c5->Draw(mSkillButtons[1]->mX - 0x94, mSkillButtons[1]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("HARD").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("NORMAL_SKILL_DESC1").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("NORMAL_SKILL_DESC2").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("NORMAL_SKILL_DESC3").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 0x2e);
		g->SetColorizeImages(false);
		if ((mApp->mPuzzleSkills & 4) == 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(100, 100, 100, 0xff));
		}
		mApp->mPicWizardgraphics_c6->Draw(mSkillButtons[2]->mX - 0x94, mSkillButtons[2]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("EXPERT").c_str(), mSkillButtons[2]->mX - 0x4e, mSkillButtons[2]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("EXPERT_SKILL_DESC1").c_str(), mSkillButtons[2]->mX - 0x4e, mSkillButtons[2]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("EXPERT_SKILL_DESC2").c_str(), mSkillButtons[2]->mX - 0x4e, mSkillButtons[2]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("EXPERT_SKILL_DESC3").c_str(), mSkillButtons[2]->mX - 0x4e, mSkillButtons[2]->mY + 0x2e);
		g->SetColorizeImages(false);
		break;
	case 3:
	{
			g->SetColor(Color(0, 0, 0, 0x80));
			g->FillRect(0xb9, 0x82, 0x10e, 0x10e);
			mApp->mPicPlayButton->Draw(mX + 0x8b, mY + 0x1a1);
			g->FillRect(mX + 0x8f, mY + 0x1a5, 0x5e, 0x18);
			if (mSlotCount == 0)
			{
				std::string aWidthText = mApp->GetString("NO_GAMES");
				std::string aText = mApp->GetString("NO_GAMES");
				PicFont* aFont = mApp->mAnnounceFont1928;
				int y = 0x109 - aFont->mHeight / 2;
				int aWidth = aFont->Width(aWidthText.c_str());
				mApp->mAnnounceFont1928->Draw(aText.c_str(), 0x140 - aWidth / 2, y);
			}
			int i = 0;
			if (0 < mSlotCount)
			{
				do
				{
					if (mChosenSlot == i)
						g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
					else
						g->SetColor(Color(0x80, 0x80, 100, 0x80));
					CellButton* aSlot = mSlotButtons[i];
					int x = aSlot->mX - mX;
					int y = aSlot->mY - mY;
					g->FillRect(x - 2, y - 2, aSlot->mWidth + 4, aSlot->mHeight + 4);
					int aTop = y - 10;
					int aWidth = mApp->mTinyFont->Width(mSlotModes[i]);
					mApp->mTinyFont->Draw(mSlotModes[i], (x - aWidth / 2) + 0x1e, aTop);
					y = y + 0x3f;
					aWidth = mApp->mTinyFont->Width(mSlotInfos[i]);
					mApp->mTinyFont->Draw(mSlotInfos[i], (x - aWidth / 2) + 0x1e, y);
					i = i + 1;
				} while (i < mSlotCount);
			}
			g->SetColor(Color(0, 0, 0, 0xff));
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT1"), 0x2f, 0xca);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT2"), 0x2f, 0xd9);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT3"), 0x2f, 0xf7);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT4"), 0x2f, 0x106);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT5"), 0x2f, 0x115);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT6"), 0x2f, 0x124);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT7"), 0x2f, 0x133);
			g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT1"), 0x2d, 200);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT2"), 0x2d, 0xd7);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT3"), 0x2d, 0xf5);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT4"), 0x2d, 0x104);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT5"), 0x2d, 0x113);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT6"), 0x2d, 0x122);
			g->DrawString(mApp->GetString("SAVED_GAME_SELECT7"), 0x2d, 0x131);
		break;
	}
	case 4:
		break;
	case 5:
		g->SetColor(Color(0, 0, 0, 0x80));
		mApp->mPicNextButton->Draw(mX + 0x8b, mY + 0x1a1);
		g->FillRect(mX + 0x8f, mY + 0x1a5, 0x5e, 0x18);
		g->SetColor(Color(0, 0, 0, 0xff));
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG1"), 0x2f, 0xca);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG2"), 0x2f, 0xd9);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG3"), 0x2f, 0xe8);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG4"), 0x2f, 0x106);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG5"), 0x2f, 0x115);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG6"), 0x2f, 0x124);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG7"), 0x2f, 0x133);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG1"), 0x2d, 200);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG2"), 0x2d, 0xd7);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG3"), 0x2d, 0xe6);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG4"), 0x2d, 0x104);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG5"), 0x2d, 0x113);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG6"), 0x2d, 0x122);
		g->DrawString(mApp->GetString("MULT_STOMPED_PUZZLE_DIALOG7"), 0x2d, 0x131);
		break;
	case 6:
		g->SetColor(Color(0, 0, 0, 0xff));
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG1"), 0x2f, 0xca);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG2"), 0x2f, 0xd9);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG3"), 0x2f, 0xe8);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG4"), 0x2f, 0x106);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG5"), 0x2f, 0x115);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG6"), 0x2f, 0x124);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG7"), 0x2f, 0x133);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG1"), 0x2d, 200);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG2"), 0x2d, 0xd7);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG3"), 0x2d, 0xe6);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG4"), 0x2d, 0x104);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG5"), 0x2d, 0x113);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG6"), 0x2d, 0x122);
		g->DrawString(mApp->GetString("GAME_TYPE_DIALOG7"), 0x2d, 0x131);
		aChoice = mApp->mFossilKind;
		if (aChoice == 0)
		{
			aButton = mSkillButtons[0];
		}
		if (aChoice == 1)
		{
			aButton = mSkillButtons[1];
		}
		g->SetColor(Color(0xad, 0x73, 0, 0x80));
		g->FillRect(aButton->mX - 0x9b, aButton->mY - 5, aButton->mWidth + 10, aButton->mHeight + 10);
		g->FillRect(aButton->mX - 0x96, aButton->mY, aButton->mWidth, 0x1b);
		g->SetColorizeImages(true);
		g->SetColor(Color::Black);
		mApp->mWizardFont->Draw(mApp->GetString("STRATEGIC").c_str(), mSkillButtons[0]->mX - 0x4c, mSkillButtons[0]->mY + 4);
		mApp->mWizardFont->Draw(mApp->GetString("PANIC").c_str(), mSkillButtons[1]->mX - 0x4c, mSkillButtons[1]->mY + 4);
		g->SetColorizeImages(false);
		if ((mApp->mPuzzleSkills & 1) == 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(100, 100, 100, 0xff));
		}
		mApp->mPicWizardgraphics_c3->Draw(mSkillButtons[0]->mX - 0x94, mSkillButtons[0]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("STRATEGIC").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("STRATEGIC_MODE_DESC1").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("STRATEGIC_MODE_DESC2").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("STRATEGIC_MODE_DESC3").c_str(), mSkillButtons[0]->mX - 0x4e, mSkillButtons[0]->mY + 0x2e);
		g->SetColorizeImages(false);
		if ((mApp->mPuzzleSkills & 2) == 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(100, 100, 100, 0xff));
		}
		mApp->mPicPanicIcon->Draw(mSkillButtons[1]->mX - 0x94, mSkillButtons[1]->mY + 2);
		mApp->mWizardFont->Draw(mApp->GetString("PANIC").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 2);
		mApp->mTinyFont->Draw(mApp->GetString("PANIC_MODE_DESC1").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 0x1c);
		mApp->mTinyFont->Draw(mApp->GetString("PANIC_MODE_DESC2").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 0x25);
		mApp->mTinyFont->Draw(mApp->GetString("PANIC_MODE_DESC3").c_str(), mSkillButtons[1]->mX - 0x4e, mSkillButtons[1]->mY + 0x2e);
		g->SetColorizeImages(false);
		break;
	}
	Cell* aWelcome;
	if (gIntroReturning == true)
		aWelcome = mApp->mPicWelcomeSign;
	else
		aWelcome = mApp->mPicStartNewGameSign;
	aWelcome->Draw(0xb4, 0x23);
	mTalker->Draw();
}

// 0x424740: a choice on a page; choosing the chosen one again is the OK button (egglands), a new one clicks
void IntroScreen::ButtonPress(int theId)
{
	int aMode = mApp->mGameMode;
	int aStartChoice = mStartChoice;
	int aSkill = mApp->mSkill;
	int aFossilSkill = mApp->mFossilKind;
	if ((theId == 7) || (theId == 9))
	{
		mApp->mSndClick->Play();
		return;
	}

	bool chosen = false;
	if (theId == 10)
	{
		mStartChoice = 0;
		chosen = true;
	}
	else if (theId == 11)
	{
		mStartChoice = 1;
		chosen = true;
	}
	else if (theId == 12)
	{
		mStartChoice = 2;
		chosen = true;
	}
	if (aStartChoice == mStartChoice)
	{
		if (chosen)
		{
			mApp->mSndEgglands->Play();
			ButtonDepress(7);
		}
	}
	else
		mApp->mSndClick->Play();

	chosen = false;
	if (theId == 0)
	{
		if (mPage == 6)
			mApp->mFossilKind = 0;
		else
			mApp->mSkill = 1;
		chosen = true;
	}
	else if (theId == 1)
	{
		if (mPage == 6)
			mApp->mFossilKind = 1;
		else
			mApp->mSkill = 2;
		chosen = true;
	}
	else if (theId == 2)
	{
		mApp->mSkill = 3;
		chosen = true;
	}
	if (mApp->mSkill == aSkill)
	{
		if (mApp->mFossilKind == aFossilSkill)
		{
			if (chosen)
			{
				mApp->mSndEgglands->Play();
				ButtonDepress(7);
			}
		}
		else
			mApp->mSndClick->Play();
	}
	else
		mApp->mSndClick->Play();

	chosen = false;
	if (theId == 3)
	{
		mApp->mGameMode = 1;
		chosen = true;
	}
	else if (theId == 4)
	{
		mApp->mGameMode = 2;
		chosen = true;
	}
	else if (theId == 5)
	{
		mApp->mGameMode = 3;
		chosen = true;
	}
	else if (theId == 6)
	{
		mApp->mGameMode = 4;
		chosen = true;
	}
	if (mApp->mGameMode == aMode)
	{
		if (!chosen)
			return;
		mApp->mSndEgglands->Play();
		ButtonDepress(7);
		return;
	}
	mOkButton->mButtonCell = mApp->mPicNextButton;
	mOkButton->mOverCell = mApp->mPicNextButtonOver;
	mOkButton->mDownCell = mApp->mPicNextButtonOver;
	mOkButton->MarkDirty();
	mApp->mSndClick->Play();
}

// 0x42af50: OK (7, or 8 when the puzzle list has one pack) goes on to the next page or starts; Back (9) goes back
void IntroScreen::ButtonDepress(int theId)
{
	if ((theId == 7) || (theId == 8))
	{
		switch (mPage)
		{
		case 0:
			if (mStartChoice == 0)
			{
				mPage = 1;
				break;
			}
			if (mStartChoice != 1)
			{
				mApp->mInTutorial = true;
				mApp->mSndEgglands->Play();
				mApp->StartTutorial();
				mKeepMusic = true;
				goto back;
			}
			mPage = 3;
			ReadSlots();
			break;
		case 1:
			mApp->mPuzzleSkills = -1;
			if (mApp->mGameMode == 3)
				mPage = 6;
			else
			{
				if (mApp->mGameMode != 1)
				{
					mPage = 2;
					break;
				}
				mPage = 5;
				if (mPuzzleList->mPacks.mCount == 1)
					mApp->mIntroScreen->ButtonDepress(8);
			}
			break;
		case 2:
		case 6:
			mApp->mInTutorial = false;
			mApp->mSndEgglands->Play();
			mApp->StartGame();
			mApp->mMusicInterface->PlayMusic(2, 0x29, false);
			mKeepMusic = true;
			mApp->WarpToBoard();
			goto back;
		case 3:
			LoadChosenSlot();
			goto back;
		case 5:
		{
			if (mApp->mGameMode == 1)
				mApp->mPuzzleSkills = mPuzzleList->mPackSkills;
			unsigned int aSkills = mApp->mPuzzleSkills;
			bool allowed[4] = {false, false, false, false};
			if ((aSkills & 1) != 0)
				allowed[1] = true;
			if ((aSkills & 2) != 0)
				allowed[2] = true;
			if ((aSkills & 4) != 0)
				allowed[3] = true;
			while (!allowed[mApp->mSkill])
			{
				mApp->mSkill++;
				if (3 < mApp->mSkill)
					mApp->mSkill = 1;
			}
			mPage = 2;
			break;
		}
		default:
			goto back;
		}
		Resize(mX, mY, mWidth, mHeight);
	}
back:
	if (theId != 9)
		return;
	FreeSlots();
	switch (mPage)
	{
	case 1:
	case 3:
		mPage = 0;
		break;
	case 2:
		if ((mApp->mGameMode == 1) && (1 < mPuzzleList->mPacks.mCount))
		{
			mPage = 5;
			break;
		}
	case 5:
	case 6:
		mPage = 1;
	}
	Resize(mX, mY, mWidth, mHeight);
}

// 0x424970
void IntroScreen::FreeSlots()
{
	if ((mWidgetManager != NULL) && (0 < mSlotCount))
	{
		int i = 0;
		do
		{
			if (mSlotButtons[i] != NULL)
			{
				mWidgetManager->RemoveWidget(mSlotButtons[i]);
				mSlotButtons[i]->mButtonCell = NULL;
				mSlotButtons[i]->mOverCell = NULL;
				mSlotButtons[i]->mDownCell = NULL;
			}
			i++;
		} while (i < mSlotCount);
	}
	int i = 0;
	if (0 < mSlotCount)
	{
		do
		{
			delete mSlotCells[i];
			mSlotCells[i] = NULL;
			delete mSlotPics[i];
			mSlotPics[i] = NULL;
			if (mSlotButtons[i] != NULL)
			{
				mApp->SafeDeleteWidget(mSlotButtons[i]);
				mSlotButtons[i] = NULL;
			}
			i++;
		} while (i < mSlotCount);
	}
	mSlotCount = 0;
}

// 0x424a20: SLOT0.SAV-SLOT7.SAV: the mode's name (20 bytes), 20 more, and a 60 x 60 picture
void IntroScreen::ReadSlots()
{
	FreeSlots();
	mChosenSlot = -1;
	unsigned int* aBits = (unsigned int*) malloc(0x3840);
	int aSlot = 0;
	do
	{
		char aName[20];
		sprintf(aName, "SLOT%d.SAV", aSlot);
		// port: the saved games are in the user folder (the original's are beside the exe)
		std::string aPath = Platform::VFS::Get().GetUserDir() + "/" + aName;
		FILE* aFile = fopen(aPath.c_str(), "rb");
		if (aFile != NULL)
		{
			fread(mSlotModes[mSlotCount], 1, 0x14, aFile);
			fread(mSlotInfos[mSlotCount], 1, 0x14, aFile);
			fread(aBits, 1, 0x3840, aFile);
			fclose(aFile);
			mSlotPics[mSlotCount] = new Pic(mApp, aBits, 0x3c, 0x3c);
			mSlotCells[mSlotCount] = new Cell(mSlotPics[mSlotCount], 0x3c, 0x3c, 0);
			mSlotButtons[mSlotCount] = new CellButton(mSlotCount + 900, mApp);
			mSlotButtons[mSlotCount]->mBtnNoDraw = false;
			mSlotButtons[mSlotCount]->mFrameNoDraw = true;
			mSlotButtons[mSlotCount]->mDoFinger = true;
			mSlotButtons[mSlotCount]->SetVisible(true);
			mSlotButtons[mSlotCount]->mButtonCell = mSlotCells[mSlotCount];
			mSlotButtons[mSlotCount]->mOverCell = mSlotCells[mSlotCount];
			mSlotButtons[mSlotCount]->mDownCell = mSlotCells[mSlotCount];
			mSlotNumbers[mSlotCount] = aSlot;
			mSlotCount++;
		}
		aSlot++;
	} while (aSlot < 8);
	free(aBits);
	int i = 0;
	if (0 < mSlotCount)
	{
		do
		{
			if (mSlotButtons[i] != NULL)
				mWidgetManager->AddWidget(mSlotButtons[i]);
			i++;
		} while (i < mSlotCount);
	}
}

// 0x424cc0
void IntroScreen::PlaceOkButton()
{
	if (mPage == 3)
	{
		if (mChosenSlot == -1)
		{
			mOkButton->Resize(mX - 1000, mY - 1000, mOkButton->mWidth, mOkButton->mHeight);
			return;
		}
		mOkButton->Resize(mX + 0x121, mY + 0x1a1, 0x69, 0x23);
	}
}

// 0x424d10: the saved game's mode from its name, then the app loads it
void IntroScreen::LoadChosenSlot()
{
	const char* aMode = mSlotModes[mChosenSlot];
	if (strncmp(aMode, "Stomped Puzzle", 0xf) == 0)
	{
		mApp->mSkill = 1;
		mApp->mGameMode = 1;
	}
	if (strncmp(aMode, "Endless Puzzle", 0xf) == 0)
	{
		mApp->mSkill = 1;
		mApp->mGameMode = 2;
	}
	if (strncmp(aMode, "Fossil Puzzle", 0xe) == 0)
	{
		mApp->mSkill = 1;
		mApp->mGameMode = 3;
	}
	if (strncmp(aMode, "Time Trial", 0xb) == 0)
	{
		mApp->mSkill = 1;
		mApp->mGameMode = 4;
	}
	mApp->mInTutorial = false;
	mApp->mSndEgglands->Play();
	char aName[20];
	sprintf(aName, "SLOT%d.SAV", mSlotNumbers[mChosenSlot]);
	mApp->LoadSavedGame(aName);
	mApp->WarpToBoard();
}
