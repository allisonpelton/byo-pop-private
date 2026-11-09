#include "SelectUserDialog.h"
#include "WinWSApp.h"
#include "UserProfileMgr.h"
#include "UserProfile.h"
#include "GameText.h"

#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "ListWidget.h"
#include "WidgetManager.h"
#include <list>
#include <vector>
#include <stdlib.h>

using namespace Sexy;

// Not translated (forwarders to Dialog's own functions; nothing to override): 0x437ca0 (slot 7, SetColor), 0x438e60
// (slot 16, Draw) and 0x438340 (slot 17, Update), MoneyDialog's constructor 0x4382c0, destructor body 0x438310 and
// Draw 0x438330; the scalar deleting destructor of this class 0x43a270 (thunk of 0x43a1a0)

// 0x55fdd0: the list's five colours (background, outline, text, hilite, select)
static int gSelectUserListColors[5][3] =
{
	{128, 128, 128},
	{0, 0, 0},
	{255, 255, 255},
	{255, 200, 86},
	{100, 100, 100}
};

// 0x43edb0: MoneyDialog (0x4382c0) first. OK / CANCEL, header "WHO ARE YOU?", no lines or footer. The creator
// (WinWSApp::ShowSelectUserDialog 0x4128a0) recolours it and sizes it.
SelectUserDialog::SelectUserDialog(WinWSApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_SELECT_USER, true, GT(0x0052ed1c), "", "",
		Dialog::BUTTONS_OK_CANCEL),
	mApp(theApp),
	mUserList(NULL),
	mSelectedName("")
{
	mTextAlign = -1;
	SetHeaderFont(theApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	SetButtonFont(mApp->mFontRockwell13);
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));

	// the original also reads a value of the profile manager here (a call of Font::GetHeight, folded with a plain
	// "return [this+0xc]", on mApp->mProfileMgr) and drops the result: not ported

	mUserList = new ListWidget(0, mApp->mFontRockwell13, this);
	mUserList->SetColors(gSelectUserListColors, 5);
	mUserList->mDrawOutline = true;
	mUserList->mFont = mApp->mFontRockwell13;
	mUserList->mJustify = 1;

	mSelectedName = "";
	UpdateList();
}

// 0x43a1a0 (body; the scalar deleting destructor is 0x43a270): the list goes, then the name, then MoneyDialog's body. The
// list is out of the widget manager by now (RemovedFromManager).
SelectUserDialog::~SelectUserDialog()
{
	delete mUserList;
}

// 0x438e00 slot 1 (DeleteUserDialog uses this function too)
void SelectUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->AddWidget(mUserList);
}

// 0x438e30 slot 2 (DeleteUserDialog uses this function too)
void SelectUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->RemoveWidget(mUserList);
}

// 0x437c20 (ButtonListener slot 3; DeleteUserDialog's and AbyssModeDialog's are this function too): the click goes to
// the app; a press of any other button (there is none) plays the button sample
void SelectUserDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + ID_YES_2004) && (theId != mId + ID_NO_2004))
		mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x43d6b0 (ButtonListener slot 4). The base first: the app's handler of 0xc1e (the OK button) makes mSelectedName the
// current user and kills the dialog (the delete is deferred, so this object stays valid here). Then OK again, from the
// selected line of the list: a line that is no user's name (the new-user or delete line) finds no profile and nothing
// happens.
void SelectUserDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);

	if ((theId == mId + ID_YES_2004) && (mUserList != NULL) && (mUserList->mSelectIdx >= 0))
	{
		unsigned int aLineCount = mUserList->mLines.size();
		if ((unsigned int) mUserList->mSelectIdx < aLineCount)
		{
			std::string aName = mUserList->mLines[mUserList->mSelectIdx];
			UserProfile* aProfile = mApp->mProfileMgr->LoadUser(aName);		// 0x425f20
			if (aProfile != NULL)
			{
				// data of a version before 0.48 is not compatible: the player is told and the profile takes the
				// game's version
				if (atof(aProfile->mGameVersion.c_str()) < 0.48)		// 0x52c568
				{
					mApp->ShowMessageDialog(GT(0x0052c6c8), GT(0x0052c658), GT(0x0052b4d0), true);	// 0x41bd80: "Error", "OK"
					aProfile->mGameVersion = mApp->mProductVersion;		// +0x288 = app +0x3b8
				}
				mApp->mProfileMgr->RegisterDefaultUserName(aProfile->GetName());	// 0x419da0, 0x415060
				mApp->mUser = aProfile;									// +0x8e8
				mApp->mHintsAllowed = aProfile->mFlag1fc;				// +0x8ed
				mApp->MarkMenuScreenDirty();							// 0x412cc0
			}
		}
	}
}

