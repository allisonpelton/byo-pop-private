// The game options box (see Dialogs.h).

#include "Dialogs.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x422f10
OptionsDialog::OptionsDialog(AtomicaApp* theApp) :
	GameDialog(theApp, false),
	mCreditsPage(false),
	mFirstUpdate(true)
{
	mDone = AddButton(0);
	mHighScores = AddButton(3);
	mCredits = AddButton(5);
	mResetHints = AddButton(9);
	mBoxFullscreen = AddButton(6);
	mBoxCursors = AddButton(7);
	mBoxSymbols = AddButton(8);
	mBoxHints = AddButton(10);
	// port: the buttons "more games" and "register" of the original open web pages; the port has no online features
	mSfx = new Slider(mApp->mSliderBar->mImage, mApp->mSlider->mImage, 0, this);
	mSfx->SetValue(mApp->GetSfxVolume());
	mOthers.Add(mSfx);
	mMusic = new Slider(mApp->mSliderBar->mImage, mApp->mSlider->mImage, 1, this);
	mMusic->SetValue(mApp->GetMusicVolume());
	mOthers.Add(mMusic);
	mFullscreen = !mApp->mIsWindowed;
	mCursors = mApp->mCustomCursorsEnabled;
	mSymbols = mApp->mShowSymbols;
	mNoHints = !mApp->mShowTips;
	mApp->mPauseCount++;
}

// 0x4229d0: the choices take effect when the box goes
OptionsDialog::~OptionsDialog()
{
	mApp->mOptionsDialog = NULL;
	if (mApp->mIsWindowed == mFullscreen)
		mApp->SwitchScreenMode(!mFullscreen);
	mApp->mPauseCount--;
	if (mApp->mPauseCount < 0)
		mApp->mPauseCount = 0;
	mApp->mShowSymbols = mSymbols;
	mApp->mShowTips = !mNoHints;
	mApp->EnableCustomCursors(mCursors);
}

// 0x422ac0
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	mDone->Resize(mX + 0x91, mY + 0x15f, 0x73, 0x19);
	// the original's two buttons that open web pages (register, more games) are out; one stood left of this button (0x422ac0), the other in a
	// row of its own under it: this button now fills its row, as wide as both columns
	mHighScores->Resize(mX + 0x55, mY + 0x125, 0xf0, 0x19);
	mCredits->Resize(mX + 0x55, mY + 0x108, 0x76, 0x19);
	mResetHints->Resize(mX + 0xcf, mY + 0x108, 0x76, 0x19);
	mMusic->Resize(mX + 0x84, mY + 0xbe, 0xe1, 0xd);
	mSfx->Resize(mX + 0x84, mY + 0xaa, 0xe1, 0xd);
	mBoxFullscreen->Resize(mX + 0x41, mY + 0xd2, 0x14, 0x14);
	mBoxCursors->Resize(mX + 0xd2, mY + 0xd2, 0x14, 0x14);
	mBoxSymbols->Resize(mX + 0x41, mY + 0xeb, 0x14, 0x14);
	mBoxHints->Resize(mX + 0xd2, mY + 0xeb, 0x14, 0x14);
}

// 0x4234a0
void OptionsDialog::Update()
{
	GameDialog::Update();
	if ((mBoxY == 0) && mFirstUpdate)
	{
		// the box is in place: its widgets come in front of the others
		mManager->BringToFront(this);
		mButtons.Reset();
		Widget* aWidget = mButtons.Next();
		while (aWidget != NULL)
		{
			mManager->BringToFront(aWidget);
			aWidget = mButtons.Next();
		}
		mManager->BringToFront(mMusic);
		mManager->BringToFront(mSfx);
		mFirstUpdate = false;
	}
}

// 0x424430 (the buttons) and 0x422c70 (the check boxes)
void OptionsDialog::ButtonDepress(int theId)
{
	if ((theId == 6) || (theId == 7) || (theId == 8) || (theId == 10))
	{
		mApp->mSndCapClick->Play();
		if (theId == 6)
			mFullscreen = !mFullscreen;
		else if (theId == 7)
			mCursors = !mCursors;
		else if (theId == 8)
			mSymbols = !mSymbols;
		else
			mNoHints = !mNoHints;
		return;
	}
	if (theId == 0)
	{
		if (mCreditsPage)
		{
			mHighScores->SetVisible(true);
			mCredits->SetVisible(true);
			mResetHints->SetVisible(true);
			mBoxFullscreen->SetVisible(true);
			mBoxCursors->SetVisible(true);
			mBoxSymbols->SetVisible(true);
			mBoxHints->SetVisible(true);
			mMusic->SetVisible(true);
			mSfx->SetVisible(true);
			mCreditsPage = false;
			return;
		}
		mClosing = true;
	}
	else if (theId == 5)
	{
		mHighScores->SetVisible(false);
		mCredits->SetVisible(false);
		mResetHints->SetVisible(false);
		mBoxFullscreen->SetVisible(false);
		mBoxCursors->SetVisible(false);
		mBoxSymbols->SetVisible(false);
		mBoxHints->SetVisible(false);
		mMusic->SetVisible(false);
		mSfx->SetVisible(false);
		mCreditsPage = true;
	}
	else if (theId == 9)
	{
		// the hints that were shown are shown again
		mApp->mSndSelectAtom->Play();
		mApp->ResetHints();
		mResetHints->SetDisabled(true);
	}
	else if (theId == 3)
		mApp->ShowHighScores();
}

