// OptionsDialog.cpp: the options (Sexy::OptionsDialog, 0x5cae38-0x5cd63c). Translated from the Ghidra export and the
// assembly. The Register button, the buttons that open PopCap's pages, the Update Check button and the Automatic Update
// checkbox are not ported (OptionsDialog.h); the layout is the original's for a registered copy with update checks off.

#include "OptionsDialog.h"
#include "WinDMApp.h"
#include "Board.h"
#include "MainMenu.h"
#include "Profile.h"
#include "Res.h"
#include "Slider.h"
#include "Checkbox.h"
#include "Graphics.h"
#include "Color.h"
#include "Rect.h"
#include "Insets.h"
#include "Font.h"
#include "MusicInterface.h"
#include "WidgetManager.h"

using namespace Sexy;

// a dialog with one button, OK (dialog 0x25): the display cannot do what the player asked for
static void ShowOptionMessage(WinDMApp* theApp, const SexyString& theHeader, const SexyString& theLines)
{
	theApp->DoDialog(DIALOG_MESSAGE, true, theHeader, theLines, "OK", Dialog::BUTTONS_FOOTER);
}

// a yes-or-no question (dialog 3); FinishOptionVerify does what OptionsDialog::mVerifyType says when the answer is yes
static void AskOptionVerify(WinDMApp* theApp, const SexyString& theHeader, const SexyString& theLines)
{
	theApp->DoDialog(DIALOG_OPTION_VERIFY, true, theHeader, theLines, "", Dialog::BUTTONS_YES_NO);
}

// a checkbox of the options: its two pictures are cut out of one strip, the box is drawn by the dialog's Draw
static Checkbox* MakeOptionCheckbox(WinDMApp* theApp, int theId, CheckboxListener* theListener)
{
	Checkbox* aCheckbox = new Checkbox(IMAGE_CHECKBOX, nullptr, theId, theListener);
	aCheckbox->mUncheckedRect = Rect(0, 0, theApp->ScaleNumUp(0xc6, 0), theApp->ScaleNumUp(0x3c, 0));
	aCheckbox->mCheckedRect = Rect(theApp->ScaleNum(200, 0), 0, theApp->ScaleNumUp(0xc6, 0), theApp->ScaleNumUp(0x3c, 0));
	return aCheckbox;
}

// 0x5ccaf0
OptionsDialog::OptionsDialog(WinDMApp* theApp) :
	PodDialog(theApp, DIALOG_OPTIONS, true, "Options", "", "", BUTTONS_NONE)
{
	mInVerify = false;
	mVerifyType = 0;

	SetButtonFont(FONT_CONTINUUM_MED_14);
	SetHeaderFont(FONT_1);
	SetLinesFont(FONT_2);

	// the Done button is the dialog's yes button (the dialog was made without buttons)
	mYesButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, ID_YES, this);
	mYesButton->SetFont(FONT_CONTINUUM_MED_14);
	if (theApp->mBoard == nullptr)
		mYesButton->mLabel = "Done";
	else
		mYesButton->mLabel = "Back to Game";

	mHelpButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, BUTTON_HELP, this);
	mHelpButton->SetFont(FONT_CONTINUUM_MED_14);
	mHelpButton->mLabel = theApp->GetString("OPTIONSDLG_HELP");

	mMainMenuButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, BUTTON_MAIN_MENU, this);
	mMainMenuButton->SetFont(FONT_CONTINUUM_MED_14);
	mMainMenuButton->mLabel = "Main Menu";

	mMusicSlider = new Slider(nullptr, IMAGE_SLIDER_THUMB, 0, this);
	mMusicSlider->SetValue(theApp->GetMusicVolume());
	mSfxSlider = new Slider(nullptr, IMAGE_SLIDER_THUMB, 1, this);
	mSfxSlider->SetValue(theApp->GetSfxVolume());

	// the listener hears the first SetChecked of each, and ignores it: the dialog is not in the manager yet
	mFullscreenCheckbox = MakeOptionCheckbox(theApp, CHECKBOX_FULLSCREEN, this);
	mFullscreenCheckbox->SetChecked(!theApp->mIsWindowed, true);

	m3DCheckbox = MakeOptionCheckbox(theApp, CHECKBOX_ACCELERATION, this);
	m3DCheckbox->SetChecked(theApp->Is3DAccelerated(), true);

	mCursorsCheckbox = MakeOptionCheckbox(theApp, CHECKBOX_CURSORS, this);
	mCursorsCheckbox->SetChecked(theApp->mCustomCursorsEnabled, true);

	mHighResCheckbox = MakeOptionCheckbox(theApp, CHECKBOX_HIGH_RES, this);
	mHighResCheckbox->SetChecked(theApp->mHighRes, true);

	mBackdropsCheckbox = MakeOptionCheckbox(theApp, CHECKBOX_BACKDROPS, this);
	mBackdropsCheckbox->SetChecked(theApp->mProfile->mFlag78, true);

	mInVerify = false;
}

