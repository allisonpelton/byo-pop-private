// ListDialog, UserSelectDialog and DeleteUserDialog: the boxes with the list of the players (see ListDialog.h).

#include "ListDialog.h"
#include "AstroPopApp.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "ListWidget.h"
#include "MemoryImage.h"
#include "Res.h"
#include "AstroScrollbar.h"
#include "UserProfile.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// 0x59e100: the colours of the list (background, outline, text, hilite, selection, selected text)
int gListColors[6][3] =
{
	{ 23, 37, 48 },
	{ 0, 0, 0 },
	{ 72, 153, 144 },
	{ 255, 255, 255 },
	{ 190, 0, 80 },
	{ 255, 255, 0 }
};

}

// 0x486570: the box is of the colour set of the selector (1), like the box of the new player. The list is eight lines
// high, its text the small font; the scroll bar stands beside it.
ListDialog::ListDialog(AstroPopApp* theApp, int theId, const std::string& theHeader, const std::string& theLines) :
	Text2004<Dialog>(IMAGE_DIALOG, IMAGE_DIALOG_BTN, theId, true, theHeader, theLines, std::string(), 2)
{
	mApp = theApp;
	m11c = 0xf;

	mList = new ListWidget(0, GetFontById(0xb0), this);
	mList->mDrawOutline = true;
	mList->mFont = GetFontById(0xb0);
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->SetColors(gListColors, 6);

	mScrollbar = new AstroScrollbar(0x38d, mList);
	mScrollbar->mHorizontal = false;
	mList->mScrollbar = mScrollbar;

	MakeButtonImages();
}

// the picture of the box and the one of the buttons, in the colours of set 1, and the three pictures' rectangles of the
// buttons (the same as in the new player box)
void ListDialog::MakeButtonImages()
{
	MemoryImage* anInsetImage = NULL;
	MemoryImage* aButtonImage = NULL;
	gApp->MakeDialogImages(gApp->GetDialogInsetColor(1), gApp->GetDialogColor(1), &anInsetImage, &aButtonImage, false);
	mComponentImage = anInsetImage;
	// the 2004 Dialog reads the button height from the button picture at each use (GetPreferredHeight 0x4e91d0 and Resize); the
	// port keeps the height it took at construction, so it follows the picture that replaces it
	mYesButton->mComponentImage = aButtonImage;
	mButtonHeight = aButtonImage->mHeight;
	mYesButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
	mYesButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
	mYesButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
	if (mNoButton != NULL)
	{
		mNoButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
		mNoButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
		mNoButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
		mNoButton->mComponentImage = aButtonImage;
	}
}

// 0x484610 (the body of the base class's deleting destructor): the scroll bar, the list, the box's picture and the buttons' picture are the box's own
ListDialog::~ListDialog()
{
	delete mScrollbar;
	delete mList;
	delete mComponentImage;
	mComponentImage = NULL;
	if (mYesButton != NULL)
		delete mYesButton->mComponentImage;
}

// 0x482f50: the list and its scroll bar join the box, the first name is selected
void ListDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog::AddedToManager(theManager);
	theManager->AddWidget(mList);
	theManager->AddWidget(mScrollbar);
	theManager->BringToFront(mScrollbar);
	mList->SetSelect(0);
}

// 0x482fa0
void ListDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mList);
	theManager->RemoveWidget(mScrollbar);
}

// 0x482d90: the box's own height, and the list (eight lines of its font) with 0x32 for its margins
int ListDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog::GetPreferredHeight(theWidth);
	RebuildList();
	if (mList != NULL)
		return aHeight + 0x32 + mList->mFont->GetHeight() * 8;
	return aHeight;
}

