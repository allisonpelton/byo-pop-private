// PodDialog.cpp: PodButton, PodDialogButton and PodDialog (PodDialog.h).
#include "PodDialog.h"

#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"
#include "WidgetManager.h"
#include "WinDMApp.h"

using namespace Sexy;

// The way the buttons' glow moves toward its target: down by 0.1 an update, up by 0.2, and never past the target.
static float StepToward(float theCurrent, float theTarget, double theDown, double theUp)
{
	if (theTarget <= theCurrent) {
		float aStep = (float)(theCurrent - theDown);
		if (theTarget < aStep)
			return aStep;
	} else {
		float aStep = (float)(theCurrent + theUp);
		if (aStep < theTarget)
			return aStep;
	}
	return theTarget;
}

// ---- PodButton

// 0x5cde4b
PodButton::PodButton(int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
	mSelectedGlowNow = 0;
	mGlow = 0;
	mGlowImage = nullptr;
	mSelectedGlowImage = nullptr;
	mSelectedGlow = 0.85f;
	mSelected = false;
	mSounds = false;
	mSelectedHoverGlow = 1.0f;
	mBlinkOn = false;
	mPressedGlow = 1.0f;
	mBlinking = false;
	mBlinkCount = 0;
	mHoverGlow = 0.5f;
	mBlinkTimer = 0;
	mDoFinger = true;
	mBlinkGlow = 0.32f;
	mHasAlpha = true;
	mBlinkPeriod = 50;
	mUnusedGlow = 0.5f;
	mUnusedGlow2 = 1.0f;
}

// 0x5cdf1e (the destructor proper; 0x5ce2ef is its deleting one)
PodButton::~PodButton()
{
}

// 0x5cdf29
void PodButton::Update()
{
	Widget::Update();
	if (mBlinking) {
		mBlinkTimer++;
		if (mBlinkTimer == mBlinkPeriod) {
			mBlinkTimer = 0;
			mBlinkOn = !mBlinkOn;
			if (mBlinkCount > 0 && --mBlinkCount == 0)
				mBlinking = false;
		}
	}
	bool aPressed = mIsDown && mIsOver && !mDisabled;
	bool aWasInverted = mInverted;

	float aTarget = 0.0f;
	if (mSelected) {
		if (!mIsOver && !mIsDown)
			aTarget = mSelectedGlow;
		else
			aTarget = mSelectedHoverGlow;
	}
	if (mSelectedGlowNow != aTarget) {
		mSelectedGlowNow = StepToward(mSelectedGlowNow, aTarget, 0.1, 0.2);
		MarkDirty();
	}
	if (aPressed != aWasInverted) {
		mGlow = mPressedGlow;
		return;
	}
	aTarget = 0.0f;
	if (!mSelected) {
		if (mIsOver || mIsDown) {
			aTarget = mHoverGlow;
		} else if (mBlinkOn) {
			aTarget = mBlinkGlow;
		}
	}
	if (mGlow == aTarget)
		return;
	mGlow = StepToward(mGlow, aTarget, 0.1, 0.2);
	MarkDirty();
}

// 0x5ce0c9
void PodButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;
	g->SetColorizeImages(true);
	if (0.0f < mGlow) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aBlue = (int)(mGlowColor.mBlue * mGlow);
		int aGreen = (int)(mGlowColor.mGreen * mGlow);
		int aRed = (int)(mGlowColor.mRed * mGlow);
		g->SetColor(Color(aRed, aGreen, aBlue));
		g->DrawImage(mGlowImage, 0, 0);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (0.0f < mSelectedGlowNow) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aBlue = (int)(mGlowColor.mBlue * mSelectedGlowNow);
		int aGreen = (int)(mGlowColor.mGreen * mSelectedGlowNow);
		int aRed = (int)(mGlowColor.mRed * mSelectedGlowNow);
		g->SetColor(Color(aRed, aGreen, aBlue));
		g->DrawImage(mSelectedGlowImage, 0, 0);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	// (the original also sets a color from the larger glow here and replaces it at once)
	g->SetColor(Color(255, 255, 255, 255));
	Image* anImage = mButtonImage;
	g->DrawImage(anImage, (mWidth - anImage->GetWidth()) / 2, (mHeight - anImage->GetHeight()) / 2);
	g->SetColorizeImages(false);
}

