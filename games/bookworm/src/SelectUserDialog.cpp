#include "SelectUserDialog.h"
#include "BookWormApp.h"
#include "UserManager.h"
#include "BWGameSelector.h"
#include "Res.h"
#include "GameText.h"

#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "ListWidget.h"
#include "WidgetManager.h"
#include <vector>

using namespace Sexy;

// 0x52f788
static int gSelectUserListColors[5][3] =
{
	{53, 43, 29},       // background
	{0, 0, 0},          // outline
	{255, 225, 73},     // text
	{255, 255, 255},    // hilited text
	{188, 22, 22}       // selection fill
};

// 0x4325f0 (body; scalar deleting dtor 0x4326f0)
SelectUserDialog::~SelectUserDialog()
{
	if (mApp2->mGameSelector != NULL)
		mApp2->mGameSelector->SetDisabled(false);

	// port: the original never deletes the list (0xc0 bytes leaked per dialog). It is out of the widget
	// manager by now (RemovedFromManager).
	delete mUserList;
}

// 0x432670 (slot 7; folded with DeleteUserDialog's)
void SelectUserDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
}

// 0x432680 (slot 1; folded with DeleteUserDialog's)
void SelectUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->AddWidget(mUserList);
}

// 0x4326b0 (slot 2; folded with DeleteUserDialog's)
void SelectUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->RemoveWidget(mUserList);
}

// 0x4326e0 (slot 16; folded with DeleteUserDialog's): the header only
void SelectUserDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);
}

// 0x41df60 (ButtonListener +0x58 slot 3; folded with DeleteUserDialog's). The dialog has only OK and
// CANCEL, which click through the app, so the click here never plays.
void SelectUserDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + ID_YES_2004) && (theId != mId + ID_NO_2004))
		mApp2->PlaySample(SOUND_BUTTONCLICK);
}

// 0x432720 (ButtonListener +0x58 slot 4)
void SelectUserDialog::ButtonDepress(int theId)
{
	// The app kills dialog 19 for both buttons; the delete is deferred, so this object stays valid here
	Dialog2004::ButtonDepress(theId);

	// OK: switch to the selected user and load their settings. A special line selected just closes.
	if ((theId == mId + ID_YES_2004) && (mUserList != NULL) && (mUserList->mSelectIdx >= 0) &&
		(mUserList->mSelectIdx < (int) mUserList->mLines.size()))
	{
		SexyString aName = mUserList->mLines[mUserList->mSelectIdx];
		User* aUser = mApp2->mUserManager->FindUser(aName);
		if (aUser != NULL)
		{
			mApp2->mUserManager->SetCurrentUser(aUser->mName);
			mApp2->LoadUserSettings();
		}
	}
}

// 0x432810: refills the list from the user manager. Fewer than 5 users: the new-user line
// (GT_USER_LIST_CREATE) then the names; exactly 5: the names then the delete line (GT_USER_LIST_DELETE); 6
// or more (registry editing only): the first 5 and no special line; none: an empty list. The current user
// is selected (exact compare); if it is not found the old selection index stays.
void SelectUserDialog::UpdateList()
{
	if (mUserList == NULL)
		return;

	// Only the lines: colours, selection, hilite and scroll position are left as they are
	mUserList->mLines.clear();

	UserManager* aUserManager = mApp2->mUserManager;
	int i = 0;
	for (std::list<User*>::iterator anItr = aUserManager->mUsers.begin(); anItr != aUserManager->mUsers.end(); ++anItr)
	{
		if ((i == 0) && ((int) aUserManager->mUsers.size() < MAX_LISTED_USERS))
		{
			mUserList->AddLine(GameText(GT_USER_LIST_CREATE), false);
			i = 1;
		}

		User* aUser = *anItr;
		if (aUser != NULL)
		{
			if (i < MAX_LISTED_USERS)
			{
				int anIdx = mUserList->AddLine(aUser->mName, false);
				if (aUser->mName == aUserManager->GetCurrentUserName())
					mUserList->SetSelect(anIdx);
			}
			else
			{
				// OutputDebugString("ERROR: Too many user profiles. '%s' skipped.\n") not ported: debugger output only
			}
		}
		i++;
	}

	if ((int) aUserManager->mUsers.size() == MAX_LISTED_USERS)
		mUserList->AddLine(GameText(GT_USER_LIST_DELETE), false);

	MarkDirty();
}