// 0x5cb5e7
OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mMainMenuButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mCursorsCheckbox;
	delete m3DCheckbox;
	delete mHighResCheckbox;
	delete mBackdropsCheckbox;
}

// 0x5cae38
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return mApp->ScaleNum(0x1f9, 0);
}

// 0x5cae4d: the sound effects play a sample at the new volume, so that the player hears it (not while the thumb is dragged)
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
	{
		mApp->SetSfxVolume(theVal);
		if (!mSfxSlider->mDragging && mWidgetManager != nullptr)
			mApp->PlaySample(SOUND_TRANSFER);
	}
}

// 0x5cb09c
void OptionsDialog::AddedToManager(WidgetManager* theManager)
{
	PodDialog::AddedToManager(theManager);
	theManager->AddWidget(mHelpButton);
	theManager->AddWidget(mMainMenuButton);
	theManager->AddWidget(mMusicSlider);
	theManager->AddWidget(mSfxSlider);
	theManager->AddWidget(mCursorsCheckbox);
	theManager->AddWidget(mFullscreenCheckbox);
	theManager->AddWidget(m3DCheckbox);
	theManager->AddWidget(mHighResCheckbox);
	theManager->AddWidget(mBackdropsCheckbox);
}

// 0x5cb189
void OptionsDialog::RemovedFromManager(WidgetManager* theManager)
{
	PodDialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mHelpButton);
	theManager->RemoveWidget(mMainMenuButton);
	theManager->RemoveWidget(mMusicSlider);
	theManager->RemoveWidget(mSfxSlider);
	theManager->RemoveWidget(mFullscreenCheckbox);
	theManager->RemoveWidget(m3DCheckbox);
	theManager->RemoveWidget(mHighResCheckbox);
	theManager->RemoveWidget(mBackdropsCheckbox);
	theManager->RemoveWidget(mCursorsCheckbox);
}

// 0x5cb276: the picture of a checkbox with its label, one line or two (the strip holds the box and the lettering's room)
void OptionsDialog::DrawCheckbox(Graphics* g, Checkbox* theCheckbox, const SexyString& theLine1, const SexyString& theLine2)
{
	Rect aSrcRect(0, 0, mApp->ScaleNum(0xc6, 0), mApp->ScaleNum(0x3c, 0));
	g->DrawImage(IMAGE_CHECKBOX, theCheckbox->mX - mX, theCheckbox->mY - mY, aSrcRect);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(FONT_CONTINUUM_MED_14);
	if (theLine2.empty())
	{
		int aY = mApp->ScaleNum(0x25, 0) + (theCheckbox->mY - mY);
		int aX = mApp->ScaleNum(0x48, 0) + (theCheckbox->mX - mX);
		g->DrawString(theLine1, aX, aY);
	}
	else
	{
		int aY = mApp->ScaleNum(0x1c, 0) + (theCheckbox->mY - mY);
		int aX = mApp->ScaleNum(0x44, 0) + (theCheckbox->mX - mX);
		g->DrawString(theLine1, aX, aY);
		aY = mApp->ScaleNum(0x2d, 0) + (theCheckbox->mY - mY);
		aX = mApp->ScaleNum(0x44, 0) + (theCheckbox->mX - mX);
		g->DrawString(theLine2, aX, aY);
	}
}

