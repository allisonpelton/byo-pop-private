#include "OptionsDialog.h"

#include <SDL.h>

#include "Checkbox.h"
#include "DialogButton.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Profile.h"
#include "Res.h"
#include "Slider.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace {

// 0x4be830: a checkbox on IMAGE_DIALOG_CHECKBOX, whose left half is the box unticked and right half ticked
Checkbox* MakeCheckbox(int theId, CheckboxListener* theListener)
{
	Image* anImage = IMAGE_DIALOG_CHECKBOX;
	Checkbox* aBox = new Checkbox(anImage, anImage, theId, theListener);
	int aHalf = anImage->mWidth / 2;
	aBox->mCheckedRect = Rect(aHalf, 0, aHalf, anImage->mHeight);
	aBox->mUncheckedRect = Rect(0, 0, aHalf, anImage->mHeight);
	aBox->Resize(0, 0, aHalf, anImage->mHeight);
	aBox->mHasTransparencies = true;
	aBox->mHasAlpha = true;
	return aBox;
}

// 0x4be680 (GameDialog::Layout's too, for the Dialog's own buttons, which the port makes FadeButtons): the game's
// look for a DialogButton: label white and yellow while lit, a click moves it a pixel, the label two pixels up, the
// cels of its picture side by side (normal, lit, pressed and, with a fourth, disabled)
void SetupDialogButton(DialogButton* theButton, Font* theFont)
{
	theButton->mTranslateX = -1;
	theButton->mTranslateY = 1;
	theButton->mTextOffsetX = 0;
	theButton->mTextOffsetY = -2;
	if (theFont == nullptr)
		theFont = FONT_LARGE_OUTLINE;
	theButton->SetFont(theFont);
	theButton->SetColor(ButtonWidget::COLOR_LABEL, Color(255, 255, 255));
	theButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(255, 255, 0));
	theButton->mHasAlpha = true;
	theButton->mHasTransparencies = true;
	Image* anImage = theButton->mComponentImage;
	if (anImage == nullptr)
		return;
	int aWidth = anImage->GetCelWidth();
	int aHeight = anImage->GetCelHeight();
	theButton->mNormalRect = Rect(0, 0, aWidth, aHeight);
	theButton->mOverRect = Rect(aWidth, 0, aWidth, aHeight);
	theButton->mDownRect = Rect(aWidth * 2, 0, aWidth, aHeight);
	if (anImage->mNumCols >= 4)
		theButton->mDisabledRect = Rect(aWidth * 3, 0, aWidth, aHeight);
	if (theButton->mWidth == 0)
		theButton->Resize(theButton->mX, theButton->mY, theButton->mFont->StringWidth(theButton->mLabel), anImage->mHeight);
}

}  // namespace

DialogButton* Sexy::MakeDialogButton(ButtonListener* theListener, const std::string& theLabel)
{
	DialogButton* aButton = new DialogButton(IMAGE_DIALOG_BUTTON, 1, theListener);
	aButton->mLabel = theLabel;
	aButton->mWidth = IMAGE_DIALOG_BUTTON->mWidth / 3;
	aButton->mHeight = IMAGE_DIALOG_BUTTON->mHeight;
	SetupDialogButton(aButton, nullptr);
	return aButton;
}

