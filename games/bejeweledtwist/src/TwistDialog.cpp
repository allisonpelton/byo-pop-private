// The game's dialogs (TwistDialog.h).
#include "TwistDialog.h"

#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistD3D.h"
#include "TwistDialogListener.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"

#include <climits>
#include <cmath>

using namespace Sexy;

namespace {

// 0x75551b: a dialog's button with its label
TwistDialogButton* MakeButton(int theId, ButtonListener* theListener, SexyString theLabel)
{
	TwistDialogButton* aButton = new TwistDialogButton(IMAGE_SIZEABLEBUTTON, IMAGE_SIZEABLEBUTTON, theId, theListener);
	aButton->mLabel = theLabel;
	aButton->SetFont(FONT_DIALOG_BUTTONS);
	aButton->SetColor(0, Color(255, 200, 32));
	aButton->mClip = false;
	aButton->mSilent = true;
	return aButton;
}

}  // namespace

TwistDialogButton::TwistDialogButton(Image* theComponentImage, Image* theImage2, int theId,
	ButtonListener* theListener)
	: DialogButton(theComponentImage, theId, theListener)
{
	mImage2 = theImage2;
	mHeight = theComponentImage->mHeight;
	SetFont(FONT_DIALOG_BUTTONS);
	mLabelOffsetY = (int)gApp->mScale;
	mLabelOffsetX = (int)(gApp->mScale * 0.0);
}

TwistDialogButton::~TwistDialogButton()
{
	delete mOwned;
}

void TwistDialogButton::Update()
{
	Widget::Update();
	bool aPressed = mIsDown && mIsOver && !mDisabled;
	if (mInverted != aPressed) {
		mGlow = 1.0f;
		return;
	}
	float aTarget = 0.0f;
	if (mIsOver || mIsDown)
		aTarget = 0.5f;
	if (mGlow == aTarget)
		return;
	float aGlow;
	if (aTarget <= mGlow) {
		aGlow = (float)(mGlow - 0.1);
		if (aGlow <= aTarget)
			aGlow = aTarget;
	} else {
		aGlow = (float)(mGlow + 0.2);
		if (aTarget <= aGlow)
			aGlow = aTarget;
	}
	mGlow = aGlow;
	MarkDirty();
}

void TwistDialogButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;
	IsButtonDown();  // the original calls it and drops the answer
	int aCel = 0;
	if (mIsDown)
		aCel = 2;
	else if (mIsOver)
		aCel = 1;
	int aCelCount = mComponentImage->mNumRows * mComponentImage->mNumCols;
	if (aCelCount - 1 < aCel)
		aCel = aCelCount - 1;
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(mAlpha * 255.0)));
	// 0x422980: the box from one cel of the image, its row the cel over the columns (passed in edi) and its column the
	// remainder
	g->DrawImageBox(mComponentImage->GetCelRect(aCel), Rect(0, 0, mWidth, mHeight),
		mComponentImage);
	g->SetColorizeImages(false);

	Color aLayerColor = GetFontLayerColor(mFont, 0);
	SetFontLayerColor(mFont, 0, Color::Black);
	g->SetFont(mFont);
	int aLabelWidth = mFont->StringWidth(mLabel);
	int anAscent = mFont->GetAscent();
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(true);
	g->SetColor(mColors[0]);
	g->DrawString(mLabel, (mWidth - aLabelWidth) / 2 + mLabelOffsetX, (anAscent + (mHeight - anAscent / 4) - 1) / 2);
	g->SetColorizeImages(false);
	SetFontLayerColor(mFont, 0, aLayerColor);
}

void TwistDialogButton::MouseEnter()
{
	if (!mSilent)
		gApp->PlaySoundEx(SOUND_MOUSEOVER1, 0, (float)mId, 0, true, true);
	ButtonWidget::MouseEnter();
}

TwistDialog::TwistDialog(Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
	const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter,
	int theButtonMode)
	: Dialog(theComponentImage, theButtonComponentImage, theId, isModal, _S(""), theDialogLines, theDialogFooter,
		BUTTONS_NONE)
{
	mButtonImage = theButtonComponentImage;
	float aScale = gApp->mScale;
	int aVert = (int)(aScale * 51.0);
	int aHorz = (int)(aScale * 80.0);
	mContentInsets = Insets(aHorz, aVert, aHorz, aVert);
	mDialogHeader = theDialogHeader;
	mLineSpacingOffset = (int)(gApp->mScale * -8.0);
	mButtonBottom = (int)(gApp->mScale * 58.0);
	mButtonWidth = theButtonComponentImage->GetCelWidth();
	mButtonHeight = theButtonComponentImage->GetCelHeight();
	mButtonSpacing = (int)(gApp->mScale * 32.0);
	mColors[COLOR_LINES] = Color::White;
	mColors[COLOR_HEADER] = Color(255, 255, 0);
	mYesButton = nullptr;
	mNoButton = nullptr;
	SetHeaderFont(FONT_DIALOG_HEADER);
	SetLinesFont(FONT_DIALOG_TEXT);
	mSpaceAfterHeader = (int)(gApp->mScale * 24.0);
	mYesLabel = GT(0x8144e0);
	mNoLabel = GT(0x7f246c);
	mThirdLabel = GT(0x7f246c);
	mTwistButtonMode = theButtonMode;
}

