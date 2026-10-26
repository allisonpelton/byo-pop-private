// The dialogs of Rocket Mania Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "Board.h"
#include "Buffer.h"
#include "RocketApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>
#include <ctime>

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
			mApp->PlaySample(SOUND_TIMER_DANGER_06);
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

// ---------------------------------------------------------------------------------------------------------
// CreditsDialog

// 0x403990
CreditsDialog::CreditsDialog(RocketApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_CREDITS, true, "", "",
		GT(0x004e4350), BUTTONS_FOOTER),
	mApp(theApp),
	mPhase(0.0),
	mScroll1(0.0),
	mScroll2(0.0)
{
	mContentInsets = Insets(0x24, 0x24, 0x24, 0x14);
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	// port: the original leaves the lines font unset (it has no lines); the compat dialog's Draw measures with it
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
}

// 0x403c10
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return 0x18c;
}

// 0x403c20: the picture with its inner edge shaded, and the names
void CreditsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);

	// six outlines, each a pixel further in and fainter, along the edge of the field the names are on
	Graphics aShade(*g);
	int aShadeX = 0x27;
	int aShadeY = 0x22;
	int aShadeWidth = mWidth - 0x4e;
	int aShadeHeight = mHeight - 0x6e;
	aShade.ClipRect(Rect(aShadeX, aShadeY, aShadeWidth, aShadeHeight));
	int anAlpha = 0x5fa;
	int anIndex = 0;
	int anInset = 0;
	do
	{
		aShade.SetColor(Color(0x30, 0x30, 0x30, anAlpha / 7));
		aShade.DrawRect(anIndex + aShadeX, aShadeY + anIndex, aShadeWidth - anInset, aShadeHeight - anInset);
		anAlpha -= 0xff;
		anIndex++;
		anInset += 2;
	} while (0 < anAlpha);

	g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
	g->SetColor(mApp->mDialogColors[0]);
	WriteCenteredLine(g, 0x3e, GT(0x004e447c));

	g->SetFont(mApp->mFonts[FONT_ARIAL10]);
	g->SetColor(mApp->mDialogColors[0]);
	WriteCenteredLine(g, 0x54, GT(0x004e4458));
	WriteCenteredLine(g, 0x6a, GT(0x004e4434));
	WriteCenteredLine(g, 0x80, GT(0x004e4424));
	g->SetColor(mApp->mDialogColors[0]);
	WriteCenteredLine(g, 0x95, GT(0x004e4414));

	g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
	g->SetColor(mApp->mDialogColors[0]);
	WriteCenteredLine(g, 0xbe, GT(0x004e4404));

	g->SetFont(mApp->mFonts[FONT_ARIAL10]);
	g->SetColor(mApp->mDialogColors[0]);
	WriteCenteredLine(g, 0xd4, GT(0x004e43d8));
	WriteCenteredLine(g, 0xea, GT(0x004e43a8));
	WriteCenteredLine(g, 0x100, GT(0x004e438c));
	WriteCenteredLine(g, 0x12c, GT(0x004e4358));
}

// 0x4042c0
void CreditsDialog::Update()
{
	Dialog2004::Update();
	mPhase += 0.004;
	mScroll1 += 0.2;
	if (mScroll1 > 0.0)
		mScroll1 -= 512.0;
	mScroll2 += 0.3;
	if (mScroll2 > 0.0)
		mScroll2 -= 512.0;
	MarkDirty();
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreDisplayDialog

// the lists' file (the original names it "Players//hiscores.dat")
static const char* const HIGH_SCORE_FILE = "Players/hiscores.dat";

// 0x406c40: the day as a number (the year times 366 plus the day of the year); the best score of a day is only kept for
// that day
static int DayNumber()
{
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	if (aTime == NULL)
		return 0;
	return aTime->tm_year * 366 + aTime->tm_yday;
}

// the check the file keeps for each name and score (0x406c80, 0x407080); a name shorter than theLength counts as zeros
static unsigned int ScoreCheck(const SexyString& theName, int theLength, int theScore)
{
	unsigned int aCheck = theScore ^ 0x42beef;
	for (int i = 0; i < theLength; i++)
	{
		int aChar = (i < (int) theName.size()) ? (signed char) theName[i] : 0;
		aCheck ^= aChar * 0x11;
	}
	return aCheck;
}

// 0x407880: the ending of an ordinal number
static const SexyString& OrdinalSuffix(int theNumber)
{
	if ((theNumber % 100) / 10 != 1)
	{
		int aDigit = theNumber % 10;
		if (aDigit == 1)
			return GT(0x004e4990);
		if (aDigit == 2)
			return GT(0x004e4994);
		if (aDigit == 3)
			return GT(0x004e4998);
	}
	return GT(0x004e499c);
}

// 0x4064e0
HighScoreDisplayDialog::HighScoreDisplayDialog(RocketApp* theApp, int theOpenedFrom) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_HIGH_SCORES, true, "", "", "",
		BUTTONS_NONE),
	mApp(theApp),
	mOpenedFrom(theOpenedFrom),
	mTodayMode(-1),
	mHighlightMode(-1),
	mHighlightRank(-1),
	mCheckPending(false),
	mTick(0)
{
	// the lists begin with 30 names of the game's makers and friends, taken in turn from two lists (13 and 17 names), and
	// scores from 10000 down in steps of 250
	static const unsigned int aFirstNames[13] = {0x004e47e4, 0x004e47e0, 0x004e47d4, 0x004e47c8, 0x004e47c0, 0x004e47b8,
		0x004e47b0, 0x004e47ac, 0x004e47a4, 0x004e479c, 0x004e4794, 0x004e4788, 0x004e4780};
	static const unsigned int aSecondNames[17] = {0x004e4778, 0x004e4770, 0x004e4760, 0x004e4758, 0x004e4750, 0x004e4744,
		0x004e473c, 0x004e4730, 0x004e4728, 0x004e471c, 0x004e4714, 0x004e4704, 0x004e46fc, 0x004e46f4, 0x004e46ec,
		0x004e46e4, 0x004e46d8};
	bool aTakeFirst = true;
	int aFirstIndex = 0;
	int aSecondIndex = 0;
	int aScore = 10000;
	int aRank = 0;
	do
	{
		for (int aMode = 0; aMode < NUM_MODES; aMode++)
		{
			mTodayScore[aMode] = 0;
			mScores[aMode * NUM_RANKS + aRank] = aScore;
			SexyString& aName = mNames[aMode * NUM_RANKS + aRank];
			if (aTakeFirst && (aFirstIndex < 13))
			{
				aName = GT(aFirstNames[aFirstIndex]);
				aFirstIndex++;
				aTakeFirst = false;
			}
			else if (aSecondIndex < 17)
			{
				aName = GT(aSecondNames[aSecondIndex]);
				aSecondIndex++;
				aTakeFirst = true;
			}
			else
				aName = GT(0x004e4844);
		}
		aScore -= 250;
		aRank++;
	} while (7500 < aScore);

	LoadScores();

	mMenuButton = new HyperlinkWidget(0, this);
	mMenuButton->mUnderlineSize = 0;
	mMenuButton->SetFont(mApp->mFonts[FONT_DOMO11_BOLD]);
	mMenuButton->SetVisible(true);
	mMenuButton->mColor = Color(0xe6, 0xf0, 0xe6);
	if ((mOpenedFrom == 0) || (mOpenedFrom == 1))
		mMenuButton->mLabel = GT(0x004e4838);
	else if (mOpenedFrom == 2)
		mMenuButton->mLabel = GT(0x004e4828);
	else if (mOpenedFrom == 3)
		mMenuButton->mLabel = GT(0x004e4818);
	int aLabelWidth = mMenuButton->mFont->StringWidth(mMenuButton->mLabel);
	mMenuButton->Resize(0x140 - (aLabelWidth + 8) / 2, 0x1b2, aLabelWidth + 8, 0x1e);

	// (the original also sets the talking head's speech to its place (0x416aa0 with 60) and its mood (0x416360 with 8):
	// the head is not ported yet)
}