// 0x5cb7ae
void OptionsDialog::Draw(Graphics* g)
{
	PodDialog::Draw(g);
	g->SetFont(FONT_CONTINUUM_MED_14);
	g->SetColor(Color(0xff, 0xff, 0xff));

	g->DrawImage(IMAGE_SLIDER_TRACK, mContentInsets.mLeft, mMusicSlider->mY - mY);
	int aY = mApp->ScaleNum(0x1e, 0) + (mMusicSlider->mY - mY);
	g->DrawString("Music Volume", mApp->ScaleNum(0x18, 0) + mContentInsets.mLeft, aY);

	g->DrawImage(IMAGE_SLIDER_TRACK, mContentInsets.mLeft, mSfxSlider->mY - mY);
	aY = mApp->ScaleNum(0x1e, 0) + (mSfxSlider->mY - mY);
	g->DrawString("Sound Effects", mApp->ScaleNum(0x18, 0) + mContentInsets.mLeft, aY);

	// the original builds each pair of strings in the order below and passes the later one first: it is the top line
	DrawCheckbox(g, mFullscreenCheckbox, "Full", "Screen");
	DrawCheckbox(g, m3DCheckbox, "Hardware", "Accelerate");
	DrawCheckbox(g, mCursorsCheckbox, "Custom", "Cursors");
	DrawCheckbox(g, mHighResCheckbox, "High", "Resolution");
	DrawCheckbox(g, mBackdropsCheckbox, "Random", "Backdrops");
}

// 0x5cbbd0: high resolution on a screen of 1024 pixels or less needs full screen
void OptionsDialog::CheckFullscreenHighres()
{
	if (mApp->mScreenWidth < 0x401)
	{
		if (!mFullscreenCheckbox->IsChecked())
		{
			mInVerify = true;
			mHighResCheckbox->SetChecked(false, true);
			mInVerify = false;
			mVerifyType = VERIFY_FULLSCREEN;
			AskOptionVerify(mApp, "Full Screen?", "Full Screen mode must be enabled to run in high resolution mode.\n\nEnable Full Screen mode?");
		}
	}
}

// 0x5cbcb6: a checkbox was clicked. The changes that the display may not like are undone and asked about (dialog 3).
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	if (mWidgetManager == nullptr || mInVerify)
		return;

	bool aClick = true;

	if (theId == CHECKBOX_FULLSCREEN)
	{
		if (!checked)
		{
			// windowed mode
			if (!mApp->mForceFullscreen)
			{
				if (mApp->mScreenWidth == 0x280)
				{
					mFullscreenCheckbox->SetChecked(true, true);
					ShowOptionMessage(mApp, "Change Resolution?", "Your desktop resolution is not high enough to allow you to switch to windowed mode.");
				}
				else
				{
					bool aHighRes = mHighResCheckbox->IsChecked();
					if (mApp->mScreenWidth <= (aHighRes ? 0x400 : 0x280))
					{
						mFullscreenCheckbox->SetChecked(true, true);
						mVerifyType = VERIFY_WINDOWED;
						AskOptionVerify(mApp, "Change Resolution?", "High Resolution must be disabled in order to switch to windowed mode.\n\nDisable High Resolution?");
					}
				}
			}
			else
			{
				ShowOptionMessage(mApp, "No Windowed Mode", "Windowed mode is only available if your desktop is running in either 16 bit or 32 bit color mode, which it is not.");
				mFullscreenCheckbox->SetChecked(true, true);
			}
		}
		else
		{
			if (m3DCheckbox->IsChecked() && !mHighResCheckbox->IsChecked())
			{
				mVerifyType = VERIFY_HIGH_RES_ON;
				AskOptionVerify(mApp, "Change Resolution?", "High Resolution is recommended in Full Screen mode.\n\nEnable High Resolution?");
			}
		}
	}
	else if (theId == CHECKBOX_ACCELERATION)
	{
		if (!checked)
		{
			if (mApp->Is3DAccelerationRecommended())
			{
				mInVerify = true;
				m3DCheckbox->SetChecked(true, true);
				mInVerify = false;
				mVerifyType = VERIFY_ACCELERATION_OFF;
				AskOptionVerify(mApp, "Disable Acceleration?", "Disabling Hardware Acceleration is not recommended and may decrease performance.\n\nDisable it anyway?");
			}
		}
		else
		{
			if (!mApp->Is3DAccelerationSupported())
			{
				m3DCheckbox->SetChecked(false, true);
				ShowOptionMessage(mApp, "Not Supported", "Hardware Acceleration cannot be enabled on this computer.\n\nYour video card does not meet the minimum requirements for this game.");
				aClick = false;
			}
			else if (!mApp->Is3DAccelerationRecommended())
			{
				m3DCheckbox->SetChecked(false, true);
				mVerifyType = VERIFY_ACCELERATION_ON;
				AskOptionVerify(mApp, "Warning", "Your video card may not fully support this feature.\n\nIf you experience slower performance, please disable Hardware Acceleration.\n\nAre you sure you want to enable hardware acceleration?");
			}
		}
	}
	else if (theId == CHECKBOX_UPDATE)
	{
		// the Automatic Update checkbox is not ported
	}
	else if (theId == CHECKBOX_HIGH_RES)
	{
		bool a3D = mApp->Is3DAccelerated();
		if (!checked)
		{
			if (a3D)
			{
				mInVerify = true;
				mHighResCheckbox->SetChecked(true, true);
				mInVerify = false;
				mVerifyType = VERIFY_HIGH_RES_OFF;
				AskOptionVerify(mApp, "Low Resolution?", "Disabling High Resolution mode will decrease the graphics quality.\n\nDisable High Resolution?");
			}
		}
		else
		{
			if (!a3D)
			{
				mInVerify = true;
				mHighResCheckbox->SetChecked(false, true);
				mInVerify = false;
				mVerifyType = VERIFY_HIGH_RES_ON;
				AskOptionVerify(mApp, "High Resolution?", "Enabling high resolution mode without hardware acceleration will reduce the performance of the game.\n\nEnable it anyway?");
			}
			CheckFullscreenHighres();
		}
	}
	else if (theId == CHECKBOX_BACKDROPS && checked)
	{
		mBackdropsCheckbox->SetChecked(false, true);
		mVerifyType = VERIFY_BACKDROPS_RANDOM;
		AskOptionVerify(mApp, "Random Backdrops?", "Enabling Random Backdrops will make the planetary backdrop order change every time you play.\n\nEnable Random Backdrops?");
	}

	if (aClick)
		mApp->PlaySample(SOUND_CLICK_DIALOG);
}

