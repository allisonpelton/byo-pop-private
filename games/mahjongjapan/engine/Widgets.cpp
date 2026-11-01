#include "Widgets.h"

#include "Font.h"
#include "Layer.h"
#include "Manager.h"

namespace jb
{

// 0x4309f0
Slider::Slider(int theImage, int theMinX, int theMaxX) : Button(theImage, 2, false), mMinX(theMinX), mMaxX(theMaxX)
{
}

// 0x430a70: dragged along its row
bool Slider::OnLButtonDown(int theFlags, int theX, int theY)
{
	PlaySound(0x69);
	mManager->StartDrag(this, 3, mMinX, mY, mMaxX, mY);
	return true;
}

// 0x430aa0
void Slider::OnDragEnd(int theFlags, int theX, int theY)
{
	ReportValue();
}

// 0x430aa0
void Slider::OnDrag(int theFlags, int theX, int theY)
{
	ReportValue();
}

// 0x430ab0
void Slider::ReportValue()
{
	int aValue = ((mX - mMinX) * 100) / (mMaxX - mMinX);
	if (aValue < 0)
	{
		((SliderBar*)mParent)->ValueChanged(0);
		return;
	}
	if (100 < aValue)
		aValue = 100;
	((SliderBar*)mParent)->ValueChanged(aValue);
}

// 0x430b00 (0x40a4e0 moves it)
void Slider::SetValue(int theValue)
{
	if (theValue < 0)
		theValue = 0;
	else if (100 < theValue)
		theValue = 100;
	mX = ((mMaxX - mMinX) * theValue) / 100 + mMinX;
}

// 0x426910
SliderBar::SliderBar(int theTrack, int theThumb, int theSliderX, int theSliderEnd)
	: mTrack(theTrack), mThumb(theThumb), mSliderX(theSliderX), mSliderEnd(theSliderEnd)
{
}

// 0x426980
void SliderBar::Draw()
{
	DrawImage(0, 0, mTrack, nullptr, true, 100, 100, 0xff);
}

// 0x4269a0: the slider's range is mSliderX to mSliderEnd plus the track's width less a quarter of the slider's
void SliderBar::Init()
{
	ImageInfo anInfo;
	GetImageInfo(mTrack, anInfo);
	int aTrackHeight = anInfo.mHeight;
	int aTrackWidth = anInfo.mWidth;
	GetImageInfo(mThumb, anInfo);
	mSlider = new Slider(mThumb, mSliderX, mSliderEnd + (aTrackWidth - anInfo.mWidth / 4));
	AddChild(mSlider, mSliderX, (aTrackHeight - anInfo.mHeight) / 2);
}

// 0x426a80
void SliderBar::SetValue(int theValue)
{
	mSlider->SetValue(theValue);
}

// 0x426aa0
void SliderBar::ValueChanged(int theValue)
{
	mValue = theValue;
	OnValue(theValue);
}

// 0x426ac0
ListBox::ListBox(int theCmd, int theWidth, int theRowHeight, int theRows, int theFixed)
	: mCmd(theCmd), mTop(theFixed), mWidth(theWidth), mRowHeight(theRowHeight), mRows(theRows), mFixed(theFixed)
{
	SetRect(mFrame, 0, 0, theWidth + 2, theRowHeight * theRows + 2);
}

// 0x426b90
ListBox::~ListBox()
{
	for (Item* anItem : mItems)
		delete anItem;
	mItems.clear();
}

// 0x426cb0
void ListBox::Init()
{
	SetCursorKind(1, false);
	RegHover(true);
	RegMove(true);
	RegClick(true);
	RegLayerCmd(true);
	OnInit();
}

// 0x426d00
void ListBox::SetFrame(uint32_t theColour1, uint32_t theColour2, uint32_t theColour3)
{
	mFrameColour1 = theColour1;
	mFramed = true;
	mFrameColour2 = theColour2;
	mFrameColour3 = theColour3;
}

// 0x426d30
void ListBox::SetFont(int theFontImage, int theFontText, int theX, int theY, int theDisabledAlpha)
{
	mFont = GetDllFont(theFontImage, theFontText, -1);
	mTextX = theX;
	mTextY = theY;
	mDisabledAlpha = theDisabledAlpha;
}

// 0x426d70: the buttons' images have four frames; the list's frame grows by a button's width
void ListBox::SetScroll(int theUpImage, int theDownImage)
{
	mUpImage = theUpImage;
	mDownImage = theDownImage;
	mScrolls = true;
	ImageInfo anInfo;
	GetImageInfo(theUpImage, anInfo);
	mUp = new Button(theUpImage, 4, false);
	AddChild(mUp, mFrame.right + 1, mFrame.top + 1);
	mUp->SetEnabled(false);
	mDown = new Button(theDownImage, 4, false);
	AddChild(mDown, mFrame.right + 1, (mFrame.bottom - anInfo.mHeight) - 1);
	mDown->SetEnabled(false);
	mBarX = mFrame.right;
	mFrame.right = anInfo.mWidth / 4 + 2 + mFrame.right;
}

// 0x426eb0
void ListBox::Draw()
{
	if (mFramed)
	{
		DrawFrame(mFrame, mFrameColour3, true);
		OffsetRect(mFrame, -1, -1);
		DrawFrame(mFrame, mFrameColour1, true);
		OffsetRect(mFrame, -1, -1);
		DrawFrame(mFrame, mFrameColour2, true);
		OffsetRect(mFrame, 2, 2);
	}
	if (mScrolls)
	{
		int x = mBarX;
		int y = mFrame.top;
		int aBottom = mFrame.bottom - 2;
		DrawLine(x, y, x, aBottom, mFrameColour3, true);
		DrawLine(x - 1, y - 1, x - 1, aBottom, mFrameColour1, true);
		DrawLine(x - 2, y - 2, x - 2, aBottom, mFrameColour2, true);
	}
	int aRow = mSelected;
	if (aRow != -1)
	{
		if (mFixed <= aRow)
			aRow = (mFixed - mTop) + aRow;
		Rect aRect;
		aRect.left = 0;
		aRect.top = mRowHeight * aRow;
		aRect.right = mWidth;
		aRect.bottom = mRowHeight + aRect.top;
		FillRect(aRect, mSelColour, true);
	}
	int anAlpha = 0xff;
	if (!IsEnabled())
		anAlpha = mDisabledAlpha;
	size_t i = 0;
	int aLine = 0;
	if (i != mItems.size())
	{
		do
		{
			if (mFixed <= aLine)
				break;
			mFont->DrawLeft(this, mTextX, mRowHeight * aLine + mTextY, mItems[i]->mText.c_str(), mWidth - mTextX,
				100, 100, anAlpha);
			i = i + 1;
			aLine = aLine + 1;
		} while (i != mItems.size());
	}
	i = (size_t)mTop;
	if (i != mItems.size())
	{
		do
		{
			if (mRows <= aLine)
				return;
			mFont->DrawLeft(this, mTextX, mRowHeight * aLine + mTextY, mItems[i]->mText.c_str(), mWidth - mTextX,
				100, 100, anAlpha);
			i = i + 1;
			aLine = aLine + 1;
		} while (i != mItems.size());
	}
}

// 0x4270e0: the hand over the rows that hold items
void ListBox::OnMouseMove(int theFlags, int theX, int theY)
{
	int aCount = (int)mItems.size();
	if (mRows < aCount)
		aCount = mRows;
	if (1 < theX && theX < mWidth - 2 && theY < mRowHeight * aCount)
	{
		SetCursorKind(1, true);
		return;
	}
	SetCursorKind(0, true);
}

// 0x427150
bool ListBox::OnLButtonDown(int theFlags, int theX, int theY)
{
	if (1 < theX && theX < mWidth - 2)
	{
		unsigned int aRow = (unsigned int)(theY / mRowHeight);
		if (!mItems.empty() && aRow < (unsigned int)mItems.size() && (int)aRow < mRows)
		{
			PlaySound(0x69);
			if ((int)aRow < mFixed)
				mSelected = (int)aRow;
			else
				mSelected = (mTop - mFixed) + (int)aRow;
			mLayer->Command(mCmd);
			Invalidate();
			return true;
		}
	}
	return false;
}

// 0x4271f0: the scroll buttons' commands are their images; the selection moves with the rows
bool ListBox::OnCommand(int theCmd)
{
	if (mScrolls)
	{
		if (theCmd == mUpImage)
		{
			if (mFixed < mTop)
			{
				mTop = mTop - 1;
				mSelected = mSelected - 1;
				Invalidate();
				return true;
			}
		}
		else if (theCmd == mDownImage)
		{
			if ((unsigned int)((mRows - mFixed) + mTop) < (unsigned int)mItems.size())
			{
				mTop = mTop + 1;
				mSelected = mSelected + 1;
				Invalidate();
				return true;
			}
		}
	}
	return false;
}

// 0x4272a0: below 0 is the first; the list scrolls down to it, never up
void ListBox::SetSelected(int theIndex)
{
	if (theIndex < 0)
		mSelected = 0;
	else if (mItems.empty() || (unsigned int)mItems.size() <= (unsigned int)theIndex)
		mSelected = (int)mItems.size() - 1;
	else
		mSelected = theIndex;
	int aSelected = mSelected;
	if (mTop <= aSelected && (mRows - mFixed) + mTop <= aSelected)
		mTop = (mFixed - mRows) + 1 + aSelected;
}

// 0x427340
int ListBox::Add(int theStringId, int theData)
{
	Item* anItem = new Item();
	anItem->mText = LoadDllString(theStringId);
	anItem->mData = theData;
	return AddItem(anItem);
}

// 0x4273c0
int ListBox::Add(const char* theText, int theData)
{
	Item* anItem = new Item();
	anItem->mText = theText;
	anItem->mData = theData;
	return AddItem(anItem);
}

// 0x427440
int ListBox::AddItem(Item* theItem)
{
	mItems.push_back(theItem);
	if (mSelected == -1)
		mSelected = 0;
	UpdateButtons();
	return (int)mItems.size() - 1;
}

// 0x4274a0
void ListBox::UpdateButtons()
{
	if (mScrolls)
	{
		bool anEnable = (unsigned int)mRows < (unsigned int)mItems.size() || mFixed < mTop;
		mUp->SetEnabled(anEnable);
		mDown->SetEnabled(anEnable);
	}
}

// 0x427510
void ListBox::Remove(int theIndex)
{
	delete mItems[theIndex];
	mItems.erase(mItems.begin() + theIndex);
	if (mItems.empty())
		mSelected = -1;
	int aCount = (int)mItems.size();
	if ((unsigned int)((mFixed - mRows) + aCount) <= (unsigned int)theIndex && mFixed < mTop)
	{
		mTop = mTop - 1;
		mSelected = mSelected - 1;
	}
	if ((unsigned int)mItems.size() <= (unsigned int)mSelected)
		mSelected = mSelected - 1;
	UpdateButtons();
}

// 0x427630 (the original disabled the scroll buttons even for a list without them, through pointers it never set;
// the port skips them)
void ListBox::ClearItems()
{
	for (Item* anItem : mItems)
		delete anItem;
	mItems.clear();
	mTop = mFixed;
	mSelected = -1;
	if (mUp != nullptr)
		mUp->SetEnabled(false);
	if (mDown != nullptr)
		mDown->SetEnabled(false);
}

// 0x4287a0: TextEdit::SetFocus. The text edit is not translated yet; until it is, nothing has the focus.
void SetObjectFocus(Object* theObject, bool theFocus)
{
	(void)theObject;
	(void)theFocus;
}

} // namespace jb
