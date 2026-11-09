// NewUserDialog: the name entry box (see NewUserDialog.h).

#include "NewUserDialog.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "Res.h"
#include "SiteProfiles.h"

using namespace Sexy;

// 0x484d80: the box is of the colour set of the selector (1). The field takes up to 24 letters and 305 pixels.
NewUserDialog::NewUserDialog() :
	Dialog(IMAGE_DIALOG, IMAGE_DIALOG_BTN, 0x6b, true, GT(0x005649d4), GT(0x0056b064), std::string(), 2)
{
	mEdit = new EditWidget(0, this);
	mEdit->SetFont(GetFontById(0xb0), GetFontById(0x120));
	mEdit->mMaxPixels = 0x131;
	mEdit->mMaxChars = 0x18;
#ifdef __EMSCRIPTEN__
	SiteFillUserName(mEdit);	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
#endif

	MemoryImage* anInsetImage = NULL;
	MemoryImage* aButtonImage = NULL;
	gApp->MakeDialogImages(gApp->GetDialogInsetColor(1), gApp->GetDialogColor(1), &anInsetImage, &aButtonImage, false);
	mComponentImage = anInsetImage;
	mYesButton->mComponentImage = aButtonImage;
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

// 0x483c20 (body 0x482140): the field, the box picture and the buttons' picture (shared by both buttons) are the box's own
NewUserDialog::~NewUserDialog()
{
	delete mEdit;
	delete mComponentImage;
	mComponentImage = NULL;
	if (mYesButton != NULL)
		delete mYesButton->mComponentImage;
}

// 0x4821f0
void NewUserDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog::AddedToManager(theManager);
	theManager->AddWidget(mEdit);
	theManager->SetFocus(mEdit);
}

// 0x482230
void NewUserDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mEdit);
}

// 0x482260: the field is 0x18 high, 0x30 in from the box's left, and sits 0x6e above its bottom
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mEdit->Resize(mX + 0x30, mHeight - 0x6e + mY, mWidth - 0x60, 0x18);
}

// 0x483c50: the frame around the field, made of the three parts of IMAGE_EDIT_BOX (left end, a middle that repeats, right
// end), the field's rectangle widened by 8 and drawn 4 up and to the left
void NewUserDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	Rect aBox(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, (mWidth - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft,
			  (mHeight - mBackgroundInsets.mBottom) - mBackgroundInsets.mTop);
	g->DrawImageBox(aBox, IMAGE_DIALOG);

	Rect aField = mEdit->GetInsetRect();
	int aWidth = aField.mWidth + 8;
	int aX = (aField.mX - mX) - 4;
	int aY = (aField.mY - mY) - 4;
	g->DrawImage(IMAGE_EDIT_BOX, aX, aY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(IMAGE_EDIT_BOX, aWidth - 0x20 + aX, aY, Rect(0x40, 0, 0x20, 0x20));

	Graphics aClipped(*g);
	aClipped.ClipRect(aX + 0x20, aY, aWidth - 0x20, 0x20);
	int aParts = (aWidth - 0x21) / 0x20;
	for (int i = 0; i < aParts; i++)
		aClipped.DrawImage(IMAGE_EDIT_BOX, i * 0x20 + aX + 0x20, aY, Rect(0x20, 0, 0x20, 0x20));
}

// 0x4822b0
void NewUserDialog::GotFocus()
{
	gApp->mWidgetManager->SetFocus(mEdit);
}

// 0x4822d0: Return in the field answers like the OK button
void NewUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
	gApp->ButtonDepress(mId + 2000);
}

// 0x4820a0: a name has no punctuation (but may have an underscore, a bar, digits, letters and spaces)
bool NewUserDialog::AllowChar(int theId, SexyChar theChar)
{
	(void) theId;
	switch (theChar)
	{
	case '!': case '"': case '#': case '$': case '%': case '&': case '\'': case '(': case ')': case '*': case '+':
	case ',': case '-': case '.': case '/': case ':': case ';': case '<': case '=': case '>': case '?': case '@':
	case '[': case '\\': case ']': case '^': case '`': case '{': case '}': case '~':
		return false;
	}
	return true;
}
