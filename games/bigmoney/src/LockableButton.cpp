#include "LockableButton.h"

using namespace Sexy;

// 0x414d80
LockableButton::LockableButton(WinBMApp* theApp, int theId, ButtonListener* theButtonListener) :
	ButtonWidget(theId, theButtonListener)
{
	mApp = theApp;
	mLocked = false;
}

// 0x414dc0
LockableButton::~LockableButton()
{
}

// 0x414dd0: no lock overlay, the plain button
void LockableButton::Draw(Graphics* g)
{
	ButtonWidget::Draw(g);
}
