#include "GameDialog.h"

#include <algorithm>

#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

GameDialog::GameDialog(int theId, const SexyString& theHeader, const SexyString& theLines,
                       const SexyString& theFooter, int theButtonMode)
	: Dialog(IMAGE_DIALOG, IMAGE_DIALOG_BUTTON, theId, true, theHeader, theLines, _S(""), BUTTONS_NONE)
{
	mApp = static_cast<GameApp*>(gSexyAppBase);
	mContentInsets = Insets(42, 42, 42, 42);

	auto aMakeButton = [this](int theButtonId, const SexyString& theLabel) {
		FadeButton* aButton = new FadeButton(theButtonId, this);
		aButton->mLabel = theLabel;
		aButton->SetupImage(IMAGE_DIALOG_BUTTON);
		aButton->mStretch = true;
		aButton->mFadeOnDisable = false;
		aButton->mFadeOnEnter = false;
		aButton->mFadeOnLeave = false;
		aButton->mFadeOnDown = false;
		aButton->mFadeOnUp = false;
		return aButton;
	};
	if (theButtonMode == BUTTONS_YES_NO) {
		mButton1 = aMakeButton(ID_YES, DIALOG_YES_STRING);
		mButton2 = aMakeButton(ID_NO, DIALOG_NO_STRING);
	} else if (theButtonMode == BUTTONS_OK_CANCEL) {
		mButton1 = aMakeButton(ID_OK, DIALOG_OK_STRING);
		mButton2 = aMakeButton(ID_CANCEL, DIALOG_CANCEL_STRING);
	} else if (theButtonMode == BUTTONS_FOOTER) {
		mButton1 = aMakeButton(ID_FOOTER, theFooter);
	} else {
		mNumButtons = 0;
	}
}

GameDialog::~GameDialog()
{
	delete mButton1;
	delete mButton2;
}

void GameDialog::Layout(int theMinWidth)
{
	SetHeaderFont(FONT_LARGE_OUTLINE);
	SetButtonFont(FONT_LARGE);
	SetLinesFont(FONT_MEDIUM);
	SetColor(COLOR_HEADER, Color::Black);
	SetColor(COLOR_LINES, Color::Black);

	int aWidth = std::max(theMinWidth, mHeaderFont->StringWidth(mDialogHeader) + mContentInsets.mRight +
	                                       mContentInsets.mLeft + mBackgroundInsets.mRight + 20 +
	                                       mBackgroundInsets.mLeft);
	int aY = 90 - mBackgroundInsets.mTop;
	if (!mApp->mDialogMap.empty())
		aY = 110 - mBackgroundInsets.mTop + mApp->mDialogMap.begin()->second->mY;
	int aHeight = GetPreferredHeight(aWidth);
	if (aY + aHeight > mApp->mHeight - 10)
		aY = 10;
	Resize((mApp->mWidth - aWidth) / 2, aY, aWidth, aHeight);
}

void GameDialog::SetButtonText(const SexyString& theYes, const SexyString& theNo)
{
	if (mButton1 != nullptr && !theYes.empty())
		mButton1->mLabel = theYes;
	if (mButton2 != nullptr && !theNo.empty())
		mButton2->mLabel = theNo;
}

void GameDialog::SetBanner(Image* theImage, int theY)
{
	mBanner = theImage;
	mBannerY = theY;
}

void GameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	if (mButton1 != nullptr)
		theWidgetManager->AddWidget(mButton1);
	if (mButton2 != nullptr)
		theWidgetManager->AddWidget(mButton2);
}

void GameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	if (mButton1 != nullptr)
		theWidgetManager->RemoveWidget(mButton1);
	if (mButton2 != nullptr)
		theWidgetManager->RemoveWidget(mButton2);
}

void GameDialog::OrderInManagerChanged()
{
	Dialog::OrderInManagerChanged();
	if (mButton1 != nullptr)
		mWidgetManager->PutInfront(mButton1, this);
	if (mButton2 != nullptr)
		mWidgetManager->PutInfront(mButton2, this);
}

void GameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	if (mButton1 == nullptr)
		return;
	// the buttons' middles on the bottom of the text's box
	int aBottom = mY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom;
	int aButtonY = aBottom + mButton1->mButtonImage->GetCelHeight() / 2 - mButtonHeight;
	if (mButton2 != nullptr) {
		int aButtonWidth = (mWidth - mButtonSidePadding * 2 - mContentInsets.mRight - mBackgroundInsets.mRight -
		                    mBackgroundInsets.mLeft - mContentInsets.mLeft - mButtonHorzSpacing) / 2;
		mButton1->Resize(mX + mBackgroundInsets.mLeft + mContentInsets.mLeft + mButtonSidePadding, aButtonY,
		                 aButtonWidth, mButtonHeight);
		mButton2->Resize(mButton1->mX + aButtonWidth + mButtonHorzSpacing, mButton1->mY, aButtonWidth,
		                 mButtonHeight);
	} else {
		int aButtonWidth = mWidth - mContentInsets.mRight - mBackgroundInsets.mRight - mBackgroundInsets.mLeft -
		                   mContentInsets.mLeft;
		mButton1->Resize(mX + mBackgroundInsets.mLeft + mContentInsets.mLeft, aButtonY, aButtonWidth,
		                 mButtonHeight);
	}
}

void GameDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	if (mBanner != nullptr)
		g->DrawImage(mBanner, mWidth / 2 - mBanner->mWidth / 2, mBannerY);
}

void GameDialog::KeyDown(KeyCode theKey)
{
	// port: the original also lets Escape be while the app's +0x8a0 is set (not known yet; nothing ported sets it)
	if (theKey != KEYCODE_ESCAPE)
		return;
	// the dialog's button mode is always BUTTONS_NONE (its own buttons are not the Dialog's), so this presses
	// yes, or ok, in every dialog: as the original does
	if (mButtonMode == BUTTONS_YES_NO || mButtonMode == BUTTONS_OK_CANCEL)
		ButtonDepress(ID_NO);
	else
		ButtonDepress(ID_YES);
}

void GameDialog::MouseDrag(int x, int y)
{
	if (mAllowDrag)
		Dialog::MouseDrag(x, y);
}