// 0x406ba0
HighScoreDisplayDialog::~HighScoreDisplayDialog()
{
	SaveScores();
	delete mMenuButton;
}

// 0x406c80: the file keeps, for each mode, the best score of the day (with the day's number) and the ten entries, each
// with a check; an entry with a wrong check keeps its name and only its score is spoiled (the lowest byte of it)
void HighScoreDisplayDialog::LoadScores()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(HIGH_SCORE_FILE, &aBuffer))
		return;

	for (int aMode = 0; aMode < NUM_MODES; aMode++)
	{
		int aDay = (int) aBuffer.ReadLong();
		short aLength = aBuffer.ReadShort();
		SexyString aName = aBuffer.ReadString();
		int aScore = (int) aBuffer.ReadLong();
		unsigned int aCheck = (unsigned int) aBuffer.ReadLong();
		if ((aDay == DayNumber()) && (aCheck == ScoreCheck(aName, aLength, aScore)))
		{
			mTodayName[aMode] = aName;
			mTodayScore[aMode] = aScore;
		}

		for (int aRank = 0; aRank < NUM_RANKS; aRank++)
		{
			short aEntryLength = aBuffer.ReadShort();
			SexyString aEntryName = aBuffer.ReadString();
			int aEntryScore = (int) aBuffer.ReadLong();
			unsigned int aEntryCheck = (unsigned int) aBuffer.ReadLong();
			mNames[aMode * NUM_RANKS + aRank] = aEntryName;
			if (aEntryCheck == ScoreCheck(aEntryName, aEntryLength, aEntryScore))
				mScores[aMode * NUM_RANKS + aRank] = aEntryScore;
			else
				mScores[aMode * NUM_RANKS + aRank] = aEntryScore % 256;
		}
	}
}

// 0x407080
void HighScoreDisplayDialog::SaveScores()
{
	Buffer aBuffer;
	for (int aMode = 0; aMode < NUM_MODES; aMode++)
	{
		aBuffer.WriteLong(DayNumber());
		aBuffer.WriteShort((short) mTodayName[aMode].size());
		aBuffer.WriteString(mTodayName[aMode]);
		aBuffer.WriteLong(mTodayScore[aMode]);
		aBuffer.WriteLong((long) ScoreCheck(mTodayName[aMode], (short) mTodayName[aMode].size(), mTodayScore[aMode]));

		for (int aRank = 0; aRank < NUM_RANKS; aRank++)
		{
			const SexyString& aName = mNames[aMode * NUM_RANKS + aRank];
			int aScore = mScores[aMode * NUM_RANKS + aRank];
			aBuffer.WriteShort((short) aName.size());
			aBuffer.WriteString(aName);
			aBuffer.WriteLong(aScore);
			aBuffer.WriteLong((long) ScoreCheck(aName, (short) aName.size(), aScore));
		}
	}
	mApp->WriteBufferToFile(HIGH_SCORE_FILE, &aBuffer);
}

// 0x407a30
int HighScoreDisplayDialog::FindRank(int theMode, int theScore)
{
	int aRank = 0;
	int anIndex = NUM_RANKS - 1;
	do
	{
		if (mScores[theMode * NUM_RANKS + anIndex] < theScore)
			aRank = anIndex + 1;
		anIndex--;
	} while (-1 < anIndex);
	return aRank;
}

