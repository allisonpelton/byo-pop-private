#include "TabbedWidget.h"
#include "TabPanel.h"
#include "WinWSApp.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "compat2004/Compat2004.h"

using namespace Sexy;

// 0x451090: the page list and the tab list are empty, the first page is current, and there is no background and no
// inactive tab image
TabbedWidget::TabbedWidget(WinWSApp* theApp, Image* thePanelImage, Image* theTabImage)
{
	mApp = theApp;
	mPanelImage = thePanelImage;
	mTabImage = theTabImage;
	mCurrentTab = 0;
	mBackground = NULL;
	mFirstTabX = 0;
	mInactiveTabImage = NULL;
	mTabY = 0;
	mTabPadding = 0;	// port: the original leaves +0xa4, +0xa8 and the font (+0x90) unset until the StatsScreen sets them
	mTextOffsetY = 0;
	mFont = NULL;
}

// 0x451170: only the two vectors are freed; the panels are the StatsScreen's
TabbedWidget::~TabbedWidget()
{
}

// 0x442f30 slot 1: the panels join the manager (each is a widget of the manager, not a child of this one), then
// each brings its own widgets in front of itself
void TabbedWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	unsigned int i;
	for (i = 0; i < mPanels.size(); i++)
		theWidgetManager->AddWidget(mPanels[i]);

	for (i = 0; i < mPanels.size(); i++)
		mPanels[i]->BringChildrenToFront();
}

// 0x442f90 slot 2
void TabbedWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	unsigned int i;
	for (i = 0; i < mPanels.size(); i++)
		theWidgetManager->RemoveWidget(mPanels[i]);
}

// 0x442c00 slot 16: the background, then the panel image under the tab strip, then every tab. A tab is drawn from three
// slices of the tab image (a third of its width each): the left one, as many middle slices as fit (the last one cut
// short), and the right one. A tab narrower than two thirds of the image gets the left slice, a middle slice cut to the
// width left, and the right slice. The tab that is shown uses mTabImage, the others mInactiveTabImage (when it is
// set); the slice width is always a third of mTabImage. The title is centred in the tab's width; a tab that is not shown
// gets a line in mDividerColor along the bottom of its image
void TabbedWidget::Draw(Graphics* g)
{
	Image* anImage = mTabImage;
	int aThird = anImage->mWidth / 3;
	if (mBackground != NULL)
		g->DrawImage(mBackground, 0, 0);
	g->DrawImage(mPanelImage, 0, anImage->mHeight);
	g->SetFont(mFont);
	g->SetColor(mTextColor);

	unsigned int i;
	for (i = 0; i < mTabRects.size(); i++)
	{
		if ((int)i == mCurrentTab || mInactiveTabImage == NULL)
			anImage = mTabImage;
		else
			anImage = mInactiveTabImage;

		int aTabX = mTabRects[i].mX;
		int aTabY = mTabRects[i].mY;
		int aTabWidth = mTabRects[i].mWidth;
		int aMiddle = aTabWidth - (anImage->mWidth * 2) / 3;
		int aNextX;
		Rect aRightSlice;
		if (aMiddle < aThird)
		{
			g->DrawImage(anImage, aTabX, aTabY, Rect(0, 0, aThird, anImage->mHeight));
			aNextX = aTabX + aThird;
			if (0 < aMiddle)
			{
				g->DrawImage(anImage, aNextX, aTabY, Rect(aThird, 0, aMiddle, anImage->mHeight));
				aNextX = aNextX + aMiddle;
			}
			aRightSlice = Rect((anImage->mWidth * 2) / 3, 0, aThird, anImage->mHeight);
		}
		else
		{
			g->DrawImage(anImage, aTabX, aTabY, Rect(0, 0, aThird, anImage->mHeight));
			int aWholeSlices = aMiddle / aThird;
			int aRest = aMiddle % aThird;
			int aSliceX = aTabX;
			int aSlicesLeft = aWholeSlices;
			if (0 < aWholeSlices)
			{
				do
				{
					g->DrawImage(anImage, aSliceX + aThird, aTabY, Rect(aThird, 0, aThird, anImage->mHeight));
					aSliceX = aSliceX + aThird;
					aSlicesLeft = aSlicesLeft - 1;
				} while (aSlicesLeft != 0);
			}
			aNextX = (aWholeSlices + 1) * aThird + aTabX;
			if (aRest != 0)
			{
				g->DrawImage(anImage, aNextX, aTabY, Rect(aThird, 0, aRest, anImage->mHeight));
				aNextX = aNextX + aRest;
			}
			aRightSlice = Rect((anImage->mWidth * 2) / 3, 0, aThird, anImage->mHeight);
		}
		g->DrawImage(anImage, aNextX, aTabY, aRightSlice);

		WriteString2004(g, mPanels[i]->mTitle, aTabX, mFont->GetAscent() + mTextOffsetY + aTabY, aTabWidth, 0, true, 0, -1);

		if ((int)i != mCurrentTab)
		{
			g->SetColor(mDividerColor);
			int aLineY = anImage->mHeight - 1 + aTabY;
			g->DrawLine(aTabX, aLineY, aTabWidth + aTabX, aLineY);
		}
	}
}