// 0x5ce28f
void PodButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	if (mSounds && gApp->mHasFocus)
		gApp->PlaySample(SOUND_MAIN_MOUSE_OVER);
}

// 0x5ce2bf
void PodButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	if (mSounds && gApp->mHasFocus)
		gApp->PlaySample(SOUND_MAIN_MOUSE_LEAVE);
}

// ---- PodDialogButton

// 0x5cec97
PodDialogButton::PodDialogButton(Image* theComponentImage, Image* theGlowImage, int theId, ButtonListener* theListener) :
	DialogButton(theComponentImage, theId, theListener)
{
	mGlow = 0;
	mTextGlow = nullptr;
	mGlowImage = theGlowImage;
	mAlpha = 1.0f;
}

// 0x5ceba1
PodDialogButton::~PodDialogButton()
{
	delete mTextGlow;
}

// 0x5cebe4
void PodDialogButton::Update()
{
	Widget::Update();
	bool aPressed = mIsDown && mIsOver && !mDisabled;
	if (mInverted != aPressed) {
		mGlow = 1.0f;
		return;
	}
	float aTarget = 0.0f;
	if (mIsDown || mIsOver)
		aTarget = 0.5f;
	if (mGlow == aTarget)
		return;
	mGlow = StepToward(mGlow, aTarget, 0.1, 0.2);
	MarkDirty();
}

// 0x5cecf7
void PodDialogButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(mAlpha * 255.0)));
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);
	if (0.0f < mGlow) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		double aBright = mGlow * 255.0;
		int aGray = (int)aBright;
		int anAlpha = (int)(aBright * mAlpha);
		g->SetColor(Color(aGray, aGray, aGray, anAlpha));
		g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mGlowImage);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetColorizeImages(false);

	Font* aGlowFont = FONT_CONTINUUM_MED_14_GLOW;
	if (mTextGlow == nullptr) {
		mTextGlow = new MemoryImage(gApp);
		mTextGlow->Create(aGlowFont->StringWidth(mLabel) + 16, aGlowFont->GetHeight());
		mTextGlow->SetImageMode(true, true);
		Graphics aGlowGraphics(mTextGlow);
		aGlowGraphics.SetFont(aGlowFont);
		aGlowGraphics.SetColor(Color(255, 255, 255));
		aGlowGraphics.DrawString(mLabel, 8, aGlowFont->GetAscent());
	}
	int aTextX = (mWidth - mFont->StringWidth(mLabel)) / 2;
	int aHeight = mFont->GetAscent();
	int aTextY = (mFont->GetAscent() + ((mHeight - aHeight / 6) - mFont->GetAscentPadding()) - 1) / 2;
	g->SetColorizeImages(true);
	int aGlowAlpha = (int)(float)(mAlpha * 100.0f);   // (the original's tuning value, SexyModVal at PodDialogButton.cpp:106)
	g->SetColor(Color(0, 0, 255, aGlowAlpha));
	g->DrawImage(mTextGlow, aTextX - 8, aTextY - aGlowFont->GetAscent());
	g->SetColorizeImages(false);
	g->SetFont(mFont);
	int anAlpha = (int)(mAlpha * 255.0);
	int aRed = 0xb2 - (int)(mGlow * -77.0);
	int aGreen = 0xcc - (int)(mGlow * -51.0);
	g->SetColor(Color(aRed, aGreen, 255, anAlpha));
	g->DrawString(mLabel, mTextOffsetX + aTextX, mTextOffsetY + aTextY);
}

// ---- PodDialog