// 0x4842e0: dialog 1 with a footer button that continues the battle (or closes), and the controls, which show the
// app's settings
OptionsDialog::OptionsDialog(bool theHasBoard)
	: GameDialog(GameApp::DIALOG_OPTIONS, _S(""), _S(""), theHasBoard ? GT(0x6fdee0) : GT(0x6fd4ac),
	             BUTTONS_FOOTER)
{
	mHasBoard = theHasBoard;
	mMusicSlider = new Slider(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, ID_MUSIC, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mSoundSlider = new Slider(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, ID_SOUND, this);
	mSoundSlider->SetValue(mApp->GetSfxVolume());
	mFullscreenBox = MakeCheckbox(ID_FULLSCREEN, this);
	mCursorsBox = MakeCheckbox(ID_CURSORS, this);
	m3DBox = MakeCheckbox(ID_3D, this);
	mLetterValuesBox = MakeCheckbox(ID_LETTER_VALUES, this);
	mNoTooltipsBox = MakeCheckbox(ID_NO_TOOLTIPS, this);
	mFullscreenBox->mChecked = !mApp->mIsWindowed;
	mCursorsBox->mChecked = mApp->mCustomCursorsEnabled;
	m3DBox->mChecked = mApp->Is3DAccelerated();
	Profile* aProfile = mApp->mProfile;
	mLetterValuesBox->mChecked = aProfile != nullptr && aProfile->KeyExists("EnablePips") && aProfile->GetBool("EnablePips");
	mNoTooltipsBox->mChecked = aProfile != nullptr && aProfile->KeyExists("DisableTips") && aProfile->GetBool("DisableTips");
	mCreditsButton = new DialogButton(IMAGE_DIALOG_BUTTON, ID_CREDITS, this);
	SetupDialogButton(mCreditsButton, FONT_LARGE_OUTLINE);
	mCreditsButton->mTextOffsetY = 0;
	mCreditsButton->mLabel = GT(0x6f9db8);
	mExtraHeight = 200;
	mBanner = IMAGE_DIALOG_BANNER_OPTIONS;
	mBannerY = 17;
	mButton1->mLabelOffsetY = -2;
}

// 0x484780: the controls go with it (the dialog only takes them out of the manager)
OptionsDialog::~OptionsDialog()
{
	delete mMusicSlider;
	delete mSoundSlider;
	delete mFullscreenBox;
	delete mCursorsBox;
	delete m3DBox;
	delete mLetterValuesBox;
	delete mNoTooltipsBox;
	delete mCreditsButton;
}

int OptionsDialog::ContentLeft() const
{
	return mBackgroundInsets.mLeft + mContentInsets.mLeft + mX;
}

int OptionsDialog::ContentWidth() const
{
	return mWidth - mBackgroundInsets.mRight - mBackgroundInsets.mLeft - mContentInsets.mRight - mContentInsets.mLeft;
}

int OptionsDialog::ContentTop() const
{
	return mHeaderFont->GetHeight() + mBackgroundInsets.mTop + mContentInsets.mTop + mY;
}

int OptionsDialog::GetPreferredHeight(int theWidth)
{
	// 0x484a80: room for the footer button's two rows and for each control that shows
	mExtraHeight = IMAGE_DIALOG_BUTTON->mHeight * 2 + 40;
	if (mMusicSlider->mVisible)
		mExtraHeight += mMusicSlider->mHeight;
	if (mSoundSlider->mVisible)
		mExtraHeight += mSoundSlider->mHeight;
	if (mFullscreenBox->mVisible)
		mExtraHeight += mFullscreenBox->mHeight + 12;
	if (m3DBox->mVisible)
		mExtraHeight += m3DBox->mHeight + 16;
	if (mNoTooltipsBox->mVisible)
		mExtraHeight += mNoTooltipsBox->mHeight + 16;
	return Dialog::GetPreferredHeight(theWidth) + mExtraHeight;
}

void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	// 0x484b30: the buttons go in as the GameDialog's, then the controls
	GameDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSoundSlider);
	theWidgetManager->AddWidget(mFullscreenBox);
	theWidgetManager->AddWidget(mCursorsBox);
	theWidgetManager->AddWidget(m3DBox);
	theWidgetManager->AddWidget(mLetterValuesBox);
	theWidgetManager->AddWidget(mNoTooltipsBox);
	if (mCreditsButton != nullptr)
		theWidgetManager->AddWidget(mCreditsButton);
}

void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	// 0x484bf0
	GameDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSoundSlider);
	theWidgetManager->RemoveWidget(mFullscreenBox);
	theWidgetManager->RemoveWidget(mCursorsBox);
	theWidgetManager->RemoveWidget(m3DBox);
	theWidgetManager->RemoveWidget(mLetterValuesBox);
	theWidgetManager->RemoveWidget(mNoTooltipsBox);
	if (mCreditsButton != nullptr)
		theWidgetManager->RemoveWidget(mCreditsButton);
}

void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// 0x484890: the sliders at the top, the checkboxes in two columns under them (full screen and 3-D on the
	// left, cursors and letter values on the right, then the tooltips), then the Credits button and the footer
	// button side by side
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	int aLeft = ContentLeft();
	int aTop = ContentTop();
	int aWidth = ContentWidth();
	mMusicSlider->Resize(aLeft + 150, aTop + 30, aWidth - 160, IMAGE_SLIDER_TRACK->mHeight);
	mSoundSlider->Layout(LAY_SameLeft | LAY_Below | LAY_SameSize, mMusicSlider);
	mFullscreenBox->Layout(LAY_Below | LAY_SetLeft, mSoundSlider, aLeft + 5, 30);
	mCursorsBox->Layout(LAY_Right | LAY_SameTop, mFullscreenBox, 160, 0);
	m3DBox->Layout(LAY_SameLeft | LAY_Below, mFullscreenBox, 0, 10);
	mLetterValuesBox->Layout(LAY_Right | LAY_SameTop, m3DBox, 160, 0);
	mNoTooltipsBox->Layout(LAY_SameLeft | LAY_Below, m3DBox, 0, 10);
	if (mCreditsButton != nullptr) {
		mCreditsButton->Layout(LAY_SameLeft | LAY_Below, mNoTooltipsBox, 0, 8);
		mCreditsButton->Resize(mButton1->mX - 2, mButton1->mY, mButton1->mWidth / 2 - 2, mButton1->mHeight);
		mButton1->Layout(LAY_SameWidth | LAY_Right | LAY_SameTop, mCreditsButton, 5, -20);
	} else
		mButton1->Layout(LAY_SameLeft | LAY_Below, mNoTooltipsBox, 0, 8);
	mButton1->Resize(mButton1->mX + 5, mButton1->mY + 20, mButton1->mWidth, mButton1->mHeight);
}

