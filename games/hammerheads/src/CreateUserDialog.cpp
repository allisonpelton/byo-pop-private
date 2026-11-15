#include "CreateUserDialog.h"

#include <cctype>

#include "Application.h"
#include "EditWidget.h"
#include "NcButton.h"
#include "Res.h"
#include "UserDialog.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The box is this tall (and 0x1c2 wide)
const int DIALOG_HEIGHT = 300;
const int DIALOG_WIDTH = 0x1c2;

// Where the name is typed, on the page of the box: in from the sides, down, and as tall as a line
const int EDIT_INSET = 0x20;
const int EDIT_TOP = 0x1e;
const int EDIT_HEIGHT = 0x18;

// How wide the name may get
const int EDIT_MAX_PIXELS = 0x99;

// The picture box around the name reaches this far out of it
const int EDIT_BOX_OUTSET = 7;
const int EDIT_BOX_EXTRA = 0xb;

// The id of the box that answers a name that is taken when a player is renamed (the application closes it like the others)
const int DIALOG_NAME_CONFLICT_RENAME = 10;

// 0x41cf10: the look of the name's text: a dark green behind it (which is not shown), black text, and white where it is chosen
void SetEditColors(EditWidget* theEditWidget)
{
	theEditWidget->SetColor(EditWidget::COLOR_BKG, Color(0x294918));
	theEditWidget->SetColor(EditWidget::COLOR_OUTLINE, Color(0));
	theEditWidget->SetColor(EditWidget::COLOR_TEXT, Color(0, 0, 0, 255));
	theEditWidget->SetColor(EditWidget::COLOR_HILITE, Color(0, 0, 0, 255));
	theEditWidget->SetColor(EditWidget::COLOR_HILITE_TEXT, Color(255, 255, 255, 255));
}

// What a name that is already taken is told
void ShowNameConflict(int theDialogId)
{
	gApp->DoDialog(theDialogId, true, "Name conflict", "The name you entered is already being used.\nPlease enter a unique player name.",
		"OK", Dialog::BUTTONS_FOOTER);
}

// What a box that is left without a name is told
void ShowNameNeeded()
{
	gApp->DoDialog(DIALOG_ERROR, true, "Enter your name",
		"Enter your name to create a new user profile for storing high score data and games in progress.", "OK", Dialog::BUTTONS_FOOTER);
}

}

// 0x44e440
CreateUserDialog::CreateUserDialog(bool theRename) :
	NcMovingDialog(DIALOG_HEIGHT, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, (theRename ? 1 : 0) + DIALOG_CREATE_USER, true,
		theRename ? "Rename user" : "New user", "Please enter your name.", "", Dialog::BUTTONS_OK_CANCEL)
{
	SetHeaderFont(FONT_PTBANANASPLIT27);
	SetLinesFont(FONT_EUROFURENCE14);
	SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	SetColor(Dialog::COLOR_LINES, Color(0, 0, 0, 255));
	mYesButton->mLabel = "Ok";
	mNoButton->mLabel = "Cancel";

	mEditWidget = new EditWidget(0, this);
	mEditWidget->SetFont(FONT_EUROFURENCE14BOLD, nullptr);
	mEditWidget->mMaxPixels = EDIT_MAX_PIXELS;
	mContentInsets = Insets(0x30, 0x3c, 0x30, 0x20);
	SetEditColors(mEditWidget);
	mEditWidget->mColors[EditWidget::COLOR_BKG].mAlpha = 0;
	mEditWidget->mColors[EditWidget::COLOR_OUTLINE].mAlpha = 0;

	if (mYesButton != nullptr)
		SetupButtonCels(mYesButton, 3);
	if (mNoButton != nullptr)
		SetupButtonCels(mNoButton, 3);
}

// 0x44dff0
CreateUserDialog::~CreateUserDialog()
{
	if (mEditWidget != nullptr)
		delete mEditWidget;
}

// 0x44e080: the name sits in the page
void CreateUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aContentX = GetContentX();
	int aContentY = GetContentY();
	int aContentWidth = GetContentWidth();
	mEditWidget->Resize(aContentX + EDIT_INSET, aContentY + EDIT_TOP, aContentWidth - 2 * EDIT_INSET, EDIT_HEIGHT);
}

// 0x44e110
int CreateUserDialog::GetPreferredHeight(int theWidth)
{
	return DIALOG_HEIGHT;
}

// 0x44e120: the name has the keyboard
void CreateUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEditWidget);
	theWidgetManager->SetFocus(mEditWidget);
}

// 0x44e170
void CreateUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEditWidget);
}

// 0x44e240: the keyboard goes back to the name unless a box that complains of it is up
void CreateUserDialog::Update()
{
	if (!mEditWidget->mHasFocus)
	{
		if (gApp->GetDialog(DIALOG_ERROR) == nullptr)
		{
			if (gApp->GetDialog(DIALOG_NAME_CONFLICT_RENAME) == nullptr)
				gApp->mWidgetManager->SetFocus(mEditWidget);
		}
	}
}

