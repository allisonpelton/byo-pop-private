// The boxes of Atomica Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstdio>

using namespace Sexy;

// the names of the ranks (the exe's table at 0x4cb0a0 points to these texts) and the scores that give them (0x4cb100)
const unsigned int Sexy::kRankNames[22] = {
	0x4ae610, 0x4ae600, 0x4ae5f0, 0x4ae5e4, 0x4ae5d4, 0x4ae5c8, 0x4ae5bc, 0x4ae5ac, 0x4ae598, 0x4ae584, 0x4ae578,
	0x4ae564, 0x4ae550, 0x4ae53c, 0x4ae52c, 0x4ae518, 0x4ae508, 0x4ae4f8, 0x4ae4f0, 0x4ae4e0, 0x4ae4d4, 0x4ae4c4
};
const int Sexy::kRankScores[23] = {
	0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 10000, 12000, 15000, 20000, 30000, 40000, 60000, 80000, 100000,
	125000, 150000, 300000, 500000, -1
};

// 0x420040: the rank is the highest whose score has been reached
int Sexy::RankOfScore(int theScore)
{
	int aRank = 0;
	while (kRankScores[aRank] != -1)
		aRank++;
	aRank--;
	while ((aRank > 0) && (theScore < kRankScores[aRank]))
		aRank--;
	return aRank;
}

namespace
{
	// a text at the right edge x
	void DrawRight(Graphics* g, Font* theFont, const std::string& theText, int theRightX, int theY)
	{
		g->SetFont(theFont);
		g->DrawString(theText, theRightX - theFont->StringWidth(theText), theY);
	}

	std::string Format(const std::string& theFormat, int theA = 0, int theB = 0)
	{
		char aBuffer[256];
		snprintf(aBuffer, sizeof(aBuffer), theFormat.c_str(), theA, theB);
		return aBuffer;
	}
}

// ---------------------------------------------------------------------------------------------------------
// GameDialog

// 0x416ac0
GameDialog::GameDialog(AtomicaApp* theApp, bool theCentered) :
	mApp(theApp),
	mManager(NULL),
	mBoxX(0xb4),
	mClosing(false),
	mCentered(theCentered)
{
	mBoxY = -mApp->mDialogMonitor->mHeight;
	mBoxEndY = mBoxY;
	if (theCentered)
	{
		mBoxY = -1;
		mBoxEndY = 0;
	}
	Resize(mBoxX, mBoxY, 0, 0);
}

// 0x4167a0
GameDialog::~GameDialog()
{
	mButtons.Reset();
	Widget* aWidget = mButtons.Next();
	while (aWidget != NULL)
	{
		mApp->SafeDeleteWidget(aWidget);
		aWidget = mButtons.Next();
	}
	mButtons.Clear();
	mOthers.Reset();
	aWidget = mOthers.Next();
	while (aWidget != NULL)
	{
		mApp->SafeDeleteWidget(aWidget);
		aWidget = mOthers.Next();
	}
	mOthers.Clear();
	// the focus goes back to the field
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mGrid != NULL) && (mApp->mWidgetManager != NULL))
		mApp->mWidgetManager->SetFocus(mApp->mBoard->mGrid);
}

// 0x416a90: the box is as big as the monitor's picture and stands at mBoxX, mBoxY whatever it is asked
void GameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	(void) theX;
	(void) theY;
	(void) theWidth;
	(void) theHeight;
	Widget::Resize(mBoxX, mBoxY, mApp->mDialogMonitor->mWidth, mApp->mDialogMonitor->mHeight);
}

// 0x416890
void GameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mManager = theWidgetManager;
	theWidgetManager->BringToFront(this);
	theWidgetManager->AddBaseModal(this);
	mButtons.Reset();
	Widget* aWidget = mButtons.Next();
	while (aWidget != NULL)
	{
		theWidgetManager->AddWidget(aWidget);
		theWidgetManager->BringToFront(aWidget);
		aWidget = mButtons.Next();
	}
	mOthers.Reset();
	aWidget = mOthers.Next();
	while (aWidget != NULL)
	{
		theWidgetManager->AddWidget(aWidget);
		theWidgetManager->BringToFront(aWidget);
		aWidget = mOthers.Next();
	}
}

