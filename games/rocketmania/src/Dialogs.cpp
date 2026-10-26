// The dialogs of Rocket Mania Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "Board.h"
#include "RocketApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "WidgetManager.h"

#include <cstdio>

using namespace Sexy;

// the game's dialog buttons: the Showcard font, the label of the given text
static DialogButton* MakeDialogButton(RocketApp* theApp, int theId, ButtonListener* theListener, const SexyString& theLabel)
{
	DialogButton* aButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTON], theId, theListener);
	aButton->SetFont(theApp->mFonts[FONT_SHOWCARD22]);
	aButton->mLabel = theLabel;
	return aButton;
}

// ---------------------------------------------------------------------------------------------------------
// DifficultyDialog

// 0x404380
DifficultyDialog::DifficultyDialog(RocketApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_DIFFICULTY, true,
		GT(0x004e44b8), "", "", BUTTONS_NONE),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_SHOWCARD22]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);

	mButtons[0] = MakeDialogButton(mApp, 0, this, RocketApp::DifficultyNameUpper(1));
	mButtons[1] = MakeDialogButton(mApp, 1, this, RocketApp::DifficultyNameUpper(2));
	mButtons[2] = MakeDialogButton(mApp, 2, this, RocketApp::DifficultyNameUpper(3));
	mButtons[3] = MakeDialogButton(mApp, 3, this, GT(0x004e44b0));
}

DifficultyDialog::~DifficultyDialog()
{
	for (int i = 0; i < 4; i++)
		delete mButtons[i];
}

// 0x404960
void DifficultyDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->AddWidget(mButtons[i]);
}

void DifficultyDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->RemoveWidget(mButtons[i]);
}

// 0x404a00: colour 3 is the buttons' label colour
void DifficultyDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		for (int i = 0; i < 4; i++)
			if (mButtons[i] != NULL)
				mButtons[i]->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	}
}

// 0x404a70: the buttons hang from the bottom
void DifficultyDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aWidth = theWidth - 0x40;
	mButtons[0]->Resize(mX + 0x20, mY + theHeight - 300, aWidth, 0x32);
	mButtons[1]->Resize(mX + 0x20, mY + theHeight - 0xf0, aWidth, 0x32);
	mButtons[2]->Resize(mX + 0x20, mY + theHeight - 0xb4, aWidth, 0x32);
	mButtons[3]->Resize(mX + 0x20, mY + theHeight - 0x55, aWidth, 0x28);
}

// 0x404b20: the picture and the title (the dialog's own header text is not drawn)
void DifficultyDialog::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);
	g->SetFont(mApp->mFonts[FONT_SHOWCARD22]);
	g->SetColor(mApp->mDialogColors[0]);
	const SexyString& aTitle = GT(0x004e44c4);
	g->DrawString(aTitle, mWidth / 2 - g->GetFont()->StringWidth(aTitle) / 2, 0x41);
}

// 0x404cd0: every button but the dialog's own plays the click
void DifficultyDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + 2000) && (theId != mId + 3000))
		mApp->ButtonPress(0);
}

// 0x404d10: EASY, MEDIUM and HARD start the game, CANCEL goes back to the menu
void DifficultyDialog::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
	case 1:
	case 2:
		mApp->mGameSelector->mDifficulty = theId + 1;
		break;
	case 3:
		mApp->KillDialog(DIALOG_DIFFICULTY);
		mApp->mGameSelector->ResetHead();
		return;
	default:
		Dialog2004::ButtonDepress(theId);
		return;
	}
	mApp->KillDialog(DIALOG_DIFFICULTY);
	mApp->mGameSelector->StartGame(false);
}

// ---------------------------------------------------------------------------------------------------------
// ContinueDialog

// 0x402630
ContinueDialog::ContinueDialog(RocketApp* theApp, int theMode) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_CONTINUE, true,
		GT(0x004e4250), "", "", BUTTONS_NONE),
	mApp(theApp),
	mMode(theMode),
	mSavedDifficulty(0),
	mSavedScore(0),
	mSavedLevel(0)
{
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_SHOWCARD22]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);

	mNewButton = MakeDialogButton(mApp, 0, this, GT(0x004e4244));
	mContinueButton = MakeDialogButton(mApp, 1, this, GT(0x004e4230));

	mApp->ReadSavedGame(mMode, &mSavedDifficulty, &mSavedScore, &mSavedLevel);
}

ContinueDialog::~ContinueDialog()
{
	delete mNewButton;
	delete mContinueButton;
}

