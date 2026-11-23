#include "TitleState.h"

#include "AtlantisApp.h"
#include "FrontEnd.h"

// 0x406e00: the front-end screens made (0x4039b0) and the main menu shown
void TitleState::enter()
{
	if (mApp->mFrontEnd == nullptr)
		mApp->mFrontEnd.reset(new FrontEnd(mApp));
	mApp->mFrontEnd->showMainMenu();
}
