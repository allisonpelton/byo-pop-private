#include "UserNameDialog.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "MessageDialog.h"
#include "UserListDialog.h"
#include "EditWidget.h"
#include "ListWidget.h"
#include "DDImage.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"
#include <cstring>

using namespace Sexy;

namespace
{
	const int DIALOG_X = 0x9b;
	const int DIALOG_Y = 0x91;
	const int DIALOG_WIDTH = 0x14a;
	const int DIALOG_HEIGHT = 0xbe;
	const int CONTENT_INSET = 0x19;
	const int BUTTON_WIDTH = 0x78;
	const int BUTTON_HEIGHT = 0x1e;
	const int BUTTON_BOTTOM = 0x37;
	const int EDIT_INSET = 0x28;
	const int EDIT_BOTTOM = 0x5f;
	const int EDIT_HEIGHT = 0x1e;
	const int EDIT_WIDTH_PIXELS = 0xe6;			// the name's width in the large font may not pass this
	const int MESSAGE_WIDTH = 0x140;
	const int MESSAGE_HEIGHT = 200;
	const int NUM_EDIT_COLORS = 5;

	// The edit box's colours: background, outline, text, selection and selected text
	const int EDIT_COLORS[NUM_EDIT_COLORS][3] = {
		{0, 0, 0}, {128, 128, 128}, {255, 255, 255}, {255, 128, 0}, {0, 0, 0}};

	std::string Capitals(const std::string& theString)
	{
		std::string aResult = theString;
		for (size_t i = 0; i < aResult.size(); i++)
			aResult[i] = (char) toupper((unsigned char) aResult[i]);
		return aResult;
	}

	std::string Trimmed(const std::string& theString)
	{
		size_t aStart = theString.find_first_not_of(" \t\r\n");
		if (aStart == std::string::npos)
			return "";
		size_t anEnd = theString.find_last_not_of(" \t\r\n");
		return theString.substr(aStart, anEnd - aStart + 1);
	}
}

// 0x428e60: the box starts with the name edit focused; a rename starts with the old name in it
UserNameDialog::UserNameDialog(HeavyWeaponApp* theApp, bool isRename, int theUserIndex) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, DIALOG_ID, true, isRename ? GT(0x52e760) : GT(0x52e104),
		GT(0x52e76c), "", BUTTONS_NONE)
{
	mApp = theApp;
	mRename = isRename;
	mUserIndex = theUserIndex;
	mFirstUser = mApp->mUsers.empty();
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0xc0, 0xc0, 0xc0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);
	mOkButton = new BracketButton(mApp, this, BUTTON_OK);
	mOkButton->SetTint(Color(0x40, 0xc0, 0x40));
	mOkButton->mLabel = GT(0x52d4f8);
	mCancelButton = new BracketButton(mApp, this, BUTTON_CANCEL);
	mCancelButton->SetTint(Color(0xc0, 0x40, 0x40));
	mCancelButton->mLabel = GT(0x52e758);
	mEdit = new EditWidget(EDIT_ID, this);
	mEdit->SetFont(mApp->mFonts[FONT_NORMAL], NULL);
	mEdit->AddWidthCheckFont(mApp->mFonts[FONT_RUBBERSTAMPLET20], EDIT_WIDTH_PIXELS);
	mEdit->mMaxChars = MAX_NAME_LENGTH;
	int aColors[NUM_EDIT_COLORS][3];
	for (int i = 0; i < NUM_EDIT_COLORS; i++)
		for (int j = 0; j < 3; j++)
			aColors[i][j] = EDIT_COLORS[i][j];
	mEdit->SetColors(aColors, NUM_EDIT_COLORS);
	if (mRename)
		mEdit->SetText(mApp->mUsers[mUserIndex].mName, true);
	mWidth = DIALOG_WIDTH;
	mHeight = DIALOG_HEIGHT;
	Resize(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
}

// 0x428e30 (body 0x428d20)
UserNameDialog::~UserNameDialog()
{
	delete mOkButton;
	delete mCancelButton;
	delete mEdit;
}

// 0x428bf0 slot 1
void UserNameDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mOkButton);
	if (!mFirstUser)
		theManager->AddWidget(mCancelButton);
	theManager->AddWidget(mEdit);
	theManager->SetFocus(mEdit);
}

