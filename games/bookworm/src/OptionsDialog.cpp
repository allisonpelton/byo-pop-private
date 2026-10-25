#include "OptionsDialog.h"
#include "BookWormApp.h"
#include "Res.h"
#include "GameText.h"

#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "WidgetManager.h"
#include <cmath>

using namespace Sexy;

// 0x52f5c0: label (221,152,83), hilite (255,255,0): register, help, credits, the links and the footer
// button (the creator's SetColor(3, black) then turns the labels black)
static int gOptionsButtonColors[2][3] =
{
	{221, 152, 83},
	{255, 255, 0}
};

// GetPreferredHeight's function-local statics (guard bits 0x534de0): read once per run
static unsigned int gOptionsDialogStaticsGuard = 0;    // 0x534de0
// 0x534ddc the partner link count (guard bit 1): the links are not ported
static int gOptionsDialogHeight = 0;                    // 0x534dd8

// 0x42eb40 (SliderListener, +0x120 slot 0)
void OptionsDialog::SliderVal(int theId, double theVal)
{
	// Applied at once. The 2004 Slider reports while dragging (mDragging set: silent) and once more on
	// every mouse-up after clearing mDragging, so the sound slider plays "goldtile" when released
	if (theId == MUSIC_SLIDER_ID)
		mApp2->SetMusicVolume(theVal);
	else if (theId == SFX_SLIDER_ID)
	{
		mApp2->SetSfxVolume(theVal);
		if (!mSfxSlider->mDragging)
			mApp2->PlaySample(SOUND_GOLDTILE);
	}
}

// 0x42eba0 (ButtonListener +0x58 slot 3)
void OptionsDialog::ButtonPress(int theId)
{
	// The footer button clicks through the app (2001); the dialog's own buttons click here
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + ID_YES_2004) && (theId != mId + ID_NO_2004))
		mApp2->PlaySample(SOUND_BUTTONCLICK);
}

// 0x42ebe0: slides in from the right after the creator set mSlideOffset = 240, out after ApplyAndClose
void OptionsDialog::Update()
{
	MoneyDialog::Update();

	if (mSlideOffset == 0)
	{
		if (mClosing)
			mApp2->KillDialog(mId);
		return;
	}

	// x87: fsin of the stored phase, fsubr 1.0, fimul by the int width, _ftol (truncation). 1.570795 is
	// the original's pi/2. Sliding in, x reaches 240 when the truncated product reaches 0 (frame 28 at
	// width 388). Sliding out, x stops at width + 240 (12 px still on screen at 640) and the next update
	// kills the dialog.
	mSlidePhase += 0.055;
	if (mSlideOffset > 0)
	{
		if (mSlidePhase < 1.570795)
			mSlideOffset = (int) ((1.0 - sin(mSlidePhase)) * mWidth);
		else
			mSlideOffset = 0;
		Resize(SLIDE_BASE_X + mSlideOffset, mY, mWidth, mHeight);
		MarkDirty();
		return;
	}

	if (mSlidePhase < 1.570795)
		mSlideOffset = -(int) ((1.0 - sin(mSlidePhase)) * mWidth);
	else
		mSlideOffset = 0;
	Resize(mWidth + SLIDE_BASE_X + mSlideOffset, mY, mWidth, mHeight);
	MarkDirty();
}

// 0x42ed00 (slot 52): the footer button (the app on 2001/3001) and BookWormApp::ShowHelp
void OptionsDialog::ApplyAndClose()
{
	bool wantWindowed = !mFullscreenCheckbox->IsChecked();
	bool want3D = (m3DCheckbox != NULL) && m3DCheckbox->IsChecked();
	if ((mApp2->mIsWindowed != wantWindowed) || (mApp2->Is3DAccelerated() != want3D))
		mApp2->SwitchScreenMode(wantWindowed, want3D);
	mApp2->EnableCustomCursors(mCustomCursorsCheckbox->IsChecked());
	mApp2->mShowHints = mShowHintsCheckbox->IsChecked();
	mApp2->mShowDefs = mShowDefsCheckbox->IsChecked();
	mApp2->SaveCurrentUser();

	// Slide out (only the sign of the offset matters); Update kills the dialog when it reaches 0. A second
	// click during the slide-out applies everything again and restarts it
	mClosing = true;
	mSlidePhase = 0.0;
	mSlideOffset = SLIDE_CLOSE_OFFSET;
}

// 0x42ee20 (slot 7)
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	// The button text colour also goes to the register, help and credits buttons (and the links, not ported)
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mRegisterButton != NULL)
			mRegisterButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mHelpButton != NULL)
			mHelpButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mCreditsButton != NULL)
			mCreditsButton->SetColor(DialogButton::COLOR_LABEL, theColor);
	}
}