// 0x4084b0: the game that has just ended (the board's score in the app's mode) goes into the list under the name; the
// best score of the day too, when it beats it
void HighScoreDisplayDialog::AddScore(const SexyString& theName)
{
	if (theName.empty())
		return;

	int aMode = mApp->mMode;
	int aScore = mApp->mBoard->mScore;
	int aRank = FindRank(aMode, aScore);
	if (mTodayScore[aMode] < aScore)
	{
		mTodayScore[aMode] = aScore;
		mTodayName[aMode] = theName;
		mTodayMode = aMode;
	}
	if (aRank == 0)
		return;

	if (aRank < NUM_RANKS)
	{
		for (int i = NUM_RANKS - 1; i >= aRank; i--)
		{
			mNames[aMode * NUM_RANKS + i] = mNames[aMode * NUM_RANKS + i - 1];
			mScores[aMode * NUM_RANKS + i] = mScores[aMode * NUM_RANKS + i - 1];
		}
	}
	mNames[aMode * NUM_RANKS + aRank - 1] = theName;
	mScores[aMode * NUM_RANKS + aRank - 1] = aScore;
	mHighlightMode = aMode;
	mHighlightRank = aRank - 1;

	// port: the original saves the lists when the dialog goes away; a browser tab that is closed never gets there
	SaveScores();
}

// 0x4073b0: after a game, says what the score made: a place of all time, the best of the day, or both, and asks for the
// name. (The original's text for an unregistered game offers to register first; the registered game goes to the name.)
void HighScoreDisplayDialog::CheckScore()
{
	if (mApp->mBoard == NULL)
		return;

	int aMode = mApp->mMode;
	int aScore = mApp->mBoard->mScore;
	bool aBestOfDay = mTodayScore[aMode] < aScore;
	int aRank = FindRank(aMode, aScore);
	// (a better score also changes the talking head: 0x416aa0 with 64 and 0x416360 with 12; not ported yet)

	char aNumber[32];
	snprintf(aNumber, sizeof(aNumber), "%d", aRank);
	SexyString aMessage;
	if (!aBestOfDay)
	{
		if (aRank < 1)
			return;
		aMessage = GT(0x004e48a0) + aNumber + OrdinalSuffix(aRank) + GT(0x004e488c);
	}
	else if (aRank < 1)
		aMessage = GT(0x004e4864);
	else
		aMessage = GT(0x004e4960) + aNumber + OrdinalSuffix(aRank) + GT(0x004e488c);
	mApp->ShowRecordScore(aMessage);
}

// 0x407a60 / 0x407a90
void HighScoreDisplayDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
}

void HighScoreDisplayDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
}

// 0x407ac0: the picture, the three titles, the lists with the colours moving along them, and under each list the best
// score of the day
void HighScoreDisplayDialog::Draw(Graphics* g)
{
	Color aTitleColor(0x78, 0, 0);
	g->DrawImage(mApp->mImages[IMAGE_HALLOFFAME03], 0, 0);
	g->SetFont(mApp->mFonts[FONT_DOMO11_BOLD]);
	g->SetColor(aTitleColor);
	g->DrawString(RocketApp::HighScoreTitle(1), 0x26, 0x27);
	g->DrawString(RocketApp::HighScoreTitle(0), 0x1bb, 0x27);
	g->DrawString(RocketApp::HighScoreTitle(2), 0xeb, 0x6e);

	// the lists: classic (mode 1) on the left, arcade (mode 0) on the right, strategy (mode 2) in the middle and lower
	for (int aMode = 0; aMode < NUM_MODES; aMode++)
	{
		int aX;
		int aY;
		if (aMode == 1)
		{
			aX = 0;
			aY = 0;
		}
		else if (aMode == 0)
		{
			aX = 0x193;
			aY = 0;
		}
		else
		{
			aX = 199;
			aY = 0x3c;
		}

		// the name colour and the line colour of the mode move with the tick, a little apart for each mode
		int aHue = ((mTick + aMode * 0x28) * 8) % 0x168 - 0xb4;
		Color aNameColor((int) mApp->HSLToRGB((aHue < 0) ? -aHue : aHue, 200, 0x32));
		int aSparkleHue = ((mTick + aMode * 0x28) * 8) % 0x28 - 0x14;
		Color aSparkleColor((int) mApp->HSLToRGB((aSparkleHue < 0) ? -aSparkleHue : aSparkleHue, 0xfa, mTick % 0x28 + 0x5a));

		for (int aRank = 0; aRank < NUM_RANKS; aRank++)
		{
			int aRowY;
			if (aRank == 0)
				aRowY = aY + 0x5c;
			else
				aRowY = aY + 0x6e + aRank * 0x12;
			bool aHighlighted = (mHighlightMode == aMode) && (mHighlightRank == aRank);

			g->SetColor(aHighlighted ? aSparkleColor : mApp->mDialogColors[0]);
			g->SetFont(mApp->mFonts[FONT_ARIAL10]);
			char aText[64];
			snprintf(aText, sizeof(aText), "%d.", aRank + 1);
			g->DrawString(aText, (aX - g->GetFont()->StringWidth(aText)) + 0x30, aRowY);

			g->SetColor(aHighlighted ? aSparkleColor : aNameColor);
			g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
			g->DrawString(mNames[aMode * NUM_RANKS + aRank], aX + 0x32, aRowY);

			g->SetColor(aHighlighted ? aSparkleColor : mApp->mDialogColors[0]);
			g->SetFont(mApp->mFonts[FONT_ARIAL10]);
			snprintf(aText, sizeof(aText), "%d", mScores[aMode * NUM_RANKS + aRank]);
			g->DrawString(aText, (aX - g->GetFont()->StringWidth(aText)) + 0xcb, aRowY);
		}

		if (mTodayScore[aMode] != 0)
		{
			g->SetColor(aTitleColor);
			g->SetFont(mApp->mFonts[FONT_DOMO11_BOLD]);
			const SexyString& aTitle = GT(0x004e49a0);
			g->DrawString(aTitle, (aX - g->GetFont()->StringWidth(aTitle) / 2) + 0x76, aY + 0x140);

			g->SetColor((mTodayMode == aMode) ? aSparkleColor : aNameColor);
			g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
			g->DrawString(mTodayName[aMode], aX + 0x32, aY + 0x15e);

			g->SetColor((mTodayMode == aMode) ? aSparkleColor : mApp->mDialogColors[0]);
			g->SetFont(mApp->mFonts[FONT_ARIAL10]);
			char aText[64];
			snprintf(aText, sizeof(aText), "%d", mTodayScore[aMode]);
			g->DrawString(aText, (aX - g->GetFont()->StringWidth(aText)) + 0xcb, aY + 0x15e);
		}
	}
}