// 0x402aa0
void ContinueDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNewButton);
	theWidgetManager->AddWidget(mContinueButton);
}

void ContinueDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNewButton);
	theWidgetManager->RemoveWidget(mContinueButton);
}

// 0x402a50
void ContinueDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mNewButton != NULL)
			mNewButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
		if (mContinueButton != NULL)
			mContinueButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	}
}

// 0x402b20
void ContinueDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNewButton->Resize(mX + 0x20, mY + theHeight - 0x96, theWidth - 0x40, 0x32);
	mContinueButton->Resize(mX + 0x20, mY + theHeight - 0x5a, theWidth - 0x40, 0x32);
}

// 0x402ba0: the title, the four lines of the question and what the saved game holds
void ContinueDialog::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);

	g->SetFont(mApp->mFonts[FONT_SHOWCARD22]);
	g->SetColor(mApp->mDialogColors[0]);
	const SexyString& aTitle = GT(0x004e4320);
	g->DrawString(aTitle, mWidth / 2 - g->GetFont()->StringWidth(aTitle) / 2, 0x41);

	g->SetColor(mApp->mDialogColors[1]);
	g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	static const struct { unsigned int mText; int mY; } aLines[4] =
	{
		{0x004e42fc, 0x7d}, {0x004e42e0, 0x9b}, {0x004e42c8, 0xb9}, {0x004e42a0, 0xeb}
	};
	for (int i = 0; i < 4; i++)
	{
		const SexyString& aLine = GT(aLines[i].mText);
		g->DrawString(aLine, mWidth / 2 - g->GetFont()->StringWidth(aLine) / 2, aLines[i].mY);
	}

	// what the saved game holds: strategy has no levels
	g->SetFont(mApp->mFonts[FONT_ARIAL10]);
	char aBuf[256];
	const char* aDifficulty = RocketApp::DifficultyName(mSavedDifficulty).c_str();
	if (mMode == 2)
		snprintf(aBuf, sizeof(aBuf), GTC(0x004e4284), mSavedScore, aDifficulty);
	else
		snprintf(aBuf, sizeof(aBuf), GTC(0x004e425c), mSavedLevel + 1, mSavedScore, aDifficulty);
	SexyString aSummary = aBuf;
	g->DrawString(aSummary, mWidth / 2 - g->GetFont()->StringWidth(aSummary) / 2, mHeight - 0x14);
}

// 0x403340
void ContinueDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + 2000) && (theId != mId + 3000))
		mApp->ButtonPress(0);
}

// 0x403380: NEW GAME asks for a difficulty, CONTINUE SAVED GAME loads the game
void ContinueDialog::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		mApp->KillDialog(DIALOG_CONTINUE);
		mApp->mGameSelector->ShowDifficultyDialog();
	}
	else if (theId == 1)
	{
		// (the original opens the register-to-load dialog here for an unregistered game)
		// 0x403570: the saved game's mode and difficulty are the ones the game starts with
		mApp->mGameSelector->mMode = mMode;
		mApp->mGameSelector->mDifficulty = mSavedDifficulty;
		mApp->KillDialog(DIALOG_CONTINUE);
		mApp->mGameSelector->ResetHead();
		mApp->mGameSelector->StartGame(true);
		// the board that was just made takes the saved game, which is then deleted
		if (mApp->mBoard != NULL)
			mApp->mBoard->Load();
		mApp->DeleteSavedGame(mMode);
	}
	else
		Dialog2004::ButtonDepress(theId);
}

// ---------------------------------------------------------------------------------------------------------
// MoneyDialog

// 0x4118d0 and the Dialog constructor 0x433e8c: the game's dialog picture and button picture
MoneyDialog::MoneyDialog(RocketApp* theApp, int theId, bool isModal, const SexyString& theHeader,
	const SexyString& theLines, const SexyString& theFooter, int theButtonMode) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], theId, isModal, theHeader, theLines,
		theFooter, theButtonMode),
	mApp(theApp),
	mPhase(0.0)
{
}

// 0x411b20: the phase goes round once in 2000 updates, and the dialog is drawn again each time
void MoneyDialog::Update()
{
	Dialog2004::Update();
	mPhase += 0.0005;
	if (mPhase > 1.0)
		mPhase -= 1.0;
	MarkDirty();
}

// 0x411980: black behind the picture. (The original goes on to work out 40 points along the border from the phase and
// switches the draw mode to additive and back, drawing nothing with them.)
void MoneyDialog::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0));
	g->FillRect(8, 8, mWidth - 16, mHeight - 16);
	Dialog2004::Draw(g);
}

