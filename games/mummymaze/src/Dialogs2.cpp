// The options dialog (WinMM.exe 0x4250f0) and the high scores dialog (0x413c30).

#include "Button2003.h"
#include "Dialogs.h"
#include "MummyApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Res.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "WidgetManager.h"

#include <cstdio>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// OptionsDialog

// 0x4250f0
OptionsDialog::OptionsDialog(MummyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_OPTIONS, true, GT(0x004950e8), "",
		GT(0x00491f90), BUTTONS_FOOTER),
	mApp(theApp)
{
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);

	// (the original also has a Register button, id 1, and a button that opens the game's web page, id 3: neither is here)
	struct Spec { DialogButton** mButton; int mId; unsigned int mText; };
	Spec aSpecs[3] =
	{
		{&mTutorialButton, 0, 0x004950dc}, {&mScoresButton, 2, 0x004950c4}, {&mCreditsButton, 4, 0x004950a8}
	};
	for (int i = 0; i < 3; i++)
	{
		DialogButton* aButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], aSpecs[i].mId, this);
		aButton->SetFont(mApp->mFont1);
		aButton->mLabel = GT(aSpecs[i].mText);
		*aSpecs[i].mButton = aButton;
	}
	// (the original also makes a sixth button, id 5, "Reset Tips"; it is added but never given a place, so it can not be
	// seen or clicked, and it is left out here)

	mMusicSlider = new Slider(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERANKH], 0, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mSfxSlider = new Slider(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERANKH], 1, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());
	mSpeedSlider = new Slider(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERANKH], 2, this);
	mSpeedSlider->SetValue(mApp->mSpeed == 2 ? 0.5 : (mApp->mSpeed == 4 ? 1.0 : 0.0));

	// the check box art: the unchecked picture is the left half of checked.gif, the checked one the right
	MemoryImage* aChecked = mCheckedImage = new MemoryImage(mApp);
	aChecked->Create(0x24, 0x25);
	{
		Graphics g(aChecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], -0x24, 0);
	}
	MemoryImage* aUnchecked = mUncheckedImage = new MemoryImage(mApp);
	aUnchecked->Create(0x24, 0x25);
	{
		Graphics g(aUnchecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], 0, 0);
	}
	mFullscreenCheckbox = new Checkbox(aUnchecked, aChecked, 0, this);
	mFullscreenCheckbox->SetChecked(!mApp->mIsWindowed, false);
	mCursorsCheckbox = new Checkbox(aUnchecked, aChecked, 1, this);
	mCursorsCheckbox->SetChecked(mApp->mCustomCursorsEnabled, false);
	mClockCheckbox = new Checkbox(aUnchecked, aChecked, 2, this);
	mClockCheckbox->SetChecked(mApp->mShowClock, false);
	mAnkhCheckbox = new Checkbox(aUnchecked, aChecked, 3, this);
	mAnkhCheckbox->SetChecked(mApp->mShowAnkh, false);
}

// 0x425e10
OptionsDialog::~OptionsDialog()
{
	delete mTutorialButton;
	delete mScoresButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mSpeedSlider;
	delete mFullscreenCheckbox;
	delete mCursorsCheckbox;
	delete mClockCheckbox;
	delete mAnkhCheckbox;
	delete mCheckedImage;
	delete mUncheckedImage;
}

// 0x425ff0
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	Widget* aWidgets[] = {mTutorialButton, mScoresButton, mMusicSlider, mSfxSlider, mSpeedSlider,
		mFullscreenCheckbox, mCursorsCheckbox, mClockCheckbox, mAnkhCheckbox, mCreditsButton};
	for (size_t i = 0; i < sizeof(aWidgets) / sizeof(aWidgets[0]); i++)
		theWidgetManager->AddWidget(aWidgets[i]);
}

// 0x4260c0
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	Widget* aWidgets[] = {mTutorialButton, mScoresButton, mMusicSlider, mSfxSlider, mSpeedSlider,
		mFullscreenCheckbox, mCursorsCheckbox, mClockCheckbox, mAnkhCheckbox, mCreditsButton};
	for (size_t i = 0; i < sizeof(aWidgets) / sizeof(aWidgets[0]); i++)
		theWidgetManager->RemoveWidget(aWidgets[i]);
}

