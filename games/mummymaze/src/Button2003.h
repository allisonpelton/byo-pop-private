#ifndef __MUMMYMAZE_BUTTON2003_H__
#define __MUMMYMAZE_BUTTON2003_H__

// The original's DialogButton (WinMM.exe 0x430050) draws its picture with the 2003 framework's DrawImageBox (0x432210): the
// picture is cut into thirds of width/3 and height/3 and the right and bottom pieces are taken from 2/3 on, where the later
// framework takes them from the picture's last third. The two differ when the picture is not a multiple of three (the
// dialog button is 98 x 50: the later one shows two more columns and rows, so the buttons' right and bottom edges were two
// pixels off). The framework has that 2003 form behind `Graphics::mDrawImageBox2003`, which MMApp's constructor turns on; it
// draws the same pieces in the same order as the original's function (and as this file's own copy did, taken out when the two were
// compared). The dialog's frame (192 x 192) is a multiple of three, so it is the same drawn either way.

#include "DialogButton.h"
#include "Dialog.h"
#include "Graphics.h"
#include "Font.h"

namespace Sexy
{

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

		g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);

		if (mFont != NULL)
		{
			g->SetFont(mFont);
			// 0x430050 sets the colour at +0x28 (colour 0), whether or not the pointer is over the button
			g->SetColor(mColors[COLOR_LABEL]);
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
