// WinWSAppScreens.cpp: the screens and dialogs the app shows (the original's unnamed WinWSApp methods between 0x4128a0
// and 0x431320): the menu, map, board, statistics and high score screens, the user dialogs, the message and hint
// dialogs and the level introductions. The button handler (0x431630) is in WinWSAppButtons.cpp.

#include "WinWSApp.h"
#include "GameText.h"
#include "Board.h"
#include "MenuScreen.h"
#include "MapScreen.h"
#include "HighScoreScreen.h"
#include "StatsScreen.h"
#include "UserProfileMgr.h"
#include "UserProfile.h"
#include "LessonManager.h"
#include "SelectUserDialog.h"
#include "AbyssModeDialog.h"
#include "OptionsDialog.h"
#include "NewUserDialog.h"
#include "DeleteUserDialog.h"
#include "KeyDialog.h"
#include "HintDialog.h"
#include "WidgetManager.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "ImageFont.h"
#include "Insets.h"
#include "Color.h"

#include <stdlib.h>

using namespace Sexy;

// 0x4128a0: "WHO ARE YOU?" (dialog 0x44e)
void WinWSApp::ShowSelectUserDialog()
{
	SelectUserDialog* aDialog = new SelectUserDialog(this);     // 0x43edb0
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->mYesButton->mHasAlpha = true;
	aDialog->mNoButton->mHasAlpha = true;
	aDialog->Resize(0x83, 0x22, 0x160, aDialog->GetPreferredHeight(0x174));
	AddDialog(DIALOG_SELECT_USER, aDialog);
}

// 0x4129a0: "CONTINUE?" of the abyss mode (dialog 0x451)
void WinWSApp::ShowAbyssModeDialog()
{
	AbyssModeDialog* aDialog = new AbyssModeDialog(this, mUser);    // 0x43e7a0
	mAbyssModeDialog = aDialog;
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->mYesButton->mHasAlpha = true;
	aDialog->mNoButton->mHasAlpha = true;
	aDialog->Resize(0x83, 0x22, 0x160, aDialog->GetPreferredHeight(0x174));
	AddDialog(DIALOG_ABYSS_MODE, aDialog);
}

// 0x412ae0: a number of the current user (+0x34; TODO: name from the profile group)
int WinWSApp::GetUserField34()
{
	return mUser->mField34;
}

// 0x412af0: takes the menu screen off the widget manager and deletes it later
void WinWSApp::RemoveMenuScreen()
{
	if (mMenuScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mMenuScreen);
		SafeDeleteWidget(mMenuScreen);
		mMenuScreen = NULL;
	}
}

// 0x412c90: TODO: calls slot 4 (SetVisible?) with 0x58 on the object embedded at +0x58 of the widget at +0x72c, then
// the widget manager gets the focus of the widget at +0x72c (the board's typing widget?)
void WinWSApp::FocusWidget72c()
{
	// TODO: 0x412ca0 call [vtable + 0x10](0x58) on (mWidget72c + 0x58)
	mWidgetManager->SetFocus(mWidget72c);
}

// 0x412cc0
void WinWSApp::MarkMenuScreenDirty()
{
	if (mMenuScreen != NULL)
		mMenuScreen->MarkDirty();
}

// 0x412ce0
void WinWSApp::BringStatsCloseButtonToFront()
{
	if (mStatsCloseButton != NULL)
		mWidgetManager->BringToFront(mStatsCloseButton);
}

// 0x412d00
void WinWSApp::FocusBoardWidget()
{
	if (mWidget72c != NULL)
		mWidgetManager->SetFocus(mWidget72c);
}

// 0x412d50: the current user's hints setting
void WinWSApp::SetUserHints(bool theHints)
{
	if (mUser != NULL)
		mUser->mShowHints = theHints;       // +0x1fd
}

// 0x412d70: hints show only when the user and the game allow them
bool WinWSApp::HintsEnabled()
{
	if (mUser == NULL)
		return false;
	return mUser->mShowHints && mHintsAllowed;
}

// 0x414020: the register dialog (id 600; "Register" / trial nag) is not ported. The original also pauses the board
// for it and counts it in mOpenDialogCount.