TwistDialog::~TwistDialog()
{
	DeleteButtons();
	delete mOwned;
	delete mThirdButton;
}

void TwistDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	// The original also keeps a field of the widget manager (+0x88) in +0x190, which nothing reads. It does not call
	// Dialog's AddedToManager: the buttons are its own.
	CreateButtons(mTwistButtonMode);
}

void TwistDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	RemoveAllWidgets(false, false);
}

void TwistDialog::Update()
{
	Widget::Update();
	if (mPopSpeed != 0.0f) {
		mPopScale += mPopSpeed;
		if (mPopScale <= 1.0f) {
			if (mPopSpeed >= 0.0f || mClosing) {
				mPopSpeed = (float)(mPopSpeed + 0.0);
			} else {
				mPopScale = 1.0f;
				mPopSpeed = 0;
			}
		} else {
			mPopSpeed = (float)(mPopSpeed - 0.007);
		}
		if (mPopScale < 0.0f && !gApp->KillDialog(mId, true, true)) {
			mParent->RemoveWidget(this);
			gApp->SafeDeleteWidget(this);
		}
	}
	mMouseUpThisUpdate = false;
	mTime = (float)(mTime + 0.05);
}

void TwistDialog::Draw(Graphics* g)
{
	if (mWidth <= 0)
		return;
	SetFontLayerColor(mHeaderFont, "OUTLINE", Color(0, 0, 0, 32));
	SetFontLayerColor(mLinesFont, "MAIN", Color(255, 255, 255, 255));
	Dialog::Draw(g);
	Rect aClipRect = g->mClipRect;
	if (mArrowX > 0) {
		g->mClipRect = Rect(0, 0, gApp->mWidth, gApp->mHeight);
		int aBob = (int)(sinf(mTime) * -20.0);
		int anAlpha = (int)(mPopScale * 255.0);
		if (anAlpha > 255)
			anAlpha = 255;
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawImageCentered(g, IMAGE_TUTORIAL_ARROW, (float)(mArrowX - aBob - mX), (float)(mArrowY - mY), mArrowRot,
			0.75f, 0.75f);
	}
	g->SetFont(FONT_INGAME);
	if (mShowClickText) {
		g->SetFont(FONT_INGAME);
		SexyString aText = GT(0x7f2490);
		int aY = mHeight - 70;
		g->DrawString(aText, mWidth / 2 - g->GetFont()->StringWidth(aText) / 2, aY);
	}
	g->mClipRect = aClipRect;
}

void TwistDialog::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	gApp->mWidgetManager->FlushDeferredOverlayWidgets(INT_MAX);
	// With Direct3D it is drawn scaled by mPopScale about its middle
	float aPopScale = mPopScale;
	bool aScaled = mPopScale != 1.0f && gD3DInterface != nullptr;
	if (aScaled) {
		SexyTransform2D aMatrix;  // 0x44f860
		aMatrix.Translate((float)((double)(float)mX * -1.0 - (double)(float)mWidth * 0.5),
			(float)((double)(float)mY * -1.0 - (double)(float)mHeight * 0.5));  // 0x44f8a0
		aMatrix.Scale(aPopScale, aPopScale);                                     // 0x44f990
		aMatrix.Translate((float)((double)(float)mWidth * 0.5 + (double)(float)mX),
			(float)((double)(float)mY + (double)(float)mHeight * 0.5));
		gD3DInterface->PushTransform(aMatrix, true);  // slot 0x14
	}
	WidgetContainer::DrawAll(theFlags, g);
	if (aScaled)
		gD3DInterface->PopTransform();  // slot 0x18
}

void TwistDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	LayoutButtons();
}

void TwistDialog::GotFocus()
{
	Widget::GotFocus();
	SetVisible(true);
}

void TwistDialog::LostFocus()
{
	Widget::LostFocus();
	SetVisible(false);
}

void TwistDialog::KeyChar(SexyChar theChar)
{
	if (theChar == '\r' && mTwistListener != nullptr)
		mTwistListener->DialogButtonDepress(mId, ID_YES);
}

void TwistDialog::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
	if (theKey == KEYCODE_ESCAPE) {
		mEscaped = true;
		if (mTwistListener != nullptr)
			mTwistListener->DialogButtonDepress(mId, ID_NO);
	} else if (theKey == KEYCODE_RETURN) {
		if (mTwistListener != nullptr)
			mTwistListener->DialogButtonDepress(mId, ID_YES);
	}
}

