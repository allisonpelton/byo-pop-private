#include "UserDialogs.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "GameText.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "Checkbox.h"
#include "ListWidget.h"
#include "ScrollbarWidget.h"
#include "WidgetManager.h"
#include "Insets.h"
#include <ctype.h>

using namespace Sexy;

static const int NAME_MAX_CHARS = 0x1c;
static const int NAME_MAX_PIXELS = 0x96;

// 0x53d440: the list's colours: background, outline, text, hilited text and selection fill
static int gUserListColors[5][3] =
{
	{147, 97, 60},
	{0, 0, 0},
	{206, 227, 33},
	{255, 255, 255},
	{155, 176, 142}
};

// 0x401af0
NewUserDialog::NewUserDialog(PixelusApp* theApp, bool theRename) :
	GameDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS],
		theRename ? 0x15 : 0x13, true, theRename ? GT(0x50b4b0) : GT(0x50b4a4), GT(0x50b4bc), "", BUTTONS_OK_CANCEL, NULL)
{
	mRename = theRename;

	mEdit = new EditWidget(0, this);
	mEdit->SetFont(theApp->mFonts[FONT_GARAMOND14], NULL);
	mEdit->mMaxChars = NAME_MAX_CHARS;
	mEdit->mMaxPixels = NAME_MAX_PIXELS;
	SetDialogColors(mEdit);

	if (!theRename)
		mExpertBox = new Checkbox(theApp->mImages[IMAGE_CHECKBOX_UNCHECKED], theApp->mImages[IMAGE_CHECKBOX_CHECKED], 1, this);
	else
		mExpertBox = NULL;
}

NewUserDialog::~NewUserDialog()
{
	delete mEdit;
	delete mExpertBox;
}

// 0x4011a0
void NewUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	GameDialog::AddedToManager(theWidgetManager);
	if (mExpertBox != NULL)
		theWidgetManager->AddWidget(mExpertBox);
	theWidgetManager->AddWidget(mEdit);
	theWidgetManager->SetFocus(mEdit);
}

// 0x4011f0
void NewUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	GameDialog::RemovedFromManager(theWidgetManager);
	if (mExpertBox != NULL)
		theWidgetManager->RemoveWidget(mExpertBox);
	theWidgetManager->RemoveWidget(mEdit);
}

// 0x4010c0
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	int aLeft = GetContentLeft();
	int aTop = GetContentTop();
	int aWidth = GetContentWidth();
	mEdit->Resize(aLeft + 0x20, aTop + 0x14, aWidth - 0x40, 0x18);
	if (mExpertBox != NULL)
	{
		Image* anImage = mApp->mImages[IMAGE_CHECKBOX_CHECKED];
		mExpertBox->Resize(aLeft + 0x34, aTop + 0x3e, anImage->GetWidth(), anImage->GetHeight());
	}
}

// 0x401f20
void NewUserDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);

	Rect aFrame(mEdit->mX - mX - 7, mEdit->mY - mY - 7, mEdit->mWidth + 0xb, mEdit->mHeight + 0xb);
	g->DrawImageBox(aFrame, mApp->mImages[IMAGE_EDITBOX]);

	g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
	g->SetColor(Color(255, 255, 255));
	if (mExpertBox != NULL)
		g->DrawString(GT(0x50b55c), mExpertBox->mX - mX + 0x28, mExpertBox->mY - mY + 0x14);
}

// 0x401170: room for the edit box, and for the check box
int NewUserDialog::GetPreferredHeight(int theWidth)
{
	return GameDialog::GetPreferredHeight(theWidth) + 0x28 + (mExpertBox != NULL ? 0x28 : 0);
}

// 0x401230: Enter in the box answers like the OK button
void NewUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp->ButtonDepress(mId + 2000);
}

// 0x401250: letters, digits and spaces
bool NewUserDialog::AllowChar(int theId, SexyChar theChar)
{
	unsigned char aChar = (unsigned char) theChar;
	return isalnum(aChar) || (aChar == ' ');
}

// 0x401df0: a warning when expert mode is switched on
void NewUserDialog::CheckboxChecked(int theId, bool theChecked)
{
	if (theChecked)
	{
		Dialog* aDialog = mApp->DoDialog(0x1e, true, GT(0x50b4d8), GT(0x50b4e8), GT(0x50b558), BUTTONS_FOOTER);
		mApp->CenterDialog(aDialog);
	}
}

// 0x401980: leading spaces and double spaces are dropped, and a trailing one
std::string NewUserDialog::GetName() const
{
	std::string aResult;
	char aPrevious = ' ';
	for (size_t i = 0; i < mEdit->mString.size(); i++)
	{
		char aChar = mEdit->mString[i];
		if ((aChar != ' ') || (aPrevious != ' '))
			aResult += aChar;
		aPrevious = aChar;
	}
	if (!aResult.empty() && aResult[aResult.size() - 1] == ' ')
		aResult.erase(aResult.size() - 1);
	return aResult;
}

// 0x401300
void NewUserDialog::SetName(const std::string& theName)
{
	mEdit->SetText(theName);
}

