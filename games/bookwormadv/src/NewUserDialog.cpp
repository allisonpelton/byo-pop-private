#include "NewUserDialog.h"

#include "EditWidget.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace {

// The characters a name is made of: letters, digits and a few marks (^ only arrives as pasted text)
bool IsNameChar(char theChar, bool isTyped)
{
	if ((theChar >= 'a' && theChar <= 'z') || (theChar >= 'A' && theChar <= 'Z') || (theChar >= '0' && theChar <= '9'))
		return true;
	switch (theChar) {
	case ' ':
	case '!':
	case '@':
	case '#':
	case '$':
	case '%':
	case '&':
	case '(':
	case ')':
		return true;
	case '^':
		return !isTyped;
	}
	return false;
}

}  // namespace

// 0x482590: dialog 7, with OK ("CANCEL" too when it asks from the players' list), an edit box in the big font
// 189 pixels wide, theName in it and the new player's banner
NewUserDialog::NewUserDialog(int theButtonMode, const std::string& theName)
	: GameDialog(GameApp::DIALOG_NEW_USER, _S(""), _S(""), _S(""), theButtonMode), mEdit(nullptr), mPrevFocus(nullptr)
{
	if (mButton1 != nullptr)
		mButton1->mLabel = GT(0x7ac5a0);
	if (theButtonMode == BUTTONS_OK_CANCEL && mButton2 != nullptr) {
		mButton2->mLabel = GT(0x7af838);
		mButton2->mDisabledRect = IMAGE_DIALOG_BUTTON->GetCelRect(4);
	}
	mEdit = new EditWidget(1, this);
	mEdit->SetFont(FONT_LARGE, nullptr);
	mEdit->mMaxPixels = 189;
	if (!theName.empty())
		mEdit->SetText(theName, true);
	mBanner = IMAGE_DIALOG_BANNER_NEWUSER;
	mBannerY = 17;
}

// 0x482840: the edit box goes with it (the dialog only takes it out of the manager)
NewUserDialog::~NewUserDialog()
{
	delete mEdit;
}

// 0x482f00
std::string NewUserDialog::GetName() const
{
	return mEdit->mString;
}

void NewUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	// 0x4828d0: the buttons go in as the GameDialog's, then the edit box, which takes the focus
	GameDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEdit);
	mPrevFocus = theWidgetManager->mFocusWidget;
	theWidgetManager->SetFocus(mEdit);
}

void NewUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	// 0x482940
	GameDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEdit);
	if (mPrevFocus != nullptr)
		theWidgetManager->SetFocus(mPrevFocus);
}

void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// 0x4829a0: the edit box in the middle, 175 down; the buttons a half of theirs lower than the GameDialog puts them
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	mEdit->Resize(mX + theWidth / 2 - mEdit->mMaxPixels / 2, mY + 175, mEdit->mMaxPixels,
	              FONT_LARGE->GetHeight() + 4);
	int aButtonY = mButton1->mY + IMAGE_DIALOG_BUTTON->mHeight / 2;
	if (mButton2 == nullptr) {
		// OK alone: half the dialog wide, in the middle
		int aWidth = mWidth / 2;
		mButton1->Resize(mX + aWidth - aWidth / 2, aButtonY, aWidth, mButton1->mHeight);
	} else {
		mButton1->Move(mButton1->mX, aButtonY);
		mButton2->Move(mButton2->mX, aButtonY);
	}
}

void NewUserDialog::Draw(Graphics* g)
{
	// 0x482aa0: the greeting over the edit box, and the frame under it
	GameDialog::Draw(g);
	g->SetFont(FONT_LARGE);
	g->SetColor(Color(128, 101, 39));
	Rect aTextRect(50, mEdit->mY - mY - 85, mWidth - 100, mHeight);
	g->WriteWordWrapped(aTextRect, GT(0x7af840), -1, 0, nullptr, -1, nullptr);
	Rect aFrame = mEdit->GetRect();
	aFrame.mX += -11 - mX;
	aFrame.mY += -7 - mY;
	aFrame.mWidth = mEdit->mMaxPixels + 23;
	aFrame.mHeight = 44;
	g->DrawImageBox(aFrame, IMAGE_DIALOG_USER_ENTRY);
}

void NewUserDialog::ButtonDepress(int theId)
{
	// 0x482c00
	if (theId == mButton1->mId) {
		const std::string& aName = mEdit->mString;
		if (aName.empty()) {
			mApp->DoOkDialog(GT(0x7ab038), GT(0x7af89c), "", nullptr);
			return;
		}
		size_t i = 0;
		while (i < aName.size() && aName[i] == ' ')
			i++;
		if (i >= aName.size()) {
			mApp->DoOkDialog(GT(0x7ab038), GT(0x7af8bc), "", nullptr);
			return;
		}
		mApp->NewUserDialogDone();
	} else if (mButton2 != nullptr && theId == mButton2->mId) {
		mEdit->SetText(_S(""), true);
		mApp->NewUserDialogDone();
	}
}

void NewUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	// 0x482ee0: Return in the box is OK
	ButtonDepress(mButton1->mId);
}

bool NewUserDialog::AllowChar(int theId, SexyChar theChar)
{
	// 0x482f30: a character the font has, and one of a name's
	if (mEdit->mFont == nullptr || mEdit->mFont->CharWidth(theChar) == 0)
		return false;
	return IsNameChar(theChar, true);
}

bool NewUserDialog::AllowText(int theId, const SexyString& theText)
{
	// 0x4833c0: every character of a name's, and in the font
	for (size_t i = 0; i < theText.size(); i++) {
		if (!IsNameChar(theText[i], false) || mEdit->mFont == nullptr || mEdit->mFont->CharWidth(theText[i]) == 0)
			return false;
	}
	return true;
}