// ---------------------------------------------------------------------------------------------------------
// OptionsDialog

// 0x411ba0
OptionsDialog::OptionsDialog(RocketApp* theApp, bool theFromMenu) :
	MoneyDialog(theApp, DIALOG_OPTIONS, true, GT(0x004e55f8), "", GT(0x004e4350), BUTTONS_FOOTER),
	mFromMenu(theFromMenu),
	mLastSoundUpdate(mUpdateCnt - 100),
	mMusicSlider(NULL),
	mSfxSlider(NULL),
	mFullscreenCheckbox(NULL),
	m3DCheckbox(NULL),
	mCursorsCheckbox(NULL),
	mHintsCheckbox(NULL),
	mResetHintsButton(NULL),
	mCreditsButton(NULL)
{
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_VERDANA18_BOLD]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);

	mResetHintsButton = MakeDialogButton(mApp, BUTTON_RESET_HINTS, this, GT(0x004e55d4));
	mResetHintsButton->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	mCreditsButton = MakeDialogButton(mApp, BUTTON_CREDITS, this, GT(0x004e55cc));
	mCreditsButton->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);

	mMusicSlider = new Slider2004(mApp->mImages[IMAGE_SLIDERTRACK], mApp->mImages[IMAGE_SLIDERTHUMB], 0, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mSfxSlider = new Slider2004(mApp->mImages[IMAGE_SLIDERTRACK], mApp->mImages[IMAGE_SLIDERTHUMB], 1, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());

	mFullscreenCheckbox = new Checkbox(mApp->mCheckUnchecked, mApp->mCheckChecked, CHECK_FULLSCREEN, this);
	mFullscreenCheckbox->SetChecked(!mApp->mIsWindowed);
	m3DCheckbox = new Checkbox(mApp->mCheckUnchecked, mApp->mCheckChecked, CHECK_HARDWARE, this);
	m3DCheckbox->mChecked = mApp->Is3DAccelerated();
	mCursorsCheckbox = new Checkbox(mApp->mCheckUnchecked, mApp->mCheckChecked, CHECK_CURSORS, this);
	mCursorsCheckbox->SetChecked(mApp->mCustomCursorsEnabled);
	mHintsCheckbox = new Checkbox(mApp->mCheckUnchecked, mApp->mCheckChecked, CHECK_HINTS, this);
	mHintsCheckbox->SetChecked(mApp->mNoHints);
}

OptionsDialog::~OptionsDialog()
{
	delete mResetHintsButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete m3DCheckbox;
	delete mCursorsCheckbox;
	delete mHintsCheckbox;
}

// 0x4127c0: colour 3 is the buttons' label colour
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	MoneyDialog::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mResetHintsButton != NULL)
			mResetHintsButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
		if (mCreditsButton != NULL)
			mCreditsButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	}
}

// 0x412860
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	MoneyDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCursorsCheckbox);
	theWidgetManager->AddWidget(mHintsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(m3DCheckbox);
	theWidgetManager->AddWidget(mResetHintsButton);
	theWidgetManager->AddWidget(mCreditsButton);
}

// 0x412940
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MoneyDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(m3DCheckbox);
	theWidgetManager->RemoveWidget(mCursorsCheckbox);
	theWidgetManager->RemoveWidget(mHintsCheckbox);
	theWidgetManager->RemoveWidget(mResetHintsButton);
	theWidgetManager->RemoveWidget(mCreditsButton);
}

// 0x412a20: sliders, then the checkboxes (the hardware acceleration one hangs from the hints one), then the two buttons
// that share the width under them. (The original also puts the register button, the update check and the partner links
// around them.)
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	MoneyDialog::Resize(theX, theY, theWidth, theHeight);
	mMusicSlider->Resize(mX + 0x98, mY + 0x35, mWidth - 0xc0, 0x24);
	mSfxSlider->Resize(mX + 0x98, mY + 0x55, mWidth - 0xc0, 0x24);
	mFullscreenCheckbox->Resize(mX + 0x30, mY + 0x7b, 0x18, 0x18);
	mCursorsCheckbox->Resize(mX + 0xb0, mY + 0x7b, 0x18, 0x18);
	mHintsCheckbox->Resize(mX + 0x30, mY + 0x94, 0x18, 0x18);
	m3DCheckbox->Layout(LAY_SameSize | LAY_SameLeft | LAY_Below, mHintsCheckbox);

	int aHalf = ((mWidth - mContentInsets.mLeft) - mContentInsets.mRight) / 2;
	mResetHintsButton->Resize(mX + mContentInsets.mLeft, mY + 0xd0, aHalf, 0x24);
	mCreditsButton->Resize(mX + aHalf + mContentInsets.mLeft, mY + 0xd0, aHalf, 0x24);
}