// 0x401290
bool NewUserDialog::IsExpert() const
{
	return (mExpertBox != NULL) && mExpertBox->IsChecked();
}

// 0x41d710
SelectUserDialog::SelectUserDialog(PixelusApp* theApp) :
	GameDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], 0x12, true,
		GT(0x50ef40), "", "", BUTTONS_OK_CANCEL, NULL)
{
	mList = new ListWidget(LIST_ID, theApp->mFonts[FONT_GARAMOND14], this);
	mList->SetColors(gUserListColors, 5);
	mList->mItemHeight = LINE_HEIGHT;
	mList->mDrawOutline = false;
	mList->mJustify = ListWidget::JUSTIFY_CENTER;

	mRenameButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_RENAME, this);
	mRenameButton->mLabel = GT(0x50ef38);
	mDeleteButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_DELETE, this);
	mDeleteButton->mLabel = GT(0x50dbe4);
	StyleButton(theApp, mRenameButton, 3);
	StyleButton(theApp, mDeleteButton, 3);

	// the line that makes a new player, then the current player (selected), then the others
	UserProfile* aCurrent = theApp->mUserDirectory->GetCurrent();
	FillList(aCurrent != NULL ? aCurrent->mName : "");
}

SelectUserDialog::~SelectUserDialog()
{
	delete mList;
	delete mRenameButton;
	delete mDeleteButton;
}

void SelectUserDialog::FillList(const std::string& theSelect)
{
	mList->mLines.clear();
	mList->mLineColors.clear();
	mList->AddLine(GT(0x50ef24), false);
	UserDirectory* aDirectory = mApp->mUserDirectory;
	int aSelect = 0;
	if (!theSelect.empty())
		aSelect = mList->AddLine(theSelect, false);
	for (size_t i = 0; i < aDirectory->mUsers.size(); i++)
	{
		const std::string& aName = aDirectory->mUsers[i]->mName;
		if (theSelect.empty() || (aName != theSelect))
			mList->AddLine(aName, false);
	}
	mList->SetSelect(aSelect);
	if (mList->mScrollbar != NULL)
		mList->mScrollbar->SetVisible((int) mList->mLines.size() > VISIBLE_LINES);
	MarkDirty();
}

// 0x41d560
void SelectUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	GameDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mList);
	theWidgetManager->AddWidget(mRenameButton);
	theWidgetManager->AddWidget(mDeleteButton);
}

// 0x41d5b0
void SelectUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	GameDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mList);
	theWidgetManager->RemoveWidget(mRenameButton);
	theWidgetManager->RemoveWidget(mDeleteButton);
}

// 0x41d460: the list on top, the rename and delete buttons right above OK and Cancel
void SelectUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	int aLeft = GetContentLeft();
	int aTop = GetContentTop();
	int aWidth = GetContentWidth();
	mList->Resize(aLeft + 10, aTop, aWidth - 0x14, LINE_HEIGHT * VISIBLE_LINES + 8);
	if (mYesButton != NULL)
		mRenameButton->Resize(mYesButton->mX, mYesButton->mY - mYesButton->mHeight, mYesButton->mWidth, mYesButton->mHeight);
	if (mNoButton != NULL)
		mDeleteButton->Resize(mNoButton->mX, mNoButton->mY - mNoButton->mHeight, mNoButton->mWidth, mNoButton->mHeight);
}

// 0x41d690: the frame of the list
void SelectUserDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	Rect aFrame(mList->mX - mX - 4, mList->mY - mY - 4, mList->mWidth + 8, mList->mHeight + 8);
	g->DrawImageBox(aFrame, mApp->mImages[IMAGE_EDITBOX]);
}

// 0x41d540
int SelectUserDialog::GetPreferredHeight(int theWidth)
{
	return GameDialog::GetPreferredHeight(theWidth) + 0xbe;
}

// 0x41dbe0
std::string SelectUserDialog::GetSelectedName() const
{
	if ((mList->mSelectIdx > 0) && (mList->mSelectIdx < (int) mList->mLines.size()))
		return mList->mLines[mList->mSelectIdx];
	return "";
}

// 0x41d600
void SelectUserDialog::RenameSelected(const std::string& theName)
{
	if (mList->mSelectIdx > 0)
		mList->SetLine(mList->mSelectIdx, theName);
}

// 0x41de10: rename and delete work on the selected player
void SelectUserDialog::ButtonDepress(int theId)
{
	if ((theId != ID_RENAME) && (theId != ID_DELETE))
	{
		GameDialog::ButtonDepress(theId);
		return;
	}

	std::string aName = GetSelectedName();
	if (aName.empty())
		return;
	if (theId == ID_RENAME)
		mApp->ShowRenameDialog(aName);
	else
		mApp->ShowDeleteQuestion(aName);
}

// 0x41d630: the first line makes a new player; a double click on a name is the OK button
void SelectUserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if (theIdx == 0)
	{
		mApp->ShowNewUser();
		return;
	}
	mList->SetSelect(theIdx);
	if (theClickCount == 2)
		mApp->SelectUserOK(true);
}