// 0x416940
void GameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveBaseModal(this);
	mButtons.Reset();
	Widget* aWidget = mButtons.Next();
	while (aWidget != NULL)
	{
		theWidgetManager->RemoveWidget(aWidget);
		aWidget = mButtons.Next();
	}
	mOthers.Reset();
	aWidget = mOthers.Next();
	while (aWidget != NULL)
	{
		theWidgetManager->RemoveWidget(aWidget);
		aWidget = mOthers.Next();
	}
}

// 0x4169d0: the box slides down to its place (20 pixels an update) and, closing, up and away (30)
void GameDialog::Update()
{
	Widget::Update();
	if (mClosing)
	{
		mBoxY -= 0x1e;
		Resize(mBoxX, mBoxY, mApp->mDialogMonitor->mWidth, mApp->mDialogMonitor->mHeight);
		if (mBoxY < mBoxEndY)
		{
			mManager->RemoveWidget(this);
			mApp->SafeDeleteWidget(this);
		}
	}
	else if (mBoxY < 0)
	{
		mBoxY += 0x14;
		if (0 < mBoxY)
			mBoxY = 0;
		Resize(mBoxX, mBoxY, mApp->mDialogMonitor->mWidth, mApp->mDialogMonitor->mHeight);
	}
}

// 0x416c00
void GameDialog::Draw(Graphics* g)
{
	mApp->mDialogMonitor->Draw(g, 0, 0);
}

// 0x416c30: a button of the box: a dark field with a green edge, lit when the mouse is over it, brighter pressed
void GameDialog::DrawButton(Graphics* g, Widget* theButton, const std::string& theText)
{
	Graphics aG(*g);
	aG.SetColor(Color(0, 0, 0, 0));
	if (theButton->mIsOver)
		aG.SetColor(Color(0, 0x80, 0, 255));
	if (theButton->mIsDown)
		aG.SetColor(Color(0, 0x4b, 0, 0xaf));
	int aX = theButton->mX - mX;
	int aY = theButton->mY - mY;
	aG.FillRect(aX, aY, theButton->mWidth, theButton->mHeight);
	aG.SetColor(Color(0, 0x32, 0, 255));
	aG.SetColor(Color(0, theButton->mDisabled ? 100 : 255, 0, 255));
	aG.DrawLine(aX, aY, aX + theButton->mWidth, aY);
	aG.DrawLine(aX, aY, aX, aY + theButton->mHeight);
	aG.DrawLine(aX + theButton->mWidth, aY, aX + theButton->mWidth, aY + theButton->mHeight);
	aG.DrawLine(aX, aY + theButton->mHeight, aX + theButton->mWidth, aY + theButton->mHeight);
	aG.SetFont(mApp->mMonitorFontMedium);
	aG.DrawString(theText, aX + (theButton->mWidth - mApp->mMonitorFontMedium->StringWidth(theText)) / 2,
		aY + theButton->mHeight / 2 + 6);
}

// the invisible button of a box: the box draws it
ButtonWidget* GameDialog::AddButton(int theId)
{
	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->mBtnNoDraw = true;
	aButton->mDoFinger = true;
	mButtons.Add(aButton);
	return aButton;
}

void GameDialog::Close()
{
	mClosing = true;
}

// ---------------------------------------------------------------------------------------------------------
// LevelUpDialog

// 0x420040
LevelUpDialog::LevelUpDialog(AtomicaApp* theApp, Board* theBoard) :
	GameDialog(theApp, false),
	mBoard(theBoard)
{
	mContinue = AddButton(0);
	mGrid = mBoard->mGrid;
	mRank = RankOfScore(mBoard->mScore);
}

// 0x420170: closed by its button, the next level begins
LevelUpDialog::~LevelUpDialog()
{
	mApp->mLevelUpDialog = NULL;
	if (mClosing && (mApp->mBoard != NULL))
		mApp->mBoard->NextLevel();
}