// 0x432b00 (slot 50): the header part of the Dialog formula, then the list (refilled first) and 16 px
int SelectUserDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog2004::GetPreferredHeight(theWidth);
	UpdateList();
	if (mUserList != NULL)
		return aHeight + 16 + mUserList->GetOptimalHeight();
	return aHeight;
}

// 0x432b40 (slot 13). Screen coordinates.
void SelectUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	UpdateList();

	// The list 16 px above the buttons as Dialog::Resize placed them, 40 px in from each side
	if (mUserList != NULL)
	{
		int aListHeight = mUserList->GetOptimalHeight();
		mUserList->Resize(mX + 40, mYesButton->mY - (aListHeight + 16), mWidth - 80, mUserList->GetOptimalHeight());
	}

	// The buttons again: 16 px in from the content edges with 16 px between them (W 400: 40..192, 208..360)
	if (mYesButton != NULL)
	{
		if (mNoButton != NULL)
		{
			int aButtonWidth = (mWidth - mContentInsets.mRight - mContentInsets.mLeft - 48) / 2;
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + 16 + mContentInsets.mLeft, mHeight - mContentInsets.mBottom - aButtonHeight + mY,
				aButtonWidth, aButtonHeight);
			mNoButton->Resize(mX + mContentInsets.mLeft + 32 + aButtonWidth,
				mHeight - mContentInsets.mBottom - aButtonHeight + mY, aButtonWidth, aButtonHeight);
			return;
		}

		int aButtonHeight = mYesButton->mComponentImage->GetHeight();
		mYesButton->Resize(mX + mContentInsets.mLeft, mHeight - mContentInsets.mBottom + mY - aButtonHeight,
			mWidth - mContentInsets.mRight - mContentInsets.mLeft, aButtonHeight);
	}
}

// 0x432c90. The creator (BookWormApp::ShowSelectUserDialog) sets colour 0 (white) and sizes it to
// (0, 17, 400, GetPreferredHeight(400)).
SelectUserDialog::SelectUserDialog(BookWormApp* theApp) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_SELECT_USER, true,
		GameText(GT_SELECT_USER_HEADER), "", "", BUTTONS_OK_CANCEL),
	mApp2(theApp),
	mUserList(NULL)
{
	SetHeaderFont(theApp->mFonts[FONT_VERDANA18BOLDCOLOR]);
	SetLinesFont(mApp2->mFonts[FONT_VERDANA14BOLD]);
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	SetButtonFont(mApp2->mFonts[FONT_VERDANA10BOLD]);
	// (0x496c00 here reads mUserManager->mUsers.size() and drops the result)

	mUserList = new ListWidget(USER_LIST_ID, mApp2->mFonts[FONT_VERDANA10BOLD], this);
	mUserList->SetColors(gSelectUserListColors, 5);
	mUserList->mDrawOutline = true;
	mUserList->mJustify = ListWidget::JUSTIFY_CENTER;
	UpdateList();

	if (mApp2->mGameSelector != NULL)
		mApp2->mGameSelector->SetDisabled(true);
}

// 0x432f00 (ListListener, +0x120 slot 0)
void SelectUserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if ((mUserList == NULL) || (mUserList->mId != theId))
		return;

	if (theIdx != mUserList->mSelectIdx)
	{
		mApp2->PlaySample(SOUND_BUTTONCLICK);
		mUserList->SetSelect(theIdx);
	}

	SexyString aName = mUserList->mLines[theIdx];
	if (aName == GameText(GT_USER_LIST_CREATE))
	{
		// Any click count; this dialog stays open under the new-user dialog
		mApp2->ShowNewUserDialog(GameText(GT_SELECT_USER_ENTER_NAME));
	}
	else if (aName == GameText(GT_USER_LIST_DELETE))
	{
		// Only listed with exactly five users, so the first five lines are their names
		std::vector<SexyString> aNames;
		for (int i = 0; i < MAX_LISTED_USERS; i++)
			aNames.push_back(mUserList->mLines[i]);
		mApp2->ShowDeleteUserDialog(aNames);
	}
	else if (theClickCount == 2)
	{
		// A double click switches the user and closes, without LoadUserSettings (OK does call it; the
		// settings are reloaded at the next game start anyway, NAMES D44)
		User* aUser = mApp2->mUserManager->FindUser(aName);
		if (aUser != NULL)
			mApp2->mUserManager->SetCurrentUser(aUser->mName);
		mApp2->KillDialog(mId);
	}
}