// 0x42eeb0 (slot 1)
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mShowHintsCheckbox);
	theWidgetManager->AddWidget(mShowDefsCheckbox);
	if (m3DCheckbox != NULL)
		theWidgetManager->AddWidget(m3DCheckbox);
	theWidgetManager->AddWidget(mCreditsButton);
	if (!mApp2->mBuildUnlocked)     // the original reads gSexyAppBase (the same app)
		theWidgetManager->AddWidget(mRegisterButton);
}

// 0x42efa0 (slot 2)
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	theWidgetManager->RemoveWidget(mShowHintsCheckbox);
	theWidgetManager->RemoveWidget(mShowDefsCheckbox);
	if (m3DCheckbox != NULL)
		theWidgetManager->RemoveWidget(m3DCheckbox);
	theWidgetManager->RemoveWidget(mCreditsButton);
	if (!mApp2->mBuildUnlocked)
		theWidgetManager->RemoveWidget(mRegisterButton);
}

// 0x42f090 (slot 13; also every slide frame). Screen coordinates (2004 widgets are all top-level).
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);

	// The footer button: centred, its label + 40 wide, 60 px above the bottom edge
	int aLabelWidth = mYesButton->mFont->StringWidth(mYesButton->mLabel);
	mYesButton->Resize((mWidth - (aLabelWidth + 40)) / 2 + mX, mHeight - 60 + mY, aLabelWidth + 40, 32);

	// The help / register / credits row 36 px above it. The original stacks the partner links (not ported)
	// upwards from there first, one row each, and puts this row above them.
	int aRowY = mYesButton->mY - ROW_PITCH;
	if (!mApp2->mBuildUnlocked)
	{
		int aButtonWidth = (mWidth - mContentInsets.mRight - mContentInsets.mLeft - 6) / 3;
		if (mHelpButton != NULL)
			mHelpButton->Resize(mX + mContentInsets.mLeft, aRowY, aButtonWidth, 32);
		if (mRegisterButton != NULL)
			mRegisterButton->Resize(mContentInsets.mLeft + mX + 3 + aButtonWidth, aRowY, aButtonWidth, 32);
		if (mCreditsButton != NULL)
			mCreditsButton->Resize(mContentInsets.mLeft + 6 + aButtonWidth * 2 + mX, aRowY, aButtonWidth, 32);
	}
	else
	{
		// The registered build: help and credits only (the pair is 3 px wider than the content area)
		int aButtonWidth = (mWidth - mContentInsets.mRight - mContentInsets.mLeft - 3) / 2;
		mHelpButton->Resize(mX + mContentInsets.mLeft, aRowY, aButtonWidth, 32);
		mCreditsButton->Resize(mContentInsets.mLeft + mX + 6 + aButtonWidth, aRowY, aButtonWidth, 32);
	}

	if (mMusicSlider != NULL)
		mMusicSlider->Resize(mX + 152, mY + 68, mWidth - 192, 36);
	if (mSfxSlider != NULL)
		mSfxSlider->Resize(mX + 152, mY + 100, mWidth - 192, 36);

	// Checkboxes 26x26: full screen, hints, hardware acceleration on the left, custom cursors and
	// definitions on the right
	if (mFullscreenCheckbox != NULL)
		mFullscreenCheckbox->Resize(mX + 30, mY + 138, 26, 26);
	if (mShowHintsCheckbox != NULL)
		mShowHintsCheckbox->Resize(mX + 30, mY + 164, 26, 26);
	if (m3DCheckbox != NULL)
		m3DCheckbox->Resize(mX + 30, mY + 190, 26, 26);
	if (mCustomCursorsCheckbox != NULL)
		mCustomCursorsCheckbox->Resize(mX + 190, mY + 138, 26, 26);
	if (mShowDefsCheckbox != NULL)
		mShowDefsCheckbox->Resize(mX + 190, mY + 164, 26, 26);
}

// 0x42f450 (slot 50): ignores the width and the Dialog formula. OptionsDialogHeight is read on the first
// call only (a function-local static). port: the original adds 36 px per partner link (NumOptionsLinks,
// read the same way, at most 3); the links are not ported, so the dialog is 315 px high with the retail
// partner.xml, where the original's one link made it 351.
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	if ((gOptionsDialogStaticsGuard & 2) == 0)
	{
		gOptionsDialogStaticsGuard |= 2;
		gOptionsDialogHeight = mApp2->GetInteger("OptionsDialogHeight", DEFAULT_DIALOG_HEIGHT);
	}
	return gOptionsDialogHeight;
}

