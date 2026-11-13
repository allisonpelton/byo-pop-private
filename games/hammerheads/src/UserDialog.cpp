#include "UserDialog.h"

#include "Application.h"
#include "ListWidgetEx.h"
#include "NcButton.h"
#include "Res.h"
#include "ScrollbarWidgetEx.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The box is this wide and tall
const int DIALOG_SIZE = 0x1c2;

// The list: its height, the room around it, and how wide the scroll bar beside it is when it shows
const int LIST_HEIGHT = 0xa0;
const int LIST_MARGIN = 10;
const int SCROLLBAR_WIDTH = 0x10;

// The height of a line of the list
const int LINE_HEIGHT = 0xf;

// How many players the list shows without a scroll bar
const int PLAYERS_WITHOUT_SCROLLBAR = 10;

// How far above the box's own buttons the buttons that rename and delete sit
const int OWN_BUTTONS_RISE = -0x1e;

// The ids of the buttons the box makes (the box's own two are 1000 and 1001)
const int BUTTON_RENAME = 1;
const int BUTTON_DELETE = 2;

// 0x5927b8: the colours of the list (background, outline, text, hilite, select)
int LIST_COLORS[5][3] =
{
	{ 255, 255, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 255, 255, 40 }
};

// How see-through the shade of the chosen line is
const int SELECT_ALPHA = 0x80;

}

// 0x4622b0
UserDialog::UserDialog() :
	NcMovingDialog(DIALOG_SIZE, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_USER, true, "Who are you?", "", "", Dialog::BUTTONS_OK_CANCEL)
{
	SetHeaderFont(FONT_PTBANANASPLIT27);
	SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	mYesButton->mLabel = "Ok";
	mNoButton->mLabel = "Cancel";
	mContentInsets = Insets(0x30, 0x3c, 0x30, 0x20);

	mList = new ListWidgetEx(0, FONT_EUROFURENCE14, IMAGE_LISTBOX, this);
	mScrollbar = new ScrollbarWidgetEx(0, IMAGE_SCROLLBAR, IMAGE_SCROLLBAR_THUMB, IMAGE_SCROLLBAR_BUTTON_UP, mList);
	mScrollbar->mInvisIfNoScroll = true;
	mRenameButton = MakeNcButton(BUTTON_RENAME, this, "Rename");
	mDeleteButton = MakeNcButton(BUTTON_DELETE, this, "Delete");
	if (mYesButton != nullptr)
		SetupButtonCels(mYesButton, 3);
	if (mNoButton != nullptr)
		SetupButtonCels(mNoButton, 3);

	mList->mScrollbar = mScrollbar;
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->mItemHeight = LINE_HEIGHT;
	mList->SetColors(LIST_COLORS, 5);
	mList->mColors[ListWidget::COLOR_SELECT].mAlpha = SELECT_ALPHA;
	mList->mDrawOutline = false;

	// the first line makes a new player; the current player comes next, chosen, and the others after
	mList->AddLine("(Create a New User)", false);
	User* aCurrentUser = gApp->mUserManager->GetCurrentUser();
	if (aCurrentUser != nullptr)
		mList->SetSelect(mList->AddLine(aCurrentUser->mName, false));

	const std::vector<User*>& aUsers = gApp->mUserManager->GetUsers();
	for (std::vector<User*>::const_iterator anItr = aUsers.begin(); anItr != aUsers.end(); ++anItr)
	{
		if (aCurrentUser == nullptr || (*anItr)->mName != aCurrentUser->mName)
			mList->AddLine((*anItr)->mName, false);
	}

	if (aUsers.empty() || (int) aUsers.size() < PLAYERS_WITHOUT_SCROLLBAR)
		mScrollbar->SetVisible(false);
}

// 0x461ec0
UserDialog::~UserDialog()
{
	if (mList != nullptr)
		delete mList;
	if (mScrollbar != nullptr)
		delete mScrollbar;
	if (mRenameButton != nullptr)
		delete mRenameButton;
	if (mDeleteButton != nullptr)
		delete mDeleteButton;
}