// 0x43e220: refills the list from the profile manager. Fewer than 10 players: the new-user line (text 0x52ec10) then the
// names; exactly 10: the names then the delete line (text 0x52ec00); more (registry editing only): the first 10 and no
// special line; none: an empty list. The current user is selected (an exact compare); if it is not found the old
// selection index stays.
void SelectUserDialog::UpdateList()
{
	if (mUserList != NULL)
	{
		// only the lines: colours, selection, hilite and scroll position are left as they are
		mUserList->mLines.clear();

		std::list<UserProfile*>& aUsers = mApp->mProfileMgr->mUsers;
		std::list<UserProfile*>::iterator anItr = aUsers.begin();
		int i = 0;
		while (anItr != aUsers.end())
		{
			if ((i == 0) && (aUsers.size() < MAX_LISTED_USERS))
			{
				mUserList->AddLine(GT(0x0052ec10), false);
				i = 1;
			}

			UserProfile* aProfile = *anItr;
			if ((aProfile != NULL) && (i < MAX_LISTED_USERS))
			{
				int anIdx = mUserList->AddLine(aProfile->GetName(), false);
				if (aProfile->GetName() == mApp->mProfileMgr->GetCurrentUserName())
					mUserList->SetSelect(anIdx);
			}

			++anItr;
			i++;
		}

		if (aUsers.size() == MAX_LISTED_USERS)
			mUserList->AddLine(GT(0x0052ec00), false);

		MarkDirty();
	}
}

// 0x43e520 slot 50: the header part of the Dialog formula, then the list (refilled first) and 16 pixels
int SelectUserDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog2004::GetPreferredHeight(theWidth);
	UpdateList();
	if (mUserList != NULL)
		return aHeight + 0x10 + mUserList->GetOptimalHeight();
	return aHeight;
}

// 0x43e560 slot 13. Screen coordinates.
void SelectUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	UpdateList();

	// The list 16 pixels above the buttons as Dialog::Resize placed them, 40 pixels in from each side
	if (mUserList != NULL)
	{
		int aListHeight = mUserList->GetOptimalHeight();
		mUserList->Resize(mX + 0x28, mYesButton->mY - (aListHeight + 0x10), mWidth - 0x50, mUserList->GetOptimalHeight());
	}

	// The buttons again: 16 pixels in from the content edges with 16 pixels between them
	if (mYesButton != NULL)
	{
		if (mNoButton != NULL)
		{
			int aButtonWidth = (((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - 0x30) / 2;
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + 0x10 + mContentInsets.mLeft, ((mHeight - mContentInsets.mBottom) - aButtonHeight) + mY,
				aButtonWidth, aButtonHeight);
			mNoButton->Resize(mX + mContentInsets.mLeft + 0x20 + aButtonWidth,
				((mHeight - mContentInsets.mBottom) - aButtonHeight) + mY, aButtonWidth, aButtonHeight);
			return;
		}

		int aButtonHeight = mYesButton->mComponentImage->GetHeight();
		mYesButton->Resize(mX + mContentInsets.mLeft, ((mHeight - mContentInsets.mBottom) + mY) - aButtonHeight,
			(mWidth - mContentInsets.mRight) - mContentInsets.mLeft, aButtonHeight);
	}
}

// 0x43fa10 ListListener (+0x120) slot 0: a click on another line selects it (with the button sample). The new-user line
// opens the NewUserDialog (with its own prompt, text 0x52ed98) and the delete line the DeleteUserDialog with the ten
// names; any other line is remembered in mSelectedName (the click count does not matter: there is no double click to
// take a name, the OK button does that).
void SelectUserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if ((mUserList != NULL) && (mUserList->mId == theId))
	{
		if (theIdx != mUserList->mSelectIdx)
		{
			mApp->PlaySample(SOUND_BUTTONCLICK);
			mUserList->SetSelect(theIdx);
		}

		std::string aLine = mUserList->mLines[theIdx];
		if (aLine == GT(0x0052ec10))
		{
			mApp->ShowNewUserDialog(GT(0x0052ed98));		// 0x414270
		}
		else if (aLine == GT(0x0052ec00))
		{
			// only listed with exactly ten players, so the first ten lines are their names
			std::vector<std::string> aNames;
			int i = 0;
			do
			{
				aNames.push_back(mUserList->mLines[i]);
				i++;
			}
			while (i < MAX_LISTED_USERS);
			mApp->ShowDeleteUserDialog(aNames);				// 0x4267d0
		}
		else
			mSelectedName = aLine;
	}
}
