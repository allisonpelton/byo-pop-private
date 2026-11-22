#include "PlayMenu.h"

#include "MainMenu.h"

namespace Sexy {

// 0x680760 (bejeweled3). port: the constructor's modes, balls and buttons are not translated yet
PlayMenu::PlayMenu(MainMenu* theMainMenu) : mMainMenu(theMainMenu)
{
	mClip = false;   // +0x51
	mVisible = false;   // +0x64
	mFadeCurve.SetConstant(0.0);
	mAuxCurveA.SetCurve("b+0,1,0,1,####       8####  i~###");
	mAuxCurveB.SetConstant(0.0);
	for (int i = 0; i < 4; i++)
		mAuxCurves[i].SetConstant(0.0);
}

// 0x696ad0 (bejeweled3). port: from the menu's intro on it shows while the fade is above 0, unless +0x490 is 1; the rest of
// the Update (the modes' balls, the buttons) is not translated yet
void PlayMenu::Update()
{
	Widget::Update();
	if (!mMainMenu->mIntroStarted)
		return;
	double aFade = mFadeCurve.GetOutVal();
	mVisible = !(aFade <= 0.0 || mHideCurve.GetOutVal() == 1.0);
	MarkDirty();
}

bool PlayMenu::IsActive()
{
	return mFadeCurve.GetOutVal() != 0.0 || mFadeCurve.GetOutVal(mFadeCurve.mInMax) == 1.0;
}

}  // namespace Sexy
