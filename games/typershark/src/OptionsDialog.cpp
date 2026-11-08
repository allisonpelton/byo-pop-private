#include "OptionsDialog.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "SoundSlider.h"
#include "GameText.h"

#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "WidgetManager.h"

using namespace Sexy;

// Not translated (the scalar deleting destructor of this class 0x439150, a thunk of 0x4384f0). Left out because the retail
// partner.xml makes the original skip them too: the Register button (id 1; made only when the app is not registered and
// the property NoReg, read as a boolean with the default false, is false; its label is the property RegBtnText with the
// default text 0x52e8c8; its click calls 0x414020, the register dialog), the partner link button (id 2; made when the
// integer property NumOptionsLinks is above 0; label text 0x52e898, its click opens the address at 0x52ea98 through the
// app's OpenURL) and every use of them (their AddWidget, RemoveWidget, delete, SetColor and their rows in Resize).

// 0x43bf10: Dialog first, with the footer button only (mode 3) and the header, no lines. The controls are sized by
// Resize; the creator (WinWSApp::ShowOptions 0x414150) sizes the dialog.
OptionsDialog::OptionsDialog(WinWSApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_OPTIONS, true, GT(0x0052e8dc), "",
		GT(0x0052e8e4), Dialog::BUTTONS_FOOTER),
	mApp(theApp),
	mMusicSlider(NULL),
	mSfxSlider(NULL),
	mField124(NULL),
	mCreditsButton(NULL),
	mCheckedImage(NULL),
	mUncheckedImage(NULL),
	mFlag13c(false),
	mFullScreenCheckbox(NULL),
	mEffectsCheckbox(NULL),
	mSpacePausesCheckbox(NULL),
	mCursorsCheckbox(NULL),
	m3DCheckbox(NULL)
{
	SetButtonFont(theApp->mFontRockwell13);
	SetHeaderFont(mApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);

	// The Register button (+0x128) and the partner link button (+0x12c) go here in the original: not ported (see above)

	mCreditsButton = new DialogButton(theApp->mImages[IMAGE_DBUTTON], CREDITS_BUTTON_ID, this);
	mCreditsButton->SetFont(mApp->mFontRockwell13);
	mCreditsButton->mLabel = GT(0x0052e890);
	mCreditsButton->mHasAlpha = true;

	mMusicSlider = new Slider2004(theApp->mImages[IMAGE_SLIDER], theApp->mImages[IMAGE_SLIDERANCHOR], MUSIC_SLIDER_ID, this);
	mMusicSlider->SetValue(theApp->GetMusicVolume());

	mSfxSlider = new SoundSlider(theApp->mImages[IMAGE_SLIDER], theApp->mImages[IMAGE_SLIDERANCHOR], SFX_SLIDER_ID, this, 8,
		mApp);
	mSfxSlider->SetValue(theApp->GetSfxVolume());

	// The check box pictures are the right and the left half of the checked image (the checked and the unchecked box),
	// copied into two memory images of 24 x 25. The graphics of both stay until the end of the constructor.
	mCheckedImage = new MemoryImage(mApp);
	mCheckedImage->Create(0x18, 0x19);
	mCheckedImage->mHasAlpha = true;
	Graphics aCheckedGraphics(mCheckedImage);
	aCheckedGraphics.DrawImage(mApp->mImages[IMAGE_CHECKED], -0x18, 0);

	mUncheckedImage = new MemoryImage(mApp);
	mUncheckedImage->Create(0x18, 0x19);
	mCheckedImage->mHasAlpha = true;		// the original stores into the first image again; the second keeps what Create made
	Graphics anUncheckedGraphics(mUncheckedImage);
	anUncheckedGraphics.DrawImage(mApp->mImages[IMAGE_CHECKED], 0, 0);

	mFlag13c = true;

	// SetChecked tells the listener (this dialog), which only reacts to the hardware check box's id
	mFullScreenCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, 0, this);
	mFullScreenCheckbox->SetChecked(!mApp->mIsWindowed, true);
	mFullScreenCheckbox->mHasAlpha = true;

	mEffectsCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, 0, this);
	mEffectsCheckbox->SetChecked(mApp->mFastCPU, true);
	mEffectsCheckbox->mHasAlpha = true;

	mCursorsCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, 0, this);
	mCursorsCheckbox->SetChecked(mApp->mCustomCursorsEnabled, true);
	mCursorsCheckbox->mHasAlpha = true;

	mSpacePausesCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, 0, this);
	mSpacePausesCheckbox->SetChecked(mApp->mUser->mSpacePauses, true);
	mSpacePausesCheckbox->mHasAlpha = true;
	if (mApp->mTypingTutor != NULL)
		mSpacePausesCheckbox->mDisabled = true;		// the typing tutor needs the space bar

	// This one gets its state directly, without the listener
	m3DCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, HARDWARE_CHECKBOX_ID, this);
	m3DCheckbox->mChecked = mApp->Is3DAccelerated();
	m3DCheckbox->mHasAlpha = true;

	SetColor(COLOR_HEADER, Color(0, 0, 0));
	mYesButton->mHasAlpha = true;
}