// 0x42f600 (slot 16). DrawString's y is the baseline; coordinates are dialog-relative.
void OptionsDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);

	g->SetFont(mApp2->mFonts[FONT_VERDANA12BOLD]);
	g->SetColorizeImages(true);

	// The slider captions: a black shadow one pixel down-right, then the lines colour (white)
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GameText(GT_OPTIONS_MUSIC), 49, 89);
	g->DrawString(GameText(GT_OPTIONS_SOUND_FX), 49, 121);
	g->SetColor(mColors[COLOR_LINES]);
	g->DrawString(GameText(GT_OPTIONS_MUSIC), 48, 88);
	g->DrawString(GameText(GT_OPTIONS_SOUND_FX), 48, 120);

	// The checkbox captions (no shadow): 28 px right of the box, baseline 18 px below its top
	g->DrawString(GameText(GT_OPTIONS_FULL_SCREEN), mFullscreenCheckbox->mX - mX + 28,
		mFullscreenCheckbox->mY - mY + 18);
	g->DrawString(GameText(GT_OPTIONS_CUSTOM_CURSORS), mCustomCursorsCheckbox->mX - mX + 28,
		mCustomCursorsCheckbox->mY - mY + 18);
	if (m3DCheckbox != NULL)
		g->DrawString(GameText(GT_OPTIONS_HARDWARE_ACCELERATION), m3DCheckbox->mX - mX + 28, m3DCheckbox->mY - mY + 18);
	g->DrawString(GameText(GT_OPTIONS_SHOW_HINTS), mShowHintsCheckbox->mX - mX + 28, mShowHintsCheckbox->mY - mY + 18);
	g->DrawString(GameText(GT_OPTIONS_SHOW_DEFINITIONS), mShowDefsCheckbox->mX - mX + 28,
		mShowDefsCheckbox->mY - mY + 18);

	g->SetColorizeImages(false);
}

