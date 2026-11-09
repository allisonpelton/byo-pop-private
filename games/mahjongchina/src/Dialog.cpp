#include "Dialog.h"

#include "App.h"
#include "Font.h"
#include "Manager.h"

#include <cstdio>

namespace mahjongchina
{

using namespace jb;

// 0x403420: "<common folder><slot>.bmp" and ".txt"
void GetSlotFiles(std::string& theBmp, std::string& theTxt, int theSlot)
{
	char aName[512];
	snprintf(aName, sizeof(aName), "%s%d.", GetCommonFolder().c_str(), theSlot);
	theBmp = aName;
	theTxt = theBmp;
	theTxt += "txt";
	theBmp += "bmp";
}

// 0x4034c0
YesNoBox::YesNoBox(DialogListener* theListener, int theSize, int theId, int theYesText, int theNoText,
	bool theImageButtons, int theYesWidth, int theNoWidth)
	: Dialog(0xce, 0x96), mListener(theListener), mSize(theSize), mId(theId), mYesText(theYesText),
	  mNoText(theNoText), mImageButtons(theImageButtons), mYesWidth(theYesWidth), mNoWidth(theNoWidth)
{
}

// 0x403550: the frame grows with the size; yes on the left, no on the right
void YesNoBox::Create()
{
	int aTop[] = {2, 2, 3, 0};
	int aBottom[] = {3, 2, 2, 0};
	int aLeft[3];
	int aRight[3];
	int y;
	if (mSize == 0)
	{
		aLeft[0] = 1;
		aLeft[1] = 0;
		aRight[0] = 1;
		aRight[1] = 0;
		y = 0x74;
	}
	else
	{
		aLeft[0] = 2;
		if (mSize == 1)
		{
			aLeft[1] = 0;
			aRight[0] = 2;
			aRight[1] = 0;
			y = 0x8e;
		}
		else
		{
			aLeft[1] = 2;
			aRight[0] = 2;
			aRight[1] = 2;
			y = 0xc8;
		}
	}
	aLeft[2] = 0;
	aRight[2] = 0;
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* aNo = nullptr;
	if (!mImageButtons)
	{
		DialogButton* aYes = new DialogButton(1, mYesWidth);
		Add(aYes, 0xbe - mYesWidth, y);
		aYes->SetText(mYesText);
		aNo = new DialogButton(0, mNoWidth);
	}
	else
	{
		DialogButton* aYes = new DialogButton(1);
		Add(aYes, 0x69, y);
		aYes->SetText(mYesText);
		aNo = new DialogButton(0);
	}
	Add(aNo, 0xc6, y);
	aNo->SetText(mNoText);
}

// 0x403750: without a listener, yes does nothing
bool YesNoBox::OnCommand(int theCmd)
{
	if (theCmd == 0)
	{
		if (mListener != nullptr && !mListener->OnNo(mId))
			return true;
		gManager->Pop(true, 1);
	}
	else
	{
		if (theCmd != 1)
			return false;
		if (mListener != nullptr && mListener->OnYes(mId))
		{
			gManager->Pop(true, 1);
			return true;
		}
	}
	return true;
}

// 0x4037b0
DialogButton::DialogButton(int theCmd) : Button(0x129, 4, false), mImage(true)
{
	SetCmd(theCmd);
}

// 0x403830: the strip is 0x2e + 8 * n + 0x30 wide
DialogButton::DialogButton(int theCmd, int theWidth) : Button(theCmd, 4, true), mImage(false)
{
	if (theWidth < 0x66)
	{
		mMiddles = 1;
		return;
	}
	mMiddles = (theWidth - 0x66) / 8 + 1;
}

// 0x403890
void DialogButton::SetText(int theStringId)
{
	if (!mImage)
	{
		SetLabel(theStringId, 0xa0, 0x9f, 0, mMiddles * 8 + 0x5e, 0x18, 2, 2, 0xff);
		return;
	}
	SetLabel(theStringId, 0xa0, 0x9f, 0, GetFrameWidth(), 0x17, 2, 2, 0xff);
}

// 0x403900
void DialogButton::DrawNormal()
{
	if (!mImage)
	{
		DrawStrip(0);
		DrawLabel();
		return;
	}
	Button::DrawNormal();
}

// 0x403930
void DialogButton::DrawHover()
{
	if (!mImage)
	{
		DrawStrip(0x66);
		DrawLabel();
		return;
	}
	Button::DrawHover();
}

// 0x403960
void DialogButton::DrawPressed()
{
	if (!mImage)
	{
		DrawStrip(0xcc);
		DrawLabelPressed();
		return;
	}
	Button::DrawPressed();
}

// 0x403990
void DialogButton::DrawDisabled()
{
	if (!mImage)
	{
		DrawStrip(0x132);
		DrawLabelDisabled();
		return;
	}
	Button::DrawDisabled();
}

// 0x4039c0: the frame's left end, its middle repeated, its right end
void DialogButton::DrawStrip(int theLeft)
{
	Rect aRect;
	aRect.left = theLeft;
	aRect.top = 0;
	aRect.bottom = 0x2a;
	aRect.right = theLeft + 0x2e;
	DrawImage(0, 0, 0x11f, &aRect, true, 100, 100, 0xff);
	aRect.right = theLeft + 0x36;
	int x = 0x2e;
	int i = 0;
	aRect.top = 0;
	aRect.bottom = 0x2a;
	aRect.left = theLeft + 0x2e;
	if (0 < mMiddles)
	{
		do
		{
			DrawImage(x, 0, 0x11f, &aRect, true, 100, 100, 0xff);
			x = x + 8;
			i = i + 1;
		} while (i < mMiddles);
	}
	aRect.left = theLeft + 0x36;
	aRect.right = theLeft + 0x66;
	aRect.top = 0;
	aRect.bottom = 0x2a;
	DrawImage(x, 0, 0x11f, &aRect, true, 100, 100, 0xff);
}

// 0x403ab0
DialogFrame::DialogFrame()
{
}

// 0x403bc0
DialogFrame::~DialogFrame()
{
	for (int i = 0; i < 4; i++)
	{
		if (mEdges[i] != nullptr)
			delete[] mEdges[i];
	}
}

// 0x403ca0
void DialogFrame::SetAlwaysUpdate(bool theOn)
{
	mAlwaysUpdate = theOn;
	RegUpdate(theOn);
}

// 0x403cc0: the background, the corners (image 0x115 large, 0x11a small) with the edge pieces between them (0x116
// across, 0x114 down), the title and subtitle, the texts and the message
void DialogFrame::Draw()
{
	if (!mLaidOut)
		Layout();
	DrawImage(mLeft, mTop, 0x118, nullptr, true, mScaleX, mScaleY, 0xff);
	Rect aRect;
	int aCorner;
	int anImage;
	int aLeft = 0;  // China: the dragon corner (2) puts the left edge in by 0x43
	int x;
	if (mCorners[0] == 2)
	{
		aRect.left = 0;
		aRect.top = 0;
		aRect.right = 0x43;
		aRect.bottom = 0x58;
		DrawImage(0, 0xc, 0x117, &aRect, true, 100, 100, 0xff);
		aCorner = 0x40;
		aRect.left = 0x80;
		aRect.top = 0x40;
		aRect.right = 0xc0;
		aRect.bottom = 0x80;
		DrawImage(0x43, 0, 0x11a, &aRect, true, 100, 100, 0xff);
		aLeft = 0x43;
		x = 0x83;
	}
	else
	{
		aRect.left = 0;
		aRect.top = 0;
		if (mCorners[0] == 1)
		{
			aCorner = 0x40;
			aRect.right = 0x40;
			aRect.bottom = 0x40;
			anImage = 0x11a;
		}
		else
		{
			aCorner = 0x54;
			aRect.right = 0x54;
			aRect.bottom = 0x54;
			anImage = 0x115;
		}
		DrawImage(0, 0, anImage, &aRect, true, 100, 100, 0xff);
		x = aCorner;
	}
	int i = 0;
	if (0 < mEdgeCounts[2])
	{
		do
		{
			int aKind = mEdges[2][i];
			if (aKind == 1)
			{
				aRect.left = 0xa6;
				aRect.top = 0;
				aRect.right = 0xd6;
				aRect.bottom = 0xb;
				DrawImage(x, 0xc, 0x116, &aRect, true, 100, 100, 0xff);
				x = x + 0x30;
			}
			else if (aKind == 2)
			{
				aRect.left = 0x5f;
				aRect.top = 0;
				aRect.right = 0xa6;
				aRect.bottom = 0xb;
				DrawImage(x, 0xc, 0x116, &aRect, true, 100, 100, 0xff);
				x = x + 0x47;
			}
			else
			{
				aRect.left = 0;
				aRect.top = 0;
				aRect.right = 0x5f;
				aRect.bottom = 0xb;
				DrawImage(x, 0xc, 0x116, &aRect, true, 100, 100, 0xff);
				x = x + 0x5f;
			}
			i = i + 1;
		} while (i < mEdgeCounts[2]);
	}
	int aRightY;
	if (mCorners[1] == 2)
	{
		aRect.left = 0x80;
		aRect.top = 0;
		aRect.right = 0xc0;
		aRect.bottom = 0x40;
		DrawImage(x, 0, 0x11a, &aRect, true, 100, 100, 0xff);
		x = x + 0x40;
		aRect.left = 0x43;
		aRect.top = 0;
		aRect.right = 0x86;
		aRect.bottom = 0x58;
		DrawImage(x, 0xc, 0x117, &aRect, true, 100, 100, 0xff);
		aRightY = 0x40;
	}
	else if (mCorners[1] == 1)
	{
		aRect.left = 0x40;
		aRect.top = 0;
		aRect.right = 0x80;
		aRect.bottom = 0x40;
		DrawImage(x, 0, 0x11a, &aRect, true, 100, 100, 0xff);
		x = x + 0x40;
		aRightY = 0x40;
	}
	else
	{
		aRect.left = 0x54;
		aRect.bottom = 0x54;
		aRect.top = 0;
		aRect.right = 0xa8;
		DrawImage(x, 0, 0x115, &aRect, true, 100, 100, 0xff);
		x = x + 0x54;
		aRightY = 0x54;
	}
	int y = aCorner;
	i = 0;
	if (0 < mEdgeCounts[0])
	{
		do
		{
			int aKind = mEdges[0][i];
			if (aKind == 1)
			{
				aRect.top = 0;
				aRect.left = 0x18;
				aRect.right = 0x24;
				aRect.bottom = 0x2a;
				DrawImage(aLeft + 0x12, y, 0x114, &aRect, true, 100, 100, 0xff);
				y = y + 0x2a;
			}
			else if (aKind == 2)
			{
				aRect.left = 0xc;
				aRect.top = 0;
				aRect.right = 0x18;
				aRect.bottom = 0x3f;
				DrawImage(aLeft + 0x12, y, 0x114, &aRect, true, 100, 100, 0xff);
				y = y + 0x3f;
			}
			else
			{
				aRect.left = 0;
				aRect.top = 0;
				aRect.right = 0xc;
				aRect.bottom = 0x54;
				DrawImage(aLeft + 0x12, y, 0x114, &aRect, true, 100, 100, 0xff);
				y = y + 0x54;
			}
			i = i + 1;
		} while (i < mEdgeCounts[0]);
	}
	x = x - 0x1e;
	i = 0;
	if (0 < mEdgeCounts[1])
	{
		do
		{
			int aKind = mEdges[1][i];
			if (aKind == 1)
			{
				aRect.top = 0;
				aRect.left = 0x18;
				aRect.right = 0x24;
				aRect.bottom = 0x2a;
				DrawImage(x, aRightY, 0x114, &aRect, true, 100, 100, 0xff);
				aRightY = aRightY + 0x2a;
			}
			else if (aKind == 2)
			{
				aRect.left = 0xc;
				aRect.top = 0;
				aRect.right = 0x18;
				aRect.bottom = 0x3f;
				DrawImage(x, aRightY, 0x114, &aRect, true, 100, 100, 0xff);
				aRightY = aRightY + 0x3f;
			}
			else
			{
				aRect.left = 0;
				aRect.top = 0;
				aRect.right = 0xc;
				aRect.bottom = 0x54;
				DrawImage(x, aRightY, 0x114, &aRect, true, 100, 100, 0xff);
				aRightY = aRightY + 0x54;
			}
			i = i + 1;
		} while (i < mEdgeCounts[1]);
	}
	aRect.left = 0;
	if (mCorners[2] == 1)
	{
		x = aLeft + 0x40;
		aRect.top = 0x40;
		aRect.right = 0x40;
		aRect.bottom = 0x80;
		DrawImage(aLeft, y, 0x11a, &aRect, true, 100, 100, 0xff);
		y = y + 0x40;
	}
	else
	{
		aRect.top = 0x54;
		aRect.right = 0x54;
		aRect.bottom = 0xa8;
		DrawImage(aLeft, y, 0x115, &aRect, true, 100, 100, 0xff);
		x = aLeft + 0x54;
		y = y + 0x54;
	}
	i = 0;
	if (0 < mEdgeCounts[3])
	{
		do
		{
			int aKind = mEdges[3][i];
			if (aKind == 1)
			{
				aRect.left = 0xa6;
				aRect.top = 0xb;
				aRect.right = 0xd6;
				aRect.bottom = 0x16;
				DrawImage(x, y - 0x19, 0x116, &aRect, true, 100, 100, 0xff);
				x = x + 0x30;
			}
			else if (aKind == 2)
			{
				aRect.left = 0x5f;
				aRect.top = 0xb;
				aRect.right = 0xa6;
				aRect.bottom = 0x16;
				DrawImage(x, y - 0x19, 0x116, &aRect, true, 100, 100, 0xff);
				x = x + 0x47;
			}
			else
			{
				aRect.left = 0;
				aRect.top = 0xb;
				aRect.right = 0x5f;
				aRect.bottom = 0x16;
				DrawImage(x, y - 0x19, 0x116, &aRect, true, 100, 100, 0xff);
				x = x + 0x5f;
			}
			i = i + 1;
		} while (i < mEdgeCounts[3]);
	}
	if (mCorners[3] == 1)
	{
		aRect.left = 0x40;
		aRect.top = 0x40;
		aRect.right = 0x80;
		aRect.bottom = 0x80;
		anImage = 0x11a;
		y = y - 0x40;
	}
	else
	{
		aRect.right = 0xa8;
		aRect.bottom = 0xa8;
		aRect.left = 0x54;
		aRect.top = 0x54;
		anImage = 0x115;
		y = y - 0x54;
	}
	DrawImage(x, y, anImage, &aRect, true, 100, 100, 0xff);
	if (!mTitle.empty())
		mTitleFont->DrawLinesCentred(this, mLeft, 0x32, mTitle.c_str(), mRight - mLeft,
			mTitleFont->GetLineHeight() - 5, 100, 100, 0xff);
	i = 0;
	if (0 < mTextCount)
	{
		Text* aText = mTexts;
		do
		{
			Font* aFont;
			if (!aText->mAltFont)
				aFont = mTextFont;
			else
				aFont = mAltFont;
			switch (aText->mKind)
			{
			case 0:
				aFont->DrawLinesCentred(this, mLeft, aText->mY, aText->mText.c_str(), mRight - mLeft,
					aFont->GetLineHeight() - 1, 100, 100, 0xff);
				break;
			case 1:
				aFont->DrawLines(this, aText->mX, aText->mY, aText->mText.c_str(), aFont->GetLineHeight() - 1,
					100, 100, 0xff);
				break;
			}
			i = i + 1;
			aText = aText + 1;
		} while (i < mTextCount);
	}
	if (0 < mMessageSeconds)
		mSubtitleFont->DrawLeft(this, mMessageX, mMessageY, mMessage.c_str(), -1, 100, 100, 0xff);
	if (mClickCmd != -1)
	{
		static const Rect kScreen = {0, 0, 800, 600};  // 0x51c820
		AddDirty(kScreen);
	}
}

// 0x4045e0
void DialogFrame::Init()
{
	mSubtitleFont = GetDllFont(0x11c, 0x11b);
	mTitleFont = GetDllFont(0x11e, 0x11d);
	mTextFont = GetDllFont(0x122, 0x121);
	mAltFont = GetDllFont(0x133, 0x132);
}

// 0x404650: the message's time (its first update starts it)
bool DialogFrame::Update()
{
	if (0 < mMessageSeconds)
	{
		if (mMessageStart == 0)
		{
			mMessageStart = GetTime();
			return true;
		}
		int aNow = GetTime();
		int aSeconds = mMessageSeconds;
		if ((unsigned int)(aSeconds * 1000) < (unsigned int)(aNow - mMessageStart))
		{
			mMessageSeconds = 0;
			return true;
		}
	}
	return mAlwaysUpdate;
}

// 0x404720
void DialogFrame::SetCorners(int theTopLeft, int theTopRight, int theBottomLeft, int theBottomRight)
{
	mCorners[1] = theTopRight;
	mCorners[0] = theTopLeft;
	mCorners[2] = theBottomLeft;
	mCorners[3] = theBottomRight;
	// China: a top corner of kind 2 has a dragon, an animation the frame updates
	if (theTopLeft == 2)
	{
		mDragons[0] = new jb::Animation();
		Attach(mDragons[0]);
	}
	if (mCorners[1] == 2)
	{
		mDragons[1] = new jb::Animation();
		Attach(mDragons[1]);
	}
	if (mCorners[0] == 2 || mCorners[1] == 2)
		RegUpdate(true);
}

// 0x404750
void DialogFrame::SetEdges(const int* theLeft, const int* theRight, const int* theTop, const int* theBottom)
{
	mEdgeCounts[0] = CopyEdgeList(mEdges[0], theLeft);
	mEdgeCounts[1] = CopyEdgeList(mEdges[1], theRight);
	mEdgeCounts[2] = CopyEdgeList(mEdges[2], theTop);
	mEdgeCounts[3] = CopyEdgeList(mEdges[3], theBottom);
}

// 0x4047e0
void DialogFrame::AddText(const char* theText, int theKind, int theX, int theY, bool theAltFont)
{
	Text& aText = mTexts[mTextCount];
	aText.mX = theX;
	aText.mY = theY;
	aText.mText = theText;
	aText.mAltFont = theAltFont;
	aText.mKind = theKind;
	mTextCount = mTextCount + 1;
}

// 0x4048e0
void DialogFrame::ShowMessage(int theX, int theY, const char* theText, int theSeconds)
{
	mMessageY = theY;
	mMessageX = theX;
	mMessage = theText;
	mMessageSeconds = theSeconds;
	mMessageStart = 0;
	RegUpdate(true);
}

// 0x404930
int CopyEdgeList(int*& theList, const int* theSource)
{
	if (theList != nullptr)
		delete[] theList;
	int aCount = 0;
	int aKind = *theSource;
	const int* p = theSource;
	while (aKind != 0)
	{
		p = p + 1;
		aCount = aCount + 1;
		aKind = *p;
	}
	theList = new int[aCount];
	int i = 0;
	if (0 < aCount)
	{
		do
		{
			theList[i] = theSource[i];
			i = i + 1;
		} while (i < aCount);
	}
	return aCount;
}

// 0x404990
int DialogFrame::GetTextWidth(const char* theText)
{
	return mTextFont->GetWidth(theText, 100);
}

// 0x4049b0: a top left corner of neither size makes a wider frame
void DialogFrame::Layout()
{
	mWidth = 0;
	mHeight = 0;
	int aCorner;
	if (mCorners[0] == 0)
		aCorner = 0x54;
	else
	{
		if (mCorners[0] != 1)
		{
			if (mDragons[0] != nullptr)
			{
				mDragons[0]->SetImage(0x16, 100, 0x120, 0x16);
				mDragons[0]->Setup(100, 10, 4, 4, 100, -1, 0, true);
			}
			mWidth = 0x83;
			mHeight = 0x40;
			mLeft = 0x5f;
			goto edges;
		}
		aCorner = 0x40;
	}
	mWidth = aCorner;
	mHeight = aCorner;
	mLeft = 0x1c;
edges:
	int aCount = mEdgeCounts[0];
	mTop = 0x15;
	if (0 < aCount)
	{
		int* p = mEdges[0];
		do
		{
			int aHeight;
			if (*p == 3)
				aHeight = mHeight + 0x54;
			else if (*p == 2)
				aHeight = mHeight + 0x3f;
			else
				aHeight = mHeight + 0x2a;
			p = p + 1;
			aCount = aCount - 1;
			mHeight = aHeight;
		} while (aCount != 0);
	}
	aCount = mEdgeCounts[2];
	if (0 < aCount)
	{
		int* p = mEdges[2];
		do
		{
			int aWidth;
			if (*p == 3)
				aWidth = mWidth + 0x5f;
			else if (*p == 2)
				aWidth = mWidth + 0x47;
			else
				aWidth = mWidth + 0x30;
			p = p + 1;
			aCount = aCount - 1;
			mWidth = aWidth;
		} while (aCount != 0);
	}
	if (mCorners[1] == 0)
		mWidth = mWidth + 0x54;
	else if (mCorners[1] == 1)
		mWidth = mWidth + 0x40;
	else
	{
		int aWidth = mWidth;
		mWidth = aWidth + 0x40;
		if (mDragons[1] != nullptr)
		{
			mDragons[1]->SetImage(aWidth + 0x57, 100, 0x120, 0x16);
			mDragons[1]->Setup(100, 10, 4, 4, 100, -1, 0, true);
		}
	}
	if (mCorners[2] == 0)
		mHeight = mHeight + 0x54;
	else
		mHeight = mHeight + 0x40;
	mRight = mWidth - 0x19;
	mBottom = mHeight - 0x13;
	mScaleX = ((mRight - mLeft) * 100) / 256;
	mScaleY = ((mBottom - mTop) * 100) / 256;
	// China: the dragon's corner on the right widens the frame after the background is placed
	if (mCorners[1] == 2)
		mWidth = mWidth + 0x83;
	mLaidOut = true;
}

// 0x404b60
bool DialogFrame::OnLButtonDown(int theFlags, int theX, int theY)
{
	((Dialog*)mLayer)->SendCommand(mClickCmd);
	return true;
}

// 0x404b80
void DialogFrame::SetClickCmd(int theCmd)
{
	mClickCmd = theCmd;
	RegClick(true);
}

// 0x404ba0
void DialogFrame::GetSize(int* theSize)
{
	if (!mLaidOut)
		Layout();
	theSize[0] = mWidth;
	theSize[1] = mHeight;
}

// 0x404bd0
Dialog::Dialog(int theX, int theY) : Layer(theX, theY)
{
}

// 0x404c40: the frame is the first object
void Dialog::Load()
{
	mFrame = new DialogFrame();
	Add(mFrame, 0, 0);
	Create();
}

// 0x404cf0
void Dialog::SetTitle(int theStringId)
{
	std::string aTitle = LoadDllString(theStringId);
	mFrame->SetTitle(aTitle.c_str());
}

// 0x404d70
void Dialog::AddText(int theStringId, int theKind, int theX, int theY, bool theAltFont)
{
	std::string aText = LoadDllString(theStringId);
	mFrame->AddText(aText.c_str(), theKind, theX, theY, theAltFont);
}

// 0x404e30
void Dialog::ShowMessage(int theX, int theY, int theStringId, int theSeconds)
{
	std::string aText = LoadDllString(theStringId);
	mFrame->ShowMessage(theX, theY, aText.c_str(), theSeconds);
}

} // namespace mahjongchina
