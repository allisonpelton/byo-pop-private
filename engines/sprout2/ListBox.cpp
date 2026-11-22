#include "ListBox.h"

#include "DrawContext.h"
#include "Font.h"

using namespace engine;

ListBox::ListBox()
	: mWidth(0.0f), mHeight(0.0f), mBackgroundColor(0.0f, 0.0f, 0.0f, 1.0f), mTextColor(1.0f, 1.0f, 1.0f, 1.0f),
	  mHighlightColor(1.0f, 1.0f, 0.0f, 1.0f), mSelectionColor(1.0f, 1.0f, 1.0f, 1.0f),
	  mSelectionTextColor(0.0f, 0.0f, 0.0f, 1.0f), mSelectionHighlightColor(0.0f, 0.0f, 1.0f, 1.0f), mIndent(0),
	  mHighlighted(-1), mSelected(-1), mTopRow(0), mAlignH(TEXT_ALIGN_LEFT)
{
	addOwnState(STATE_VISIBLE | STATE_INPUT);
}

void ListBox::paint(DrawContext* theContext)
{
	theContext->back().mColor = mBackgroundColor;
	theContext->fillRect(mBounds.right - mBounds.left, mBounds.bottom - mBounds.top);
	Font* aFont = theContext->back().mFont;
	float aLineHeight = aFont != nullptr ? (float)aFont->getHeight() : 0.0f;
	float y = 0.0f;
	int anEnd;
	if (mModel != nullptr)
	{
		int aCount = mModel->getCount();
		int aLast = getVisibleRows() + mTopRow;
		anEnd = aCount < aLast ? aCount : aLast;
	}
	else
		anEnd = getVisibleRows();
	for (int aRow = mTopRow; aRow < anEnd; aRow++)
	{
		if (aRow == mSelected)
		{
			// The line's corners truncated from the x87's sums
			RectI aLine(0, (int)y, (int)((double)mBounds.right - mBounds.left), (int)((double)y + aLineHeight));
			theContext->back().mColor = mSelectionColor;
			theContext->back().mColorMode = COLOR_MODE_NONE;
			theContext->fillRect(aLine);
			theContext->back().mColor = aRow == mHighlighted ? mSelectionHighlightColor : mSelectionTextColor;
		}
		else if (aRow == mHighlighted)
			theContext->back().mColor = mHighlightColor;
		else
			theContext->back().mColor = mTextColor;
		std::string anItem = mModel != nullptr ? mModel->getItem(aRow) : std::string();
		theContext->back().mColorMode = COLOR_MODE_MULTIPLY;
		theContext->back().mTextAlignH = (TextAlignH)mAlignH;
		float x;
		if (mAlignH == TEXT_ALIGN_CENTER)
			x = mWidth * 0.5f;
		else if (mAlignH == TEXT_ALIGN_RIGHT)
			x = (float)((double)mWidth - (double)mIndent);
		else
			x = (float)mIndent;
		theContext->drawText(anItem.c_str(), x, y);
		y = (float)((double)y + aLineHeight);
	}
}

void ListBox::computeBounds()
{
	mBounds.left = mPosition.x;
	mBounds.right = mPosition.x + mWidth;
	mBounds.top = mPosition.y;
	mBounds.bottom = mPosition.y + mHeight;
	removeState(STATE_BOUNDS_DIRTY);
}

void ListBox::mouseDown(MouseEvent& theEvent)
{
	int aRow = rowAt(theEvent.mPosition);
	if (aRow == mSelected || aRow < 0)
		return;
	mSelected = aRow;
	mSelectSignal.emit(aRow);
	// The click's sound played (its instance's slot 0, 0x4f4910 (feedingfrenzy2)): the sounds are not yet translated
}

void ListBox::mouseMove(MouseEvent& theEvent)
{
	int aPrevious = mHighlighted;
	mHighlighted = rowAt(theEvent.mPosition);
	if (mHighlighted == aPrevious || mHighlighted < 0)
		return;
	// The over sound played (0x4ed6b8 (feedingfrenzy2)): the sounds are not yet translated
}

void ListBox::mouseLeave(MouseEvent& theEvent)
{
	mHighlighted = -1;
}

void ListBox::setModel(ListModel* theModel)
{
	mModel = theModel;
	mModel->mChangedSignal.connect(this, &ListBox::modelChanged);
	modelChanged();
}

int ListBox::getCount()
{
	return mModel != nullptr ? mModel->getCount() : 0;
}

int ListBox::getVisibleRows()
{
	float aLineHeight = mFont != nullptr ? (float)mFont->getHeight() : 1.0f;
	return (int)((double)mHeight / aLineHeight);
}

int ListBox::rowAt(const Vector2f& thePoint)
{
	Vector2f aLocal(thePoint.x - mPosition.x, thePoint.y - mPosition.y);
	int aRow = -1;
	if (aLocal.y >= 0.0f && (double)mBounds.bottom - mBounds.top > aLocal.y)
	{
		float aLineHeight = mFont != nullptr ? (float)mFont->getHeight() : 1.0f;
		aRow = (int)((double)aLocal.y / aLineHeight) + mTopRow;
	}
	if (aRow >= getCount())
		aRow = -1;
	return aRow;
}