// 0x412770: the original's 0x156, less 0x24 when the update check is switched off (as it is here), less 0x24 when there
// is no register button (the game is registered), plus 0x24 for each partner link (none here)
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return 0x156 - 0x24 - 0x24 + 0x24;
}

// 0x412c60: the captions (the dialog's lines colour is still set)
void OptionsDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);
	g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	g->DrawString(GT(0x004e565c), 0x30, 0x4d);
	g->DrawString(GT(0x004e5650), 0x30, 0x6d);
	g->DrawString(GT(0x004e5644), mFullscreenCheckbox->mX - mX + 0x1c, mFullscreenCheckbox->mY - mY + 0x11);
	g->DrawString(GT(0x004e5634), mCursorsCheckbox->mX - mX + 0x1c, mCursorsCheckbox->mY - mY + 0x11);
	g->DrawString(GT(0x004e5618), mHintsCheckbox->mX - mX + 0x1c, mHintsCheckbox->mY - mY + 0x11);
	g->DrawString(GT(0x004e5600), m3DCheckbox->mX - mX + 0x1c, m3DCheckbox->mY - mY + 0x11);
}

// 0x413470: every button but the dialog's own plays the click
void OptionsDialog::ButtonPress(int theId)
{
	MoneyDialog::ButtonPress(theId);
	if ((theId != mId + 2000) && (theId != mId + 3000))
		mApp->ButtonPress(0);
}

// 0x4134b0
void OptionsDialog::ButtonDepress(int theId)
{
	// DONE goes to the app (RocketApp::ButtonDepress applies the choices and closes the dialog)
	MoneyDialog::ButtonDepress(theId);
	if (theId == mId + 2000)
	{
		if (!mFromMenu)
			mApp->mMusicInterface->FadeOut(0, true, 0.02);
		else
			mApp->mGameSelector->ResetHead();
		return;
	}
	// (the original's other buttons: register, check for updates, the partner links, a PopCap web page: not ported)
	if (theId == BUTTON_CREDITS)
		mApp->ShowCredits();
	else if (theId == BUTTON_RESET_HINTS)
	{
		// every hint is shown again, and the button waits dimmed until the dialog is opened again
		mApp->mNoHints = false;
		mHintsCheckbox->SetChecked(mApp->mNoHints);
		mResetHintsButton->SetDisabled(true);
		const Color& aColor = mApp->mDialogColors[2];
		mResetHintsButton->SetColor(ButtonWidget::COLOR_LABEL, Color(aColor.mRed, aColor.mGreen, aColor.mBlue, 0x80));
		for (int i = 0; i < 8; i++)
			mApp->mHintFlags[i] = false;
	}
}

// 0x413040: the volume sliders act at once; the sound one clicks while it is dragged, at most once in 50 updates
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
	{
		mApp->SetSfxVolume(theVal);
		if (mUpdateCnt - mLastSoundUpdate > 0x32)
		{
			mApp->ButtonPress(5);
			mLastSoundUpdate = mUpdateCnt;
		}
	}
}

// 0x4130b0: nothing is applied before DONE, except the hints option. Hardware acceleration says when the computer
// cannot do it, or may not do it well.
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	if (theId == CHECK_FULLSCREEN)
	{
		// the game could not run in a window (the user was asked and chose full screen): it stays full screen
		if (!checked && mApp->mForceFullscreen)
			mFullscreenCheckbox->SetChecked(true);
	}
	else if (theId == CHECK_HINTS)
		mApp->mNoHints = checked;
	else if ((theId == CHECK_HARDWARE) && checked)
	{
		if (!mApp->Is3DAccelerationSupported())
		{
			m3DCheckbox->SetChecked(false);
			Dialog* aDialog = mApp->DoDialog(DIALOG_MESSAGE, true, GT(0x004e56f0), GT(0x004e5700), GT(0x004e5780),
				BUTTONS_FOOTER);
			mApp->CenterDialog(aDialog);
		}
		else if (!mApp->Is3DAccelerationRecommended())
		{
			Dialog* aDialog = mApp->DoDialog(DIALOG_MESSAGE, true, GT(0x004e5664), GT(0x004e5670), GT(0x004e5780),
				BUTTONS_FOOTER);
			mApp->CenterDialog(aDialog);
		}
	}
}