void TwistDialog::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (mMouseDownOn)
		Dialog::MouseDown(x, y, theBtnNum, theClickCount);
}

void TwistDialog::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	Dialog::MouseUp(x, y, theBtnNum, theClickCount);
	mMouseUpThisUpdate = true;
}

int TwistDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog::GetPreferredHeight(theWidth);
	if (mTwistButtonMode != 0)
		aHeight += (int)(mButtonHeight * (double)1.8f);
	return aHeight - (int)(gApp->mScale * -0.0);
}

int TwistDialog::GetPreferredWidth()
{
	int aCount = (mTwistButtonMode & BUTTON_MODE_YES) != 0;
	if ((mTwistButtonMode & BUTTON_MODE_NO) != 0)
		aCount++;
	if ((mTwistButtonMode & BUTTON_MODE_THIRD) != 0)
		aCount++;
	int anInset = (int)(mComponentImage->mWidth * -0.125);
	return mButtonWidth * aCount + (aCount + 2) * mButtonSpacing + anInset * -2;
}

void TwistDialog::ButtonPress(int theId)
{
	if (!gApp->mShutdown)
		gApp->PlaySample(SOUND_CLICK_DIALOG);
}

void TwistDialog::ButtonDepress(int theId)
{
	if (!mMouseVisible)
		return;
	if (theId == ID_NO)
		mEscaped = true;
	if (mTwistListener != nullptr) {
		if (theId == ID_THIRD)
			mTwistListener->DialogThirdButtonDepress(mId);
		else
			mTwistListener->DialogButtonDepress(mId, theId);
		mMouseVisible = false;
	}
}

void TwistDialog::PopUp()
{
	mPopScale = 0.4f;
	mPopSpeed = 0.04f;
	gApp->PlaySample(SOUND_BUBBLEPOP);
}

void TwistDialog::Close()
{
	mClosing = true;
	mPopSpeed = -0.1f;
}

void TwistDialog::CreateButtons(int theButtonMode)
{
	ButtonListener* aListener = this;
	if ((theButtonMode & BUTTON_MODE_YES) != 0) {
		mYesButton = MakeButton(ID_YES, aListener, mYesLabel);
		mYesButton->mComponentImage = IMAGE_SIZEABLEBUTTON;
		mButtonCount++;
	}
	if ((theButtonMode & BUTTON_MODE_NO) != 0) {
		DialogButton* aButton = nullptr;
		if (theButtonMode != BUTTON_MODE_YES)
			aButton = MakeButton(ID_NO, aListener, mNoLabel);
		mButtonCount++;
		mNoButton = aButton;
	}
	if ((theButtonMode & BUTTON_MODE_THIRD) == 0) {
		mThirdButton = nullptr;
	} else {
		mThirdButton = MakeButton(ID_THIRD, aListener, mThirdLabel);
		mButtonCount++;
	}
	if (mYesButton != nullptr)
		AddWidget(mYesButton);
	if (mNoButton != nullptr)
		AddWidget(mNoButton);
	if (mThirdButton != nullptr)
		AddWidget(mThirdButton);
	LayoutButtons();
}

void TwistDialog::DeleteButtons()
{
	if (mYesButton != nullptr)
		RemoveWidget(mYesButton);
	if (mNoButton != nullptr)
		RemoveWidget(mNoButton);
	if (mThirdButton != nullptr)
		RemoveWidget(mThirdButton);
	delete mYesButton;
	delete mNoButton;
	delete mThirdButton;
	mThirdButton = nullptr;
	mNoButton = nullptr;
	mYesButton = nullptr;
	mButtonCount = 0;
}

void TwistDialog::LayoutButtons()
{
	int aWidth = mButtonWidth;
	int aHeight = mButtonHeight;
	DialogButton* aButtons[3] = {mYesButton, mNoButton, mThirdButton};
	for (DialogButton* aButton : aButtons) {
		if (aButton == nullptr)
			continue;
		double aLabelWidth = aButton->mFont->StringWidth(aButton->mLabel) * (double)1.2f;
		aWidth = (int)((double)aWidth <= aLabelWidth ? aLabelWidth : (double)aWidth);
	}
	int aY = mHeight - mButtonBottom - mButtonHeight;
	int aX = mWidth / 2 - ((mButtonCount - 1) * mButtonSpacing + mButtonCount * aWidth) / 2;
	if (mYesButton != nullptr) {
		mYesButton->Resize(aX, aY, aWidth, aHeight);
		aX += mButtonSpacing + aWidth;
	}
	if (mNoButton != nullptr) {
		mNoButton->Resize(aX, aY, aWidth, aHeight);
		aX += mButtonSpacing + aWidth;
	}
	if (mThirdButton != nullptr)
		mThirdButton->Resize(aX, aY, aWidth, aHeight);
}
