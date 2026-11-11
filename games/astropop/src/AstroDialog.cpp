// AstroDialog: the game's dialog box (see AstroDialog.h).

#include "AstroDialog.h"
#include "AstroPopApp.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x489c60 and its setup 0x483ee0. The base class is made with the box picture and the button picture of the game;
// the buttons are hidden until the box has come in.
AstroDialog::AstroDialog(AstroPopApp* theApp, int theId, const std::string& theHeader, int theButtonMode, bool theColoured,
						 const Color& theInsetColor, const Color& theButtonColor) :
	Text2004<Dialog>(IMAGE_DIALOG, IMAGE_DIALOG_BTN, theId, true, theHeader, std::string(), std::string(), theButtonMode)
{
	mImageTop = 0;
	mInsetImage = NULL;
	mButtonImage = NULL;
	mPicture = NULL;
	Setup(theApp, theColoured, theInsetColor, theButtonColor);
}

// 0x489e20: the same with a picture for the title and nothing written in the header, lines or footer
AstroDialog::AstroDialog(AstroPopApp* theApp, int theId, Image* thePicture, int theImageTop, int theButtonMode, bool theColoured,
						 const Color& theInsetColor, const Color& theButtonColor) :
	Text2004<Dialog>(IMAGE_DIALOG, IMAGE_DIALOG_BTN, theId, true, std::string(), std::string(), std::string(), theButtonMode)
{
	mInsetImage = NULL;
	mButtonImage = NULL;
	mPicture = thePicture;
	mImageTop = theImageTop;
	Setup(theApp, theColoured, theInsetColor, theButtonColor);
	m110 = theImageTop;
}

// 0x483ee0
void AstroDialog::Setup(AstroPopApp* theApp, bool theColoured, const Color& theInsetColor, const Color& theButtonColor)
{
	mApp = theApp;
	mInsetColor = theInsetColor;
	mButtonColor = theButtonColor;
	mColoured = theColoured;
	mCursorEnabled = true;

	if (mYesButton != NULL)
	{
		mYesButton->SetVisible(false);
		mYesButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
		mYesButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
		mYesButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
	}
	if (mColoured)
	{
		theApp->MakeDialogImages(mInsetColor, mButtonColor, &mInsetImage, &mButtonImage, false);
		if (mYesButton != NULL)
		{
			mYesButton->mComponentImage = mButtonImage;
			mButtonHeight = mButtonImage->mHeight;	// see NewUserDialog
		}
		mComponentImage = mInsetImage;
	}
	if (mNoButton != NULL)
	{
		mNoButton->SetVisible(false);
		mNoButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
		mNoButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
		mNoButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
		if (mColoured)
			mNoButton->mComponentImage = mButtonImage;
	}

	mSettled = false;
	mClip = false;
	mTypedChars = 0;
	mTypedLine = 0;
	mTypeSpeed = 0;
	mTyping = false;
	mState = 8;
	mCursorOn = true;
	// (the members the original leaves to the first Resize)
	m110 = 0;
	m114 = 0;
	m118 = 0;
	m11c = 0;
	m120 = 0;
	m124 = 0;
}

// 0x48a640 (body 0x48a030)
AstroDialog::~AstroDialog()
{
	if (mColoured)
	{
		delete mInsetImage;
		delete mButtonImage;
	}
}

int AstroDialog::HeaderWidth()
{
	if (mPicture == NULL)
		return mHeaderFont->StringWidth(mDialogHeader);
	return mPicture->mWidth;
}

// 0x483640: the title and the box are put where the box starts: the box below the screen, the title with it
void AstroDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	if (!mSettled)
	{
		m110 = mContentInsets.mTop + mBackgroundInsets.mTop + mImageTop;
		if (mPicture == NULL)
			m110 += mHeaderFont->GetAscent() - mHeaderFont->GetAscentPadding();
		else
			m110 = mImageTop;
		m114 = 0;
		m120 = 0;
		m124 = 0x1e0;
		m11c = mWidth / 2 - HeaderWidth() / 2;
		m118 = m114 - 0x1e0 + m110;
	}
}

// 0x4840c0: the height the box wants for its text at a width (at least 0x140); typed lines take a line each
int AstroDialog::GetPreferredHeight(int theWidth)
{
	if (!mTypedLines.empty())
		return mLinesFont->GetHeight() * (int) mTypedLines.size();

	Graphics g((Image*) NULL);
	g.SetFont(mLinesFont);
	int aTextHeight = GetWordWrappedHeight(&g, theWidth - mContentInsets.mRight - mContentInsets.mLeft, mDialogLines, mLinesFont->GetHeight());
	int aRoom = ((0x140 - IMAGE_DIALOG_BTN->mHeight) - mSpaceAfterHeader - mContentInsets.mBottom) - mContentInsets.mTop;
	if (aRoom < aTextHeight)
		return (aTextHeight - aRoom) + 0x140;
	return 0x140;
}

