#include "ListBox.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include "Sound.h"

using namespace engine;

namespace
{
// The height of a row: the height of the node's font
float rowHeight(Font* theFont)
{
	return theFont != nullptr ? (float)theFont->getHeight() : 0.0f;
}
}

// 0x492070 (pizzafrenzy): the model's signal goes to the list's slot that asks for a layout (0x491580 (pizzafrenzy))
void ListBox::setModel(ListModel* theModel)
{
	if (mModel != nullptr)
		mModel->mChanged.disconnect(this);
	mModel = theModel;
	mModel->mChanged.connect(this, &ListBox::modelChanged);
	setFlags(NODE_LAYOUT);
}

// 0x491580 (pizzafrenzy)
void ListBox::modelChanged()
{
	setFlags(NODE_LAYOUT);
}

// 0x491830 (pizzafrenzy)
int ListBox::getVisibleRows()
{
	float aHeight = rowHeight(mFont);
	if (aHeight == 0)
		return 0;
	return (int)((mBounds.bottom - mBounds.top) / aHeight);
}

// 0x491720 (pizzafrenzy)
void ListBox::layout()
{
	mBounds.left = mPosition.x;
	mBounds.right = mPosition.x + mSize.x;
	mBounds.top = mPosition.y;
	mBounds.bottom = mPosition.y + mSize.y;
	clearFlags(NODE_LAYOUT);
}

// 0x491880 (pizzafrenzy)
int ListBox::rowAt(const Point& thePoint)
{
	int aRow = -1;
	float aY = thePoint.y - mPosition.y;
	if (0.0f <= aY && aY < mBounds.bottom - mBounds.top && rowHeight(mFont) != 0)
		aRow = (int)(aY / rowHeight(mFont));
	if (aRow < mModel->rowCount())
		return aRow;
	return -1;
}

// 0x491930 (pizzafrenzy): the enter sound is stopped when it still plays, then played
void ListBox::mouseMoveEvent(MouseEvent& e)
{
	int aOldRow = mHoverIndex;
	mHoverIndex = rowAt(e.mPosition);
	if (mHoverIndex != aOldRow && mHoverIndex > -1 && mEnterSound != nullptr)
	{
		if (mEnterSound->isPlaying())
			mEnterSound->stop();
		mEnterSound->play();
	}
}

// 0x491c00 (pizzafrenzy)
void ListBox::mouseDownEvent(MouseEvent& e)
{
	int aRow = rowAt(e.mPosition);
	if (aRow != mSelectedIndex)
	{
		mSelectedIndex = aRow;
		mSelectedSignal.emit(aRow);
		if (mSelectedIndex > -1 && mPressSound != nullptr)
		{
			if (mPressSound->isPlaying())
				mPressSound->stop();
			mPressSound->play();
		}
	}
}

// 0x491750 (pizzafrenzy)
void ListBox::mouseExitEvent(MouseEvent& e)
{
	mHoverIndex = -1;
}

// 0x4919d0 (pizzafrenzy): the background, then each row in the colour of its state (picked, under the mouse); the
// picked one on its own background
void ListBox::draw(Graphics* g)
{
	g->setColor(mBackgroundColor.r, mBackgroundColor.g, mBackgroundColor.b);
	g->fillRect(Rect(0, 0, mBounds.right - mBounds.left, mBounds.bottom - mBounds.top));
	float aLineHeight = rowHeight(g->state().mFont);
	float aY = 0;
	int aRowCount = mModel != nullptr ? mModel->rowCount() : 0;
	for (int aRow = 0; aRow < aRowCount; aRow++)
	{
		const Color4* aColor;
		if (aRow == mSelectedIndex)
		{
			g->setColor(mSelectionBackgroundColor.r, mSelectionBackgroundColor.g, mSelectionBackgroundColor.b);
			g->setColorMode(0);
			g->fillRect(Rect(0, aY, mBounds.right - mBounds.left, aY + aLineHeight));
			aColor = aRow == mHoverIndex ? &mSelectionHighlightColor : &mSelectionTextColor;
		}
		else
			aColor = aRow == mHoverIndex ? &mHighlightColor : &mTextColor;
		g->setColor(aColor->r, aColor->g, aColor->b);
		std::string aText = mModel->getText(aRow);
		g->setColorMode(2);
		g->setHAlign(0);
		g->drawString(aText.c_str(), (float)mIndent, aY);
		aY = aY + aLineHeight;
	}
}
// 0x491ea0 (pizzafrenzy): black background, near-black text (the colours are bytes over 255, and the original gives 1
// where it means to give white)
ListBox::ListBox()
{
	mBackgroundColor = Color4::fromBytes(0, 0, 0);
	mTextColor = Color4::fromBytes(1, 1, 1);
	mHighlightColor = Color4::fromBytes(1, 1, 0);
	mSelectionBackgroundColor = Color4::fromBytes(1, 1, 1);
	mSelectionTextColor = Color4::fromBytes(0, 0, 0);
	mSelectionHighlightColor = Color4::fromBytes(0, 0, 1);
	mIndent = 0;
	mHoverIndex = -1;
	mSelectedIndex = -1;
	mEnterSound = nullptr;
	mPressSound = nullptr;
	addOwnFlags(NODE_DRAWABLE | NODE_MOUSE);
}

// 0x491760 (pizzafrenzy): each sound is loaded under its own name, then a voice is made of it (like the button's,
// through the application's slots: the archive's file)
void ListBox::setSounds(const std::string& theEnterSound, const std::string& thePressSound)
{
	if (gSignalBuild)
	{
		App* anApp = App::instance();
		if (theEnterSound.size() != 0)
		{
			anApp->loadSound(theEnterSound, theEnterSound);
			mEnterSound = anApp->createSound(theEnterSound, true, 0);
		}
		if (thePressSound.size() != 0)
		{
			anApp->loadSound(thePressSound, thePressSound);
			mPressSound = anApp->createSound(thePressSound, true, 0);
		}
		return;
	}
	if (theEnterSound.size() != 0 && App::getSoundSystem() != nullptr)
	{
		App::getSoundSystem()->load(theEnterSound.c_str(), theEnterSound.c_str());
		mEnterSound = App::getSoundSystem()->createSound(theEnterSound.c_str(), 1, 0);
	}
	if (thePressSound.size() != 0 && App::getSoundSystem() != nullptr)
	{
		App::getSoundSystem()->load(thePressSound.c_str(), thePressSound.c_str());
		mPressSound = App::getSoundSystem()->createSound(thePressSound.c_str(), 1, 0);
	}
}