// 0x426190: two rows of buttons above the footer button, then the sliders and the switches (without the Register and the web
// page buttons, the Tutorial and Scores buttons share the first row and the Credits button takes the second)
// 0x4261f0, 0x426240
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aContent = (mWidth - mContentInsets.mLeft) - mContentInsets.mRight;
	int aHalf = aContent / 2;
	int aFooterY = (mYesButton != NULL) ? mYesButton->mY : mY + mHeight - 0x50;
	mTutorialButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x54, aHalf - 3, 0x28);
	mScoresButton->Resize(mX + aHalf + 6 + mContentInsets.mLeft, aFooterY - 0x54, aHalf - 3, 0x28);
	mCreditsButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x2a, aContent, 0x28);
	mMusicSlider->Resize(mX + 0x98, mY + 0x44, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 0x64, mWidth - 0xc0, 0x20);
	mSpeedSlider->Resize(mX + 0x98, mY + 0x84, mWidth - 0xc0, 0x20);
	mFullscreenCheckbox->Resize(mX + 0x30, mY + 0xbc, 0x24, 0x25);
	mCursorsCheckbox->Resize(mX + 0xcc, mY + 0xbc, 0x24, 0x25);
	mClockCheckbox->Resize(mX + 0x30, mY + 0xe2, 0x24, 0x25);
	mAnkhCheckbox->Resize(mX + 0xcc, mY + 0xe2, 0x24, 0x25);
}

// 0x425f50
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x1b6;
}

// 0x425f60: the button colour reaches the dialog's own buttons
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		DialogButton* aButtons[3] = {mTutorialButton, mScoresButton, mCreditsButton};
		for (int i = 0; i < 3; i++)
			if (aButtons[i] != NULL)
				aButtons[i]->SetColor(0, theColor);
	}
}

// 0x4263c0
void OptionsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0x2e, 0x14, 0));
	g->DrawString(GT(0x00495150), 0x30, 0x58);
	g->DrawString(GT(0x00495144), 0x30, 0x78);
	g->DrawString(GT(0x00493e8c), 0x30, 0x98);
	SexyString aSpeed = GT(mApp->mSpeed == 1 ? 0x00495138 : (mApp->mSpeed == 2 ? 0x00495130 : 0x00495124));
	g->DrawString(aSpeed, ((mSpeedSlider->mWidth - g->GetFont()->StringWidth(aSpeed)) / 2 + mSpeedSlider->mX) - mX,
		(mSpeedSlider->mY - mY) + 0x2e);
	Checkbox* aBoxes[4] = {mFullscreenCheckbox, mCursorsCheckbox, mClockCheckbox, mAnkhCheckbox};
	unsigned int aTexts[4] = {0x00495118, 0x00495108, 0x004950fc, 0x004950f0};
	for (int i = 0; i < 4; i++)
		g->DrawString(GT(aTexts[i]), (aBoxes[i]->mX - mX) + 0x28, (aBoxes[i]->mY - mY) + 0x15);
}

// 0x426b50: every press of a button of the dialog clicks; the footer button's also reaches the app, which clicks too
void OptionsDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(SOUND_CLICK);
}

// 0x426b70, 0x41ca70 (its question for the Tutorial button): the dialog's own buttons (the footer button is the app's: ButtonDepress(2005))
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);
	switch (theId)
	{
	case 0:
		mApp->DoMessage(DIALOG_STARTED_TUTORIAL, GT(0x00492228), GT(0x004940f8), 1, 0xa2, 0x52, 0x13c);
		break;
	case 2:
		mApp->ShowHighScoreDialog();
		break;
	case 4:
		mApp->DoMessage(DIALOG_CREDITS, GT(0x00495158), GT(0x00495160), 3, 0xa2, 0x52, 0x13c);
		break;
	}
}

// 0x426a30: the volumes, and the speed in three steps
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
		mApp->SetSfxVolume(theVal);
	else if (theId == 2)
	{
		if (0.33 <= theVal)
			mApp->mSpeed = (theVal <= 0.66) ? 2 : 4;
		else
			mApp->mSpeed = 1;
		mSpeedSlider->SetValue(mApp->mSpeed == 2 ? 0.5 : (mApp->mSpeed == 4 ? 1.0 : 0.0));
		MarkDirty();
	}
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreDialog

// 0x413c30
HighScoreDialog::HighScoreDialog(MummyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_HIGH_SCORES, true, GT(0x00491f84), "",
		GT(0x00491f90), BUTTONS_FOOTER),
	mApp(theApp)
{
	mScores = new HighScoreScreen(theApp);
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	SetColor(Dialog::COLOR_HEADER, Color(0x66, 0x5c, 0x38));
}

HighScoreDialog::~HighScoreDialog()
{
	delete mScores;
}

// 0x413f80
int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 300;
}

// 0x413f90: the five scores
void HighScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFont1);
	int aY = 0x5e;
	for (int i = 0; i < 5; i++)
	{
		g->SetColor(Color(0x2e, 0x14, 0));
		g->DrawString(mScores->mNames[i], 0x32, aY);
		char aBuf[32];
		sprintf(aBuf, "%d", mScores->mScores[i]);
		g->DrawString(aBuf, (mWidth - mApp->mFont1->StringWidth(aBuf)) - 0x32, aY);
		aY += 0x16;
	}
}

// ---------------------------------------------------------------------------------------------------------
// TipDialog

// the lines of each tip (the addresses of their texts) and the first line's place: four lines from 0x5f, the tip of the
// traps three from 0x69
static const unsigned int kTipLines[7][4] =
{
	{0x00495b70, 0x00495b4c, 0x00495b24, 0x00495b00},
	{0x00495ad8, 0x00495ab4, 0x00495a8c, 0x00495a6c},
	{0x00495a44, 0x00495a1c, 0x004959f8, 0},
	{0x004959d8, 0x004959b0, 0x00495988, 0x00495970},
	{0x0049594c, 0x00495924, 0x00495900, 0x004958e8},
	{0x004958c8, 0x004958a8, 0x0049588c, 0x0049587c},
	{0x00495854, 0x00495828, 0x004957fc, 0x004957e0}
};

// 0x42b720
TipDialog::TipDialog(MummyApp* theApp, int theWhich) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_TIP, true, "", "", GT(0x004955b4),
		BUTTONS_FOOTER),
	mApp(theApp),
	mWhich(theWhich)
{
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);
	MemoryImage* aChecked = mCheckedImage = new MemoryImage(mApp);
	aChecked->Create(0x24, 0x25);
	{
		Graphics g(aChecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], -0x24, 0);
	}
	MemoryImage* aUnchecked = mUncheckedImage = new MemoryImage(mApp);
	aUnchecked->Create(0x24, 0x25);
	{
		Graphics g(aUnchecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], 0, 0);
	}
	mCheckbox = new Checkbox(aUnchecked, aChecked, 0, this);
}

// 0x42bb00 (its deleting form 0x42bae0); the two pictures of the check box are freed here, and are not in the original
TipDialog::~TipDialog()
{
	delete mCheckbox;
	delete mCheckedImage;
	delete mUncheckedImage;
}

// 0x412d80
void TipDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(SOUND_CLICK);
}

// 0x42bba0
void TipDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mCheckbox);
}

// 0x42bbd0
void TipDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mCheckbox);
}

// 0x42bc00
void TipDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mCheckbox->Resize(mX + 0x41, mY + 0xa8, 0x24, 0x25);
}

// 0x42bb80
int TipDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x136;
}

// 0x42bc50: the warning and its lines, and the label of the box
void TipDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0xff, 0xff, 0xff));
	SexyString aWarning = GT(0x00495b90);
	g->DrawString(aWarning, (mWidth - g->GetFont()->StringWidth(aWarning)) / 2, 0x3a);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0x2e, 0x14, 0));
	int aY = (mWhich == 2) ? 0x69 : 0x5f;
	for (int i = 0; i < 4 && kTipLines[mWhich][i] != 0; i++)
	{
		const SexyString& aLine = GT(kTipLines[mWhich][i]);
		g->DrawString(aLine, (mWidth - g->GetFont()->StringWidth(aLine)) / 2, aY);
		aY += 0x14;
	}
	g->SetColor(Color(2, 2, 6));
	g->DrawString(GT(0x004955bc), (mCheckbox->mX - mX) + 0x2c, (mCheckbox->mY - mY) + 0x15);
}