// 0x483560: false puts the box in its settled state at once (the title in place, the buttons shown); true makes it start
// as a box that has yet to come in
void AstroDialog::SetAnimated(bool theAnimated)
{
	if (!theAnimated)
	{
		mState = 3;
		mSettled = true;
		mClip = true;
		m120 = 0;
		m11c = mWidth / 2 - HeaderWidth() / 2;
		int aTop = (mPicture == NULL) ? 0x19 : mImageTop;
		m118 = aTop;
		m124 = aTop + 0x19;
		if (mYesButton != NULL)
			mYesButton->SetVisible(true);
		if (mNoButton != NULL)
			mNoButton->SetVisible(true);
	}
	else
	{
		mState = 8;
		mSettled = false;
		mClip = false;
		if (mNoButton != NULL)
			mNoButton->SetVisible(false);
		if (mYesButton != NULL)
			mYesButton->SetVisible(false);
	}
}

// 0x483750: while the box is settled it can be dragged, and the title stays on it
void AstroDialog::MouseDrag(int x, int y)
{
	if (mSettled)
	{
		Dialog::MouseDrag(x, y);
		m120 = 0;
		m11c = mWidth / 2 - HeaderWidth() / 2;
		int aTop = (mPicture == NULL) ? 0x19 : mImageTop;
		m118 = aTop;
		m124 = aTop + 0x19;
	}
}

// 0x4879d0
void AstroDialog::SetButtonLabels(const std::string& theYes, const std::string& theNo)
{
	if (mYesButton != NULL)
		mYesButton->mLabel = theYes;
	if (mNoButton != NULL)
		mNoButton->mLabel = theNo;
}

// 0x487970: the lines of the box are typed out one by one from the list in mTypedLines
void AstroDialog::SetTyping(bool theTyping)
{
	if (theTyping)
	{
		mDialogLines = "";
		mTypedChars = 0;
		mTypedLine = 0;
		mTyping = true;
		return;
	}
	mTyping = false;
}

// 0x4897b0
void AstroDialog::SetLineFont(Font* theFont, unsigned int theLine)
{
	if (theLine >= mLineFonts.size())
		mLineFonts.resize(theLine + 1, NULL);
	mLineFonts[theLine] = theFont;
}

// 0x487a70
void AstroDialog::Draw(Graphics* g)
{
	if (!mSettled)
	{
		// the box on its way in: the inset picture and the frame, the title on it
		Rect aRect(m120, m124, mWidth - mBackgroundInsets.mRight - mBackgroundInsets.mLeft,
				   mHeight - mBackgroundInsets.mTop - mBackgroundInsets.mBottom);
		g->DrawImageBox(aRect, mComponentImage);
		g->DrawImageBox(aRect, IMAGE_DIALOG);
		if (mPicture == NULL)
		{
			g->SetFont(mHeaderFont);
			g->SetColor(mColors[COLOR_HEADER]);
			WriteCenteredLine(g, m118, mDialogHeader);
		}
		else
			g->DrawImage(mPicture, m11c, m118);
	}
	else
	{
		Dialog::Draw(g);
		Rect aRect(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, mWidth - mBackgroundInsets.mRight - mBackgroundInsets.mLeft,
				   mHeight - mBackgroundInsets.mBottom - mBackgroundInsets.mTop);
		g->DrawImageBox(aRect, IMAGE_DIALOG);
		if (mPicture != NULL)
			g->DrawImage(mPicture, m11c, m118);
	}

	if (mTyping && mSettled)
	{
		// the lines typed so far, then the line being typed and the cursor
		g->SetColor(mColors[COLOR_LINES]);
		int aX = mContentInsets.mLeft;
		int aY = mSpaceAfterHeader + mHeaderFont->GetAscent() * 2 + mContentInsets.mTop + mBackgroundInsets.mTop;
		for (int aLine = 0; aLine < mTypedLine; aLine++)
		{
			if (aLine > 0)
				aY += 10 + mLinesFont->GetHeight();

			Font* aFont = mLinesFont;
			if (!mLineFonts.empty() && (unsigned int) aLine < mLineFonts.size())
			{
				unsigned int i = aLine;
				while (i < mLineFonts.size() && mLineFonts[i] == NULL)
					i++;
				if (i < mLineFonts.size())
					aFont = mLineFonts[i];
			}
			g->SetFont(aFont);
			WriteString(g, mTypedLines[aLine], aX, aY, -1, -1, true, 0, -1);
		}
		if (mTypedLine > 0)
			aY += mLinesFont->GetHeight();

		std::string aText;
		if ((unsigned int) mTypedLine < mTypedLines.size())
		{
			aText = mTypedLines[mTypedLine].substr(0, mTypedChars + 1);
			g->DrawString(aText, aX, aY);
		}

		if (mCursorOn && mCursorEnabled)
		{
			g->SetColor(Color(0xff, 0xff, 0xff));
			int aHeight = mLinesFont->GetHeight();
			g->FillRect(mLinesFont->StringWidth(aText + " ") + aX, (aY - aHeight) + 4, 0xf, aHeight);
		}
	}
}

