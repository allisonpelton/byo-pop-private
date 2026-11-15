#include "ColorizedButton.h"
#include "Graphics.h"
#include "Font.h"
#include "Image.h"

using namespace Sexy;

// 0x52e8e0: no tint in any state (no caller changes the colours)
static int gColorizedButtonColors[4][4] =
{
	{255, 255, 255, 255},
	{255, 255, 255, 255},
	{255, 255, 255, 255},
	{255, 255, 255, 255}
};

// 0x41bf50
ColorizedButton::ColorizedButton(int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
	SetImageColors(gColorizedButtonColors, 4);
}

// 0x41b650
ColorizedButton::~ColorizedButton()
{
}

// 0x41bed0
void ColorizedButton::SetImageColors(int theColors[][4], int theNumColors)
{
	mImageColors.clear();
	for (int i = 0; i < theNumColors; i++)
		mImageColors.push_back(Color(theColors[i][0], theColors[i][1], theColors[i][2], theColors[i][3]));
	MarkDirty();
}

// 0x41b550 (no bounds check)
const Color& ColorizedButton::GetImageColor(int theIdx)
{
	return mImageColors[theIdx];
}

// 0x41b3e0
void ColorizedButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	MarkDirty();
}

// 0x41b400
void ColorizedButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	MarkDirty();
}

// 0x41b450
void ColorizedButton::MouseDown(int x, int y, int theClickCount)
{
	ButtonWidget::MouseDown(x, y, theClickCount);
	MarkDirty();
}

// 0x41b420. port: the 2004 ButtonWidget::MouseUp(x, y) clears mIsDown and calls ButtonDepress before this
// MarkDirty; in the port the WidgetManager clears mIsDown and ButtonWidget::MouseUp(x, y, btn, cc), which
// runs after this one, calls ButtonDepress (the repaint request comes first; no visible difference).
void ColorizedButton::MouseUp(int x, int y)
{
	ButtonWidget::MouseUp(x, y);
	MarkDirty();
}

// 0x41b6a0: ButtonWidget::Draw with every image drawn colorized by GetImageColor(state)
void ColorizedButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;

	if ((mButtonImage == NULL) && (mDownImage == NULL))
	{
		ButtonWidget::Draw(g);
		return;
	}

	// 0x41b6f3: a SysFont("SansSerif", 10) is created when there is a label but no font. port: no SysFont;
	// never reached (LevelUpWidget sets a font).

	bool isDown = mIsDown && mIsOver && !mDisabled;

	int aFontX = 0;
	int aFontY = 0;
	if (mFont != NULL)
	{
		aFontX = (mWidth - mFont->StringWidth(mLabel)) / 2;
		aFontY = (mHeight - mFont->GetAscent() / 6 - 1 + mFont->GetAscent()) / 2;
	}

	g->SetFont(mFont);

	if (isDown != mInverted)
	{
		// Pressed: the down image in place, else the over or normal image shifted by (1,1)
		g->SetColorizeImages(true);
		g->SetColor(GetImageColor(COLOR_IMAGE_DOWN));
		if (mDownImage != NULL)
			g->DrawImage(mDownImage, 0, 0);
		else if (mOverImage != NULL)
			g->DrawImage(mOverImage, 1, 1);
		else
			g->DrawImage(mButtonImage, 1, 1);
		g->SetColorizeImages(false);

		g->SetColor(mColors[COLOR_LABEL_HILITE]);
		g->DrawString(mLabel, aFontX + 1, aFontY + 1);
		return;
	}

	Color aColor = GetImageColor(COLOR_IMAGE_NORMAL);
	if (mDisabled)
		aColor = GetImageColor(COLOR_IMAGE_DISABLED);
	else if (mIsOver)
		aColor = GetImageColor(COLOR_IMAGE_OVER);

	Image* anImage = NULL;
	if (mDisabled && (mDisabledImage != NULL))
		anImage = mDisabledImage;
	else if (mIsOver && (mOverImage != NULL))
		anImage = mOverImage;
	else if ((mOverAlpha > 0.0) && (mOverImage != NULL))
	{
		// Fading out after the mouse left: only the over image, at the fade alpha (the normal image is not
		// drawn under it)
		aColor.mAlpha = (int)(mOverAlpha * 255.0);
		anImage = mOverImage;
	}
	else if (mButtonImage != NULL)
		anImage = mButtonImage;

	if (anImage != NULL)
	{
		g->SetColorizeImages(true);
		g->SetColor(aColor);
		g->DrawImage(anImage, 0, 0);
		g->SetColorizeImages(false);
	}

	if (mLabel.length() == 0)
		return;

	g->SetColor(mColors[COLOR_LABEL]);
	g->DrawString(mLabel, aFontX, aFontY);
}