// 0x482dd0: the list stands under the heading, the scroll bar at its right, the box's buttons side by side at the foot
void ListDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	RebuildList();

	int aListHeight = mList->mFont->GetHeight() * 8 + 9;
	int aListY = m11c + mSpaceAfterHeader + mContentInsets.mTop + theY;
	mList->Resize(mX + 0x28, aListY, mWidth - 0x69, aListHeight);

	// port: the original's x87 rounds every result to a float (docs/GAME_NOTES.md, "The original's x87 rounds every result to a
	// float"), so the page size the list works out ((height - 8) / item height, 105 / 13 here) and gives the scroll bar is a
	// float's worth. The list at its last position, where its top is the maximum less the page size, then has its rows on
	// whole pixels with the value's error on the side that the truncation drops; with the page size at full precision the
	// error lies on the other side, and the lines of a scrolled list came one pixel higher in three rows out of eight.
	mList->mPageSize = (double) (float) mList->mPageSize;
	mScrollbar->SetPageSize(mList->mPageSize);
	mScrollbar->ResizeScrollbar(mWidth - 0x41 + mX, aListY, 0x19, aListHeight);

	if (mYesButton != NULL)
	{
		if (mNoButton != NULL)
		{
			int aButtonWidth = (((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - 0x30) / 2;
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mContentInsets.mLeft + 0x10 + mX, ((mHeight - mContentInsets.mBottom) - aButtonHeight) + mY,
							   aButtonWidth, aButtonHeight);
			mNoButton->Resize(mContentInsets.mLeft + mX + 0x20 + aButtonWidth,
							  ((mHeight - mContentInsets.mBottom) - aButtonHeight) + mY, aButtonWidth, aButtonHeight);
		}
		else
		{
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + mContentInsets.mLeft, ((mHeight - mContentInsets.mBottom) + mY) - aButtonHeight,
							   (mWidth - mContentInsets.mRight) - mContentInsets.mLeft, aButtonHeight);
		}
	}
}

// 0x483e50
void ListDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	Rect aBox(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, (mWidth - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft,
			  (mHeight - mBackgroundInsets.mBottom) - mBackgroundInsets.mTop);
	g->DrawImageBox(aBox, IMAGE_DIALOG);
}

// 0x482fe0: a button going down clicks, whatever it is
void ListDialog::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_BUTTON_DOWN);
}

// 0x4868c0: the OK takes the name that is selected (if one is), the Cancel leaves no name; the box then answers as any
// other (to the application)
void ListDialog::ButtonDepress(int theId)
{
	if (theId == ID_YES)
	{
		int anIndex = mList->mSelectIdx;
		if (anIndex >= 0 && anIndex < (int) mList->mLines.size())
			mSelectedName = mList->mLines[anIndex];
	}
	else if (theId == ID_NO)
		mSelectedName = "";
	Dialog::ButtonDepress(theId);
}

// 0x486a00: a click on a name selects it (with a click's sound when it was another one); two clicks are the OK
void ListDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if (mList->mId != theId)
		return;
	if (theIdx != mList->mSelectIdx)
	{
		mApp->PlaySample(SOUND_BUTTON_DOWN);
		mList->SetSelect(theIdx);
	}
	mSelectedName = mList->mLines[theIdx];
	if (theClickCount == 2)
		ButtonDepress(ID_YES);
}

// 0x48a810 (slot 52 of the choice's box; the delete box has 0x489ba0 and the base class a pure virtual): the list is made
// again out of the application's players: the current player first, the others after it in the order of the registry. The
// scroll bar starts at one line and the list's own AddLine sets it from there.
// port: the line colours are kept as long as the lines (the original's list has no colour for the line it inserts in
// front, and none at all when the current player is the only one)
void UserSelectDialog::RebuildList()
{
	mList->mLines.clear();
	mList->mLineColors.clear();
	mScrollbar->mMaxValue = 1.0;

	std::string aCurrent;
	if (mApp->mUser != NULL)
		mApp->mUser->GetString(PROFILE_NAME, &aCurrent);

	for (size_t i = 0; i < mApp->mUserNames.size(); i++)
	{
		if (mApp->mUserNames[i] != aCurrent)
		{
			mScrollbar->mMaxValue += 1.0;
			mList->AddLine(mApp->mUserNames[i], false);
		}
	}

	if (!aCurrent.empty())
	{
		mList->mLines.insert(mList->mLines.begin(), aCurrent);
		mList->mLineColors.insert(mList->mLineColors.begin(), mList->mColors[ListWidget::COLOR_TEXT]);
	}
}

// 0x484760: the first line that is this name gets selected (none: the selection stays)
void ListDialog::SelectByName(const std::string& theName)
{
	for (size_t i = 0; i < mList->mLines.size(); i++)
	{
		if (mList->mLines[i] == theName)
		{
			mList->SetSelect((int) i);
			return;
		}
	}
}

