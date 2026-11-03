#include "ListBoxDialog.h"

#include "DialogButton.h"
#include "Font.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

using namespace JeffLib;
using namespace Sexy;

ListBoxDialog::ListBoxDialog(Font* theFont, const std::string& theHeader, const std::string& theLines,
                             Image* theDialogImage, Image* theButtonImage, int theClickSound, int theColors[][3])
	: Dialog(theDialogImage, theButtonImage, 6, true, theHeader, theLines, "", BUTTONS_OK_CANCEL)
{
	// port: nothing in the original sets this dialog's lines font (Dialog's constructor leaves it null), though its
	// Dialog::Draw (0x5260f0) reads the font's line spacing all the same; the framework's Draw needs one. The dialog
	// has no lines, so nothing is drawn with it.
	SetLinesFont(theFont);

	mListGap = 15;
	mClickSound = theClickSound;

	mList = new ListWidget(0, theFont, this);
	mList->mDrawOutline = true;
	mList->mFont = theFont;
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->SetColors(theColors, 6);

	mScrollbar = new SexyScrollbarWidget(mList);
	mScrollbar->SetInvisIfNoScroll(true);
	mScrollbar->mHorizontal = false;
	mList->mScrollbar = mScrollbar;

	// OK and CANCEL show their pictures' columns: normal, over, down and (when the picture has four) disabled
	int aCelWidth = theButtonImage->GetCelWidth();
	int aCelHeight = theButtonImage->GetCelHeight();
	DialogButton* aButtons[2] = {mYesButton, mNoButton};
	int i = 0;
	do {
		DialogButton* aButton = aButtons[i];
		aButton->mNormalRect = Rect(0, 0, aCelWidth, aCelHeight);
		aButton->mOverRect = Rect(aCelWidth, 0, aCelWidth, aCelHeight);
		aButton->mDownRect = Rect(aCelWidth * 2, 0, aCelWidth, aCelHeight);
		if (theButtonImage->mNumCols > 3)
			aButton->mDisabledRect = Rect(aCelWidth * 3, 0, aCelWidth, aCelHeight);
		i++;
	} while (i < 2);
}

ListBoxDialog::~ListBoxDialog()
{
	delete mScrollbar;
	delete mList;
}

int ListBoxDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog::GetPreferredHeight(theWidth);
	RefreshList();
	if (mList != nullptr)
		return aHeight + 50 + mList->mFont->GetHeight() * 8;
	return aHeight;
}

void ListBoxDialog::Resize(const Rect& theRect)
{
	Resize(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void ListBoxDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	RefreshList();

	// the list, eight lines high, and the scrollbar beside it
	int aListHeight = mList->mFont->GetHeight() * 8 + 9;
	int aListY = mListGap + mSpaceAfterHeader + mContentInsets.mTop + theY;
	mList->Resize(mX + 40, aListY, mWidth - 105, aListHeight);
	mScrollbar->ResizeScrollbar(mWidth + mX - 65, aListY, 25, aListHeight);

	// the buttons are put again, along the bottom of the box
	if (mYesButton != nullptr) {
		if (mNoButton != nullptr) {
			int aWidth = (mWidth - mContentInsets.mRight - mContentInsets.mLeft - 48) / 2;
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + mContentInsets.mLeft + 16, mHeight - mContentInsets.mBottom - aButtonHeight + mY,
			                   aWidth, aButtonHeight);
			mNoButton->Resize(mContentInsets.mLeft + mX + aWidth + 32,
			                  mHeight - mContentInsets.mBottom - aButtonHeight + mY, aWidth, aButtonHeight);
		} else {
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + mContentInsets.mLeft, mHeight - mContentInsets.mBottom + mY - aButtonHeight,
			                   mWidth - mContentInsets.mRight - mContentInsets.mLeft, aButtonHeight);
		}
	}
}

void ListBoxDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mList);
	theWidgetManager->AddWidget(mScrollbar);
	theWidgetManager->BringToFront(mScrollbar);
	mList->SetSelect(0);
}

void ListBoxDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mList);
	theWidgetManager->RemoveWidget(mScrollbar);
}

void ListBoxDialog::MouseWheel(int theDelta)
{
	if (theDelta > 0)
		mScrollbar->SetValue(mScrollbar->mValue - 1.0);
	else if (theDelta < 0)
		mScrollbar->SetValue(mScrollbar->mValue + 1.0);
}

void ListBoxDialog::ButtonPress(int theId)
{
	gSexyAppBase->PlaySample(mClickSound);
}

void ListBoxDialog::ButtonDepress(int theId)
{
	// OK (or a double click on a line) takes the chosen line, CANCEL none
	if (theId == mId + 2000 || theId == ID_YES) {
		if (mList->mSelectIdx >= 0 && mList->mSelectIdx < (int)mList->mLines.size())
			mSelected = mList->mLines[mList->mSelectIdx];
	} else if (theId == mId + 3000 || theId == ID_NO) {
		mSelected = "";
	}
	int aResult = ID_NO;
	if (theId == mId + 2000 || theId == ID_YES)
		aResult = ID_YES;
	Dialog::ButtonDepress(aResult);
}

void ListBoxDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if (mList->mId == theId) {
		if (theIdx != mList->mSelectIdx) {
			gSexyAppBase->PlaySample(mClickSound);
			mList->SetSelect(theIdx);
		}
		mSelected = mList->mLines[theIdx];
		if (theClickCount == 2)
			ButtonDepress(mId + 2000);
	}
}