// 0x461f60: the list and its bar fill the page, and the buttons that rename and delete sit above the box's own two
void UserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aContentX = GetContentX();
	int aContentY = GetContentY();
	int aContentWidth = GetContentWidth();
	int aScrollbarWidth = SCROLLBAR_WIDTH;
	if (!mScrollbar->mVisible)
		aScrollbarWidth = 0;

	int aListWidth = aContentWidth - 2 * LIST_MARGIN - aScrollbarWidth;
	mList->Resize(aContentX + LIST_MARGIN, aContentY, aListWidth, LIST_HEIGHT);
	mScrollbar->ResizeScrollbar(aListWidth + aContentX + LIST_MARGIN, aContentY, aScrollbarWidth, LIST_HEIGHT);
	mRenameButton->Layout(LAY_SameLeft | LAY_Above | LAY_SameSize, mYesButton, 0, OWN_BUTTONS_RISE, 0, 0);
	mDeleteButton->Layout(LAY_SameLeft | LAY_Above | LAY_SameSize, mNoButton, 0, OWN_BUTTONS_RISE, 0, 0);
}

// 0x462040
int UserDialog::GetPreferredHeight(int theWidth)
{
	return DIALOG_SIZE;
}

// 0x462050
void UserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mList);
	theWidgetManager->AddWidget(mScrollbar);
	theWidgetManager->AddWidget(mDeleteButton);
	theWidgetManager->AddWidget(mRenameButton);
}

// 0x4620a0
void UserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mList);
	theWidgetManager->RemoveWidget(mScrollbar);
	theWidgetManager->RemoveWidget(mDeleteButton);
	theWidgetManager->RemoveWidget(mRenameButton);
}

// 0x462230: the box of the list
void UserDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	g->DrawImageBox(Rect(mList->mX - mX - 4, mList->mY - mY - 4, mScrollbar->mWidth + 8 + mList->mWidth, mList->mHeight + 8), IMAGE_EDITBOX);
}

// 0x462b80: the buttons act only on a player (not on the "new user" line)
void UserDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	std::string aName = GetSelectedName();
	if (!aName.empty())
	{
		if (theId == Dialog::ID_OK || theId == Dialog::ID_CANCEL)
			CloseWith(theId);
		else if (theId == BUTTON_RENAME)
			gApp->ShowCreateAnotherUserDialog(aName);
		else if (theId == BUTTON_DELETE)
			gApp->DeleteUserDialog(aName);
	}
}

// 0x4621d0
void UserDialog::Closed()
{
	if (mCloseResult == Dialog::ID_OK)
		gApp->UserDialogClosed(true);
	else if (mCloseResult == Dialog::ID_CANCEL)
		gApp->UserDialogClosed(false);
}

// 0x462180: a click on the first line makes a new player; on another it chooses that one, and a double click picks it
void UserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if (theIdx == 0)
	{
		gApp->ReopenCreateUserDialog();
		return;
	}

	mList->SetSelect(theIdx);
	if (theClickCount == 2)
		CloseWith(Dialog::ID_OK);
}

// 0x462950
std::string UserDialog::GetSelectedName()
{
	if (0 < mList->mSelectIdx)
	{
		if (mList->mSelectIdx < mList->GetLineCount())
			return mList->GetStringAt(mList->mSelectIdx);
	}
	return "";
}

// 0x4620f0
void UserDialog::RenameSelected(const std::string& theNewName)
{
	int aSelectedIdx = mList->mSelectIdx;
	if (0 < aSelectedIdx)
		mList->SetLine(aSelectedIdx, theNewName);
}

// 0x462120
void UserDialog::AddUser(const std::string& theName)
{
	int aLineIdx = mList->AddLine(theName, false);
	mList->SetSelect(aLineIdx);
	Resize(mX, mY, mWidth, mHeight);
	if (mScrollbar->mVisible)
		mScrollbar->GoToBottom();
}

// 0x4629d0
void UserDialog::RemoveSelected(const std::string& theNameToSelect)
{
	mList->RemoveLine(mList->mSelectIdx);
	if (theNameToSelect == "")
		mList->SetSelect(0);
	else
	{
		int aLineIdx = 1;
		while (aLineIdx < mList->GetLineCount())
		{
			if (mList->GetStringAt(aLineIdx) == theNameToSelect)
			{
				mList->SetSelect(aLineIdx);
				break;
			}
			aLineIdx++;
		}
	}
	Resize(mX, mY, mWidth, mHeight);
}

// 0x462c50
UserDialog* Sexy::MakeUserDialog()
{
	UserDialog* aDialog = new UserDialog();
	aDialog->Resize((gApp->mWidth - DIALOG_SIZE) / 2, 0xe1 - gApp->mHeight / 2, DIALOG_SIZE, DIALOG_SIZE);
	gApp->AddDialog(DIALOG_USER, aDialog);
	return aDialog;
}