// 0x44e970: the picture box around the name
void CreateUserDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	g->DrawImageBox(Rect(mEditWidget->mX - mX - EDIT_BOX_OUTSET, mEditWidget->mY - mY - EDIT_BOX_OUTSET,
		mEditWidget->mWidth + EDIT_BOX_EXTRA, mEditWidget->mHeight + EDIT_BOX_EXTRA), IMAGE_EDITBOX);
}

// 0x44ea80: the first player cannot be left out; a name that is empty or taken is complained of, and the box stays
void CreateUserDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	if (mId == DIALOG_CREATE_USER)
	{
		UserDialog* aUserDialog = static_cast<UserDialog*>(gApp->GetDialog(DIALOG_USER));
		bool aCancelled = theId != Dialog::ID_OK;
		std::string aName = GetName();
		if (aCancelled || !aName.empty())
		{
			if (gApp->mUserManager->GetCurrentUser() != nullptr || !aCancelled)
			{
				if (aCancelled)
				{
					CloseWith(theId);
					return;
				}

				// the new player becomes the current one, and the one that was stays so
				int aCurrentUserId = -1;
				User* aCurrentUser = gApp->mUserManager->GetCurrentUser();
				if (aCurrentUser != nullptr)
					aCurrentUserId = aCurrentUser->mId;
				if (gApp->mUserManager->CreateUser(aName) != nullptr)
				{
					if (aCurrentUserId != -1)
						gApp->mUserManager->SetCurrentUser(aCurrentUserId);
					gApp->mUserManager->SaveRoster();
					if (aUserDialog != nullptr)
						aUserDialog->AddUser(aName);
					CloseWith(theId);
					return;
				}
				ShowNameConflict(DIALOG_ERROR);
				return;
			}
			ShowNameNeeded();
		}
		else
			ShowNameNeeded();
	}
	else if (mId == DIALOG_CREATE_ANOTHER_USER)
	{
		UserDialog* aUserDialog = static_cast<UserDialog*>(gApp->GetDialog(DIALOG_USER));
		CreateUserDialog* aRenameDialog = static_cast<CreateUserDialog*>(gApp->GetDialog(DIALOG_CREATE_ANOTHER_USER));
		if (aUserDialog == nullptr)
			return;
		if (aRenameDialog == nullptr)
			return;

		std::string aSelectedName = aUserDialog->GetSelectedName();
		std::string aNewName = aRenameDialog->GetName();
		if (theId == Dialog::ID_OK)
		{
			if (aNewName.empty())
			{
				gApp->DoDialog(DIALOG_ERROR, true, "Enter a new name", "Enter a new name for your existing user profile.", "OK",
					Dialog::BUTTONS_FOOTER);
			}
			else if (gApp->mUserManager->RenameUser(aSelectedName, aNewName))
			{
				gApp->mUserManager->SaveRoster();
				aUserDialog->RenameSelected(aNewName);
				CloseWith(theId);
			}
			else
				ShowNameConflict(DIALOG_NAME_CONFLICT_RENAME);
		}
		else
			CloseWith(theId);
	}
}

// 0x44e1b0
void CreateUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	ButtonDepress(Dialog::ID_OK);
}

// 0x44e1d0
bool CreateUserDialog::AllowChar(int theId, SexyChar theChar)
{
	if (isalnum((unsigned char) theChar) != 0)
		return true;
	return theChar == ' ';
}

// 0x44e300: a space is kept only after something that is not one, and the last one goes
std::string CreateUserDialog::GetName()
{
	std::string aName;
	int aCharIdx = 0;
	char aPreviousChar = ' ';
	if (0 < (int) mEditWidget->mString.length())
	{
		do
		{
			char aChar = mEditWidget->mString[aCharIdx];
			if (aChar != ' ' || aPreviousChar != ' ')
				aName += aChar;
			aCharIdx++;
			aPreviousChar = aChar;
		}
		while (aCharIdx < (int) mEditWidget->mString.length());

		if (!aName.empty())
		{
			if (aName[aName.length() - 1] == ' ')
				aName.erase(aName.length() - 1);
		}
	}
	return aName;
}

// 0x44e2c0
void CreateUserDialog::SetName(const std::string& theName)
{
	mEditWidget->SetText(theName, true);
	mEditWidget->mCursorPos = (int) theName.length();
	mEditWidget->mHilitePos = 0;
}

// 0x44e750
CreateUserDialog* Sexy::MakeCreateUserDialog(bool theRename)
{
	CreateUserDialog* aDialog = new CreateUserDialog(theRename);
	aDialog->Resize((gApp->mWidth - DIALOG_WIDTH) / 2, 0x96 - gApp->mHeight / 2, DIALOG_WIDTH, DIALOG_HEIGHT);
	gApp->AddDialog((theRename ? 1 : 0) + DIALOG_CREATE_USER, aDialog);
	return aDialog;
}