// 0x486b00: the choice of the player. The two buttons under the list are the box's own (their ids are 0xfb0 and 0xfb1).
UserSelectDialog::UserSelectDialog(AstroPopApp* theApp) :
	ListDialog(theApp, 0x6d, GT(0x0056b390), std::string())
{
	mNewButton = new DialogButton(mYesButton->mComponentImage, 0xfb0, this);
	mNewButton->mLabel = "NEW";
	mDeleteButton = new DialogButton(mYesButton->mComponentImage, 0xfb1, this);
	mDeleteButton->mLabel = "DELETE";
	StyleButton(mNewButton);
	StyleButton(mDeleteButton);
}

void UserSelectDialog::StyleButton(DialogButton* theButton)
{
	theButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
	theButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
	theButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
	theButton->SetFont(GetFontById(0xb1));
	theButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xff, 0));
	theButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xff, 0xff, 0xff));
}

// 0x484b50 (body 0x4846e0)
UserSelectDialog::~UserSelectDialog()
{
	delete mNewButton;
	delete mDeleteButton;
}

// 0x4830f0
void UserSelectDialog::AddedToManager(WidgetManager* theManager)
{
	ListDialog::AddedToManager(theManager);
	theManager->AddWidget(mNewButton);
	theManager->AddWidget(mDeleteButton);
}

// 0x483130
void UserSelectDialog::RemovedFromManager(WidgetManager* theManager)
{
	ListDialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mNewButton);
	theManager->RemoveWidget(mDeleteButton);
}

// 0x483000: NEW and DELETE stand side by side over the OK and Cancel of the box
void UserSelectDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	ListDialog::Resize(theX, theY, theWidth, theHeight);
	int aButtonWidth = (((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - 0x30) / 2;
	int aButtonHeight = mYesButton->mComponentImage->GetHeight();
	int aButtonY = (((mHeight - mContentInsets.mBottom) + mY) - 7) - aButtonHeight * 2;
	mNewButton->Resize(mContentInsets.mLeft + 0x10 + mX, aButtonY, aButtonWidth, aButtonHeight);
	mDeleteButton->Resize(mContentInsets.mLeft + mX + 0x20 + aButtonWidth, aButtonY, aButtonWidth, aButtonHeight);
}

// 0x4830c0: room for the row of the two buttons
int UserSelectDialog::GetPreferredHeight(int theWidth)
{
	int aButtonHeight = mYesButton->mComponentImage->GetHeight();
	return ListDialog::GetPreferredHeight(theWidth) + aButtonHeight;
}

// 0x486ea0: NEW and DELETE are answered at once, with an entry that no player's name can be (the new player box refuses
// both), and the box does not close by itself
void UserSelectDialog::ButtonDepress(int theId)
{
	if (theId == 0xfb0)
	{
		mSelectedName = GT(0x55dce0);
		mApp->ButtonDepress(mId + 2000);
	}
	else if (theId == 0xfb1)
	{
		mSelectedName = GT(0x55dcd0);
		mApp->ButtonDepress(mId + 2000);
	}
	else
		ListDialog::ButtonDepress(theId);
}

// 0x486f20: the list is as high as seven lines of its font under the line of text
DeleteUserDialog::DeleteUserDialog(AstroPopApp* theApp) :
	ListDialog(theApp, 0x6e, GT(0x0056b3a0), GT(0x56b3b0))
{
	m11c = mList->mFont->GetHeight() * 7;
}

// 0x484b80 (body 0x484830)
DeleteUserDialog::~DeleteUserDialog()
{
}

// 0x489ba0 (slot 52): the list is made again out of every player of the application, in the order of the registry (the
// current player is not put first, as the choice's box does). The scroll bar starts at no lines and AddLine's own count
// follows.
// port: the line colours are kept as long as the lines, as in the choice's box
void DeleteUserDialog::RebuildList()
{
	mList->mLines.clear();
	mList->mLineColors.clear();
	mScrollbar->mMaxValue = 0.0;

	for (size_t i = 0; i < mApp->mUserNames.size(); i++)
	{
		mScrollbar->mMaxValue += 1.0;
		mList->AddLine(mApp->mUserNames[i], false);
	}
}