// 0x5cc95a: the answer to the confirmation (dialog 3); a no leaves the checkboxes as CheckboxChecked put them back
void OptionsDialog::FinishOptionVerify(bool theYes)
{
	if (!theYes)
		return;

	mInVerify = true;
	switch (mVerifyType)
	{
	case VERIFY_ACCELERATION_ON:
		m3DCheckbox->SetChecked(true, true);
		break;
	case VERIFY_ACCELERATION_OFF:
		m3DCheckbox->SetChecked(false, true);
		break;
	case VERIFY_HIGH_RES_ON:
		mHighResCheckbox->SetChecked(true, true);
		CheckFullscreenHighres();
		break;
	case VERIFY_WINDOWED:
		mFullscreenCheckbox->SetChecked(false, true);
		mHighResCheckbox->SetChecked(false, true);
		break;
	case VERIFY_HIGH_RES_OFF:
		mHighResCheckbox->SetChecked(false, true);
		break;
	case VERIFY_FULLSCREEN:
		mFullscreenCheckbox->SetChecked(true, true);
		mHighResCheckbox->SetChecked(true, true);
		break;
	case VERIFY_BACKDROPS_RANDOM:
		mBackdropsCheckbox->SetChecked(true, true);
		break;
	default:
		break;
	}
	mInVerify = false;
}

// 0x5cc563
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	if (theId == BUTTON_HELP)
	{
		mApp->FinishOptionsDialog(false);
		mApp->DoHelpScreen();
	}
	else if (theId == BUTTON_MAIN_MENU)
	{
		if (mApp->mBoard == nullptr)
		{
			// (the button is not laid out without a game: Resize leaves it at no size)
			mApp->FinishOptionsDialog(false);
			mApp->Shutdown();
			return;
		}

		mApp->PlaySample(SOUND_PANEL_SLIDE);
		mApp->FinishOptionsDialog(true);
		if (mApp->mBoard == nullptr)
			mApp->mWasInGame = false;
		else
		{
			mApp->mBoard->Hide();
			mApp->DoMainMenu();
			mApp->mMainMenu->Show();
			mApp->mMainMenu->mBackdropClear = true;
		}
		mApp->mMusicInterface->FadeIn(mApp->mCurrentSong, 1, 0.006);
		mApp->mPendingMusicOffset = mApp->GetInteger("MUSICOFFSET_MAIN_MENU");
	}
	// ids 11 (Register), 13 and 100 to 199 (PopCap's pages) and 14 (Update Check) are not ported
}

