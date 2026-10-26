#include "SexyScrollbarWidget.h"

#include "Graphics.h"

using namespace JeffLib;
using namespace Sexy;

void SexyScrollbuttonWidget::Draw(Graphics* g)
{
	int anArrowOffset = 0;

	g->SetColor(Color(212, 212, 212));
	g->FillRect(0, 0, mWidth, mHeight);

	if (mIsDown && mIsOver && !mDisabled) {
		anArrowOffset = 1;
		g->SetColor(Color(132, 132, 132));
		g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
	} else {
		g->SetColor(Color(255, 255, 255));
		g->FillRect(1, 1, mWidth - 2, 1);
		g->FillRect(1, 1, 1, mHeight - 2);

		g->SetColor(Color::Black);
		g->FillRect(0, mHeight - 1, mWidth, 1);
		g->FillRect(mWidth - 1, 0, 1, mHeight);

		g->SetColor(Color(132, 132, 132));
		g->FillRect(1, mHeight - 2, mWidth - 2, 1);
		g->FillRect(mWidth - 2, 1, 1, mHeight - 2);
	}

	if (!mDisabled)
		g->SetColor(Color(231, 173, 24));
	else
		g->SetColor(Color(132, 132, 132));

	if (mHorizontal) {
		int i = 0;
		int aSize = 1;
		do {
			int aY = mHeight / 2 - i;
			int aX;
			if (mId == 0)
				aX = (mWidth - 4) / 2 + i;
			else
				aX = (mWidth - 4) / 2 - i + 3;
			g->FillRect(aX + anArrowOffset, aY - 1 + anArrowOffset, 1, aSize);
			i++;
			aSize += 2;
		} while (aSize < 9);
	} else {
		int i = 0;
		int aSize = 1;
		do {
			int aY = (mHeight - 4) / 2;
			int aX = mWidth / 2 - i;
			if (mId == 0)
				aY += i;
			else
				aY = aY - i + 3;
			g->FillRect(aX - 1 + anArrowOffset, aY + anArrowOffset, aSize, 1);
			i++;
			aSize += 2;
		} while (aSize < 9);
	}
}

SexyScrollbarWidget::SexyScrollbarWidget(ScrollListener* theListener)
	: ScrollbarWidget(1, theListener), mTrackColor(0, 0, 0)
{
	// the buttons the base made go, and this kind's come in their place
	RemoveWidget(mUpButton);
	RemoveWidget(mDownButton);
	delete mUpButton;
	delete mDownButton;

	mId = 1;
	mScrollListener = theListener;
	Widget::SetDisabled(true);

	mUpButton = new SexyScrollbuttonWidget(0, this);
	mUpButton->SetDisabled(true);
	mDownButton = new SexyScrollbuttonWidget(1, this);
	mDownButton->SetDisabled(true);
	AddWidget(mUpButton);
	AddWidget(mDownButton);

	mValue = 0;
	mInvisIfNoScroll = false;
	mMaxValue = 0;
	mPressedOnThumb = false;
	mPageSize = 0;
	mUpdateAcc = 0;
	mButtonAcc = 0;
	mUpdateMode = 0;
	mHorizontal = false;
}

void SexyScrollbarWidget::DrawThumb(Graphics* g, int theX, int theY, int theWidth, int theHeight)
{
	g->SetColor(Color(212, 212, 212));
	g->FillRect(theX, theY, theWidth, theHeight);

	g->SetColor(Color(255, 255, 255));
	g->FillRect(theX + 1, theY + 1, theWidth - 2, 1);
	g->FillRect(theX + 1, theY + 1, 1, theHeight - 2);

	g->SetColor(Color::Black);
	g->FillRect(theX, theY + theHeight - 1, theWidth, 1);
	g->FillRect(theX + theWidth - 1, theY, 1, theHeight);

	g->SetColor(Color(132, 132, 132));
	g->FillRect(theX + 1, theY + theHeight - 2, theWidth - 2, 1);
	g->FillRect(theX + theWidth - 2, theY + 1, 1, theHeight - 2);
}

void SexyScrollbarWidget::Draw(Graphics* g)
{
	int aThumbSize = GetThumbSize();
	int aThumbPosition = GetThumbPosition();

	if (mHorizontal) {
		if (mUpdateMode == UPDATE_MODE_PGUP)
			g->SetColor(Color(48, 48, 48));
		else
			g->SetColor(mTrackColor);
		g->FillRect(0, 0, aThumbPosition, mHeight);

		if (aThumbSize > 0)
			DrawThumb(g, aThumbPosition, 0, aThumbSize, mHeight);

		// (a horizontal bar's second part keeps the framework's grey, not the track's colour)
		if (mUpdateMode == UPDATE_MODE_PGDN)
			g->SetColor(Color(48, 48, 48));
		else
			g->SetColor(Color(232, 232, 232));
		g->FillRect(aThumbPosition + aThumbSize, 0, mWidth - aThumbPosition - aThumbSize, mHeight);
	} else {
		if (mUpdateMode == UPDATE_MODE_PGUP)
			g->SetColor(Color(48, 48, 48));
		else
			g->SetColor(mTrackColor);
		g->FillRect(0, 0, mWidth, aThumbPosition);

		if (aThumbSize > 0)
			DrawThumb(g, 0, aThumbPosition, mWidth, aThumbSize);

		if (mUpdateMode == UPDATE_MODE_PGDN)
			g->SetColor(Color(48, 48, 48));
		else
			g->SetColor(mTrackColor);
		g->FillRect(0, aThumbPosition + aThumbSize, mWidth, mHeight - aThumbPosition - aThumbSize);
	}
}
