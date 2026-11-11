#include "Widgets.h"

#include "EngineRes.h"
#include "Font.h"
#include "Host.h"
#include "Layer.h"
#include "Manager.h"

#include <algorithm>
#include <cstdlib>

namespace jb
{

// 0x4309f0
Slider::Slider(int theImage, int theMinX, int theMaxX) : Button(theImage, 2, false), mMinX(theMinX), mMaxX(theMaxX)
{
}

// 0x430a70: dragged along its row
bool Slider::OnLButtonDown(int theFlags, int theX, int theY)
{
	PlaySound(SOUND_CLICK);
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
	SetCursorKind(CURSOR_KIND_HAND, false);
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
	mFont = GetDllFont(theFontImage, theFontText);
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
		SetCursorKind(CURSOR_KIND_HAND, true);
		return;
	}
	SetCursorKind(CURSOR_KIND_ARROW, true);
}

// 0x427150
bool ListBox::OnLButtonDown(int theFlags, int theX, int theY)
{
	if (1 < theX && theX < mWidth - 2)
	{
		unsigned int aRow = (unsigned int)(theY / mRowHeight);
		if (!mItems.empty() && aRow < (unsigned int)mItems.size() && (int)aRow < mRows)
		{
			PlaySound(SOUND_CLICK);
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

// IsCharAlphaNumericA in the Western code page (1252)
static bool IsAlphaNumeric(uint8_t theChar)
{
	if ((theChar >= '0' && theChar <= '9') || (theChar >= 'A' && theChar <= 'Z') || (theChar >= 'a' && theChar <= 'z'))
		return true;
	switch (theChar)
	{
	case 0x83: case 0x8a: case 0x8c: case 0x8e: case 0x9a: case 0x9c: case 0x9e: case 0x9f:
	case 0xaa: case 0xb2: case 0xb3: case 0xb5: case 0xb9: case 0xba:
		return true;
	}
	return theChar >= 0xc0 && theChar != 0xd7 && theChar != 0xf7;
}

// GetKeyState(VK_SHIFT) and GetKeyState(VK_CONTROL)
static bool ShiftDown()
{
	return (GetHost()->InputFlags() & 4) != 0;
}

static bool ControlDown()
{
	return (GetHost()->InputFlags() & 8) != 0;
}

// 0x427920
TextEdit::TextEdit(int theWidth, int theHeight, int theMaxLength, bool theUnderline)
	: mWidth(theWidth), mHeight(theHeight), mMaxLength(theMaxLength), mUnderline(theUnderline)
{
	SetRect(mFrame, -4, -2, theWidth + 4, theHeight + 4);
}

// 0x427af0
void TextEdit::SetFrame(uint32_t theColour1, uint32_t theColour2, uint32_t theColour3)
{
	mFrameColour1 = theColour1;
	mFramed = true;
	mFrameColour2 = theColour2;
	mFrameColour3 = theColour3;
}

// 0x427b20
void TextEdit::SetFont(int theFontImage, int theFontText, int theTextY, int theDisabledAlpha)
{
	mFont = GetDllFont(theFontImage, theFontText);
	mTextY = theTextY;
	mDisabledAlpha = theDisabledAlpha;
}

// 0x427b60: the scroll follows the cursor with five characters to spare. The selection's end is clamped to the last
// character shown plus the scroll a second time, as in the original.
void TextEdit::Draw()
{
	if (!IsVisible())
		return;
	if (mUnderline)
	{
		Rect aRect;
		aRect.bottom = mHeight;
		aRect.right = mWidth;
		aRect.top = aRect.bottom - 1;
		aRect.left = 0;
		FillRect(aRect, 0xffffff, true);
	}
	if (mFramed)
	{
		DrawFrame(mFrame, mFrameColour3, true);
		OffsetRect(mFrame, -1, -1);
		DrawFrame(mFrame, mFrameColour1, true);
		OffsetRect(mFrame, -1, -1);
		DrawFrame(mFrame, mFrameColour2, true);
		OffsetRect(mFrame, 2, 2);
	}
	if (mCursor < mScroll)
	{
		mScroll = mCursor - 5;
		if (mScroll < 0)
			mScroll = 0;
	}
	int aLast = mFont->FitCount(Mid(mText, mScroll).c_str(), mWidth) + mScroll;
	if (aLast < mCursor)
	{
		int anEnd = mCursor + 5;
		if ((int)mText.size() < anEnd)
			anEnd = (int)mText.size();
		std::string aBefore = Mid(mText, 0, anEnd);
		std::reverse(aBefore.begin(), aBefore.end());
		mScroll = anEnd - mFont->FitCount(aBefore.c_str(), mWidth);
	}
	if (IsEnabled() && mFocused)
	{
		if (mAnchor != -1)
		{
			int aStart = mAnchor;
			int anEnd = mCursor;
			if (anEnd < aStart)
			{
				aStart = mCursor;
				anEnd = mAnchor;
			}
			int aScroll = mScroll;
			if (aStart < aScroll)
				aStart = aScroll;
			aLast = aScroll + aLast;
			if (aLast < anEnd)
				anEnd = aLast;
			int aLeft = mFont->GetWidth(Mid(mText, aScroll, aStart - aScroll).c_str(), 100);
			int aRight = mFont->GetWidth(Mid(mText, mScroll, anEnd - mScroll).c_str(), 100);
			if (aLeft != aRight)
			{
				Rect aRect;
				aRect.bottom = mHeight;
				aRect.left = aLeft - 1;
				aRect.top = 0;
				aRect.right = aRight;
				FillRect(aRect, mSelColour, true);
			}
		}
		if (mFocused && mCaretOn)
		{
			int x = mFont->GetWidth(Mid(mText, mScroll, mCursor - mScroll).c_str(), 100);
			Rect aRect;
			aRect.bottom = mHeight;
			aRect.left = x - 1;
			aRect.top = 0;
			aRect.right = x;
			FillRect(aRect, 0xffffff, true);
		}
	}
	if (mScroll != 0)
	{
		int anAlpha = 0xff;
		if (!IsEnabled())
			anAlpha = mDisabledAlpha;
		mFont->DrawLeft(this, 0, mTextY, Mid(mText, mScroll).c_str(), mWidth, 100, 100, anAlpha);
		return;
	}
	int anAlpha = 0xff;
	if (!IsEnabled())
		anAlpha = mDisabledAlpha;
	mFont->DrawLeft(this, 0, mTextY, mText.c_str(), mWidth, 100, 100, anAlpha);
}

// 0x427fa0: the caret blinks every half second
void TextEdit::Init()
{
	int aPeriod = 500 / mManager->GetPeriod();
	mBlinkPeriod = aPeriod;
	mBlinkCount = aPeriod;
	SetCursorKind(CURSOR_KIND_TEXT, false);
	RegChar(true);
	RegKey(true);
	RegUpdate(true);
	RegClick(true);
	RegHover(true);
	SetUserFlag(true);
}

// 0x428010
bool TextEdit::Update()
{
	if (mFocused && IsActive())
	{
		mBlinkCount = mBlinkCount - 1;
		if (mBlinkCount == 0)
		{
			mBlinkCount = mBlinkPeriod;
			mCaretOn = !mCaretOn;
			return true;
		}
	}
	return false;
}

// 0x428060: ^C copies, Enter goes to the layer, ^V pastes, ^X cuts, ^Z undoes; a character of the font is typed
void TextEdit::OnChar(int theChar, int theRepeat, int theScan)
{
	if (!mFocused)
		return;
	switch (theChar)
	{
	case 3:
		Copy();
		return;
	default:
	{
		uint8_t aChar = (uint8_t)theChar;
		if (!mFont->HasChar(aChar))
			return;
		std::string aText(1, (char)aChar);
		if (!Fits(aText))
			return;
		RecordUndo(2, 0);
		DeleteSelection();
		Insert(mText, mCursor, (char)aChar);
		mCursor = mCursor + 1;
		break;
	}
	case 0xd:
		mLayer->OnTextEnter();
		return;
	case 0x16:
		Paste();
		return;
	case 0x18:
		Cut();
		return;
	case 0x1a:
		Undo();
	}
	Changed();
}

// 0x428210: Backspace, Delete (with Shift: as Backspace, or a cut), End, Home, the arrows (Ctrl: by words; Shift:
// selecting) and Insert (Ctrl: copy, Shift: paste). Never taken, so the other objects get the key too.
bool TextEdit::OnKeyDown(int theKey, int theRepeat, int theScan)
{
	if (!mFocused)
		return false;
	bool aChanged = false;
	if (theKey == 0x27 || theKey == 0x25 || theKey == 0x24 || theKey == 0x23)
	{
		int anAnchor;
		if (ShiftDown())
		{
			if (mAnchor != -1)
				goto keys;
			anAnchor = mCursor;
		}
		else
		{
			anAnchor = -1;
			if (mAnchor != -1)
				aChanged = true;
		}
		mAnchor = anAnchor;
	}
keys:
	int aPos;
	switch (theKey)
	{
	case 8:
	case 0x2e:
		if (theKey == 8 || ShiftDown())
		{
			if (mAnchor != -1)
			{
				if (theKey == 0x2e)
				{
					Cut();
					return false;
				}
				goto deleteSelection;
			}
			if (0 < mCursor)
			{
				RecordUndo(1, 0);
				Delete(mText, mCursor - 1, 1);
				mCursor = mCursor - 1;
				goto changed;
			}
		}
		else
		{
			if (mAnchor != -1)
			{
			deleteSelection:
				RecordUndo(1, 2);
				DeleteSelection();
				goto changed;
			}
			if (mCursor < (int)mText.size())
			{
				RecordUndo(1, 1);
				Delete(mText, mCursor, 1);
				goto changed;
			}
		}
		break;
	case 0x23:
		aPos = (int)mText.size();
	moved:
		mCursor = aPos;
		mUndoKind = 0;
		goto changed;
	case 0x24:
		mCursor = 0;
		mUndoKind = 0;
	changed:
		aChanged = true;
		break;
	case 0x25:
		if (0 < mCursor)
		{
			if (ControlDown())
				aPos = WordStart(mCursor);
			else
				aPos = mCursor - 1;
			goto moved;
		}
		break;
	case 0x27:
		if (mCursor < (int)mText.size())
		{
			if (ControlDown())
				aPos = WordEnd(mCursor);
			else
				aPos = mCursor + 1;
			goto moved;
		}
		break;
	case 0x2d:
		if (ControlDown())
			Copy();
		else if (ShiftDown())
		{
			Paste();
			return false;
		}
	}
	if (mAnchor == mCursor)
		mAnchor = -1;
	if (aChanged)
		Changed();
	return false;
}

// 0x428490: Shift extends the selection; the click takes the layer's focus and drags the cursor
bool TextEdit::OnLButtonDown(int theFlags, int theX, int theY)
{
	if (ShiftDown())
	{
		if (mAnchor == -1)
			mAnchor = mCursor;
	}
	else
		mAnchor = -1;
	mCursor = HitTest(theX);
	if (!mFocused)
		mLayer->SetFocus(this);
	mManager->StartDrag(this, 2, 0, 0, 0, 0);
	return true;
}

// 0x428500
void TextEdit::OnDragEnd(int theFlags, int theX, int theY)
{
	mCaretOn = true;
	Invalidate();
}

// 0x428510
void TextEdit::OnDrag(int theFlags, int theX, int theY)
{
	if (mAnchor == -1)
		mAnchor = mCursor;
	int aPos = HitTest(theX);
	if (mCursor != aPos)
	{
		mCursor = aPos;
		Invalidate();
	}
}

// 0x428550: the word is selected (with Shift, the selection grows to it)
bool TextEdit::OnLButtonDblClk(int theFlags, int theX, int theY)
{
	int aPos = HitTest(theX);
	int aStart = WordStart(aPos);
	int anEnd = WordEnd(aPos);
	if (ShiftDown())
	{
		if (mAnchor == -1)
			mAnchor = mCursor;
		if (aStart < mAnchor)
		{
			mCursor = aStart;
			Changed();
			return true;
		}
	}
	else
		mAnchor = aStart;
	mCursor = anEnd;
	Changed();
	return true;
}

// 0x4285d0: past the word, then past what follows it up to the next word
int TextEdit::WordEnd(int thePos)
{
	int aLength = (int)mText.size();
	int i = thePos;
	if (thePos < aLength)
	{
		while (true)
		{
			if (!IsAlphaNumeric((uint8_t)mText[i]))
				break;
			i = i + 1;
			if (aLength <= i)
				return i;
		}
		for (; i < aLength; i = i + 1)
		{
			if (IsAlphaNumeric((uint8_t)mText[i]))
				return i;
		}
	}
	return i;
}

// 0x428640: back over what precedes the word, then to the word's start
int TextEdit::WordStart(int thePos)
{
	int i = thePos - 1;
	int j = i;
	if (-1 < i)
	{
		while (true)
		{
			i = j;
			if (IsAlphaNumeric((uint8_t)mText[i]))
				break;
			j = i - 1;
			if (i - 1 < 0)
				return i;
		}
		for (; -1 < i; i = i - 1)
		{
			if (!IsAlphaNumeric((uint8_t)mText[i]))
				break;
		}
	}
	return i + 1;
}

// 0x4286b0: left of the edit is the character before the scroll; past a character's middle is the next one
int TextEdit::HitTest(int theX)
{
	int x = 0;
	if (theX < 0)
	{
		if (mScroll < 1)
			x = 0;
		else
			x = mScroll - 1;
	}
	else
	{
		int aCount;
		int* aWidths = mFont->GetWidths(Mid(mText, mScroll).c_str(), &aCount);
		int i = 0;
		if (0 < aCount)
		{
			do
			{
				if (theX < *aWidths / 2 + x)
					return mScroll + i;
				x = x + *aWidths;
				i = i + 1;
				aWidths = aWidths + 1;
			} while (i < aCount);
		}
		x = (int)mText.size();
	}
	return x;
}

// 0x4287a0
void TextEdit::SetFocus(bool theFocus)
{
	mFocused = theFocus;
	if (theFocus)
	{
		int aLength = (int)mText.size();
		mCursor = aLength;
		if (aLength == 0)
		{
			mAnchor = -1;
			Changed();
			return;
		}
		mAnchor = 0;
		Changed();
	}
}

// 0x4287f0
bool TextEdit::DeleteSelection()
{
	int anAnchor = mAnchor;
	if (anAnchor == -1)
		return false;
	int aCursor = mCursor;
	int aStart = anAnchor;
	if (aCursor <= anAnchor)
		aStart = aCursor;
	Delete(mText, aStart, std::abs(anAnchor - aCursor));
	mCursor = aStart;
	mAnchor = -1;
	return true;
}

// 0x428840
bool TextEdit::Copy()
{
	int anAnchor = mAnchor;
	if (anAnchor != -1)
	{
		int aStart = mCursor;
		int anEnd = anAnchor;
		if (anAnchor < aStart)
		{
			anEnd = aStart;
			aStart = anAnchor;
		}
		mManager->SetClipboard(Mid(mText, aStart, anEnd - aStart).c_str());
		return true;
	}
	return false;
}

// 0x4288e0: a new kind of edit keeps the text as it was; the counts say what an undo selects
void TextEdit::RecordUndo(int theKind, int theArg)
{
	if (mUndoKind != theKind)
	{
		mUndoText = mText;
		int anAnchor = mAnchor;
		mUndoKind = theKind;
		if (anAnchor == -1 || mCursor <= anAnchor)
			mUndoPos = mCursor;
		else
			mUndoPos = anAnchor;
		mUndoInserted = 0;
		mUndoDeleted = 0;
	}
	if (theKind == 1)
	{
		if (theArg == 0)
		{
			mUndoPos = mUndoPos - 1;
			mUndoDeleted = mUndoDeleted + 1;
			return;
		}
		if (theArg == 1)
		{
			mUndoDeleted = mUndoDeleted + 1;
			return;
		}
		if (theArg == 2 && mAnchor != -1)
		{
			mUndoDeleted = mUndoDeleted + std::abs(mCursor - mAnchor);
			return;
		}
	}
	else
	{
		if (theKind == 2)
		{
			if (mAnchor != -1)
				mUndoDeleted = mUndoDeleted + std::abs(mCursor - mAnchor);
			mUndoInserted = mUndoInserted + 1;
			return;
		}
		if (theKind == 3)
		{
			if (mAnchor != -1)
				mUndoDeleted = mUndoDeleted + std::abs(mCursor - mAnchor);
			mUndoInserted = mUndoInserted + theArg;
		}
	}
}

// 0x428a40: the texts swap, so a second undo redoes
void TextEdit::Undo()
{
	std::string aText = mText;
	mText = mUndoText;
	mUndoText = aText;
	int aDeleted = mUndoDeleted;
	int aPos = mUndoPos;
	if (aDeleted < 1)
	{
		mCursor = aPos;
		mAnchor = -1;
	}
	else
	{
		mAnchor = aPos;
		mCursor = aPos + aDeleted;
	}
	int anInserted = mUndoInserted;
	mUndoInserted = aDeleted;
	mUndoDeleted = anInserted;
	mUndoKind = -1;
}

// 0x428b00
void TextEdit::Changed()
{
	mCaretOn = true;
	Invalidate();
	OnChanged(mText.c_str());
}

// 0x428b30
bool TextEdit::Cut()
{
	if (!Copy())
		return false;
	RecordUndo(1, 2);
	DeleteSelection();
	Changed();
	return true;
}

// 0x428b70: only the clipboard's characters that the font has
bool TextEdit::Paste()
{
	std::string aText;
	if (!mManager->GetClipboard(aText))
		return false;
	mFont->Filter(aText);
	if (!Fits(aText))
		return false;
	RecordUndo(3, (int)aText.size());
	DeleteSelection();
	Insert(mText, mCursor, aText.c_str());
	mCursor = mCursor + (int)aText.size();
	Changed();
	return true;
}

// 0x428c90 (the width is measured with theText at the end, wherever the cursor is)
bool TextEdit::Fits(const std::string& theText)
{
	int aLength = (int)theText.size() + (int)mText.size();
	if (mAnchor != -1)
		aLength = aLength - std::abs(mAnchor - mCursor);
	if (mMaxLength < aLength)
		return false;
	std::string aRest = mText;
	if (mAnchor != -1)
	{
		int aStart = mAnchor;
		if (mCursor <= mAnchor)
			aStart = mCursor;
		Delete(aRest, aStart, std::abs(mAnchor - mCursor));
	}
	if (mWidth <= mFont->GetWidth((aRest + theText).c_str(), 100))
		return false;
	return true;
}

// 0x4287a0 through a layer's focus list
void SetObjectFocus(Object* theObject, bool theFocus)
{
	((TextEdit*)theObject)->SetFocus(theFocus);
}

} // namespace jb