// 0x4414b0 slot 24 (no Widget::MouseLeave)
void TabbedWidget::MouseLeave()
{
	mApp->SetCursor(0);
}

// 0x442b80 slot 25: the hand cursor over a tab (the shown one too), the arrow elsewhere
void TabbedWidget::MouseMove(int x, int y)
{
	unsigned int i = 0;
	while (true)
	{
		if (i >= mTabRects.size())
		{
			mApp->SetCursor(0);
			return;
		}
		const Rect& aRect = mTabRects[i];
		if (aRect.mX <= x && x <= aRect.mWidth + aRect.mX && aRect.mY <= y && y <= aRect.mHeight + aRect.mY)
			break;
		i++;
	}
	mApp->SetCursor(1);
}

// 0x442ae0 slot 27 (the 2004 MouseDown(x, y, clickCount); no Widget::MouseDown): a click on a tab other than the shown
// one hides the shown page and shows that one. The widget is marked dirty for every click
void TabbedWidget::MouseDown(int x, int y, int theClickCount)
{
	unsigned int i = 0;
	while (true)
	{
		if (i >= mTabRects.size())
		{
			MarkDirty();
			return;
		}
		const Rect& aRect = mTabRects[i];
		if (aRect.mX <= x && x <= aRect.mWidth + aRect.mX && aRect.mY <= y && y <= aRect.mHeight + aRect.mY &&
			(int)i != mCurrentTab)
			break;
		i++;
	}

	mPanels[mCurrentTab]->SetShown(false);
	mCurrentTab = i;
	mPanels[i]->SetShown(true);
	MarkDirty();
}

// 0x453ac0: the tab is as wide as the title, but at least two thirds of a tab image, plus the padding on both sides; as
// high as the tab image; it goes at mFirstTabX, or right of the last tab, at mTabY
void TabbedWidget::AddTab(TabPanel* thePanel)
{
	int aTwoThirds = (mTabImage->mWidth / 3) * 2;
	int aWidth = mFont->StringWidth(thePanel->mTitle);
	if (aTwoThirds - aWidth != 0 && aWidth <= aTwoThirds)
		aWidth = aTwoThirds;
	aWidth = aWidth + mTabPadding * 2;
	int aHeight = mTabImage->mHeight;

	int aX;
	if (mTabRects.empty())
		aX = mFirstTabX;
	else
		aX = mTabRects[mTabRects.size() - 1].mX + mTabRects[mTabRects.size() - 1].mWidth;
	int aY = mTabY;

	mPanels.push_back(thePanel);
	mTabRects.push_back(Rect(aX, aY, aWidth, aHeight));
}

// 0x442fd0: the new page is shown without a repaint (the StatsScreen's constructor uses it)
void TabbedWidget::SetCurrentTab(int theTab)
{
	mPanels[mCurrentTab]->SetShown(false);
	mCurrentTab = theTab;
	mPanels[theTab]->SetShown(true);
}