// 0x5cd63c
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	PodDialog::Resize(theX, theY, theWidth, theHeight);

	int aGap = mApp->ScaleNumUp(10, 0);
	int aSide = mApp->ScaleNum(0x18, 0) + mContentInsets.mLeft;
	int aRowY = mApp->ScaleNum(0x15b, 0);

	// the row of the original's extra buttons (Register, Update Check): here the How To Play button alone fills it
	int aHelpWidth = mWidth - aSide * 2;
	mHelpButton->Resize(mX + aSide, mY + aRowY + mContentInsets.mTop, aHelpWidth, mApp->ScaleNumUp(0x33, 0));

	// Done, and Main Menu beside it while a game is in progress
	if (mApp->mBoard == nullptr)
	{
		int aButtonSide = mApp->ScaleNum(0x38, 0) + mContentInsets.mLeft;
		mYesButton->Resize(mX + aButtonSide, mApp->ScaleNum(0x40, 0) + mY + aRowY + mContentInsets.mTop,
			mWidth - aButtonSide * 2, mApp->ScaleNumUp(0x33, 0));
	}
	else
	{
		int aButtonSide = mApp->ScaleNum(0x38, 0) + mContentInsets.mLeft;
		int aButtonWidth = ((mWidth - aButtonSide * 2) - aGap) / 2;
		mMainMenuButton->Resize(mX + aButtonSide, mApp->ScaleNum(0x40, 0) + mY + aRowY + mContentInsets.mTop,
			aButtonWidth, mApp->ScaleNumUp(0x33, 0));
		mYesButton->Resize(aButtonWidth + aButtonSide + aGap + mX, mApp->ScaleNum(0x40, 0) + mY + aRowY + mContentInsets.mTop,
			aButtonWidth, mApp->ScaleNumUp(0x33, 0));
	}

	mMusicSlider->Resize(mApp->ScaleNum(0xd4, 0) + mContentInsets.mLeft + mX, mApp->ScaleNum(0x51, 0) + mY,
		mApp->ScaleNumUp(0x180, 0), mApp->ScaleNumUp(0x32, 0));
	mSfxSlider->Resize(mApp->ScaleNum(0xd4, 0) + mContentInsets.mLeft + mX, mApp->ScaleNum(0x8d, 0) + mY,
		mApp->ScaleNumUp(0x180, 0), mApp->ScaleNumUp(0x32, 0));

	// full screen, acceleration and high resolution across the first row, cursors and backdrops across the second
	mFullscreenCheckbox->Resize(mContentInsets.mLeft + mX, mApp->ScaleNum(0xd0, 0) + mY,
		mApp->ScaleNumUp(0x3c, 0), mApp->ScaleNumUp(0x3c, 0));
	m3DCheckbox->Resize((mWidth - mApp->ScaleNum(0xc6, 0)) / 2 + mX, mApp->ScaleNum(0xd0, 0) + mY,
		mApp->ScaleNumUp(0x3c, 0), mApp->ScaleNumUp(0x3c, 0));
	mHighResCheckbox->Resize(((mWidth - mContentInsets.mRight) - mApp->ScaleNum(0xc6, 0)) + mX, mApp->ScaleNum(0xd0, 0) + mY,
		mApp->ScaleNumUp(0x3c, 0), mApp->ScaleNumUp(0x3c, 0));

	mCursorsCheckbox->Resize(mApp->ScaleNum(0x67, 0) + mContentInsets.mLeft + mX, mApp->ScaleNum(0x116, 0) + mY,
		mApp->ScaleNumUp(0x3c, 0), mApp->ScaleNumUp(0x3c, 0));
	mBackdropsCheckbox->Resize(mApp->ScaleNum(0x67, 0) + (mWidth - mApp->ScaleNum(0xc6, 0)) / 2 + mX, mApp->ScaleNum(0x116, 0) + mY,
		mApp->ScaleNumUp(0x3c, 0), mApp->ScaleNumUp(0x3c, 0));
}