// 0x5cea31
PodDialog::PodDialog(WinDMApp* theApp, int theId, bool isModal, const SexyString& theHeader, const SexyString& theLines,
	const SexyString& theFooter, int theButtonMode) :
	Dialog(IMAGE_DIALOG, IMAGE_DIALOG_BUTTON, theId, isModal, " ", theLines, theFooter, theButtonMode)
{
	mApp = theApp;
	mHeaderText = theHeader;
	mHeaderGlow = nullptr;
	mEscaped = false;
	if (mYesButton != nullptr) {
		DialogButton* aButton = MakePodButton(mYesButton);
		delete mYesButton;
		mYesButton = aButton;
	}
	if (mNoButton != nullptr) {
		DialogButton* aButton = MakePodButton(mNoButton);
		delete mNoButton;
		mNoButton = aButton;
	}
	mHasCancel = false;
	mCancelButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, 2, this);
	mCancelButton->mLabel = "CANCEL";
	mCancelButton->SetFont(FONT_CONTINUUM_MED_14);
	mHeaderWidth = 0;
}

// 0x5ce91e
PodDialog::~PodDialog()
{
	delete mHeaderGlow;
	delete mCancelButton;
}

// 0x5ce9aa: a dialog button as a PodDialogButton with the same id, listener and label
DialogButton* PodDialog::MakePodButton(DialogButton* theOldButton)
{
	PodDialogButton* aButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, theOldButton->mId,
		theOldButton->mButtonListener);
	aButton->mLabel = theOldButton->mLabel;
	aButton->SetFont(FONT_CONTINUUM_MED_14);
	return aButton;
}

// 0x5ce30e: the dialog gets a cancel button below the others (the "Load Game?" dialog)
void PodDialog::SetHasCancel(bool theHasCancel)
{
	if (theHasCancel == mHasCancel)
		return;
	mHasCancel = theHasCancel;
	if (mWidgetManager != nullptr) {
		if (!theHasCancel)
			mWidgetManager->RemoveWidget(mCancelButton);
		else
			mWidgetManager->AddWidget(mCancelButton);
	}
}

// 0x5ce341
int PodDialog::GetPreferredHeight(int theWidth)
{
	int anExtra = 0;
	if (mHasCancel)
		anExtra = mApp->ScaleNum(0x32, 0);
	return Dialog::GetPreferredHeight(theWidth) + anExtra;
}

// 0x5ce378
void PodDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog::AddedToManager(theManager);
	if (mHasCancel)
		theManager->AddWidget(mCancelButton);
}

// 0x5ce3a4
void PodDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog::RemovedFromManager(theManager);
	if (mHasCancel)
		theManager->RemoveWidget(mCancelButton);
}

// 0x5ce3d0
void PodDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	if (!mHasCancel)
		return;
	// the yes and no buttons move up to make room for the cancel button
	mYesButton->Move(mYesButton->mX, mYesButton->mY - mApp->ScaleNum(0x36, 0));
	mNoButton->Move(mNoButton->mX, mNoButton->mY - mApp->ScaleNum(0x36, 0));
	int aButtonHeight = mYesButton->mComponentImage->GetHeight();
	int aWidth = (((mWidth - mContentInsets.mRight) - mBackgroundInsets.mRight) - mContentInsets.mLeft) - mBackgroundInsets.mLeft;
	mCancelButton->Resize(mX + mContentInsets.mLeft + mBackgroundInsets.mLeft,
		mApp->ScaleNum(6, 0) + ((((mHeight - mContentInsets.mBottom) - mBackgroundInsets.mBottom) + mY) - aButtonHeight),
		aWidth, aButtonHeight);
}

// 0x5ce4db
void PodDialog::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
	if (theKey == KEYCODE_ESCAPE) {
		mEscaped = true;
		mApp->DialogButtonDepress(mId, ID_NO);
	}
}

// 0x5ce514, 0x5caeb1, 0x5db5e9: a click in a dialog (the original gives OptionsDialog and UserDialog a copy of this each, with
// the same click sound; the port's two classes inherit this one)
void PodDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK_DIALOG);
}