// 0x484240: the typing, and the opening of the box: it comes up from below the screen by 8 a step (state 8), sinks back to
// 0x28 off by 4 a step (state 4), then rises again by 3 a step (state 3) and is settled
void AstroDialog::Update()
{
	Dialog::Update();

	if (!mTyping)
	{
		if (mSettled)
			return;
	}
	else if (mSettled)
	{
		if (mTypeSpeed < 1)
			mTypedLine = (int) mTypedLines.size();
		else if (mUpdateCnt % mTypeSpeed == 0)
		{
			unsigned int aLine = (unsigned int) mTypedLine;
			if (aLine < mTypedLines.size())
			{
				mTypedChars++;
				if (mTypedLines[aLine].size() <= (unsigned int) mTypedChars)
				{
					mTypedChars = 0;
					mTypedLine = (int) aLine + 1;
				}
			}
		}
		if (mUpdateCnt % 0x32 == 0)
		{
			if (mTypedLines.size() <= (unsigned int) mTypedLine)
				mCursorOn = !mCursorOn;
		}
		MarkDirty();
		return;
	}

	if (mState == 8)
	{
		int aTitle = m118;
		int aBox = m124;
		m118 = aTitle + 8;
		m124 = aBox - 8;
		if (m110 <= aTitle + 8)
			m118 = m110;
		if (aBox - 8 <= m114)
			m124 = m114;
		if (m124 == m114 && m118 == m110)
			mState = 4;
	}
	else if (mState == 4)
	{
		int aTitle = m118;
		int aBox = m124;
		int aTitleLimit = m110 - 0x28;
		m118 = aTitle - 4;
		m124 = aBox + 4;
		if (aTitle - 4 <= aTitleLimit)
			m118 = aTitleLimit;
		int aBoxLimit = m114 + 0x28;
		if (aBoxLimit <= aBox + 4)
			m124 = aBoxLimit;
		if (m124 == aBoxLimit && m118 == aTitleLimit)
			mState = 3;
	}
	else if (mState == 3)
	{
		int aTitle = m118;
		int aBox = m124;
		m118 = aTitle + 3;
		m124 = aBox - 3;
		if (m110 <= aTitle + 3)
			m118 = m110;
		if (aBox - 3 <= m114)
			m124 = m114;
		if (m124 == m114 && m118 == m110)
		{
			mSettled = true;
			mClip = true;
			m120 = 0;
			m11c = mWidth / 2 - HeaderWidth() / 2;
			int aTop = (mPicture == NULL) ? 0x19 : mImageTop;
			m118 = aTop;
			m124 = aTop + 0x19;
			if (mYesButton != NULL)
				mYesButton->SetVisible(true);
			if (mNoButton != NULL)
				mNoButton->SetVisible(true);
		}
	}
	MarkDirty();
}

// 0x40bab0: the fonts, insets and text colours of the game's boxes
void Sexy::StyleDialog(Dialog* theDialog)
{
	theDialog->mTextAlign = -1;
	theDialog->SetHeaderFont(GetFontById(0xb2));
	theDialog->SetButtonFont(GetFontById(0xb1));
	theDialog->SetLinesFont(GetFontById(0xb0));
	theDialog->mContentInsets = Insets(40, 22, 40, 30);
	theDialog->mSpaceAfterHeader = 10;
	theDialog->mHasTransparencies = true;
	theDialog->SetColor(Dialog::COLOR_HEADER, Color(0xff, 0xff, 0xff));
	theDialog->SetColor(Dialog::COLOR_LINES, Color(0xff, 0xff, 0xff));
	theDialog->SetColor(Dialog::COLOR_FOOTER, Color(0xff, 0xff, 0xff));
	theDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xff, 0xff, 0));
	theDialog->SetColor(Dialog::COLOR_BUTTON_TEXT_HILITE, Color(0xff, 0xff, 0xff));
}
