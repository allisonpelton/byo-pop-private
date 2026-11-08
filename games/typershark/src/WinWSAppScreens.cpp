// WinWSAppScreens.cpp: the screens and dialogs the app shows (the original's unnamed WinWSApp methods between 0x4128a0
// and 0x431320): the menu, map, board, statistics and high score screens, the user dialogs, the message and hint
// dialogs and the level introductions. The button handler (0x431630) is in WinWSAppButtons.cpp.

#include "WinWSApp.h"
#include "GameText.h"
#include "Board.h"
#include "TitleScreen.h"
#include "MenuScreen.h"
#include "MapScreen.h"
#include "HighScoreScreen.h"
#include "StatsScreen.h"
#include "TypingTutor.h"
#include "StatusBar.h"
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

// 0x412ae0: a number of the current user (+0x34; the profile group names it); the original does not test mUser
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

// 0x412c90: the typing tutor's own ButtonListener (the object at +0x58, slot 4 = ButtonDepress) gets button 0x58, then the
// tutor takes the focus. The original does not test mTypingTutor.
void WinWSApp::ActivateTypingTutor()
{
	mTypingTutor->ButtonDepress(0x58);
	mWidgetManager->SetFocus(mTypingTutor);
}

// 0x412cc0: slot 46 (MarkDirty)
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
void WinWSApp::FocusTypingTutor()
{
	if (mTypingTutor != NULL)
		mWidgetManager->SetFocus(mTypingTutor);
}

// 0x412d50: the current user's hints setting
void WinWSApp::SetUserHints(bool theHints)
{
	if (mUser != NULL)
		mUser->mShowHints = theHints;       // +0x1fd
}

// 0x412d70: hints show when no user is chosen, or when the user has them on and the game allows them
bool WinWSApp::HintsEnabled()
{
	if (mUser == NULL)
		return true;
	if (mUser->mShowHints)      // +0x1fd
	{
		if (mHintsAllowed)
			return true;
	}
	return false;
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
	if (mTypingTutor != NULL)
		mTypingTutor->SetVisible(false);

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
		mRegisterDialog = NULL;         // +0x620 (the register dialog is not ported; the original clears it here)
		mOpenDialogCount = 0;

		mMenuScreen = new MenuScreen(this);     // 0x4436e0
		mMenuScreen->Resize(0, 0, mWidth, mHeight);
		mMenuScreen->mDisabled = false;
		mMenuScreen->mHasAlpha = true;
		mWidgetManager->AddWidget(mMenuScreen);
		mMenuScreen->SetVisible(true);
		mMenuScreen->MarkDirtyFull();       // slot 45

		if (GetDialog(DIALOG_OPTIONS) != NULL)
			KillDialog(DIALOG_OPTIONS);

		if (mBoard != NULL)
		{
			mWidgetManager->RemoveWidget(mStatusBar);
			mWidgetManager->RemoveWidget(mBoard);
			SafeDeleteWidget(mStatusBar);
			SafeDeleteWidget(mBoard);
		}
		if (mTypingTutor != NULL)
		{
			mWidgetManager->RemoveWidget(mTypingTutor);
			SafeDeleteWidget(mTypingTutor);
		}
		mTypingTutor = NULL;
		mBoard = NULL;
		mStatusBar = NULL;

		if (mProfileMgr->mUserCount != 0)   // +0xc of the manager (a count or a list; the profile group names it)
		{
			if (mProfileMgr->GetCurrentUserName() != "")
				return;
		}
		if (GetDialog(DIALOG_NEW_USER) == NULL)
		{
			// the prompt for a first name (with an example)
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

// 0x41bd80: the message dialog (dialog 500, a KeyDialog with one button; the stack arguments are by value). theFlatten turns the line breaks of theLines
// into spaces (the dialog wraps the text; the height still counts the breaks). The dialog is made 10 pixels wider
// than the dialog image and centred. Escape and Space press its button.
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
	aDialog->RemoveCancelKey(0x1b);     // 0x43a3e0 (takes the key out of the list at +0x110, which the constructor starts with 0x1b)
	aDialog->AddOkKey(0x1b);            // 0x43ed20 (adds to the list at +0x120)
	aDialog->AddOkKey(0x20);
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

// 0x41c8c0: the hint box (HintDialog, dialog 0x3f3) with the footer "Click here or press space", centred under the
// level: the strings are passed by value; only when hints are allowed (mHintsAllowed; the callers test HintsEnabled)
void WinWSApp::ShowHintDialog(std::string theLines, std::string theHeader)
{
	if (!mHintsAllowed)
		return;

	mOpenDialogCount++;
	mHintDialog = new HintDialog(this, 0x3f3, theHeader, theLines, GT(0x0052b504),
		mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], mImages[IMAGE_CHECKED]);     // 0x439ef0
	mHintDialog->SetHeaderFont(mFontRockwell13Bold);
	mHintDialog->SetButtonFont(mFontRockwell13);
	mHintDialog->SetLinesFont(mFontRockwell13);
	mHintDialog->mSpaceAfterHeader = 0x19;
	mHintDialog->mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);
	mHintDialog->mField120 = 5;         // +0x120 (the hint dialog's own field; the dialog group names it)
	mHintDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mHintDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	mHintDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	mHintDialog->SetColor(Dialog::COLOR_FOOTER, Color(0, 0, 0));
	mHintDialog->mYesButton->mHasAlpha = true;

	Image* aDialogImage = mImages[IMAGE_DIALOG];
	int aDialogHeight = aDialogImage->GetHeight();
	int aWidth = aDialogImage->GetWidth();

	int aLineBreaks = 0;
	size_t i = 0;
	while (i < theLines.size())
	{
		if (theLines[i] == '\n')
			aLineBreaks++;
		i++;
	}

	int aFontHeight = mFontRockwell13->GetHeight();
	int aHeight = mImages[IMAGE_CHECKED]->GetHeight() + aDialogHeight / 2 + aFontHeight * aLineBreaks + 0x23;
	int aCenterY = mHeight / 2;
	mHintDialog->Resize(mWidth / 2 - aWidth / 2, aCenterY - aHeight / 2, aWidth, aHeight);

	if (mBoard != NULL)
		mBoard->SetPaused(true);        // 0x442100
	AddDialog(0x3f3, mHintDialog);
	mWidgetManager->SetFocus(mHintDialog);
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

	ShowHintDialog(aText, GT(0x0052b520));
}