// 0x414150: the options dialog (dialog 0x30a), over the board it pauses
void WinWSApp::ShowOptions()
{
	mOpenDialogCount++;
	mOptionsDialog = new OptionsDialog(this);   // 0x43bf10
	mOptionsDialog->Resize(mWidth / 2 - 0xd2, mHeight / 2 - 0xed, 0x1a4, 0x1da);
	mOptionsDialog->mSpaceAfterHeader = 0x1e;
	AddDialog(DIALOG_OPTIONS, mOptionsDialog);
	mWidgetManager->SetFocus(mOptionsDialog);
	if (mBoard != NULL)
	{
		int aMode = mBoard->mLevelConfig->mGameMode;    // +0xd68 -> +0xb8 (TODO: names from the board group)
		if ((aMode == 0xc) || (aMode == 7))
		{
			if (!mLessonManager->IsTutorialActive())    // 0x4137d0
				return;
			if (aMode != 6)
				return;
		}
		mBoard->SetPaused(true);    // 0x442100
	}
}

// 0x414270: the new-user dialog (dialog 0x44d) with theText as its prompt
void WinWSApp::ShowNewUserDialog(const std::string& theText)
{
	NewUserDialog* aDialog = new NewUserDialog(this, theText);  // 0x43bcc0
	aDialog->Resize(0xfa, 0x20, 0x13c, aDialog->GetPreferredHeight(0x13c));
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->mYesButton->mHasAlpha = true;
	aDialog->mNoButton->mHasAlpha = true;
	AddDialog(DIALOG_NEW_USER, aDialog);
}

// 0x41b9e0: the statistics screen (the original passes a parameter to the screen's constructor 0x45baa0) with its Close
// button
void WinWSApp::ShowStatsScreen(int theWhich)
{
	RemoveMenuScreen();
	if (mWidget72c != NULL)
		mWidget72c->SetVisible(false);

	mStatsScreen = new StatsScreen(this, theWhich);     // 0x45baa0
	mStatsScreen->SetUser(mUser);                       // 0x451050
	mStatsScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mStatsScreen);
	mStatsScreen->MarkDirty();

	if (mStatsCloseButton == NULL)
	{
		mStatsCloseButton = new DialogButton(mImages[IMAGE_TAB_BUTTON], DIALOG_STATS_CLOSE, this);
		mStatsCloseButton->SetFont(mFontRockwell13);
		mStatsCloseButton->Resize(0x1e0, 0x1a4, 100, 0x1b);
		mStatsCloseButton->mDoFinger = true;
		mStatsCloseButton->mLabel = "Close";
		mStatsCloseButton->mDoFinger = true;
		mStatsCloseButton->SetDisabled(false);
		mStatsCloseButton->mHasAlpha = true;
		mWidgetManager->AddWidget(mStatsCloseButton);
		return;
	}
	mStatsCloseButton->Resize(0x1e0, 0x1a4, 100, 0x1b);
	mStatsCloseButton->SetDisabled(false);
	mStatsCloseButton->SetVisible(true);
	mWidgetManager->BringToFront(mStatsCloseButton);
}

// 0x41bbd0: the high score screen with its "Close This Screen" button
void WinWSApp::ShowHighScoreScreen()
{
	RemoveMenuScreen();
	mHighScoreScreen = new HighScoreScreen(this);   // the original builds the widget inline: app at +0x5c, profile manager at +0x58
	mHighScoreScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHighScoreScreen);

	if (mHighScoreCloseButton == NULL)
	{
		mHighScoreCloseButton = new DialogButton(mImages[IMAGE_DBUTTON], DIALOG_HIGHSCORE_CLOSE, this);
		mHighScoreCloseButton->SetFont(mFontRockwell13);
		mHighScoreCloseButton->Resize(0xdd, 0x1bb, 0xc1, 0x1b);
		mHighScoreCloseButton->mDoFinger = true;
		mHighScoreCloseButton->mLabel = "Close This Screen";
		mHighScoreCloseButton->mDoFinger = true;
		mHighScoreCloseButton->SetDisabled(false);
		mHighScoreCloseButton->mHasAlpha = true;
		mWidgetManager->AddWidget(mHighScoreCloseButton);
		return;
	}
	mHighScoreCloseButton->SetVisible(true);
	mWidgetManager->BringToFront(mHighScoreCloseButton);
}