// 0x408450: after a game the score is looked at in the 10th update, and the colours move on every 20th
void HighScoreDisplayDialog::Update()
{
	Dialog2004::Update();
	if ((mOpenedFrom == 0) && ((mUpdateCnt == 10) || mCheckPending))
	{
		mCheckPending = false;
		CheckScore();
	}
	if (mUpdateCnt % 20 == 0)
	{
		mTick++;
		MarkDirty();
	}
}

// 0x408800
void HighScoreDisplayDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_UI_LITTLE_BEEP_05B);
}

// 0x408820: the link under the lists closes them (not while a score is being entered): after a game and from the menu
// it shows the menu again
void HighScoreDisplayDialog::ButtonDepress(int theId)
{
	if (theId != 0)
	{
		Dialog2004::ButtonDepress(theId);
		return;
	}

	if ((mApp->GetDialog(DIALOG_RECORD_SCORE) == NULL) && (mApp->GetDialog(9) == NULL))
	{
		mApp->KillDialog(DIALOG_HIGH_SCORES);
		if (mOpenedFrom == 0)
			mApp->ShowGameSelector();
		else if (mOpenedFrom == 1)
			mApp->mGameSelector->ResetHead();
	}
}

// ---------------------------------------------------------------------------------------------------------
// RecordScoreDialog

// 0x414060
RecordScoreDialog::RecordScoreDialog(RocketApp* theApp, const SexyString& theMessage) :
	MoneyDialog(theApp, DIALOG_RECORD_SCORE, false, GT(0x004e5e58), theMessage, "", BUTTONS_OK_CANCEL)
{
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_VERDANA18_BOLD]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	mContentInsets = Insets(0x26, 0x19, 0x26, 0x19);
	SetColor(0, mApp->mDialogColors[0]);
	SetColor(1, mApp->mDialogColors[1]);
	SetColor(3, mApp->mDialogColors[2]);

	// the colours of the name field (0x4e5dc0): the background, outline, text, selection and selected text
	static int aEditColors[5][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 240}, {240, 255, 255}, {255, 255, 255}};
	mEdit = new EditWidget(0, this);
	mEdit->SetFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	mEdit->SetColors(aEditColors, 5);
	mEdit->mMaxPixels = 150;
	mEdit->SetText(mApp->mLastScoreName);
}

// 0x414300
RecordScoreDialog::~RecordScoreDialog()
{
	delete mEdit;
}

// 0x414370
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return MoneyDialog::GetPreferredHeight(theWidth) + 0x38;
}

// 0x414390: the name field is in the dialog and has the keyboard
void RecordScoreDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	MoneyDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEdit);
	theWidgetManager->SetFocus(mEdit);
}

// 0x4143d0
void RecordScoreDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MoneyDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEdit);
}

// 0x414400
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	MoneyDialog::Resize(theX, theY, theWidth, theHeight);
	mEdit->Resize(mX + 0x44, mY + mHeight - 100, mWidth - 0x88, 0x18);
}

// 0x414450
void RecordScoreDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);

	Rect aFrame = mEdit->GetRect();
	aFrame.mWidth += 8;
	aFrame.mY = aFrame.mY - mY - 4;
	aFrame.mHeight += 8;
	aFrame.mX = aFrame.mX - mX - 4;
	DrawEditFrame(g, aFrame);

	g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	g->SetColor(mApp->mDialogColors[0]);
	g->DrawString(GT(0x004e5e64), (mEdit->mX - mX) - 0x14, (mEdit->mY - mY) - 9);
}

// 0x4145c0: the box round the name field from the edit box picture: a left end, a right end, and the middle tiled between
void RecordScoreDialog::DrawEditFrame(Graphics* g, const Rect& theRect)
{
	Image* anImage = mApp->mImages[IMAGE_EDITBOX];
	g->DrawImage(anImage, theRect.mX, theRect.mY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(anImage, theRect.mX + theRect.mWidth - 0x20, theRect.mY, Rect(0x40, 0, 0x20, 0x20));

	Graphics aTiles(*g);
	aTiles.ClipRect(theRect.mX + 0x20, theRect.mY, theRect.mWidth - 0x20, 0x20);
	int aCount = (theRect.mWidth - 0x21) / 32;
	if (0 < aCount)
	{
		int anIndex = 0;
		int anOffset = 0x20;
		do
		{
			aTiles.DrawImage(anImage, theRect.mX + anOffset, theRect.mY, Rect(0x20, 0, 0x20, 0x20));
			anIndex++;
			anOffset += 0x20;
		} while (anIndex < aCount);
	}
}

// 0x414720: the Return key in the field is the OK button
void RecordScoreDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp->ButtonDepress(mId + 2000);
}

// ---------------------------------------------------------------------------------------------------------
// HintDialog

// 0x408f60
HintDialog::HintDialog(RocketApp* theApp, int theHint, bool theContinuing) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_HINT, true, "", "", "",
		BUTTONS_NONE),
	mApp(theApp),
	mCheckbox(NULL),
	mPlayButton(NULL),
	mHint(theHint),
	mStartTick(0),
	mContinuing(theContinuing),
	mLabelX(0),
	mLabelY(0),
	mHelpWidth(theApp->mImages[IMAGE_HELP5]->GetWidth())
{
	// a bonus item's hint is up while the board waits: a game saved now comes back at the buy-rockets screen (Board::m104)
	if (!IsModeHint(mHint))
	{
		mApp->PlaySample(SOUND_HINT_POPUP_01);
		if (mApp->mBoard != NULL)
			mApp->mBoard->m104 = 2;
	}

	mContentInsets = Insets(-0x24, -0x24, -0x24, -0x14);
	SetButtonFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	SetLinesFont(mApp->mFonts[FONT_DOMO11_BOLD]);

	mCheckbox = new Checkbox(mApp->mCheckUnchecked, mApp->mCheckChecked, 0, this);
	mCheckbox->SetChecked(false);

	mPlayButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], 1, this);
	mPlayButton->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	mPlayButton->SetColor(ButtonWidget::COLOR_LABEL, mApp->mDialogColors[2]);
	mPlayButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(255, 255, 255));
	mPlayButton->mLabel = GT(0x004e49fc);
	mPlayButton->mDoFinger = true;

	mStartTick = mUpdateCnt;
}