// 0x4384f0 (body; the scalar deleting destructor is 0x439150): the credits box goes first if it is open, then the
// controls (they are out of the widget manager by now: RemovedFromManager), the two pictures and the check boxes.
// port: the original never sets +0x124 (see the header), which its RemovedFromManager still hands to RemoveWidget.
OptionsDialog::~OptionsDialog()
{
	if (mApp->GetDialog(DIALOG_CREDITS) != NULL)
		mApp->KillDialog(DIALOG_CREDITS);

	delete mMusicSlider;
	delete mSfxSlider;
	delete mCreditsButton;
	delete mUncheckedImage;
	delete mCheckedImage;
	delete mFullScreenCheckbox;
	delete mEffectsCheckbox;
	delete mCursorsCheckbox;
	delete mSpacePausesCheckbox;
	delete m3DCheckbox;
}

// 0x438650 slot 50: ignores the width and the Dialog formula
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return 0x1b6;
}

// 0x438660 slot 7. Quirk: the button text colour (index 3) goes to the Credits button (and the two buttons not ported)
// as index 3 of the button's own colours, not as its label colour (0); the dialog's own SetColor(0, black) in the
// constructor does not reach them. The original passes the literal 3 (the first control gets the argument itself, which
// is the same).
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == 3)
	{
		if (mField124 != NULL)
			mField124->SetColor(3, theColor);
		if (mCreditsButton != NULL)
			mCreditsButton->SetColor(3, theColor);
	}
}

// 0x4386d0 slot 1
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mFullScreenCheckbox);
	theWidgetManager->AddWidget(mCreditsButton);
	theWidgetManager->AddWidget(mEffectsCheckbox);
	theWidgetManager->AddWidget(mCursorsCheckbox);
	theWidgetManager->AddWidget(mSpacePausesCheckbox);
	theWidgetManager->AddWidget(m3DCheckbox);
}

// 0x438790 slot 2
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mField124);		// port: NULL (nothing is removed), see the header
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullScreenCheckbox);
	theWidgetManager->RemoveWidget(mCreditsButton);
	theWidgetManager->RemoveWidget(mEffectsCheckbox);
	theWidgetManager->RemoveWidget(mCursorsCheckbox);
	theWidgetManager->RemoveWidget(mSpacePausesCheckbox);
	theWidgetManager->RemoveWidget(m3DCheckbox);
}

// 0x438860 slot 13. Screen coordinates. The footer button is at the bottom as Dialog::Resize placed it; the Credits
// button 42 pixels above its top edge, over the whole content width (the Register and link buttons would share the row
// with it: not ported).
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);

	int aContentWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	int aButtonWidth = (aContentWidth - 6) / 2;

	// no link button: the row is two buttons wide and the Credits button takes all of it
	aButtonWidth = aButtonWidth * 2 + 6;
	mCreditsButton->Resize(mContentInsets.mLeft + mX, mYesButton->mY - 0x2a, aButtonWidth, 0x24);

	mMusicSlider->Resize(mX + 0x98, mY + 0x4e, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 0x6e, mWidth - 0xc0, 0x20);

	mFullScreenCheckbox->Resize(mX + 0x30, mY + 0x94, 0x24, 0x25);
	mEffectsCheckbox->Resize(mX + 0x30, mY + 0xb3, 0x24, 0x25);
	mCursorsCheckbox->Resize(mX + 0x30, mY + 0xd5, 0x24, 0x25);
	mSpacePausesCheckbox->Resize(mX + 0x30, mY + 0xf7, 0x24, 0x25);
	m3DCheckbox->Resize(mX + 0x30, mY + 0x11d, 0x24, 0x25);
}

