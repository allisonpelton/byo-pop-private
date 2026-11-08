#include "TabPanel.h"
#include "WinWSApp.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x450cd0: the panel has alpha (so the widget behind it is repainted), is shown, and has no children and no title
TabPanel::TabPanel(WinWSApp* theApp)
{
	mApp = theApp;
	mHasAlpha = true;
	mShown = true;
	mTitle = "";
}

// 0x444690: the children are not deleted here (the subclass that made them deletes them)
TabPanel::~TabPanel()
{
}

// 0x43fbd0 slot 1
void TabPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
}

// 0x43fbe0 slot 2
void TabPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
}

// 0x442a10 slot 47: a shown panel is brought to the front of the manager with its children in front of it; a hidden one
// goes to the back. The children and the panel are made visible or invisible, the stats screen's Close button is brought
// back in front, and the state is stored
void TabPanel::SetShown(bool theShow)
{
	if (!theShow)
		mApp->mWidgetManager->BringToBack(this);
	else
		mApp->mWidgetManager->BringToFront(this);

	unsigned int i;
	for (i = 0; i < mChildren.size(); i++)
	{
		mChildren[i]->SetVisible(theShow);
		if (theShow)
			mApp->mWidgetManager->BringToFront(mChildren[i]);
	}

	SetVisible(theShow);
	mApp->BringStatsCloseButtonToFront();
	mShown = theShow;
}

// 0x4516a0 slot 48
void TabPanel::AddChild(Widget* theChild)
{
	mChildren.push_back(theChild);
}

// 0x4446f0 slot 49
Widget* TabPanel::GetChild(int theIndex)
{
	return mChildren[theIndex];
}

// 0x442aa0
void TabPanel::BringChildrenToFront()
{
	unsigned int i;
	for (i = 0; i < mChildren.size(); i++)
		mApp->mWidgetManager->BringToFront(mChildren[i]);
}