// 0x41d030: "No Windowed Mode" (dialog 0x30b) when the desktop is not 16 or 32 bit colour
void WinWSApp::ShowNoWindowedModeMessage()
{
	if (mBoard != NULL)
		mBoard->SetPaused(true);    // 0x442100
	mOpenDialogCount++;
	DoDialog(0x30b, true, GT(0x0052c248), GT(0x0052c260), GT(0x0052b4d0), Dialog::BUTTONS_FOOTER);
	Dialog* aDialog = GetDialog(0x30b);
	aDialog->SetHeaderFont(mFontRockwell13Bold);
	aDialog->SetButtonFont(mFontRockwell13);
	aDialog->SetLinesFont(mFontRockwell13);
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);
}

// 0x41d5c0: shows the main menu (the menu screen) and takes the board and its widgets away; with no user chosen yet it
// asks for a name
void WinWSApp::ShowMenuScreen()
{
	if (mMenuScreen == NULL)
	{
		mAbyssModeDialog = NULL;        // [0x188] +0x620 in the original (the register dialog)
		mOpenDialogCount = 0;

		mMenuScreen = new MenuScreen(this);     // 0x4436e0
		mMenuScreen->Resize(0, 0, mWidth, mHeight);
		mMenuScreen->mDisabled = false;
		mMenuScreen->mHasAlpha = true;
		mWidgetManager->AddWidget(mMenuScreen);
		mMenuScreen->SetVisible(true);
		mMenuScreen->MarkDirty();           // slot 45 (+0xb4)

		if (GetDialog(0) != NULL)           // slot 56 (GetDialog with the id lost; TODO: check the id)
			KillDialog(0);                  // slot 58

		if (mBoard != NULL)
		{
			mWidgetManager->RemoveWidget(mWidget8dc);
			mWidgetManager->RemoveWidget(mBoard);
			SafeDeleteWidget(mWidget8dc);
			SafeDeleteWidget(mBoard);
		}
		if (mWidget72c != NULL)
		{
			mWidgetManager->RemoveWidget(mWidget72c);
			SafeDeleteWidget(mWidget72c);
		}
		mWidget72c = NULL;
		mBoard = NULL;
		mWidget8dc = NULL;

		if (mProfileMgr->mUserCount != 0)   // +0xc of the manager
		{
			if (mProfileMgr->GetCurrentUserName() != "")
				return;
		}
		if (GetDialog(0) == NULL)           // (id lost; TODO: check the id)
		{
			// "Enter your name.\n(Example: \"Andrea\" or \"Andrea Weinstein\")\n"
			ShowNewUserDialog(GT(0x0052c2d4));
		}
	}
}

// 0x4267d0: "DELETE WHO?" (dialog 0x44f) with the names of the users
void WinWSApp::ShowDeleteUserDialog(const std::vector<std::string>& theNames)
{
	mDeleteUserDialog = new DeleteUserDialog(this, theNames);   // 0x43dcb0
	mDeleteUserDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	mDeleteUserDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	mDeleteUserDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mDeleteUserDialog->mSpaceAfterHeader = 0x14;
	mDeleteUserDialog->Resize(0x83, 0x22, 0x160, mDeleteUserDialog->GetPreferredHeight(0x160));
	AddDialog(DIALOG_DELETE_USER, mDeleteUserDialog);
}