// 0x4201f0
void LevelUpDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	mContinue->Resize(mX + 0x5f, mY + 0x159, 0xdc, 0x14);
}

// 0x420240
void LevelUpDialog::ButtonDepress(int theId)
{
	if (theId == 0)
		mClosing = true;
}

// 0x4202b0
void LevelUpDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	g->SetFont(mApp->mMonitorFont);
	g->SetColor(Color(0, 255, 0, 255));
	std::string aTitle = Format(GT(0x4b0534), mBoard->mMeter.mLevel);
	g->DrawString(aTitle, mWidth / 2 - mApp->mMonitorFont->StringWidth(aTitle) / 2 - 5, 0x9b);

	// the button
	g->SetColor(Color(0, 255, 0, 200));
	if (mContinue->mIsOver)
		g->SetColor(Color(0x96, 255, 0, 255));
	if (mContinue->mIsDown)
		g->SetColor(Color(0, 255, 0, 255));
	g->FillRect(mContinue->mX - mX, mContinue->mY - mY, mContinue->mWidth, mContinue->mHeight);
	g->SetFont(mApp->mMonitorFontMedium);
	g->SetColor(Color(0, 0x32, 0, 255));
	const std::string& aNext = GT(0x4b0518);
	g->DrawString(aNext, (mContinue->mWidth / 2 + mContinue->mX - mX) - mApp->mMonitorFontMedium->StringWidth(aNext) / 2,
		mContinue->mY - mY + 0xf);

	// the statistics: each line in green with its number over it in pale green
	Font* aSmall = mApp->mMonitorFontSmall;
	int aTime = mGrid->mStat378;
	struct Line { const char* mFormat; unsigned int mLine; unsigned int mNumber; int mA; int mB; int mY; };
	g->SetFont(aSmall);
	g->SetColor(Color(0, 255, 0, 255));
	DrawRight(g, aSmall, Format(GT(0x4b0504), aTime / 60, aTime % 60), 0xbb, 0xb6);
	g->SetColor(Color(0x9e, 255, 0x9e, 255));
	DrawRight(g, aSmall, Format(GT(0x4afefc), aTime / 60, aTime % 60), 0xbb, 0xb6);
	g->SetColor(Color(0, 255, 0, 255));
	DrawRight(g, aSmall, Format(GT(0x4b04f0), mGrid->mStat37c), 0xbb, 0xc5);
	g->SetColor(Color(0x9e, 255, 0x9e, 255));
	DrawRight(g, aSmall, Format(GT(0x4aff20), mGrid->mStat37c), 0xbb, 0xc5);
	g->SetColor(Color(0, 255, 0, 255));
	DrawRight(g, aSmall, Format(GT(0x4b04dc), mGrid->mStat380), 0xbb, 0xd4);
	g->SetColor(Color(0x9e, 255, 0x9e, 255));
	DrawRight(g, aSmall, Format(GT(0x4aff20), mGrid->mStat380), 0xbb, 0xd4);

	int aCombo = mGrid->mStat384 - 1;
	if (aCombo < 0)
		aCombo = 0;
	struct Pair { unsigned int mLabel; int mValue; const std::string* mFormat; int mY; };
	g->SetColor(Color(0, 255, 0, 255));
	const std::string& aComboLabel = GT(0x4b04cc);
	g->DrawString(aComboLabel, 0xd6, 0xb6);
	g->SetColor(Color(0x9e, 255, 0x9e, 255));
	g->DrawString(Format(GT(0x4afec4), aCombo), 0xd6 + aSmall->StringWidth(aComboLabel), 0xb6);
	g->SetColor(Color(0, 255, 0, 255));
	const std::string& aBonusLabel = GT(0x4b04b8);
	g->DrawString(aBonusLabel, 0xd6, 0xc5);
	g->SetColor(Color(0x9e, 255, 0x9e, 255));
	g->DrawString(Format(GT(0x4aff20), mGrid->mStat388), 0xd6 + aSmall->StringWidth(aBonusLabel), 0xc5);
	g->SetColor(Color(0, 255, 0, 255));
	const std::string& anUpgradeLabel = GT(0x4b04a4);
	g->DrawString(anUpgradeLabel, 0xd6, 0xd4);
	g->SetColor(Color(0x9e, 255, 0x9e, 255));
	g->DrawString(Format(GT(0x4aff20), mGrid->mStat38c), 0xd6 + aSmall->StringWidth(anUpgradeLabel), 0xd4);

	// the rank
	g->SetColor(Color(0, 255, 0, 255));
	DrawRight(g, aSmall, GT(0x4afe94), 0x140, 0x100);
	g->SetColor(Color(200, 255, 0x9e, 255));
	DrawRight(g, aSmall, GT(kRankNames[mRank]), 0x145, 0x115);
	if (kRankScores[mRank + 1] != -1)
	{
		g->SetColor(Color(0, 0xaf, 0, 255));
		DrawRight(g, aSmall, GT(0x4b0494), 0x140, 0x135);
		DrawRight(g, aSmall, Format(GT(0x4b048c), kRankScores[mRank + 1]), 0x140, 0x143);
	}
	mApp->mRank[mRank]->Draw(g, 0x46, 0xe1);
}