// 0x5ce52b: the cancel button (id 2) closes the dialog like the Escape key does
void PodDialog::ButtonDepress(int theId)
{
	if (theId == 2) {
		mEscaped = true;
		mApp->DialogButtonDepress(mId, ID_NO);
		return;
	}
	Dialog::ButtonDepress(theId);
}

// 0x5ce560: the box that holds the header
void PodDialog::GetHeaderRect(Rect& theRect)
{
	int aWidth = mHeaderWidth;
	if (aWidth == 0) {
		int aMargin = mApp->ScaleNum(0x40, 0);
		if (mWidth - aMargin < mApp->ScaleNumUp(0x154, 0))
			aWidth = mWidth - mApp->ScaleNum(0x40, 0);
		else
			aWidth = mApp->ScaleNumUp(0x154, 0);
	}
	int aHeight = mApp->ScaleNumUp(0x45, 0);
	theRect.mX = (mWidth - aWidth) / 2;
	theRect.mY = 0;
	theRect.mWidth = aWidth;
	theRect.mHeight = aHeight;
}

// 0x5ce5f0: the box of an edit field, in three parts: the left end, the middle repeated and the right end
void PodDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	int aSize = mApp->ScaleNum(0x28, 0);
	g->DrawImage(IMAGE_EDIT_BOX, theRect.mX, theRect.mY, Rect(0, 0, aSize, aSize));
	g->DrawImage(IMAGE_EDIT_BOX, (theRect.mWidth - aSize) + theRect.mX, theRect.mY, Rect(aSize * 2, 0, aSize, aSize));
	Graphics aClipped(*g);
	aClipped.ClipRect(theRect.mX + aSize, theRect.mY, theRect.mWidth - aSize, aSize);
	int aStep = aSize;
	for (int i = 0; i < ((theRect.mWidth - aSize) - 1) / aSize; i++) {
		aClipped.DrawImage(IMAGE_EDIT_BOX, theRect.mX + aStep, theRect.mY, Rect(aSize, 0, aSize, aSize));
		aStep += aSize;
	}
}

// 0x5ce703: the header, centered in its box
void PodDialog::DrawTopText(Graphics* g)
{
	int anAscentPadding = mHeaderFont->GetAscentPadding();
	int aTop = mContentInsets.mTop + mBackgroundInsets.mTop;
	int anAscent = mHeaderFont->GetAscent();
	g->SetColor(GetColor(COLOR_HEADER));
	WriteCenteredLine(g, anAscent + (aTop - anAscentPadding), mHeaderText);
}

// 0x5ce767: the header in the glow font, drawn into an image of the header box's size
void PodDialog::CreateFontGlowImages()
{
	Rect aRect;
	GetHeaderRect(aRect);
	if (mHeaderGlow == nullptr) {
		mHeaderGlow = new MemoryImage(mApp);
		mHeaderGlow->Create(aRect.mWidth, aRect.mHeight);
		mHeaderGlow->SetImageMode(true, true);
	}
	mHeaderGlow->Clear();
	Graphics g(mHeaderGlow);
	g.SetColor(Color(255, 255, 255));
	g.SetFont(FONT_CONTINUUM_MED_18_GLOW);
	g.Translate(-aRect.mX, -aRect.mY);
	DrawTopText(&g);
}

// 0x5ce86a
void PodDialog::Draw(Graphics* g)
{
	if (mWidth <= 0)
		return;
	Dialog::Draw(g);
	if (mHeaderText.empty())
		return;
	Rect aRect;
	GetHeaderRect(aRect);
	g->DrawImageBox(aRect, IMAGE_DIALOG_HEADER);
	if (mHeaderGlow == nullptr)
		CreateFontGlowImages();
	g->SetColorizeImages(true);
	g->SetColor(Color(0x78, 0x78, 0xff, 0xdc));
	g->DrawImage(mHeaderGlow, aRect.mX, aRect.mY);
	g->SetColorizeImages(false);
	g->SetFont(mHeaderFont);
	DrawTopText(g);
}