// 0x41bd80: the message dialog (dialog 500, a KeyDialog with one button). theFlatten turns the line breaks of theLines
// into spaces (the dialog wraps the text; the height still counts the breaks). The dialog is made 10 pixels wider
// than the dialog image and centred. Escape, Return-less: Escape and Space close it.
void WinWSApp::ShowMessageDialog(std::string theHeader, std::string theLines, std::string theButtonText, bool theFlatten)
{
	mOpenDialogCount++;

	int aLineBreaks = 0;
	for (size_t i = 0; i < theLines.size(); i++)
	{
		if (theLines[i] == '\n')
			aLineBreaks++;
	}
	if (theFlatten)
	{
		for (size_t i = 0; i < theLines.size(); i++)
		{
			if (theLines[i] == '\n')
				theLines[i] = ' ';
		}
		theLines.insert(theLines.size(), 1, '\n');
	}

	Image* aDialogImage = mImages[IMAGE_DIALOG];
	int aHeight = aDialogImage->GetHeight();
	int aWidth = aDialogImage->GetWidth() + 10;
	int aFontHeight = mFontRockwell13->GetHeight();
	aHeight = aHeight / 2 + aFontHeight * aLineBreaks;

	KeyDialog* aDialog = new KeyDialog(this, mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], DIALOG_MESSAGE, true,
		theHeader, theLines, theButtonText, Dialog::BUTTONS_FOOTER);    // 0x43ec30
	mMessageDialog = aDialog;
	aDialog->mCloseOnKey = true;        // +0x134
	aDialog->SetEscapeKey(0x1b);        // 0x43a3e0 (TODO: name from the dialog group)
	aDialog->AddKey(0x1b);              // 0x43ed20
	aDialog->AddKey(0x20);
	aDialog->SetHeaderFont(mFontRockwell13Bold);
	aDialog->SetButtonFont(mFontRockwell13);
	aDialog->SetLinesFont(mFontRockwell13);
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);
	aDialog->mYesButton->mHasAlpha = true;
	if (theButtonText == "")
		aDialog->mYesButton->mLabel = GT(0x0052b4d0);
	else
		aDialog->mYesButton->mLabel = theButtonText;
	aDialog->mHasAlpha = true;
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_FOOTER, Color(0, 0, 0));

	int aCenterY = mHeight / 2;
	aDialog->Resize(mWidth / 2 - aWidth / 2, aCenterY - aHeight / 2, aWidth, aHeight);
	if ((mBoard != NULL) && (mRegisterDialog == NULL))
		mBoard->SetPaused(true);        // 0x442100
	AddDialog(DIALOG_MESSAGE, aDialog);
	mWidgetManager->SetFocus(aDialog);
}

// 0x41c4f0: the same as ShowMessageDialog with a yes and a no button (KeyDialog mode 1) and their labels. The
// original's arguments: header, lines, footer, flatten, yes label, no label.
void WinWSApp::ShowConfirmDialog(std::string theHeader, std::string theLines, std::string theFooter, bool theFlatten,
	std::string theYesLabel, std::string theNoLabel)
{
	mOpenDialogCount++;

	int aLineBreaks = 0;
	for (size_t i = 0; i < theLines.size(); i++)
	{
		if (theLines[i] == '\n')
			aLineBreaks++;
	}
	if (theFlatten)
	{
		for (size_t i = 0; i < theLines.size(); i++)
		{
			if (theLines[i] == '\n')
				theLines[i] = ' ';
		}
		theLines.insert(theLines.size(), 1, '\n');
	}

	Image* aDialogImage = mImages[IMAGE_DIALOG];
	int aWidth = aDialogImage->GetWidth();
	int aHeight = aDialogImage->GetHeight();
	int aFontHeight = mFontRockwell13->GetHeight();
	aHeight = aHeight / 2 + aFontHeight * aLineBreaks;

	KeyDialog* aDialog = new KeyDialog(this, mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], DIALOG_MESSAGE, true,
		theHeader, theLines, theFooter, Dialog::BUTTONS_YES_NO);    // 0x43ec30
	mMessageDialog = aDialog;
	aDialog->mCloseOnKey = true;        // +0x134
	aDialog->SetHeaderFont(mFontRockwell13Bold);
	aDialog->SetButtonFont(mFontRockwell13);
	aDialog->SetLinesFont(mFontRockwell13);
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);
	aDialog->mYesButton->mHasAlpha = true;
	aDialog->mYesButton->mLabel = theYesLabel;
	aDialog->mNoButton->mHasAlpha = true;
	aDialog->mNoButton->mLabel = theNoLabel;
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_FOOTER, Color(0, 0, 0));

	int aCenterY = mHeight / 2;
	aDialog->Resize(mWidth / 2 - aWidth / 2, aCenterY - aHeight / 2, aWidth, aHeight);
	if ((mBoard != NULL) && (mRegisterDialog == NULL))
		mBoard->SetPaused(true);        // 0x442100
	AddDialog(DIALOG_MESSAGE, aDialog);
	mWidgetManager->SetFocus(aDialog);
}

