// The game's EditWidget (TwistEditWidget.h).
#include "TwistEditWidget.h"

#include "Font.h"
#include "Graphics.h"

#include <algorithm>
#include <cstdlib>

using namespace Sexy;

TwistEditWidget::TwistEditWidget(int theId, EditListener* theEditListener)
	: EditWidget(theId, theEditListener)
{
}

void TwistEditWidget::Draw(Graphics* g)
{
	// port: without a font the original makes a 10-point "Arial Unicode MS"; the game always gives its edit boxes one
	if (mFont == nullptr)
		return;

	SexyString& aString = GetDisplayString();

	g->SetColor(mColors[COLOR_BKG]);
	g->FillRect(0, 0, mWidth, mHeight);

	for (int i = 0; i < 2; i++) {
		Graphics* aClipG = g->Create();
		aClipG->SetFont(mFont);

		if (i == 1) {
			int aCursorX = mFont->StringWidth(aString.substr(0, mCursorPos)) + mTextX -
				mFont->StringWidth(aString.substr(0, mLeftPos));
			int aHiliteX = aCursorX + 2;
			if (mHilitePos != -1 && mCursorPos != mHilitePos)
				aHiliteX = mFont->StringWidth(aString.substr(0, mHilitePos)) + mTextX -
					mFont->StringWidth(aString.substr(0, mLeftPos));

			if (!mShowingCursor)
				aCursorX += 2;

			aCursorX = std::min(std::max(0, aCursorX), mWidth - 8);
			aHiliteX = std::min(std::max(0, aHiliteX), mWidth - 8);

			int anExtra = 0;
			if (mHilitePos != -1 && mHilitePos != mCursorPos)
				anExtra = mSelectionExtra;
			aClipG->ClipRect(mCursorOffsetX + std::min(aCursorX, aHiliteX) + mClipInset,
				(mHeight - mFont->GetHeight()) / 2, abs(aHiliteX - aCursorX) + anExtra, mFont->GetHeight());
		} else {
			aClipG->ClipRect(mClipInset, 0, mWidth - mClipInset * 2, mHeight);
		}

		bool hasfocus = mHasFocus || mDrawSelOverride;
		if (i == 1 && hasfocus) {
			aClipG->SetColor(mColors[COLOR_HILITE]);
			aClipG->FillRect(0, 0, mWidth, mHeight);
		}

		if (i == 0 || !hasfocus)
			aClipG->SetColor(mColors[COLOR_TEXT]);
		else
			aClipG->SetColor(mColors[COLOR_HILITE_TEXT]);
		aClipG->DrawString(aString.substr(mLeftPos), mTextX, (mHeight - mFont->GetHeight()) / 2 + mFont->GetAscent());

		delete aClipG;
	}

	g->SetColor(mColors[COLOR_OUTLINE]);
	g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
}