// 0x428db0 slot 2
void UserNameDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mOkButton);
	if (!mFirstUser)
		theManager->RemoveWidget(mCancelButton);
	theManager->RemoveWidget(mEdit);
}

// 0x428c50 slot 13: OK and Cancel side by side at the foot (OK alone in the middle for a first player), the edit above them
void UserNameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	Rect aRect = GetInsetRect();
	int aButtonY = mHeight - BUTTON_BOTTOM + mY;
	if (!mFirstUser)
	{
		mOkButton->Resize(aRect.mWidth / 2 - 0x7d + mX, aButtonY, BUTTON_WIDTH, BUTTON_HEIGHT);
		mCancelButton->Resize(aRect.mWidth / 2 + 5 + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	}
	else
		mOkButton->Resize(aRect.mWidth / 2 - 0x3c + mX, aButtonY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mEdit->Resize(aRect.mX + EDIT_INSET, mHeight - EDIT_BOTTOM + mY, aRect.mWidth - 2 * EDIT_INSET, EDIT_HEIGHT);
}

// 0x42ca70 listener slot 4
void UserNameDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x4298f0 listener slot 5: OK takes the name; Cancel closes the box
void UserNameDialog::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	if (theId == BUTTON_OK)
		Accept();
	else if (theId == BUTTON_CANCEL)
		mApp->KillDialog(DIALOG_ID);
}

// 0x429940 edit listener slot 0: Enter takes the name too
void UserNameDialog::EditWidgetText(int theId, const SexyString& theString)
{
	Accept();
}

// 0x428e00 edit listener slot 2: letters, digits and spaces
bool UserNameDialog::AllowChar(int theId, SexyChar theChar)
{
	if (((theChar < 'a') || (theChar > 'z')) && ((theChar < 'A') || (theChar > 'Z')) && ((theChar < '0') || (theChar > '9')) &&
		(theChar != ' '))
		return false;
	return true;
}

// 0x429300: a box says why a name is refused; otherwise a new player is added and chosen (the list of players, if open,
// closes), or the player is renamed (the list shows it); either way this box closes
void UserNameDialog::Accept()
{
	std::string aName = Trimmed(mEdit->mString);
	if (aName.empty())
	{
		MessageDialog* aMessage = new MessageDialog(mApp, MESSAGE_WIDTH, MESSAGE_HEIGHT, GT(0x52e7cc), GT(0x52e7dc));
		mApp->AddDialog(MessageDialog::DIALOG_ID, aMessage);
		aMessage->WaitForResult(false);
		mApp->mWidgetManager->SetFocus(mEdit);
		return;
	}
	for (size_t i = 0; i < mApp->mUsers.size(); i++)
	{
		if ((i != (size_t) mUserIndex) || !mRename)
		{
			if (Capitals(mApp->mUsers[i].mName) == Capitals(aName))
			{
				MessageDialog* aMessage = new MessageDialog(mApp, MESSAGE_WIDTH, MESSAGE_HEIGHT, GT(0x52e784), GT(0x52e794));
				mApp->AddDialog(MessageDialog::DIALOG_ID, aMessage);
				aMessage->WaitForResult(false);
				mApp->mWidgetManager->SetFocus(mEdit);
				return;
			}
		}
	}
	if (!mRename)
	{
		UserRecord aRecord;
		memset(&aRecord, 0, sizeof(aRecord));
		strcpy(aRecord.mName, aName.c_str());
		mApp->mUsers.push_back(aRecord);
		mApp->mCurrentUser = (int) mApp->mUsers.size() - 1;
		mApp->mIsNewUser = true;
		mApp->KillDialog(HeavyWeaponApp::DIALOG_USER_LIST);
		mApp->SaveUsers();
	}
	else
	{
		strcpy(mApp->mUsers[mUserIndex].mName, aName.c_str());
		UserListDialog* aList = (UserListDialog*) mApp->GetDialog(HeavyWeaponApp::DIALOG_USER_LIST);
		aList->mSelectedUser = mUserIndex;
		aList->Refresh();
		aList->mList->SetSelect(mUserIndex + 1);
	}
	mApp->KillDialog(DIALOG_ID);
}
