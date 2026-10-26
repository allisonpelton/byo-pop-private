#include "SideBar.h"
#include "CellButton.h"
#include "DDImage.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "InchySmall.h"
#include "WidgetManager.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

// 0x435420
SideBar::SideBar(FossilizedApp* theApp)
{
	mApp = theApp;
	mScoreShown = 0;
	mScore = 0;
	mShowTime = false;

	mMenuButton = new CellButton(0, this);
	mMenuButton->mBtnNoDraw = false;
	mMenuButton->mDoFinger = true;
	mMenuButton->SetVisible(true);
	mButton1 = new CellButton(1, this);
	mButton1->mBtnNoDraw = false;
	mButton1->mDoFinger = true;
	mButton1->SetVisible(true);
	mPauseButton = new CellButton(3, this);
	mPauseButton->mBtnNoDraw = false;
	mPauseButton->mDoFinger = true;
	mPauseButton->SetVisible(true);
	mSaveButton = new CellButton(4, this);
	mSaveButton->mBtnNoDraw = false;
	mSaveButton->mDoFinger = true;
	mSaveButton->SetVisible(true);
	mButton5 = new CellButton(5, this);
	mButton5->mBtnNoDraw = false;
	mButton5->mDoFinger = true;
	mButton5->SetVisible(true);
	mButton2 = new CellButton(2, this);
	mButton2->mBtnNoDraw = false;
	mButton2->mDoFinger = true;
	mButton2->SetVisible(true);

	mMenuButton->mButtonCell = mApp->mPicSidebarbuttons_r1344;
	mMenuButton->mOverCell = mApp->mPicSidebarbuttons_r1348;
	mMenuButton->mDownCell = mApp->mPicSidebarbuttons_r1348;
	mButton1->mButtonCell = mApp->mPicSidebarbuttons_r134c;
	mButton1->mOverCell = mApp->mPicSidebarbuttons_r1350;
	mButton1->mDownCell = mApp->mPicSidebarbuttons_r1350;
	mButton2->mButtonCell = mApp->mPicSidebarbuttons_r1354;
	mButton2->mOverCell = mApp->mPicSidebarbuttons_r1358;
	mButton2->mDownCell = mApp->mPicSidebarbuttons_r1358;
	mPauseButton->mButtonCell = mApp->mPauseCells[0];
	mPauseButton->mOverCell = mApp->mPauseCells[1];
	mPauseButton->mDownCell = mApp->mPauseCells[1];
	mSaveButton->mButtonCell = mApp->mPauseCells[4];
	mSaveButton->mOverCell = mApp->mPauseCells[5];
	mSaveButton->mDownCell = mApp->mPauseCells[5];
	mButton5->mButtonCell = mApp->mPicPausebuttons_r1398;
	mButton5->mOverCell = mApp->mPicPausebuttons_r139c;
	mButton5->mDownCell = mApp->mPicPausebuttons_r139c;
	mButton5->SetDisabled(true);
	mButton5->SetVisible(false);
}

// 0x434dc0
SideBar::~SideBar()
{
	if (mPauseButton != NULL)
	{
		mApp->SafeDeleteWidget(mPauseButton);
		mPauseButton = NULL;
	}
	if (mButton1 != NULL)
	{
		mApp->SafeDeleteWidget(mButton1);
		mButton1 = NULL;
	}
	if (mMenuButton != NULL)
	{
		mApp->SafeDeleteWidget(mMenuButton);
		mMenuButton = NULL;
	}
	if (mButton2 != NULL)
	{
		mApp->SafeDeleteWidget(mButton2);
		mButton2 = NULL;
	}
	if (mButton5 != NULL)
	{
		mApp->SafeDeleteWidget(mButton5);
		mButton5 = NULL;
	}
	if (mSaveButton != NULL)
	{
		mApp->SafeDeleteWidget(mSaveButton);
		mSaveButton = NULL;
	}
	delete mInchy;
	mInchy = NULL;
}

// 0x435020
void SideBar::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
	theWidgetManager->AddWidget(mButton1);
	theWidgetManager->AddWidget(mButton2);
	theWidgetManager->AddWidget(mPauseButton);
	theWidgetManager->AddWidget(mSaveButton);
	theWidgetManager->AddWidget(mButton5);
	mInchy = new InchySmall(mApp, this);
	mInchy->CreateBubble();
}

// 0x4350e0
void SideBar::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
	theWidgetManager->RemoveWidget(mButton1);
	theWidgetManager->RemoveWidget(mButton2);
	theWidgetManager->RemoveWidget(mPauseButton);
	theWidgetManager->RemoveWidget(mSaveButton);
	theWidgetManager->RemoveWidget(mButton5);
}

// 0x435140
void SideBar::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMenuButton->Resize(mX + 0x18, mY + 0x94, 0x68, 0x36);
	mButton1->Resize(mX + 0x1f, mY + 0xf3, 0x5b, 0x22);
	mButton2->Resize(mX + 0x1a, mY + 0x19e, 0x61, 0x1f);
	mPauseButton->Resize(mX + 0x62, mY + 0x11d, 0x1a, 0x19);
	mSaveButton->Resize(mX + 0x20, mY + 0x11d, 0x1a, 0x19);
	mButton5->Resize(mX + 0x62, mY + 0x186, 0x19, 0x18);
}

// 0x434ec0: the score counts up one a frame (and then jumps to it)
void SideBar::Update()
{
	if (mScoreShown != mScore)
	{
		mScoreShown++;
		MarkDirty();
	}
	if (mScoreShown != mScore)
		mScoreShown = mScore;
	mInchy->Update();
	Widget::Update();
}