// 0x422d00
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetSfxVolume(theVal);
	else if (theId == 1)
		mApp->SetMusicVolume(theVal);
}

// 0x4234f0
void OptionsDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	g->SetFont(mApp->mMonitorFont);
	Font* aMedium = mApp->mMonitorFontMedium;
	if (!mCreditsPage)
	{
		g->SetColor(Color(0, 255, 0, 255));
		const std::string& aTitle = GT(0x4b08b8);
		g->DrawString(aTitle, (mWidth / 2 - mApp->mMonitorFont->StringWidth(aTitle) / 2) - 5, 0x96);
		g->SetFont(aMedium);
		g->DrawString(GT(0x4b08ac), 0x5a, 0xe1);
		g->DrawString(GT(0x4b089c), 0xeb, 0xe1);
		g->DrawString(GT(0x4b088c), 0x5a, 0xfa);
		g->DrawString(GT(0x4b087c), 0xeb, 0xfa);
		g->SetColor(Color(0, 255, 0, 0x40));
		g->FillRect(0x41, 0xd2, 0x14, 0x14);
		g->FillRect(0xd2, 0xd2, 0x14, 0x14);
		g->FillRect(0x41, 0xeb, 0x14, 0x14);
		g->FillRect(0xd2, 0xeb, 0x14, 0x14);
		g->SetColor(Color(0, 255, 0, 255));
		if (mFullscreen)
			g->FillRect(0x46, 0xd7, 10, 10);
		if (mCursors)
			g->FillRect(0xd7, 0xd7, 10, 10);
		if (mSymbols)
			g->FillRect(0x46, 0xf0, 10, 10);
		if (mNoHints)
			g->FillRect(0xd7, 0xf0, 10, 10);
		const std::string& aSfx = GT(0x4b0870);
		g->DrawString(aSfx, 0x82 - aMedium->StringWidth(aSfx), 0xb4);
		const std::string& aMusic = GT(0x4b0868);
		g->DrawString(aMusic, 0x82 - aMedium->StringWidth(aMusic), 200);
		DrawButton(g, mDone, GT(0x4b0860));
		DrawButton(g, mHighScores, GT(0x4b0838));
		DrawButton(g, mCredits, GT(0x4b0824));
		DrawButton(g, mResetHints, GT(0x4b0818));
	}
	else
	{
		g->SetColor(Color(0, 255, 0, 255));
		const std::string& aTitle = GT(0x4b0808);
		g->DrawString(aTitle, (mWidth / 2 - mApp->mMonitorFont->StringWidth(aTitle) / 2) - 5, 0x96);
		Font* aSmall = mApp->mMonitorFontSmall;
		g->SetFont(aSmall);
		int aMiddle = mWidth / 2;
		static const unsigned int kLabels[6] = {0x4b07f8, 0x4b07e8, 0x4b07e0, 0x4b07d8, 0x4b07c8, 0x4b07c0};
		static const int kLabelY[6] = {200, 0xd9, 0xea, 0x119, 0x12a, 0x13b};
		static const unsigned int kNames[8] = {0x4b07b0, 0x4b07a4, 0x4b0794, 0x4b0784, 0x4b07a4, 0x4b0774, 0x4b0768, 0x4b075c};
		static const int kNameY[8] = {200, 0xd9, 0xea, 0xf9, 0x108, 0x119, 0x12a, 0x13b};
		for (int i = 0; i < 6; i++)
		{
			const std::string& aLabel = GT(kLabels[i]);
			g->DrawString(aLabel, aMiddle - 5 - aSmall->StringWidth(aLabel), kLabelY[i]);
		}
		// the names go from green towards yellow: the colour changes before some of them and stays for the others
		static const int kNameRed[8] = {0xaf, 0xb9, 0xc3, 0xc3, 0xc3, 0xcd, 0xd7, 0xe1};
		for (int i = 0; i < 8; i++)
		{
			g->SetColor(Color(kNameRed[i], 255, 0, 255));
			g->DrawString(GT(kNames[i]), aMiddle + 5, kNameY[i]);
		}
		DrawButton(g, mDone, GT(0x4b0754));
	}
}
