// WidgetMover.cpp: see WidgetMover.h.

#include "WidgetMover.h"

#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x406010
bool WidgetMover::Entry::Step()
{
	mStepsLeft--;
	float aFrom = (float)(mStepsLeft / 20.0); // how much of the way is still to go
	float aTo = 1.0f - aFrom;
	float aEndX = mEndX * aTo;
	float aEndY = aTo * mEndY;
	float aStartX = mStartX * aFrom;
	float aStartY = aFrom * mStartY;
	float aX = aStartX + aEndX;
	float aY = aStartY + aEndY;
	mWidget->Move((int)aX, (int)aY);

	if (mStepsLeft == 0)
	{
		if (mRemoveWhenDone)
		{
			gSexyAppBase->mWidgetManager->RemoveWidget(mWidget);
			delete mWidget;
			mWidget = nullptr;
		}
		return false;
	}
	return true;
}

// 0x40c670
void WidgetMover::Update()
{
	std::list<Entry>::iterator anItr = mEntries.begin();
	while (anItr != mEntries.end())
	{
		if (anItr->Step())
			++anItr;
		else
			anItr = mEntries.erase(anItr);
	}
}

// 0x421d90
WidgetMover::Entry* WidgetMover::GetEntry(Widget* theWidget, bool theCreate)
{
	for (std::list<Entry>::iterator anItr = mEntries.begin(); anItr != mEntries.end(); ++anItr)
	{
		if (anItr->mWidget == theWidget)
			return &*anItr;
	}
	if (!theCreate)
		return nullptr;

	mEntries.push_back(Entry());
	mEntries.back().mWidget = theWidget;
	return &mEntries.back();
}

// 0x421e40
void WidgetMover::ScrollOn(Widget* theWidget, WidgetScroll theFrom)
{
	Entry* anEntry = GetEntry(theWidget, true);
	float aStartX = (float)theWidget->mX;
	float aStartY = (float)theWidget->mY;
	anEntry->mStepsLeft = SCROLL_STEPS;
	anEntry->mRemoveWhenDone = false;
	anEntry->mEndX = aStartX;
	anEntry->mEndY = aStartY;
	switch (theFrom)
	{
	case SCROLL_NONE:
		break;
	case SCROLL_LEFT:
		aStartX = (float)gSexyAppBase->mWidth;
		break;
	case SCROLL_RIGHT:
		aStartX = (float)-theWidget->mWidth;
		break;
	case SCROLL_DOWN:
		aStartY = (float)-theWidget->mHeight;
		break;
	case SCROLL_UP:
		aStartY = (float)gSexyAppBase->mHeight;
		break;
	default:
		// no start position is given: the move starts from the entry's, which is the corner of the screen for a new one
		theWidget->mWidgetFlagsMod.mRemoveFlags = 0;
		theWidget->mMouseVisible = true;
		return;
	}
	anEntry->mStartX = aStartX;
	anEntry->mStartY = aStartY;
	theWidget->mWidgetFlagsMod.mRemoveFlags = 0;
	theWidget->mMouseVisible = true;
}

// 0x421f80
void WidgetMover::ScrollOff(Widget* theWidget, WidgetScroll theTo, bool theRemoveWhenDone)
{
	Entry* anEntry = GetEntry(theWidget, true);
	float anEndX = (float)theWidget->mX;
	float anEndY = (float)theWidget->mY;
	anEntry->mStepsLeft = SCROLL_STEPS;
	anEntry->mStartX = anEndX;
	anEntry->mStartY = anEndY;
	anEntry->mRemoveWhenDone = theRemoveWhenDone;
	switch (theTo)
	{
	case SCROLL_NONE:
		break;
	case SCROLL_LEFT:
		anEndX = (float)-theWidget->mWidth;
		break;
	case SCROLL_RIGHT:
		anEndX = (float)gSexyAppBase->mWidth;
		break;
	case SCROLL_DOWN:
		anEndY = (float)gSexyAppBase->mHeight;
		break;
	case SCROLL_UP:
		anEndY = (float)-theWidget->mHeight;
		break;
	default:
		// the end position is left as it was
		theWidget->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE | WIDGETFLAGS_ALLOW_MOUSE | WIDGETFLAGS_ALLOW_FOCUS;
		theWidget->mMouseVisible = false;
		return;
	}
	anEntry->mEndX = anEndX;
	anEntry->mEndY = anEndY;
	// the widget takes no more updates, mouse or focus while it leaves
	theWidget->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE | WIDGETFLAGS_ALLOW_MOUSE | WIDGETFLAGS_ALLOW_FOCUS;
	theWidget->mMouseVisible = false;
}

// 0x4220d0
bool WidgetMover::WillRemove(Widget* theWidget)
{
	Entry* anEntry = GetEntry(theWidget, false);
	return anEntry != nullptr && anEntry->mRemoveWhenDone;
}

// 0x40c7a0
void WidgetMover::Forget(Widget* theWidget)
{
	for (std::list<Entry>::iterator anItr = mEntries.begin(); anItr != mEntries.end(); ++anItr)
	{
		if (anItr->mWidget == theWidget)
		{
			mEntries.erase(anItr);
			return;
		}
	}
}