// 0x41c8c0: the hint box (HintDialog, dialog 0x3f3, footer "Click here or press space"); only when hints are on
// (mHintsAllowed). TODO: the original's arguments are lost in the decompile (two strings: a header and the text) and
// the size is computed from the line count; check against the disassembly before relying on the layout.
void WinWSApp::ShowHintDialog(std::string theHeader, std::string theLines)
{
	if (!mHintsAllowed)
		return;

	mOpenDialogCount++;
	mDialog624 = new HintDialog(this, 0x3f3, theLines, GT(0x0052b504));     // 0x439ef0 (TODO: argument list)
	mDialog624->SetHeaderFont(mFontRockwell13Bold);
	mDialog624->SetButtonFont(mFontRockwell13);
	mDialog624->SetLinesFont(mFontRockwell13);
	mDialog624->mSpaceAfterHeader = 0x19;
	mDialog624->mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);
	// +0x120 = 5 (a field of the hint dialog)
	mDialog624->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mDialog624->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	mDialog624->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	mDialog624->SetColor(Dialog::COLOR_FOOTER, Color(0, 0, 0));
	mDialog624->mYesButton->mHasAlpha = true;
	// TODO: Resize (0x41cb7a-0x41cbc0): centred, width of mImages[IMAGE_DIALOG], height from the line count and the font
	if (mBoard != NULL)
		mBoard->SetPaused(true);        // 0x442100
	AddDialog(0x3f3, mDialog624);
	mWidgetManager->SetFocus(mDialog624);
	(void) theHeader;
}

// 0x41cc60: the introduction of the level that is starting (a hint box with the level's text); abyss mode has its own
// text. The texts are the game's (GT addresses).
void WinWSApp::ShowLevelIntro()
{
	std::string aText;

	if (!mLessonManager->mAbyssMode)    // +0xa5
	{
		switch (mLessonManager->mLevel)     // +0x8c
		{
		case 0: aText = GT(0x0052c130); break;
		case 1: aText = GT(0x0052c0e0); break;
		case 2: aText = GT(0x0052beb0); break;
		case 3: aText = GT(0x0052bf10); break;
		case 4:
		case 0x21: aText = GT(0x0052bdac); break;
		case 5:
		case 0x12:
		case 0x14:
		case 0x16: aText = GT(0x0052bc20); break;
		case 6: aText = GT(0x0052b9c0); break;
		case 7: aText = GT(0x0052bde0); break;
		case 8:
		case 0xf:
		case 0x1a: aText = GT(0x0052c078); break;
		case 9:
		case 10: aText = GT(0x0052b848); break;
		case 0xb: aText = GT(0x0052ba48); break;
		case 0xc: aText = GT(0x0052bfe0); break;
		case 0xd:
		case 0x10:
		case 0x1b: aText = GT(0x0052b910); break;
		case 0xe: aText = GT(0x0052bce0); break;
		case 0x11:
		case 0x17: aText = GT(0x0052bbb0); break;
		case 0x13: aText = GT(0x0052bac0); break;
		case 0x15: aText = GT(0x0052b778); break;
		case 0x18: aText = GT(0x0052ba08); break;
		case 0x19: aText = GT(0x0052b984); break;
		case 0x1c: aText = GT(0x0052bd30); break;
		case 0x1d:
		case 0x1e:
		case 0x1f: aText = GT(0x0052b7b8); break;
		case 0x20: aText = GT(0x0052bb50); break;
		case 0x22: aText = GT(0x0052b728); break;
		case 0x23: aText = GT(0x0052b6b8); break;
		default: return;
		}
	}
	else
	{
		if (mBoard->mField1c4 != 0)     // +0x1c4 (TODO: name from the board group)
			return;

		// the registered game's text (0x52b5e8): the progress is saved every 400 ft; the unregistered text (0x52b528)
		// is not ported
		aText = StrFormat(GTC(0x0052b5e8), 400);
	}

	ShowHintDialog(GT(0x0052b520), aText);
}