// 0x438a40 SliderListener (+0x110) slot 0: the volumes are applied at once
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == MUSIC_SLIDER_ID)
		mApp->SetMusicVolume(theVal);		// slot 47
	else if (theId == SFX_SLIDER_ID)
		mApp->SetSfxVolume(theVal);			// slot 48
}

// 0x438a80 ButtonListener (+0x58) slot 3. Quirk: only the button sample; the base is not called, so nothing of the
// press reaches the app (the footer button's click does, on the release).
void OptionsDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x43c8f0 slot 16. DrawString's y is the baseline; coordinates are dialog-relative.
void OptionsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);

	g->SetFont(mApp->mFontRockwell13);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x0052e964), 0x30, 0x62);
	g->DrawString(GT(0x0052e958), 0x30, 0x82);
	g->DrawString(GT(0x0052e94c), (mFullScreenCheckbox->mX - mX) + 0x24, (mFullScreenCheckbox->mY - mY) + 0x11);
	g->DrawString(GT(0x0052e92c), (mEffectsCheckbox->mX - mX) + 0x24, (mEffectsCheckbox->mY - mY) + 0x11);
	g->DrawString(GT(0x0052e91c), (mCursorsCheckbox->mX - mX) + 0x24, (mCursorsCheckbox->mY - mY) + 0x11);

	// the space bar line is grey while the typing tutor is up (its check box is disabled)
	if (mApp->mTypingTutor != NULL)
		g->SetColor(Color(0x80, 0x80, 0x80));
	g->DrawString(GT(0x0052e904), (mSpacePausesCheckbox->mX - mX) + 0x24, (mSpacePausesCheckbox->mY - mY) + 0x11);

	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x0052e8ec), (m3DCheckbox->mX - mX) + 0x24, (m3DCheckbox->mY - mY) + 0x11);
}

// 0x43cc90 CheckboxListener (+0x114) slot 0: only the hardware acceleration box matters. Ticking it on a computer
// whose video card cannot do it unticks it again (and says so); on one where it is not recommended it stays ticked
// with a warning. Nothing is applied before the footer button (the app's handler reads the boxes).
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	if (theId == HARDWARE_CHECKBOX_ID)
	{
		m3DCheckbox->mChecked = checked;
		if (checked)
		{
			if (!mApp->Is3DAccelerationSupported())
			{
				m3DCheckbox->mChecked = false;
				mApp->ShowMessageDialog(GT(0x0052ea00), GT(0x0052ea10), GT(0x0052b4d0), false);	// 0x41bd80: "Not Supported", "OK"
			}
			else
			{
				if (mApp->Is3DAccelerationRecommended())
					return;
				mApp->ShowMessageDialog(GT(0x0052e96c), GT(0x0052e978), GT(0x0052b4d0), false);	// "Warning", "OK"
			}
		}
	}
}

// 0x43cd90 ButtonListener (+0x58) slot 4. The base first (the footer button's click goes to the app, which applies the
// boxes and kills this dialog). Then the Credits button opens the credits box (dialog 0x30c, one OK button; the app's
// handler of that button, 0xadc, kills it). The Register button (id 1) and the link button (id 2) would be handled here:
// not ported.
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);

	if (theId == CREDITS_BUTTON_ID)
	{
		Dialog* aDialog = mApp->DoDialog(DIALOG_CREDITS, true, GT(0x0052eab0), GT(0x0052eab8), GT(0x0052b4d0),
			Dialog::BUTTONS_FOOTER);
		aDialog->mContentInsets = Insets(0x28, 0x14, 0x28, 0x1e);
		aDialog->mSpaceAfterHeader = 0x20;
		aDialog->mTextAlign = -1;
		aDialog->mHasAlpha = true;
		aDialog->mYesButton->mHasAlpha = true;
		aDialog->SetButtonFont(mApp->mFontRockwell13);
		aDialog->SetLinesFont(mApp->mFontRockwell13);
		aDialog->SetColor(COLOR_HEADER, Color(0, 0, 0));
		aDialog->Resize(mApp->mWidth / 2 - 0xbe, 0x6e, 0x17c, 0x14a);
	}
}
