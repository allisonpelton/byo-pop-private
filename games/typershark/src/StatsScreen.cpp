#include "StatsScreen.h"
#include "TabbedWidget.h"
#include "TabPanel.h"
#include "AdventurePanel.h"
#include "AbyssPanel.h"
#include "TutorialPanel.h"
#include "UserProfile.h"
#include "WinWSApp.h"
#include "WidgetManager.h"
#include "Image.h"
#include "ImageFont.h"
#include "Res.h"

using namespace Sexy;

// 0x45baa0: the tab strip is made from the panel image and the tab image, with the inactive tab image, 10 pixels of padding
// on each side of a title and 10 under the font's top, the first tab at x = 16 and all tabs at y = 8, Rockwell 13 in black
// with black divider lines, the whole window as its size and the water image behind it. Then the three pages are made in
// the order adventure, abyss, tutorial (the last two are hidden at once), each placed under the tabs and filling the rest
// of the window, and added as tabs; the page asked for is shown at the end
StatsScreen::StatsScreen(WinWSApp* theApp, int theInitialTab)
{
	mApp = theApp;
	mTabs = new TabbedWidget(mApp, mApp->mImages[IMAGE_TAB_PANEL], mApp->mImages[IMAGE_TAB_TOP]);
	mTabs->mInactiveTabImage = mApp->mImages[IMAGE_TAB_TOP_SHADOW];
	mTabs->mTabPadding = 10;
	mTabs->mTextOffsetY = 10;
	mTabs->mFirstTabX = 0x10;
	mTabs->mTabY = 8;
	mTabs->mFont = mApp->mFontRockwell13;
	mTabs->mTextColor = Color(0, 0, 0);
	mTabs->mDividerColor = Color(0, 0, 0);
	mTabs->Resize(0, 0, mApp->mWidth, mApp->mHeight);
	mTabs->mBackground = mApp->mImages[IMAGE_WATER1];

	mAdventurePanel = NULL;	// port: the original leaves the three fields unset until the loop makes them
	mAbyssPanel = NULL;
	mTutorialPanel = NULL;

	int i = 0;
	do
	{
		TabPanel* aPanel;
		if (i == 2)
		{
			mTutorialPanel = new TutorialPanel(mApp);
			aPanel = mTutorialPanel;
			aPanel->SetShown(false);
		}
		else if (i == 0)
		{
			mAdventurePanel = new AdventurePanel(mApp);
			aPanel = mAdventurePanel;
		}
		else
		{
			mAbyssPanel = new AbyssPanel(mApp);
			aPanel = mAbyssPanel;
			aPanel->SetShown(false);
		}

		int aTabHeight = mApp->mImages[IMAGE_TAB_TOP]->mHeight;
		int aTabY = mTabs->mTabY;
		Rect aRect(0, aTabHeight + aTabY, mApp->mWidth, (mApp->mHeight - aTabY) - aTabHeight);
		aPanel->Resize(aRect);
		mTabs->AddTab(aPanel);
		i++;
	} while (i < 3);

	mTabs->SetCurrentTab(theInitialTab);
}

// 0x4412a0: the three pages (the adventure page first), then the tab strip
StatsScreen::~StatsScreen()
{
	if (mAdventurePanel != NULL)
		delete mAdventurePanel;
	if (mAbyssPanel != NULL)
		delete mAbyssPanel;
	if (mTutorialPanel != NULL)
		delete mTutorialPanel;
	if (mTabs != NULL)
		delete mTabs;
}

// 0x441330 slot 1: the tab strip (and with it the pages) joins the manager
void StatsScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mTabs);
}

// 0x441360 slot 2
void StatsScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTabs);
}

// 0x441320 (the listener sub-object's slot 3): the app plays the button-click sound (sample 5). Nothing registers this screen
// as a listener; the original's pages use their own and the Close button is the app's
void StatsScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x451050: the pages get the profile; the tutorial page is advanced to the lesson the profile ended with
void StatsScreen::SetUser(UserProfile* theUser)
{
	mTutorialPanel->mUser = theUser;
	mAdventurePanel->mUser = theUser;
	mAbyssPanel->mUser = theUser;
	mTutorialPanel->AdvanceToLesson(theUser->mLastLesson);
}