HintDialog::~HintDialog()
{
	delete mCheckbox;
	delete mPlayButton;
}

// 0x40c1c0: hints 0, 1 and 2 are the modes'
bool HintDialog::IsModeHint(int theHint)
{
	return (theHint == HINT_CLASSIC) || (theHint == HINT_ARCADE) || (theHint == HINT_STRATEGY);
}

// 0x4094f0
void HintDialog::Layout(bool theFull)
{
	if (theFull)
	{
		Resize(10, 10, 0x26c, 0x1cc);
		mLabelX = 0x20a;
		mLabelY = 0x1a2;
		mCheckbox->Resize(0x1f9, 0x1a6, 0x18, 0x18);
		mPlayButton->Resize(0x91, 0x19c, 0x15e, 0x28);
	}
	else
	{
		Resize(0x46, 0x50, 500, 0x104);
		mLabelX = 0x1a1;
		mLabelY = 0xda;
		mCheckbox->Resize(0x1cc, 0x122, 0x18, 0x18);
		mPlayButton->Resize(0xc3, 0x11a, 0xfa, 0x28);
	}
}

// 0x40c170
void HintDialog::CheckboxChecked(int theId, bool checked)
{
	mApp->mHintFlags[mHint] = checked;
}

// 0x409470
void HintDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mCheckbox);
	theWidgetManager->AddWidget(mPlayButton);
}

// 0x4094b0
void HintDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mCheckbox);
	theWidgetManager->RemoveWidget(mPlayButton);
}

// 0x40c270
void HintDialog::Update()
{
	Dialog2004::Update();
	MarkDirty();
}

// 0x40c190
void HintDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(SOUND_UI_LITTLE_BEEP_05B);
}

// 0x40c1e0: the button starts the game that waited for a mode's hint; a bonus item's hint hands over to the buy-rockets
// screen (but the strategy mode's matches hint opens during the game and goes back to it)
void HintDialog::ButtonDepress(int theId)
{
	RocketApp* anApp = mApp;
	if (IsModeHint(mHint))
	{
		Dialog2004::ButtonDepress(theId);
		anApp->KillDialog(DIALOG_HINT);
		anApp->StartBoard(mContinuing);
	}
	else
	{
		anApp->KillDialog(DIALOG_HINT);
		if ((mHint != HINT_MATCHES) && (anApp->mBoard != NULL))
			anApp->mBoard->ShowBuyRockets();
	}
	if (anApp->mBoard != NULL)
		anApp->mBoard->m104 = 0;
}

// 0x4095c0 (a whole line) and 0x409720 (a line given in fractions, as the bonus items' second text is): the text centred
// on the dialog, 35 pixels a line from 80
void HintDialog::DrawTextLine(Graphics* g, const SexyString& theText, double theLine)
{
	int aWidth = g->GetFont()->StringWidth(theText);
	g->DrawString(theText, (mWidth - aWidth) / 2, (int) (theLine * 35.0 + 80.0));
}

// 0x409880: a line of a picture's caption in black; theAlpha from 0 to 254 is a fade in (255 or none: plain)
void HintDialog::DrawFadingText(Graphics* g, const SexyString& theText, int theX, int theY, int theAlpha)
{
	g->SetFont(mApp->mFonts[FONT_MYRIAD11]);
	if ((theAlpha > 254) || (theAlpha < 0))
	{
		g->SetColor(Color(0, 0, 0));
		int aWidth = g->GetFont()->StringWidth(theText);
		g->DrawString(theText, theX - aWidth / 2, theY);
		return;
	}

	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0, 0, theAlpha));
	int aWidth = g->GetFont()->StringWidth(theText);
	g->DrawString(theText, theX - aWidth / 2, theY);
	g->SetColorizeImages(false);
}

// 0x409b30: the background of a picture (the nine help pictures are in the strip's order of the game files)
void HintDialog::DrawScene(Graphics* g, int theX, int theIndex)
{
	g->DrawImage(mApp->mImages[IMAGE_HELP0 + theIndex], theX, 0x78);
}

// 0x409b60: the picture about turning tiles: a pipe piece under the pointer turns a quarter every 200 updates
void HintDialog::DrawRotateScene(Graphics* g, int theX, int theTime)
{
	static int aLastTime = 0;   // _DAT_004f6454: the time of the last frame that was drawn at rest

	if (theTime < 0)
		return;

	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawScene(g, theX, 1);
		g->DrawImage(mApp->mPieces[1][3][6], theX + 0x2b, 0xa1);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_HIGHLIGHT], theX + 0x2b, 0xa1);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(mApp->mImages[IMAGE_CURSOR_POINTER], theX + 0x32, 0xa0);
		g->SetColorizeImages(false);
	}
	else
	{
		int aCycle = theTime % 800;
		int aStep = aCycle / 0x19;
		int aFrame = 3 - aStep / 8;
		DrawScene(g, theX, 1);

		// during the first 25 updates of each 200 the piece of the next picture turns into place
		bool aTurning = false;
		double anAngle = 0.0;
		if (aStep % 8 == 0)
		{
			anAngle = (theTime - aLastTime) * 1.57079632625 * 0.04;
			if (anAngle >= 1.57079632625)
				anAngle = 1.57079632625;
			aTurning = (anAngle != 0.0);
		}
		else
			aLastTime = theTime;

		int aHighlightY;
		if (aTurning)
		{
			g->SetColor(Color(0, 0, 0));
			g->FillRect(theX + 0x2b, 0xa0, 0x2a, 0x2a);
			g->DrawImageRotated(mApp->mPieces[1][(aFrame + 1) % 4][6], theX + 0x2b, 0xa0, anAngle);
			aHighlightY = 0xa0;
		}
		else
		{
			g->DrawImage(mApp->mPieces[1][aFrame][6], theX + 0x2b, 0xa1);
			aHighlightY = 0xa1;
		}
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_HIGHLIGHT], theX + 0x2b, aHighlightY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(mApp->mImages[IMAGE_CURSOR_POINTER], theX + 0x32, 0xa0);
		if (aCycle % 200 > 0xb9)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(mApp->mImages[IMAGE_CURSOR_POINTER], theX + 0x32, 0xa0);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	DrawFadingText(g, GT(0x004e4a44), mHelpWidth / 2 + theX, 0x10e, anAlpha);
	DrawFadingText(g, GT(0x004e4a34), mHelpWidth / 2 + theX, 0x122, anAlpha);
	DrawFadingText(g, GT(0x004e4a24), mHelpWidth / 2 + theX, 0x140, anAlpha);
	DrawFadingText(g, GT(0x004e4a10), mHelpWidth / 2 + theX, 0x154, anAlpha);
}

