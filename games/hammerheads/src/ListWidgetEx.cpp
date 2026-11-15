#include "ListWidgetEx.h"

#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// 0x401000
ListWidgetEx::ListWidgetEx(int theId, Font* theFont, Image* theBoxImage, ListListener* theListListener) :
	ListWidget(theId, theFont, theListListener)
{
	mBoxImage = theBoxImage;
	mDrawOutline = false;
}

// 0x4010c0: the box, then a shaded bar over the selected line, then the lines themselves (the framework list is drawn without
// its background and its own bar, so that only the bar drawn here shows)
void ListWidgetEx::Draw(Graphics* g)
{
	if (mBoxImage != nullptr)
		g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mBoxImage);

	Graphics aG(*g);
	aG.ClipRect(1, 1, mWidth - 2, mHeight - 2);

	int aFirstLine = ToInt(mPosition);
	int aLastLine = ToInt(mPageSize) + 1 + aFirstLine;
	int aLineCount = (int) mLines.size();
	if (aLineCount - 1 < aLastLine)
		aLastLine = aLineCount - 1;

	int anItemHeight = mItemHeight;
	if (anItemHeight == -1)
		anItemHeight = mFont->GetHeight();

	for (int aLine = aFirstLine; aLine <= aLastLine; aLine++)
	{
		int aLineY = ToInt(((double) aLine - mPosition) * (double) anItemHeight);
		if (aLine == mSelectIdx)
		{
			aG.SetColor(mColors[COLOR_SELECT]);
			aG.FillRect(0, aLineY + 4, mWidth, anItemHeight);
		}
	}

	Color aBackground = mColors[COLOR_BKG];
	Color aSelect = mColors[COLOR_SELECT];
	mColors[COLOR_BKG].mAlpha = 0;
	mColors[COLOR_SELECT].mAlpha = 0;
	ListWidget::Draw(g);
	mColors[COLOR_BKG] = aBackground;
	mColors[COLOR_SELECT] = aSelect;
}
