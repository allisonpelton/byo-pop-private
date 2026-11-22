// EndLevelDialog.cpp: see EndLevelDialog.h.

#include "EndLevelDialog.h"

#include "Board.h"
#include "NotTranslated.h"
#include "ThunderButton.h"
#include "ThunderCommon.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const int END_LEVEL_DIALOG_ID = 0xc;

// 0x4a1a10
EndLevelDialog::EndLevelDialog(Board* theBoard)
	: ThunderDialog(END_LEVEL_DIALOG_ID, false, "", "", "", BUTTONS_OK_CANCEL)
{
	mHangsOnChain = true;
	mBoard = theBoard;
	mApp = theBoard->mApp;
	mButtons[0] = MakeButton(0, this, "");
	mButtons[1] = MakeButton(1, this, "");
	AddWidget(mButtons[0]);
	AddWidget(mButtons[1]);
	mUnknown190 = 0;
	mUnknown1F8 = 0;
	mUnknown254 = false;
	mTipNumber = 0;
}

void EndLevelDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	NOT_TRANSLATED("EndLevelDialog::Resize");
}

void EndLevelDialog::SetVisible(bool isVisible)
{
	NOT_TRANSLATED("EndLevelDialog::SetVisible");
}

int EndLevelDialog::GetPreferredHeight(int theWidth)
{
	NOT_TRANSLATED("EndLevelDialog::GetPreferredHeight");
	return 0;
}

void EndLevelDialog::Update()
{
	NOT_TRANSLATED("EndLevelDialog::Update");
}

void EndLevelDialog::Draw(Graphics* g)
{
	NOT_TRANSLATED_DRAWING("EndLevelDialog::Draw");
}

void EndLevelDialog::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	NOT_TRANSLATED("EndLevelDialog::MouseDown");
}

void EndLevelDialog::MouseMove(int x, int y)
{
	NOT_TRANSLATED("EndLevelDialog::MouseMove");
}

void EndLevelDialog::MouseLeave()
{
	NOT_TRANSLATED("EndLevelDialog::MouseLeave");
}

void EndLevelDialog::ButtonDepress(int theId)
{
	NOT_TRANSLATED("EndLevelDialog::ButtonDepress");
}