// 0x409ef0: the picture about the lanterns (classic mode): the lantern lights up and fades, over and over
void HintDialog::DrawLanternsScene(Graphics* g, int theX, int theTime)
{
	if (theTime < 0)
		return;

	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawScene(g, theX, 2);
		g->DrawImage(mApp->mImages[IMAGE_LANTERNON], theX + 0x27, 0x92);
		g->SetColorizeImages(false);
	}
	else
	{
		DrawScene(g, theX, 2);
		int aPhase = theTime % 200;
		g->DrawImage(mApp->mImages[IMAGE_LANTERNDIM], theX + 0x27, 0x92);
		if (aPhase < 100)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int) ((1.0 - aPhase * 0.01) * 255.0)));
			g->DrawImage(mApp->mImages[IMAGE_LANTERNON], theX + 0x27, 0x92);
			g->SetColorizeImages(false);
		}
	}

	DrawFadingText(g, GT(0x004e4a78), mHelpWidth / 2 + theX, 0x11d, anAlpha);
	DrawFadingText(g, GT(0x004e4a68), mHelpWidth / 2 + theX, 0x13b, anAlpha);
	DrawFadingText(g, GT(0x004e4a54), mHelpWidth / 2 + theX, 0x14f, anAlpha);
}

// 0x40a0e0: the picture about launching three or more rockets at once for extra matches (strategy mode)
void HintDialog::DrawRocketsScene(Graphics* g, int theX, int theTime)
{
	if (theTime < 0)
		return;

	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawScene(g, theX, 6);
		g->SetColorizeImages(false);
	}
	else
		DrawScene(g, theX, 6);

	DrawFadingText(g, GT(0x004e4ac0), mHelpWidth / 2 + theX, 0x116, anAlpha);
	DrawFadingText(g, GT(0x004e4aac), mHelpWidth / 2 + theX, 0x12a, anAlpha);
	DrawFadingText(g, GT(0x004e4a9c), mHelpWidth / 2 + theX, 0x13e, anAlpha);
	DrawFadingText(g, GT(0x004e4a8c), mHelpWidth / 2 + theX, 0x152, anAlpha);
}

// 0x40a210: the picture about picking up the matches that burn out (strategy mode)
void HintDialog::DrawMatchesScene(Graphics* g, int theX, int theTime)
{
	if (theTime < 0)
		return;

	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
	}
	DrawScene(g, theX, 7);

	// the strip of the burning match has three pictures across and three rows; the last row is the one drawn here
	Image* aTorch = mApp->mImages[IMAGE_TORCH];
	int aWidth = aTorch->GetWidth();
	int aHeight = aTorch->GetHeight() / 3;
	g->DrawImage(aTorch, theX + 0x30, 0x9b, Rect(((theTime % 0x2d) / 0xf) * (aWidth / 3), aHeight * 2, aWidth / 3, aHeight));
	g->SetColorizeImages(false);

	DrawFadingText(g, GT(0x004e4aec), mHelpWidth / 2 + theX, 0x122, anAlpha);
	DrawFadingText(g, GT(0x004e4ae0), mHelpWidth / 2 + theX, 0x136, anAlpha);
	DrawFadingText(g, GT(0x004e4ad4), mHelpWidth / 2 + theX, 0x14a, anAlpha);
}

// 0x40a3c0: the picture about the screen filling up (arcade mode): the four tiles of the top flash
void HintDialog::DrawScreenFillScene(Graphics* g, int theX, int theTime)
{
	if (theTime < 0)
		return;

	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawScene(g, theX, 0);
		g->SetColorizeImages(false);
	}
	else
	{
		DrawScene(g, theX, 0);
		if (theTime % 100 < 0x32)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(mApp->mImages[IMAGE_HIGHLIGHT], theX + 0x14, 0x92);
			g->DrawImage(mApp->mImages[IMAGE_HIGHLIGHT], theX + 0x3e, 0x92);
			g->DrawImage(mApp->mImages[IMAGE_HIGHLIGHT], theX + 0x14, 0xbc);
			g->DrawImage(mApp->mImages[IMAGE_HIGHLIGHT], theX + 0x3e, 0xbc);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	DrawFadingText(g, GT(0x004e4b20), mHelpWidth / 2 + theX, 0x11f, anAlpha);
	DrawFadingText(g, GT(0x004e4b0c), mHelpWidth / 2 + theX, 0x133, anAlpha);
	DrawFadingText(g, GT(0x004e4b00), mHelpWidth / 2 + theX, 0x147, anAlpha);
}

