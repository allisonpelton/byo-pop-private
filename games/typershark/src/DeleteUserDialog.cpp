#include "DeleteUserDialog.h"
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

using namespace Sexy;

// Not translated (forwarders to Dialog's own functions; nothing to override): 0x437ca0 (slot 7, SetColor), 0x438e60
// (slot 16, Draw) and 0x438340 (slot 17, Update), MoneyDialog's constructor 0x4382c0, destructor body 0x438310 and
// Draw 0x438330; the scalar deleting destructor of this class 0x4391e0 (thunk of 0x438f20)

// 0x55fcf4: the list's five colours (background, outline, text, hilite, select; the same as SelectUserDialog's)
static int gDeleteUserListColors[5][3] =
{
	{128, 128, 128},
	{0, 0, 0},
	{255, 255, 255},
	{255, 200, 86},
	{100, 100, 100}
};

// 0x43dcb0: MoneyDialog (0x4382c0) first. OK / CANCEL, header "DELETE WHO?", the warning (text 0x52ebb8) as the lines, no
// footer. The creator (WinWSApp::ShowDeleteUserDialog 0x4267d0) recolours it and sizes it. Unlike SelectUserDialog's,
// the text alignment stays at its default.
DeleteUserDialog::DeleteUserDialog(WinWSApp* theApp, std::vector<std::string> theNames) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_DELETE_USER, true, GT(0x0052ebac),
		GT(0x0052ebb8), "", Dialog::BUTTONS_OK_CANCEL),
	mApp(theApp),
	mUserList(NULL)
{
	SetHeaderFont(theApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	SetButtonFont(mApp->mFontRockwell13);
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));

	mUserList = new ListWidget(0, mApp->mFontRockwell13, this);
	mUserList->SetColors(gDeleteUserListColors, 5);
	mUserList->mDrawOutline = true;
	mUserList->mJustify = 1;

	// nothing is selected (mSelectIdx stays -1)
	unsigned int i = 0;
	while (i < theNames.size())
	{
		mUserList->AddLine(theNames[i], false);
		i++;
	}
}

// 0x438f20 (body; the scalar deleting destructor is 0x4391e0): MoneyDialog's body only. port: the original never deletes
// the list (0xc0 bytes leaked for each dialog); it is out of the widget manager by now (RemovedFromManager).
DeleteUserDialog::~DeleteUserDialog()
{
	delete mUserList;
}

// 0x437c60 slot 50: the header part of the Dialog formula, then 16 pixels and the list
int DeleteUserDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog2004::GetPreferredHeight(theWidth);
	if (mUserList != NULL)
		return aHeight + 0x10 + mUserList->GetOptimalHeight();
	return aHeight;
}

// 0x437cb0 slot 13: SelectUserDialog::Resize without the UpdateList
void DeleteUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);

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

// 0x438e00 slot 1 (SelectUserDialog's function, shared)
void DeleteUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->AddWidget(mUserList);
}

// 0x438e30 slot 2 (SelectUserDialog's function, shared)
void DeleteUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->RemoveWidget(mUserList);
}

// 0x437c20 (ButtonListener slot 3; SelectUserDialog's and AbyssModeDialog's are this function too): the click goes to
// the app; a press of any other button (there is none) plays the button sample
void DeleteUserDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + ID_YES_2004) && (theId != mId + ID_NO_2004))
		mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x43a9b0: the app's confirmation (YES / NO, dialog 500; the name is in the text 0x52e598, the header is text 0x52e588)
// and the app's flag at +0x910, which makes the handler of its YES button call DeleteSelectedUser
void DeleteUserDialog::ConfirmDelete(std::string theName)
{
	std::string aLines = StrFormat(GTC(0x0052e598), theName.c_str());
	mApp->mFlag910 = true;
	mApp->ShowConfirmDialog(GT(0x0052e588), aLines, "", true, GT(0x0052d1f8), GT(0x0052d1fc));	// 0x41c4f0: "YES", "NO"
}

// 0x43ab70 ListListener (+0x120) slot 0: a click on another line selects it (with the button sample); a double click asks
// for confirmation
void DeleteUserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if ((mUserList != NULL) && (mUserList->mId == theId))
	{
		if (theIdx != mUserList->mSelectIdx)
		{
			mApp->PlaySample(SOUND_BUTTONCLICK);
			mUserList->SetSelect(theIdx);
		}

		std::string aName = mUserList->mLines[theIdx];
		if (theClickCount == 2)
			ConfirmDelete(aName);
	}
}

// 0x43eb20 (ButtonListener slot 4). The base first: the app's handler of 0x1007 (CANCEL) kills this dialog (the delete
// is deferred, so this object stays valid here); OK (0xc1f) has no case there. OK asks for confirmation (nothing happens
// without a selection and the dialog stays); CANCEL refreshes the list of the SelectUserDialog underneath.
void DeleteUserDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);

	if (theId == mId + ID_YES_2004)
	{
		if ((mUserList != NULL) && (mUserList->mSelectIdx >= 0))
		{
			std::string aName = mUserList->mLines[mUserList->mSelectIdx];
			ConfirmDelete(aName);
		}
	}
	else if (theId == mId + ID_NO_2004)
	{
		SelectUserDialog* aSelectUserDialog = (SelectUserDialog*) mApp->GetDialog(DIALOG_SELECT_USER);
		if (aSelectUserDialog != NULL)
			aSelectUserDialog->UpdateList();
	}
}

// 0x43df60: called only by the app, when the confirmation answers YES (it then kills this dialog). Every line equal to
// the selected name goes out of the list; the profile is removed (registry key and files) and, if it was the current
// user, the first remaining name becomes the current one. Then the SelectUserDialog underneath is fitted to its list
// again (its GetPreferredHeight and Resize both refill the list, so it shrinks and shows the new-user line again) and
// is sent a click on its line 1, which selects the first name.
void DeleteUserDialog::DeleteSelectedUser()
{
	if ((mUserList != NULL) && (mUserList->mSelectIdx >= 0))
	{
		std::string aName = mUserList->mLines[mUserList->mSelectIdx];

		// straight out of the vector (colours and selection stay)
		std::vector<std::string>::iterator anItr = mUserList->mLines.begin();
		while (anItr != mUserList->mLines.end())
		{
			std::string aLine = *anItr;
			if (aLine == aName)
				anItr = mUserList->mLines.erase(anItr);
			else
				++anItr;
		}

		if (mApp->mProfileMgr != NULL)
		{
			mApp->mProfileMgr->DeregisterUserProfile(aName);		// 0x420ff0
			if (aName == mApp->mProfileMgr->GetCurrentUserName())	// 0x4150a0
			{
				// the original reads the first line unchecked; with ten players there are nine left
				aName = mUserList->mLines[0];
				if (aName.size() > 0)
					mApp->mProfileMgr->RegisterDefaultUserName(aName);	// 0x419da0
			}
		}
	}

	SelectUserDialog* aSelectUserDialog = (SelectUserDialog*) mApp->GetDialog(DIALOG_SELECT_USER);
	if (aSelectUserDialog != NULL)
	{
		int aWidth = aSelectUserDialog->mWidth;
		int aHeight = aSelectUserDialog->GetPreferredHeight(aWidth);
		aSelectUserDialog->Resize(aSelectUserDialog->mX, aSelectUserDialog->mY, aWidth, aHeight);
		// the list listener of that dialog (+0x120), slot 0: line 1 (the first name, after the new-user line) is clicked once
		aSelectUserDialog->ListClicked(0, 1, 1);
	}
}
