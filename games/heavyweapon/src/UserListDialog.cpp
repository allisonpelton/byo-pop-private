#include "UserListDialog.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "UserNameDialog.h"
#include "YesNoDialog.h"
#include "DDImage.h"
#include "GameText.h"
#include "ImageFont.h"
#include "ListWidget.h"
#include "ScrollbarWidget.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

namespace
{
	const int DIALOG_X = 0xa5;
	const int DIALOG_Y = 0x28;
	const int DIALOG_WIDTH = 0x136;
	const int DIALOG_HEIGHT = 400;
	const int CONTENT_INSET = 0x19;
	const int BUTTON_WIDTH = 0x78;
	const int BUTTON_HEIGHT = 0x1e;
	const int ROW1_BOTTOM = 0x5f;		// the RENAME and DELETE buttons' tops are this far above the box's bottom
	const int ROW2_BOTTOM = 0x3c;		// the OK and CANCEL buttons'
	const int LIST_X = 0x1e;
	const int LIST_Y = 0x41;
	const int LIST_WIDTH_LESS = 0x3c;
	const int LIST_HEIGHT_LESS = 0xaa;
	const int SCROLLBAR_RIGHT = 0x31;
	const int SCROLLBAR_Y = 0x42;
	const int SCROLLBAR_WIDTH = 0x12;
	const int SCROLLBAR_HEIGHT_LESS = 0xac;
	const int CONFIRM_WIDTH = 0x140;
	const int CONFIRM_HEIGHT = 0xbe;
	const int NUM_LIST_COLORS = 6;

	// The list's colours: background, outline, text, highlight, selection and selected text
	const int LIST_COLORS[NUM_LIST_COLORS][3] = {
		{0, 0, 0}, {128, 128, 128}, {255, 255, 255}, {255, 255, 0}, {255, 128, 0}, {0, 0, 0}};
}

// 0x430760: the list is centred in its lines; the player in use is selected
UserListDialog::UserListDialog(HeavyWeaponApp* theApp) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, DIALOG_ID, true, GT(0x530128), "", "", BUTTONS_NONE)
{
	mApp = theApp;
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0x80, 0xc0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);
	mOkButton = new BracketButton(mApp, this, BUTTON_OK);
	mOkButton->SetTint(Color(0x40, 0xc0, 0x40));
	mOkButton->mLabel = GT(0x52d4f8);
	mCancelButton = new BracketButton(mApp, this, BUTTON_CANCEL);
	mCancelButton->SetTint(Color(0xc0, 0x40, 0x40));
	mCancelButton->mLabel = GT(0x52ccb8);
	mRenameButton = new BracketButton(mApp, this, BUTTON_RENAME);
	mRenameButton->SetTint(Color(0x40, 0x80, 0xff));
	mRenameButton->mLabel = GT(0x530120);
	mDeleteButton = new BracketButton(mApp, this, BUTTON_DELETE);
	mDeleteButton->SetTint(Color(0x40, 0x80, 0xff));
	mDeleteButton->mLabel = GT(0x530118);
	mList = new ListWidget(LIST_ID, mApp->mFonts[FONT_NORMAL], this);
	mScrollbar = new ScrollbarWidget(0, mList);
	mScrollbar->mInvisIfNoScroll = true;
	mList->mFont = mApp->mFonts[FONT_NORMAL];
	int aColors[NUM_LIST_COLORS][3];
	for (int i = 0; i < NUM_LIST_COLORS; i++)
		for (int j = 0; j < 3; j++)
			aColors[i][j] = LIST_COLORS[i][j];
	mList->SetColors(aColors, NUM_LIST_COLORS);
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->mScrollbar = mScrollbar;
	mSelectedUser = mApp->mCurrentUser;
	mWidth = DIALOG_WIDTH;
	mHeight = DIALOG_HEIGHT;
	Resize(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
}

// 0x4305b0 (body 0x430380)
UserListDialog::~UserListDialog()
{
	delete mOkButton;
	delete mCancelButton;
	delete mRenameButton;
	delete mDeleteButton;
	delete mList;
	delete mScrollbar;
}

// 0x4311d0 slot 1
void UserListDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mOkButton);
	theManager->AddWidget(mCancelButton);
	theManager->AddWidget(mRenameButton);
	theManager->AddWidget(mDeleteButton);
	theManager->AddWidget(mList);
	theManager->AddWidget(mScrollbar);
	Refresh();
}