// ---------------------------------------------------------------------------------------------------------
// ConfirmDialog

// 0x421f90
ConfirmDialog::ConfirmDialog(AtomicaApp* theApp) :
	GameDialog(theApp, false)
{
	mYes = AddButton(0);
	mNo = AddButton(1);
}

// 0x4220b0
ConfirmDialog::~ConfirmDialog()
{
	mApp->mConfirmDialog = NULL;
}

// 0x422100
void ConfirmDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	mYes->Resize(mX + 0x73, mY + 0x136, 0x4b, 0x1b);
	mNo->Resize(mX + 0xd4, mY + 0x136, 0x4b, 0x1b);
}

// 0x422170: yes gives up the game, no goes back to it
void ConfirmDialog::ButtonDepress(int theId)
{
	if (theId != 1)
	{
		if (theId != 0)
			return;
		mApp->EndGame();
	}
	mClosing = true;
}

// 0x4221d0
void ConfirmDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	g->SetFont(mApp->mMonitorFont);
	g->SetColor(Color(0, 255, 0, 255));
	const std::string& aTitle = GT(0x4b0668);
	g->DrawString(aTitle, (mWidth / 2 - mApp->mMonitorFont->StringWidth(aTitle) / 2) - 10, 0xcd);
	g->SetColor(Color(0, 255, 0, 0xaf));
	const std::string& aLine1 = GT(0x4afb28);
	g->DrawString(aLine1, (mWidth / 2 - mApp->mMonitorFont->StringWidth(aLine1) / 2) - 10, 0xf0);
	const std::string& aLine2 = GT(0x4b064c);
	g->DrawString(aLine2, (mWidth / 2 - mApp->mMonitorFont->StringWidth(aLine2) / 2) - 10,
		mApp->mMonitorFont->GetHeight() + 0xed);
	ButtonWidget* aButtons[2] = {mYes, mNo};
	const std::string* aTexts[2] = {&GT(0x4b0648), NULL};
	std::string aNo = "NO";
	for (int i = 0; i < 2; i++)
	{
		ButtonWidget* aButton = aButtons[i];
		g->SetColor(Color(0, 255, 0, 0x80));
		if (aButton->mIsOver)
			g->SetColor(Color(0, 255, 0, 255));
		if (aButton->mIsDown)
			g->SetColor(Color(0, 255, 0, 0xaf));
		g->FillRect(aButton->mX - mX, aButton->mY - mY, aButton->mWidth, aButton->mHeight);
		g->SetColor(Color(0, 0x32, 0, 255));
		const std::string& aText = (i == 0) ? *aTexts[0] : aNo;
		g->DrawString(aText, ((aButton->mWidth / 2 + aButton->mX) - mApp->mMonitorFont->StringWidth(aText) / 2) - mX,
			(aButton->mY - mY) + 0x14);
	}
}