// 0x40a580: a bonus item's picture, fading in, then pulsing larger every 500 updates (drawn stretched about its centre)
void HintDialog::DrawNewItem(Graphics* g, int theX, int theTime, int theImage)
{
	if (theTime < 0)
		return;

	Image* anImage = mApp->mImages[theImage];
	if (theTime < 100)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (theTime * 0.01 * 255.0)));
		g->DrawImage(anImage, theX, 0x4b);
		g->SetColorizeImages(false);
		return;
	}

	double aScale = 1.0;
	int aPhase = 499 - theTime % 500;
	if ((aPhase >= 100) && (aPhase < 600))
	{
		aScale = sin((399 - theTime % 500) * 0.002 * 31.415926535) * 0.525 + 1.0;
		if (aScale < 1.0)
			aScale = 1.0;
	}

	int aWidth = anImage->GetWidth();
	int aHeight = anImage->GetHeight();
	int aStretchedWidth = ((int) (aWidth * aScale)) | 1;
	int aStretchedHeight = ((int) (aHeight * aScale)) | 1;
	g->SetFastStretch(true);
	g->DrawImage(anImage, Rect((aWidth / 2 - aStretchedWidth / 2) + theX, (aHeight / 2 - aStretchedHeight / 2) + 0x4a,
		aStretchedWidth, aStretchedHeight), Rect(0, 0, aWidth, aHeight));
	g->SetFastStretch(false);
}

// 0x40a7a0: the matches bonus' picture: three burning matches side by side
void HintDialog::DrawMatchStrips(Graphics* g, int theX, int theTime)
{
	Image* aTorch = mApp->mImages[IMAGE_TORCH];
	int aWidth = aTorch->GetWidth() / 3;
	int aHeight = aTorch->GetHeight() / 3;
	int aFrame = (theTime % 0x2d) / 0xf;

	int aX = theX - 100;
	int aSourceY = 0;
	for (int aCount = 3; aCount != 0; aCount--)
	{
		g->DrawImage(aTorch, aX, 0x4b, Rect(aFrame * aWidth, aSourceY, aWidth, aHeight));
		aSourceY += aHeight;
		aX += 100;
	}
}

// 0x40a890: the picture about the count that is left to launch (rockets: 12 on the rocket picture, arcade has its own scene) or
// about the limited matches (10 on the match picture); the picture flashes every 300 updates
void HintDialog::DrawLaunchScene(Graphics* g, int theX, int theTime, bool theRockets, bool theArcade)
{
	if (theTime < 0)
		return;

	int aGlow = theRockets ? IMAGE_ROCKET_GLOW : IMAGE_MATCH_GLOW;
	int aScene = theRockets ? (theArcade ? 8 : 3) : 4;

	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		DrawScene(g, theX, aScene);
		g->SetColorizeImages(false);
	}
	else
	{
		DrawScene(g, theX, aScene);
		int aPhase = theTime % 300;
		if (aPhase < 0x32)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int) ((1.0 - aPhase * 0.02) * 150.0)));
			g->DrawImage(mApp->mImages[aGlow], theX + (theRockets ? 0x1f : 0x2e), 0xaa);
			g->SetColorizeImages(false);
		}

		g->SetFont(mApp->mFonts[FONT_DOMO11_BOLD]);
		g->SetColor(Color(0xff, 0xb6, 0x2b));
		if (theRockets)
			g->DrawString(GT(0x004e4ba0), theX + 0x12, 199);
		else
			g->DrawString(GT(0x004e4b9c), theX + 0x1e, 199);
	}

	if (!theRockets)
	{
		DrawFadingText(g, GT(0x004e4b58), mHelpWidth / 2 + theX, 0x122, anAlpha);
		DrawFadingText(g, GT(0x004e4b44), mHelpWidth / 2 + theX, 0x136, anAlpha);
		DrawFadingText(g, GT(0x004e4b38), mHelpWidth / 2 + theX, 0x14a, anAlpha);
	}
	else
	{
		DrawFadingText(g, GT(0x004e4b88), mHelpWidth / 2 + theX, 0x11f, anAlpha);
		DrawFadingText(g, GT(0x004e4b78), mHelpWidth / 2 + theX, 0x133, anAlpha);
		DrawFadingText(g, GT(0x004e4b68), mHelpWidth / 2 + theX, 0x147, anAlpha);
	}
}

// 0x40acc0: the picture about dragging tiles sideways (arcade mode): a tile slides over and back
void HintDialog::DrawDragScene(Graphics* g, int theX, int theTime)
{
	if (theTime < 0)
		return;

	// (when the picture has faded in, the original hands its text a stale value, which is out of range: plain text)
	int anAlpha = 255;
	if (theTime < 100)
	{
		anAlpha = (int) (theTime * 0.01 * 255.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
	}

	// the tile goes the width of one tile to the right and comes back, over 800 updates
	int aTileWidth = mApp->mImages[IMAGE_TILE]->GetWidth();
	int aCycle = theTime % 800;
	int anOffset;
	if (aCycle >= 400)
	{
		anOffset = aTileWidth;
		if (aCycle - 400 < 100)
			anOffset = (int) ((aCycle - 400.0) * 0.01 * aTileWidth);
	}
	else if (aCycle >= 100)
		anOffset = 0;
	else
		anOffset = (int) (aTileWidth - aCycle * 0.01 * aTileWidth);

	DrawScene(g, theX, 5);
	g->DrawImage(mApp->mPieces[2][1][6], anOffset + 0x17 + theX, 0xa3);
	int aCursorX = anOffset + 0x1e + theX;
	g->DrawImage(mApp->mImages[IMAGE_CURSOR_POINTER], aCursorX, 0xaa);
	if ((aCycle % 400 < 0x69) || (aCycle % 400 > 0x186))
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_CURSOR_POINTER], aCursorX, 0xaa);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetColorizeImages(false);

	DrawFadingText(g, GT(0x004e4bd8), mHelpWidth / 2 + theX, 0x115, anAlpha);
	DrawFadingText(g, GT(0x004e4bc4), mHelpWidth / 2 + theX, 0x129, anAlpha);
	DrawFadingText(g, GT(0x004e4bb0), mHelpWidth / 2 + theX, 0x13d, anAlpha);
	DrawFadingText(g, GT(0x004e4ba4), mHelpWidth / 2 + theX, 0x151, anAlpha);
}