// 0x434f00: the score (or the time) right-aligned at x 124 in the score font
void SideBar::DrawScore()
{
	char aText[100];
	int aWidth = 0;
	if (mShowTime == true)
	{
		int aSeconds = mScoreShown / -100;
		sprintf(aText, "%d:%.2d", aSeconds / 60, aSeconds % 60);
	}
	else
		sprintf(aText, "%d", mScoreShown);
	unsigned int i = 0;
	if (strlen(aText) != 0)
	{
		do
		{
			aWidth = aWidth - 2 + mApp->mScoreDigitCells[aText[i] - '0']->mWidth;
			i++;
		} while (i < strlen(aText));
	}
	int x = 0x7c - aWidth;
	i = 0;
	if (strlen(aText) != 0)
	{
		do
		{
			mApp->mScoreDigitCells[aText[i] - '0']->Draw(x, 0x3e);
			x = x - 2 + mApp->mScoreDigitCells[aText[i] - '0']->mWidth;
			i++;
		} while (i < strlen(aText));
	}
}

// 0x4357d0
void SideBar::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImgSidebar, 0, 0);
	SetDrawGraphics(g);
	SetDrawOffset(Point(0, 0));
	Dynomite* aBoard = mApp->mBoard;
	if (aBoard != NULL)
	{
		int aSkill = aBoard->mDifficulty - 1;
		if (2 < aSkill)
			aSkill = 2;
		mApp->mSkillCells[aSkill]->Draw(0x23, 0xc9);
		mApp->mSkillCells[3 + mApp->mBoard->mGameType]->Draw(0x22, 0xdd);
	}
	mInchy->Draw();
	DrawScore();
}

// 0x435210
void SideBar::ButtonPress(int theId)
{
	mApp->mSndClick->Play();
}

// 0x435920
void SideBar::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mApp->MenuButton();
		return;
	case 1:
		mApp->ShowOptions();
		return;
	case 2:
		mApp->ConfirmExit();
		return;
	case 3:
		if ((mApp->mBoard != NULL) && !mApp->mField1094)
		{
			SetPauseState(0);
			return;
		}
		break;
	case 4:
	{
		Dynomite* aBoard = mApp->mBoard;
		if ((aBoard != NULL) && !mApp->mField1094)
		{
			if (!aBoard->mField131 && !aBoard->mField130)
			{
				mApp->SaveGame();
				if (!mApp->mIsRegistered)
				{
					// port: an unregistered copy shows the register dialog here (SAVE_GAME_DENY_SCREEN2)
					return;
				}
			}
			else
				mApp->mSndWhirleyhit->Play();
		}
		break;
	}
	case 5:
		if (mApp->mBoard != NULL)
		{
			mApp->mSndEgglands->Play();
			mApp->mBoard->Hint();
			return;
		}
	}
}

// 0x435230
void SideBar::DisableGameButtons()
{
	mSaveButton->SetDisabled(true);
	mPauseButton->SetDisabled(true);
	mButton5->SetDisabled(true);
	mButton5->SetVisible(false);
}

// 0x435270
void SideBar::SetPauseState(int theMode)
{
	Dynomite* aBoard = mApp->mBoard;
	if (aBoard == NULL)
	{
		mPauseButton->mButtonCell = mApp->mPauseCells[0];
		mPauseButton->mOverCell = mApp->mPauseCells[0];
		mPauseButton->mDownCell = mApp->mPauseCells[0];
		mSaveButton->mButtonCell = mApp->mPauseCells[4];
		mSaveButton->mOverCell = mApp->mPauseCells[4];
		mSaveButton->mDownCell = mApp->mPauseCells[4];
		mPauseButton->mDoFinger = false;
		mSaveButton->mDoFinger = false;
		MarkDirty();
		return;
	}
	bool aPause;
	if (theMode == 0)
	{
		if (aBoard->mPauseCount < 1)
		{
			if (aBoard->mPauseCount != 0)
				goto buttons;
			aPause = true;
		}
		else
			aPause = false;
	}
	else if (theMode == 1)
		aPause = true;
	else if (theMode == 2)
		aPause = false;
	else
		goto buttons;
	aBoard->SetPaused(aPause);
buttons:
	if (mApp->mBoard->mPauseCount == 0)
	{
		mPauseButton->mButtonCell = mApp->mPauseCells[0];
		mPauseButton->mOverCell = mApp->mPauseCells[1];
		mPauseButton->mDownCell = mApp->mPauseCells[1];
	}
	else
	{
		mPauseButton->mButtonCell = mApp->mPauseCells[2];
		mPauseButton->mOverCell = mApp->mPauseCells[3];
		mPauseButton->mDownCell = mApp->mPauseCells[3];
	}
	mSaveButton->mButtonCell = mApp->mPauseCells[4];
	mSaveButton->mOverCell = mApp->mPauseCells[5];
	mSaveButton->mDownCell = mApp->mPauseCells[5];
	mPauseButton->mDoFinger = true;
	mSaveButton->mDoFinger = true;
	MarkDirty();
	mApp->mBoard->MarkDirty();
}

// 0x435890
void SideBar::GameStarted()
{
	SetPauseState(2);
	mScore = 0;
	mScoreShown = 0;
	mSaveButton->SetDisabled(false);
	mPauseButton->SetDisabled(false);
	mButton5->SetDisabled(true);
	mButton5->SetVisible(false);
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mGameType == 2))
	{
		mButton5->SetDisabled(false);
		mButton5->SetVisible(true);
	}
	MarkDirty();
}