void OptionsDialog::Draw(Graphics* g)
{
	// 0x484cb0: the sliders' names and the checkboxes' between the two dividers
	GameDialog::Draw(g);
	g->SetFont(FONT_LARGE);
	g->SetColor(GetColor(COLOR_LINES));
	int aLabelX = mFullscreenBox->mX - mX + 5;
	g->DrawString(GT(0x6fdef0), aLabelX, FONT_LARGE->GetAscent() + mMusicSlider->mY - mY + 5);
	g->DrawString(GT(0x6fdf00), aLabelX, FONT_LARGE->GetAscent() + mSoundSlider->mY - mY + 5);
	g->DrawImage(IMAGE_DIALOG_DIVIDER_TOP, 80, 173);
	DrawBoxLabel(g, GT(0x6fdf10), mFullscreenBox);
	DrawBoxLabel(g, GT(0x6fdf1c), mCursorsBox);
	DrawBoxLabel(g, GT(0x6fdf2c), m3DBox);
	DrawBoxLabel(g, GT(0x6fdf40), mLetterValuesBox);
	DrawBoxLabel(g, GT(0x6fdf54), mNoTooltipsBox);
	g->DrawImage(IMAGE_DIALOG_DIVIDER_BOT, 80, 325);
}

// 0x4be910: a checkbox's name to its right, in the font the graphics has, as wide as the name (a line)
void OptionsDialog::DrawBoxLabel(Graphics* g, const SexyString& theLabel, Checkbox* theBox)
{
	if (!theBox->mVisible)
		return;
	Font* aFont = g->GetFont();
	int aSpacing = aFont->GetLineSpacing() - 2;
	int aWidth = aFont->StringWidth(theLabel);
	int aHeight = theBox->GetWordWrappedHeight(g, aWidth, theLabel, aSpacing);
	Rect aRect(theBox->mX + theBox->mWidth - mX + 4, theBox->mY - mY + 6, aWidth, aHeight);
	theBox->WriteWordWrapped(g, aRect, theLabel, aSpacing, -1);
}

void OptionsDialog::ButtonPress(int theId)
{
	// 0x4853b0
	mApp->PlaySample(SOUND_CLICK1);
}

void OptionsDialog::ButtonDepress(int theId)
{
	// 0x4853d0: the footer button is the Dialog's (it ends the options); the Credits button starts the credits
	Dialog::ButtonDepress(theId);
	if (mCreditsButton != nullptr && theId == mCreditsButton->mId) {
		mCreditsButton->SetDisabled(true);
		mApp->StartCredits();
	}
}

void OptionsDialog::SliderVal(int theId, double theVal)
{
	// 0x485050: the volume follows the slider; the sound effects' slider plays a sample (unless it is being dragged)
	if (theId == ID_MUSIC)
		mApp->SetMusicVolume(theVal);
	else if (theId == ID_SOUND) {
		mApp->SetSfxVolume(theVal);
		if (!mSoundSlider->mDragging)
			mApp->PlaySample(SOUND_CLICK2);
	}
}

void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	// 0x4850c0
	switch (theId) {
	case ID_FULLSCREEN:
		if (mApp->mForceFullscreen && !checked) {
			mApp->DoDialog(GameApp::DIALOG_NO_WINDOWED, true, GT(0x6fdfe0), GT(0x6fdf68), GT(0x6fa51c),
			               BUTTONS_FOOTER);
			Dialog* aDialog = mApp->GetDialog(GameApp::DIALOG_NO_WINDOWED);
			aDialog->SetColor(COLOR_HEADER, Color::White);
			aDialog->mContentInsets.mTop -= 14;
			aDialog->mSpaceAfterHeader = 25;
			mFullscreenBox->SetChecked(true, true);
		}
		break;
	case ID_3D:
		if (!checked)
			break;
		if (!mApp->Is3DAccelerationSupported()) {
			m3DBox->SetChecked(false, true);
			mApp->DoOkDialog(GT(0x6fe078), GT(0x6fdff8), "", nullptr);
		} else if (!mApp->Is3DAccelerationRecommended())
			mApp->DoOkDialog(GT(0x6fe108), GT(0x6fe088), "", nullptr);
		break;
	case ID_LETTER_VALUES:
		mApp->SetShowTilePips(checked);
		break;
	}
}