// 0x40aef0
void HintDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);

	int aTime = mUpdateCnt - mStartTick;
	switch (mHint)
	{
	case HINT_ARCADE:
	case HINT_STRATEGY:
	{
		g->DrawImageBox(Rect(0x18, 0x6e, 0x23c, 0x104), mApp->mImages[IMAGE_GREENBOX]);

		// four pictures across, 35 hundredths of the free width from the edge and 10 hundredths apart
		int aFree = mWidth - mHelpWidth * 4;
		int aFirst = (int) (aFree * 0.35);
		int aStep = (int) (aFree * 0.1 + mHelpWidth);
		g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
		DrawRotateScene(g, aFirst, aTime);
		if (mHint == HINT_ARCADE)
		{
			DrawDragScene(g, aStep + aFirst, aTime - 100);
			DrawLaunchScene(g, aFirst + aStep * 2, aTime - 200, true, true);
			DrawScreenFillScene(g, aFirst + aStep * 3, aTime - 300);
		}
		else
		{
			DrawLaunchScene(g, aStep + aFirst, aTime - 100, false, false);
			DrawRocketsScene(g, aFirst + aStep * 2, aTime - 200);
			DrawMatchesScene(g, aFirst + aStep * 3, aTime - 300);
		}

		const SexyString& aTitle = (mHint == HINT_ARCADE) ? RocketApp::ModeTitle(0) : RocketApp::ModeTitle(2);
		g->SetFont(mApp->mFonts[FONT_VERDANA18_BOLD]);
		g->SetColor(mApp->mDialogColors[0]);
		g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, 0x32);
		g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
		g->SetColor(mApp->mDialogColors[1]);
		DrawTextLine(g, (mHint == HINT_ARCADE) ? GT(0x004e4e68) : GT(0x004e4e2c), 0);
		break;
	}
	case HINT_CLASSIC:
	{
		g->DrawImageBox(Rect(0x1e, 0x64, 0x230, 0x118), mApp->mImages[IMAGE_GREENBOX]);

		// three pictures across
		int aFree = (mWidth - mHelpWidth * 3) / 3;
		int aFirst = (int) (aFree * 0.75);
		int aStep = (int) (mHelpWidth + aFree * 0.75);
		g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
		DrawRotateScene(g, aFirst, aTime);
		DrawLaunchScene(g, aStep + aFirst, aTime - 100, true, false);
		DrawLanternsScene(g, aFirst + aStep * 2, aTime - 200);

		const SexyString& aTitle = RocketApp::ModeTitle(1);
		g->SetFont(mApp->mFonts[FONT_VERDANA18_BOLD]);
		g->SetColor(mApp->mDialogColors[0]);
		g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, 0x32);
		g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
		g->SetColor(mApp->mDialogColors[1]);
		DrawTextLine(g, GT(0x004e4ea4), 0);
		break;
	}
	default:
	{
		// a bonus item: its picture, a title (centred by the width of the title without its "!") and two lines
		const SexyString* aTitle = &GT(0x004e4e0c);
		const SexyString* aTitleWidth = &GT(0x004e4e1c);
		const SexyString* aFirstText;
		const SexyString* aSecondText;
		switch (mHint)
		{
		case HINT_GEM:
			DrawNewItem(g, mWidth / 2 - mApp->mImages[IMAGE_GEM01]->GetWidth() / 2, aTime, IMAGE_GEM01);
			aFirstText = &GT(0x004e4df0);
			aSecondText = &GT(0x004e4dc0);
			break;
		case HINT_BOMB:
			DrawNewItem(g, mWidth / 2 - mApp->mImages[IMAGE_BOMB]->GetWidth() / 2, aTime, IMAGE_BOMB);
			aFirstText = &GT(0x004e4d9c);
			aSecondText = &GT(0x004e4d64);
			break;
		case HINT_CLOCK:
			DrawNewItem(g, mWidth / 2 - mApp->mImages[IMAGE_CLOCK]->GetWidth() / 2, aTime, IMAGE_CLOCK);
			aFirstText = &GT(0x004e4d48);
			aSecondText = &GT(0x004e4d24);
			break;
		case HINT_COIN:
			DrawNewItem(g, mWidth / 2 - mApp->mImages[IMAGE_COINGOLD]->GetWidth() / 2, aTime, IMAGE_COINGOLD);
			aTitle = &GT(0x004e4cdc);
			aTitleWidth = &GT(0x004e4d00);
			aFirstText = &GT(0x004e4ca8);
			aSecondText = &GT(0x004e4c84);
			break;
		case HINT_MATCHES:
			DrawMatchStrips(g, mWidth / 2 - mApp->mImages[IMAGE_COINGOLD]->GetWidth() / 2, aTime);
			aTitle = &GT(0x004e4c74);
			aTitleWidth = &GT(0x004e4c74);
			aFirstText = &GT(0x004e4c34);
			aSecondText = &GT(0x004e4bfc);
			break;
		default:
			// (the original draws only the footer)
			aFirstText = NULL;
			aSecondText = NULL;
			break;
		}

		if (aFirstText != NULL)
		{
			g->SetFont(mApp->mFonts[FONT_VERDANA18_BOLD]);
			g->SetColor(mApp->mDialogColors[0]);
			g->DrawString(*aTitle, (mWidth - g->GetFont()->StringWidth(*aTitleWidth)) / 2, 0x32);
			g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
			g->SetColor(mApp->mDialogColors[1]);
			DrawTextLine(g, *aFirstText, 2.0);
			DrawTextLine(g, *aSecondText, 2.6);
		}
		break;
	}
	}

	g->SetFont(mApp->mFonts[FONT_ARIAL8_BOLD]);
	g->SetColor(mApp->mDialogColors[2]);
	g->DrawString(GT(0x004e4bf0), mLabelX, mLabelY);
	g->DrawString(GT(0x004e4be4), mLabelX, mLabelY + 0x10);
}