// 0x426440: after loading: the title screen goes, the menu and the (hidden) map screen come, and the saved data of the
// current user is checked
void WinWSApp::ShowMapScreen()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	ShowMenuScreen();

	mScreen62c = new MapScreen(this);       // 0x458d10
	mScreen62c->SetVisible(false);
	mScreen62c->SetDisabled(true);
	mScreen62c->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mScreen62c);

	if (mUser == NULL)
	{
		std::string aName = mProfileMgr->GetCurrentUserName();      // 0x4150a0
		if (aName != "")
		{
			// "Your saved data was either not found or is corrupted. A new one will be created."
			ShowMessageDialog(GT(0x0052c6c8), GT(0x0052c6d0), GT(0x0052b4d0), false);

			// TODO: 0x4265b9-0x4266f0: the user-profile part, a new UserProfile (0x421f50) for aName is made and saved
			// again (0x416380 name, 0x4262b0 GetDefaultUserNumber -> 0x412380, 0x41f1d0 save, 0x420c60, 0x415060,
			// 0x419da0); the profile group writes it as UserProfileMgr::RecreateUser(aName) and this calls it
			mUser = mProfileMgr->RecreateUser(aName);
			return;
		}
	}

	if (mUser != NULL)
	{
		if (atof(mUser->mGameVersion.c_str()) < 0.48)       // +0x28c: the version that wrote the file
		{
			// "Your saved data was from an older version of this game and is not compatible. A new file will be created."
			ShowMessageDialog(GT(0x0052c6c8), GT(0x0052c658), "OK", false);
			mUser->mGameVersion = mProductVersion;
		}
	}
}

// 0x431320: starts a game: the map screen steps aside and a new board comes (TODO: names of the board methods)
void WinWSApp::StartBoard()
{
	mFlag8d8 = false;
	mScreen62c->SetVisible(false);
	mScreen62c->SetDisabled(true);

	mWidgetManager->RemoveWidget(mBoard);
	if (mBoard != NULL)
		delete mBoard;

	mBoard = new Board(this);                   // 0x4544b0 (0xf98 bytes)
	mBoard->SetFastCPU(mFastCPU);               // 0x440570
	int aLevel = mScreen62c->mLevel;            // +0x228
	if (aLevel < 0x24)
	{
		mLessonManager->SetLevel(aLevel);       // 0x4309d0
		if (mScreen62c->mLevel == 0x23)
			mScreen62c->SetFinalLevel(true);    // 0x441080
	}
	else
		mLessonManager->SetLevel(mLessonManager->mLevel);   // +0x8c

	aLevel = mScreen62c->mLevel;
	if ((aLevel == 2) || ((aLevel > 5) && (aLevel < 8)))
	{
		// 0x41d870 not ported: the trial limit (register prompt) of levels 2, 6 and 7
		WriteToRegistry();                      // slot 18
	}

	mBoard->SetLessonManager(mLessonManager);   // 0x451cb0
	mBoard->Resize(0, 0, mWidth, mHeight);
	mStats->mReserved[0]++;                     // +4: the number of games started
	mStats->mReserved[5] = mField904;           // +0x1c
	mBoard->StartGame(*mStats);                 // 0x440130 (the 0x30-byte block by value)
	mWidgetManager->AddWidget(mBoard);
	mBoard->SetVisible(true);
	mBoard->SetDisabled(false);
	mWidget8dc->SetVisible(true);
	mWidget8dc->SetDisabled(false);
	mWidgetManager->BringToFront(mWidget8dc);
	mWidgetManager->SetFocus(mBoard);
}

// Not ported (registration, trial and update code):
// 0x41b030 / 0x41b230: the update check dialogs (LoadingThreadCompleted would start them)
// 0x41c1c0: the register dialog (id 0x9e5, "images\register")
// 0x41d870, 0x41d9d0, 0x41da80: the trial limits (MaxPlays, MaxTime) and their prompts
// 0x414020: the register dialog (id 600)

// TODO (not read yet): 0x426c60, 0x429450, 0x42a870, 0x42aad0, 0x42ee10, 0x42f3c0 (they use the user profile, the
// lessons and the app fields +0x8fc..+0x90c), 0x428d80, 0x429110, 0x4292b0, 0x430b20 (config parsing helpers),
// 0x426a40 (copies the lesson word lists into the user), 0x4262b0 (UserProfileMgr::GetDefaultUserNumber)