// 0x426440: after loading: the title screen goes, the menu and the (hidden) map screen come, and the saved data of the
// current user is checked
void WinWSApp::ShowMapScreen()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	ShowMenuScreen();

	mMapScreen = new MapScreen(this);       // 0x458d10
	mMapScreen->SetVisible(false);
	mMapScreen->SetDisabled(true);
	mMapScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mMapScreen);

	if (mUser == NULL)
	{
		std::string aName = mProfileMgr->GetCurrentUserName();      // 0x4150a0
		if (aName != "")
		{
			// the saved data of the chosen user is missing or damaged: a new, empty profile of that name is made
			ShowMessageDialog(GT(0x0052c6c8), GT(0x0052c6d0), GT(0x0052b4d0), true);

			mUser = new UserProfile(this);                                  // 0x421f50 (sizeof 0x2c8)
			mUser->SetName(mProfileMgr->GetCurrentUserName());              // 0x416380
			mUser->SetUserNumber(mProfileMgr->GetDefaultUserNumber());      // 0x412380, 0x4262b0
			mProfileMgr->WriteUserProfileToRegistry(mUser, false, false);   // 0x41f1d0
			mProfileMgr->SaveUser(mUser);                                   // 0x420c60
			mProfileMgr->RegisterDefaultUserName(mUser->GetName());         // 0x419da0, 0x415060
			return;
		}
	}

	if (mUser != NULL)
	{
		if (atof(mUser->mGameVersion.c_str()) < 0.48)       // +0x28c: the version that wrote the file
		{
			// the saved data is from an older version and cannot be used; a new file is made
			ShowMessageDialog(GT(0x0052c6c8), GT(0x0052c658), GT(0x0052b4d0), true);
			mUser->mGameVersion = mProductVersion;
		}
	}
}

// 0x431320: starts a game: the map screen steps aside and a new board comes (TODO: names of the board methods)
void WinWSApp::StartBoard()
{
	mFlag8d8 = false;
	mMapScreen->SetVisible(false);
	mMapScreen->SetDisabled(true);

	mWidgetManager->RemoveWidget(mBoard);
	if (mBoard != NULL)
		delete mBoard;

	mBoard = new Board(this);                   // 0x4544b0 (0xf98 bytes)
	mBoard->SetFastCPU(mFastCPU);               // 0x440570
	int aLevel = mMapScreen->mLevel;            // +0x228
	if (aLevel < 0x24)
	{
		mLessonManager->SetLevel(aLevel);       // 0x4309d0
		if (mMapScreen->mLevel == 0x23)
			mMapScreen->SetFinalLevel(true);    // 0x441080
	}
	else
		mLessonManager->SetLevel(mLessonManager->mLevel);   // +0x8c

	aLevel = mMapScreen->mLevel;
	if ((aLevel == 2) || ((aLevel > 5) && (aLevel < 8)))
	{
		// 0x41d870 not ported: the trial limit (register prompt) of levels 2, 6 and 7
		WriteToRegistry();                      // slot 18
	}

	mBoard->SetLessonManager(mLessonManager);   // 0x451cb0
	mBoard->Resize(0, 0, mWidth, mHeight);
	mStats->mReserved[0]++;                     // +4: the number of games started
	mStats->mReserved[6] = mField904;           // +0x1c
	mBoard->StartGame(*mStats);                 // 0x440130 (the 0x30-byte block by value)
	mWidgetManager->AddWidget(mBoard);
	mBoard->SetVisible(true);
	mBoard->SetDisabled(false);
	mStatusBar->SetVisible(true);
	mStatusBar->SetDisabled(false);
	mWidgetManager->BringToFront(mStatusBar);
	mWidgetManager->SetFocus(mBoard);
}

// Not ported (registration, trial and update code):
// 0x41b030 / 0x41b230: the update check dialogs (LoadingThreadCompleted would start them)
// 0x41c1c0: the register dialog (id 0x9e5, "images\register")
// 0x41d870, 0x41d9d0, 0x41da80: the trial limits (MaxPlays, MaxTime) and their prompts
// 0x414020: the register dialog (id 600)

// Not translated here (user profile, lesson and configuration code of the other jobs; most are called only from
// ButtonDepress 0x431630): 0x42a870 (back to the menu: resets the game state), 0x42ee10 and 0x42f3c0 (start a game:
// they make the lesson manager and load the configuration), 0x429450, 0x426c60, 0x429110, 0x4292b0, 0x428d80,
// 0x42aad0, 0x430b20, 0x426a40 (copies the lesson word lists into the user), 0x4262b0 (GetDefaultUserNumber),
// 0x413ff0, 0x4137d0 (LessonManager::IsTutorialActive).
