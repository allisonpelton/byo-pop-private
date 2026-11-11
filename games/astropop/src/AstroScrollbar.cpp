#include "AstroScrollbar.h"
#include "Graphics.h"

using namespace Sexy;

namespace
{

const Color kTrackColor(0x2a, 0x5d, 0x72);
const Color kPressedTrackColor(0x30, 0x30, 0x30);

}

AstroScrollbutton::AstroScrollbutton(int theId, ButtonListener* theButtonListener) : ScrollbuttonWidget(theId, theButtonListener)
{
}

// 0x4356b0: the framework's ScrollbuttonWidget::Draw with amber arrows
void AstroScrollbutton::Draw(Graphics* g)
{
	int anArrowOffset = 0;

	g->SetColor(Color(212, 212, 212));
	g->FillRect(0, 0, mWidth, mHeight);

	if (mIsDown && mIsOver && !mDisabled)
	{
		anArrowOffset = 1;
		g->SetColor(Color(132, 132, 132));
		g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
	}
	else
	{
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
		g->SetColor(Color(0xe7, 0xad, 0x18));
	else
		g->SetColor(Color(132, 132, 132));

	if (mHorizontal)
	{
		for (int i = 0; i < 4; i++)
		{
			if (mId == 0)
				g->FillRect(i + (mWidth - 4) / 2 + anArrowOffset, mHeight / 2 - i - 1 + anArrowOffset, 1, 1 + i * 2);
			else
				g->FillRect((3 - i) + (mWidth - 4) / 2 + anArrowOffset, mHeight / 2 - i - 1 + anArrowOffset, 1, 1 + i * 2);
		}
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			if (mId == 0)
				g->FillRect(mWidth / 2 - i - 1 + anArrowOffset, i + (mHeight - 4) / 2 + anArrowOffset, 1 + i * 2, 1);
			else
				g->FillRect(mWidth / 2 - i - 1 + anArrowOffset, (3 - i) + (mHeight - 4) / 2 + anArrowOffset, 1 + i * 2, 1);
		}
	}
}

// 0x436310: the two buttons are the game's
AstroScrollbar::AstroScrollbar(int theId, ScrollListener* theScrollListener) : ScrollbarWidget(theId, theScrollListener)
{
	RemoveWidget(mUpButton);
	delete mUpButton;
	RemoveWidget(mDownButton);
	delete mDownButton;

	mUpButton = new AstroScrollbutton(0, this);
	mUpButton->SetDisabled(true);
	mDownButton = new AstroScrollbutton(1, this);
	mDownButton->SetDisabled(true);
	AddWidget(mUpButton);
	AddWidget(mDownButton);
}

// 0x4354d0
void AstroScrollbar::Draw(Graphics* g)
{
	int aThumbSize = GetThumbSize();
	int aThumbPosition = GetThumbPosition();

	if (mHorizontal)
	{
		g->SetColor(mUpdateMode == UPDATE_MODE_PGUP ? kPressedTrackColor : kTrackColor);
		g->FillRect(0, 0, aThumbPosition, mHeight);
		if (aThumbSize > 0)
			DrawThumb(g, aThumbPosition, 0, aThumbSize, mHeight);
		g->SetColor(mUpdateMode == UPDATE_MODE_PGDN ? kPressedTrackColor : Color(232, 232, 232));
		g->FillRect(aThumbPosition + aThumbSize, 0, mWidth - aThumbPosition - aThumbSize, mHeight);
		return;
	}

	g->SetColor(mUpdateMode == UPDATE_MODE_PGUP ? kPressedTrackColor : kTrackColor);
	g->FillRect(0, 0, mWidth, aThumbPosition);
	if (aThumbSize > 0)
		DrawThumb(g, 0, aThumbPosition, mWidth, aThumbSize);
	g->SetColor(mUpdateMode == UPDATE_MODE_PGDN ? kPressedTrackColor : kTrackColor);
	g->FillRect(0, aThumbPosition + aThumbSize, mWidth, mHeight - aThumbPosition - aThumbSize);
}
