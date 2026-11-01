#ifndef __SEVENSEAS_BUTTON2003_H__
#define __SEVENSEAS_BUTTON2003_H__

// The 2003 framework's DrawImageBox (WinSS.exe 0x42beb0) cuts its picture into thirds of width/3 and height/3 and takes
// the right and bottom pieces from 2/3 on, where the later framework takes them from the picture's last third. The two
// differ when the picture is not a multiple of three (the wooden button is 98 x 50: the later one shows two more rows).
// The original's DialogButton (0x42c7d0) draws its picture with it.

#include "DialogButton.h"
#include "Dialog.h"
#include "Graphics.h"
#include "Font.h"

namespace Sexy
{

inline void DrawImageBox2003(Graphics* g, const Rect& theDest, Image* theImage)
{
	int cw = theImage->GetWidth() / 3;
	int ch = theImage->GetHeight() / 3;
	if (cw <= 0 || ch <= 0)
		return;

	g->DrawImage(theImage, theDest.mX, theDest.mY, Rect(0, 0, cw, ch));
	g->DrawImage(theImage, theDest.mX - cw + theDest.mWidth, theDest.mY, Rect(cw * 2, 0, cw, ch));
	g->DrawImage(theImage, theDest.mX, theDest.mY - ch + theDest.mHeight, Rect(0, ch * 2, cw, ch));
	g->DrawImage(theImage, theDest.mX - cw + theDest.mWidth, theDest.mY - ch + theDest.mHeight, Rect(cw * 2, ch * 2, cw, ch));

	Graphics aTopBottom(*g);
	aTopBottom.ClipRect(theDest.mX + cw, theDest.mY, theDest.mWidth - cw * 2, theDest.mHeight);
	for (int i = 0, n = (theDest.mWidth - cw - 1) / cw, x = cw; i < n; i++, x += cw)
	{
		aTopBottom.DrawImage(theImage, theDest.mX + x, theDest.mY, Rect(cw, 0, cw, ch));
		aTopBottom.DrawImage(theImage, theDest.mX + x, theDest.mY - ch + theDest.mHeight, Rect(cw, ch * 2, cw, ch));
	}

	Graphics aSides(*g);
	aSides.ClipRect(theDest.mX, theDest.mY + ch, theDest.mWidth, theDest.mHeight - ch * 2);
	for (int i = 0, n = (theDest.mHeight - ch - 1) / ch, y = ch; i < n; i++, y += ch)
	{
		aSides.DrawImage(theImage, theDest.mX, theDest.mY + y, Rect(0, ch, cw, ch));
		aSides.DrawImage(theImage, theDest.mX - cw + theDest.mWidth, theDest.mY + y, Rect(cw * 2, ch, cw, ch));
	}

	Graphics aMiddle(*g);
	aMiddle.ClipRect(theDest.mX + cw, theDest.mY + ch, theDest.mWidth - cw * 2, theDest.mHeight - ch * 2);
	for (int i = 0, nx = (theDest.mWidth - cw - 1) / cw, x = cw; i < nx; i++, x += cw)
		for (int j = 0, ny = (theDest.mHeight - ch - 1) / ch, y = ch; j < ny; j++, y += ch)
			aMiddle.DrawImage(theImage, theDest.mX + x, theDest.mY + y, Rect(cw, ch, cw, ch));
}

// DialogButton whose plain (whole picture) look is drawn the 2003 way
class DialogButton2003 : public DialogButton
{
public:
	DialogButton2003(Image* theComponentImage, int theId, ButtonListener* theListener) :
		DialogButton(theComponentImage, theId, theListener) {}

	virtual void Draw(Graphics* g) override
	{
		if (mBtnNoDraw || mComponentImage == NULL || mNormalRect.mWidth != 0)
		{
			DialogButton::Draw(g);
			return;
		}

		bool doTranslate = IsButtonDown();
		if (doTranslate)
			g->Translate(mTranslateX, mTranslateY);

		DrawImageBox2003(g, Rect(0, 0, mWidth, mHeight), mComponentImage);

		if (mFont != NULL)
		{
			g->SetFont(mFont);
			// the second colour is for the mouse over the button (mIsOver, +0x1f, set by MouseEnter 0x4282d0)
			g->SetColor(mIsOver ? mColors[COLOR_LABEL_HILITE] : mColors[COLOR_LABEL]);
			int aFontX = (mWidth - mFont->StringWidth(mLabel)) / 2;
			int aFontY = (mHeight + mFont->GetAscent() - mFont->GetAscentPadding() - mFont->GetAscent() / 6 - 1) / 2;
			g->DrawString(mLabel, aFontX + mTextOffsetX, aFontY + mTextOffsetY);
		}

		if (doTranslate)
			g->Translate(-mTranslateX, -mTranslateY);
	}
};


// the buttons the Dialog constructor made, made again as the 2003 kind (same id and label)
inline void Use2003Buttons(Dialog* theDialog, Image* theButtonImage)
{
	DialogButton** aButtons[2] = {&theDialog->mYesButton, &theDialog->mNoButton};
	for (int i = 0; i < 2; i++)
	{
		DialogButton* anOld = *aButtons[i];
		if (anOld == NULL)
			continue;
		DialogButton* aNew = new DialogButton2003(theButtonImage, anOld->mId, theDialog);
		aNew->mLabel = anOld->mLabel;
		*aButtons[i] = aNew;
		delete anOld;
	}
}

}

#endif