// 0x430310 slot 2
void UserListDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mOkButton);
	theManager->RemoveWidget(mCancelButton);
	theManager->RemoveWidget(mRenameButton);
	theManager->RemoveWidget(mDeleteButton);
	theManager->RemoveWidget(mList);
	theManager->RemoveWidget(mScrollbar);
}

// 0x430440 slot 13: RENAME and DELETE over OK and CANCEL at the foot, the list and its scroll bar above them
void UserListDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mRenameButton->Resize(mWidth / 2 - 0x7d + mX, mHeight - ROW1_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mDeleteButton->Resize(mWidth / 2 + 5 + mX, mHeight - ROW1_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mOkButton->Resize(mWidth / 2 - 0x7d + mX, mHeight - ROW2_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mCancelButton->Resize(mWidth / 2 + 5 + mX, mHeight - ROW2_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mList->Resize(mX + LIST_X, mY + LIST_Y, mWidth - LIST_WIDTH_LESS, mHeight - LIST_HEIGHT_LESS);
	mScrollbar->ResizeScrollbar(mWidth - SCROLLBAR_RIGHT + mX, mY + SCROLLBAR_Y, SCROLLBAR_WIDTH,
		mHeight - SCROLLBAR_HEIGHT_LESS);
}

// 0x430590 listener slot 4
void UserListDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x430c80: the list: "[Create a New User]" and the players, the one in use selected and scrolled to
void UserListDialog::Refresh()
{
	mList->RemoveAll();
	mList->AddLine(GT(0x530138), false);
	for (size_t i = 0; i < mApp->mUsers.size(); i++)
		mList->AddLine(std::string(mApp->mUsers[i].mName), false);
	mList->SetSelect(mApp->mCurrentUser + 1);
	mScrollbar->SetValue((double) (mApp->mCurrentUser + 1));
	mList->MarkDirty();
}

// 0x4305e0 list listener slot 0: a click selects a line; a double click chooses the player; the first line opens the box for
// a new player (and with no player at all, whatever is clicked)
void UserListDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if (mApp->mUsers.empty())
		theIdx = mSelectedUser + 1;
	if (theIdx != 0)
	{
		mList->SetSelect(theIdx);
		mSelectedUser = theIdx - 1;
		if (theClickCount == 2)
		{
			mApp->mCurrentUser = theIdx - 1;
			mApp->KillDialog(DIALOG_ID);
		}
		return;
	}
	mApp->AddDialog(UserNameDialog::DIALOG_ID, new UserNameDialog(mApp, false, 0));
}

// 0x430e40 listener slot 5: OK chooses the selected line's player (or asks for a new one); CANCEL closes; RENAME and DELETE
// work on the selected player; the delete box's YES removes the player
void UserListDialog::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	switch (theId)
	{
	case BUTTON_OK:
		{
			int aSelected = mList->mSelectIdx;
			if (aSelected == 0)
			{
				mApp->AddDialog(UserNameDialog::DIALOG_ID, new UserNameDialog(mApp, false, 0));
				mApp->SaveUsers();
				return;
			}
			mApp->mCurrentUser = aSelected - 1;
			mApp->KillDialog(DIALOG_ID);
			mApp->SaveUsers();
		}
		return;
	case BUTTON_CANCEL:
		mApp->KillDialog(DIALOG_ID);
		return;
	case BUTTON_RENAME:
		mApp->AddDialog(UserNameDialog::DIALOG_ID, new UserNameDialog(mApp, true, mSelectedUser));
		return;
	case BUTTON_DELETE:
		mApp->AddDialog(YesNoDialog::DIALOG_ID, new YesNoDialog(mApp, this, GT(0x53014c), GT(0x53015c), CONFIRM_WIDTH,
			CONFIRM_HEIGHT, 0, 0));
		return;
	case BUTTON_DELETE_YES:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		mApp->mUsers.erase(mApp->mUsers.begin() + mSelectedUser);
		if (mApp->mUsers.size() <= (size_t) mApp->mCurrentUser)
		{
			mApp->mCurrentUser = mApp->mCurrentUser - 1;
			if (mSelectedUser > 0)
				mSelectedUser = mSelectedUser - 1;
		}
		mApp->SaveUsers();
		Refresh();
		if (!mApp->mUsers.empty())
			return;
		mApp->AddDialog(UserNameDialog::DIALOG_ID, new UserNameDialog(mApp, false, 0));
		mApp->KillDialog(DIALOG_ID);
		return;
	case BUTTON_DELETE_NO:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		return;
	}
}