// 0x42fa50 (CheckboxListener, +0x124 slot 0). Nothing is applied before the footer button.
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	mApp2->PlaySample(SOUND_BUTTONCLICK);

	// Unticking any box while fullscreen is forced re-ticks full screen (whichever box it was; the nested
	// call comes back here with (37, true) and clicks again)
	if ((!checked) && (mApp2->mForceFullscreen))
		mFullscreenCheckbox->SetChecked(true, true);

	if ((m3DCheckbox != NULL) && (theId == HARDWARE_ACCEL_CHECKBOX_ID))
	{
		m3DCheckbox->mChecked = checked;
		if (checked)
		{
			if (mApp2->Is3DAccelerationSupported())
			{
				if (mApp2->Is3DAccelerationRecommended())
					return;
				mApp2->DoDialog(DIALOG_MESSAGE, true, GameText(GT_OPTIONS_3D_WARNING_HEADER),
					GameText(GT_OPTIONS_3D_WARNING), GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
			}
			else
			{
				m3DCheckbox->mChecked = false;
				mApp2->DoDialog(DIALOG_MESSAGE, true, GameText(GT_OPTIONS_3D_NOT_SUPPORTED_HEADER),
					GameText(GT_OPTIONS_3D_NOT_SUPPORTED), GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
			}
		}
	}
}

// 0x42fbe0 (ButtonListener +0x58 slot 4)
void OptionsDialog::ButtonDepress(int theId)
{
	// The footer button (2001) goes to the app, which calls ApplyAndClose
	Dialog2004::ButtonDepress(theId);

	// port: the original opens a partner link's URL here (id % 1000 > 35, every link being 1036): the links
	// are not ported

	if (theId == HELP_BUTTON_ID)
		mApp2->ShowHelp();         // applies and closes this dialog, then opens the help screen
	else if (theId == REGISTER_BUTTON_ID)
	{
		// Unreachable in the registered build (the button is never added)
		mApp2->mRegSource = "options";
		mApp2->ShowRegisterDialog();
	}
	else if (theId == CREDITS_BUTTON_ID)
		mApp2->ShowCredits();      // CreditsDialog on top of this one
}

// 0x42fc90 (body; scalar deleting dtor 0x42fe60). The dialog is out of the widget manager by now
// (RemovedFromManager removed every control).
OptionsDialog::~OptionsDialog()
{
	delete mRegisterButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mCustomCursorsCheckbox;
	delete mShowHintsCheckbox;
	delete mShowDefsCheckbox;
	delete m3DCheckbox;
	delete mHelpButton;
}

// 0x42fe90. The creator (BookWormApp::ShowOptions) then sets colours 0, 1 (white) and 3 (black, through
// SetColor above), sizes it to 388 x GetPreferredHeight at x = mWidth (off-screen right) and sets
// mSlideOffset = 240.
OptionsDialog::OptionsDialog(BookWormApp* theApp) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON_PLAIN], DIALOG_OPTIONS,
		true, GameText(GT_OPTIONS), "", GameText(GT_OPTIONS_RETURN_TO_GAME), BUTTONS_FOOTER),
	mApp2(theApp),
	mMusicSlider(NULL),
	mSfxSlider(NULL),
	mFullscreenCheckbox(NULL),
	mCustomCursorsCheckbox(NULL),
	mShowHintsCheckbox(NULL),
	mShowDefsCheckbox(NULL),
	m3DCheckbox(NULL),
	mRegisterButton(NULL),
	mHelpButton(NULL),
	mCreditsButton(NULL),
	mSlideOffset(0),
	mSlidePhase(0.0),
	mClosing(false)
{
	mHasAlpha = true;

	SetHeaderFont(theApp->mFonts[FONT_VERDANA18BOLDCOLOR]);
	SetLinesFont(mApp2->mFonts[FONT_VERDANA14BOLD]);
	Font* aButtonFont = mApp2->mFonts[FONT_VERDANA12BOLD];
	SetButtonFont(aButtonFont);

	// Created in every build; only added and laid out when !mBuildUnlocked
	mRegisterButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTON_PLAIN], REGISTER_BUTTON_ID, this);
	mRegisterButton->SetFont(aButtonFont);
	mRegisterButton->mLabel = mApp2->GetString("RegisterLabel", GameText(GT_OPTIONS_REGISTER));
	mRegisterButton->SetColors(gOptionsButtonColors, 2);

	mHelpButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTON_PLAIN], HELP_BUTTON_ID, this);
	mHelpButton->SetFont(aButtonFont);
	mHelpButton->mLabel = GameText(GT_OPTIONS_HELP);
	mHelpButton->SetColors(gOptionsButtonColors, 2);

	mCreditsButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTON_PLAIN], CREDITS_BUTTON_ID, this);
	mCreditsButton->SetFont(aButtonFont);
	mCreditsButton->mLabel = GameText(GT_OPTIONS_CREDITS);
	mCreditsButton->SetColors(gOptionsButtonColors, 2);

	// port: the partner links (properties/partner.xml NumOptionsLinks, OptionsLinkText%d,
	// OptionsLinkURL%d; the retail file has one, GT_OPTIONS_MORE_GAMES, opening a PopCap page) are not
	// ported, the user's decision (2026-10-24)

	mMusicSlider = new Slider2004(theApp->mImages[IMAGE_SLIDERTRACK], theApp->mImages[IMAGE_SLIDERTHUMB],
		MUSIC_SLIDER_ID, this);
	mMusicSlider->SetValue(theApp->GetMusicVolume());
	mSfxSlider = new Slider2004(theApp->mImages[IMAGE_SLIDERTRACK], theApp->mImages[IMAGE_SLIDERTHUMB],
		SFX_SLIDER_ID, this);
	mSfxSlider->SetValue(theApp->GetSfxVolume());

	// The checkboxes get their state directly (no listener call)
	mFullscreenCheckbox = new Checkbox(mApp2->mCheckboxOffImage, mApp2->mCheckboxOnImage, FULLSCREEN_CHECKBOX_ID, this);
	mFullscreenCheckbox->mChecked = !theApp->mIsWindowed;
	mCustomCursorsCheckbox = new Checkbox(mApp2->mCheckboxOffImage, mApp2->mCheckboxOnImage,
		CUSTOM_CURSORS_CHECKBOX_ID, this);
	mCustomCursorsCheckbox->mChecked = theApp->mCustomCursorsEnabled;
	mShowHintsCheckbox = new Checkbox(mApp2->mCheckboxOffImage, mApp2->mCheckboxOnImage, SHOW_HINTS_CHECKBOX_ID, this);
	mShowHintsCheckbox->mChecked = theApp->mShowHints;
	mShowDefsCheckbox = new Checkbox(mApp2->mCheckboxOffImage, mApp2->mCheckboxOnImage, SHOW_DEFS_CHECKBOX_ID, this);
	mShowDefsCheckbox->mChecked = theApp->mShowDefs;
	m3DCheckbox = new Checkbox(mApp2->mCheckboxOffImage, mApp2->mCheckboxOnImage, HARDWARE_ACCEL_CHECKBOX_ID, this);
	m3DCheckbox->mChecked = mApp2->Is3DAccelerated();

	mYesButton->SetColors(gOptionsButtonColors, 2);
}
