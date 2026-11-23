#include "Bej3Dialog.h"

#include "Bej3App.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "Res.h"
#include "WidgetManager.h"

#include <cctype>

namespace Sexy {

namespace {

// The exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 0x4b0;
}

// 0x475e30: the text of a numbered string in default.xml
std::string TextById(int theId, const std::string& theDefault)
{
	return gApp->GetString(std::to_string(theId), theDefault);
}

}  // namespace

Bej3Dialog::Bej3Dialog(int theId, bool isModal, const std::string& theHeader, const std::string& theLines,
	const std::string& theFooter, int theButtonMode) :
	Dialog(IMAGE_DIALOG_BACKGROUND, IMAGE_DIALOG_BUTTON, theId, isModal, theHeader, theLines, theFooter, theButtonMode)
{
	mClip = false;   // +0x51
	SetColor(COLOR_HEADER, Color(255, 255, 255));
	SetColor(COLOR_LINES, Color(0, 0, 0));
	mSpaceAfterHeader = Scaled(0x2d);
	mContentInsets = Insets(Scaled(0x5a), Scaled(0x16), Scaled(0x5a), Scaled(0x2d));
	mButtonHorzSpacing = Scaled(10);
	mButtonSidePadding = Scaled(0x19);
	mLineSpacingOffset = Scaled(-6);
	SetHeaderFont(FONT_DIALOG_HEADER);
	SetLinesFont(FONT_DIALOG_SMALL_TEXT);
	SetButtonFont(FONT_DIALOG_BUTTONS);
	mFadeCurve.SetCurve("b+0,2,0.033333,1,####        cY### >P###");
	// port: the exe's own buttons (0x76a190) show one cel of the picture each (rest, over, pressed); the framework's DialogButtons need the
	// rectangles of those cels and the height of one
	mButtonHeight = IMAGE_DIALOG_BUTTON->GetCelHeight();
	for (DialogButton* aButton : {mYesButton, mNoButton}) {
		if (aButton == nullptr)
			continue;
		aButton->mNormalRect = IMAGE_DIALOG_BUTTON->GetCelRect(0);
		aButton->mOverRect = IMAGE_DIALOG_BUTTON->GetCelRect(1);
		aButton->mDownRect = IMAGE_DIALOG_BUTTON->GetCelRect(2);
	}
}

// 0x76a8a0 (bejeweled3). port: the exe also tells the screen at app+0xbe8 when the dialog goes, and refreshes the
// widget manager's mouse point (0x585ac0) once the dialog is whole
void Bej3Dialog::Update()
{
	Widget::Update();
	if (!mFadeCurve.HasBeenTriggered())
		MarkDirty();
	bool anEnded = mFadeCurve.IncInVal();
	if (!anEnded && mFadeCurve.mRamp != CurvedVal::RAMP_LINEAR) {
		double aValue = mFadeCurve.GetOutVal();
		if (aValue == 0.0) {
			gApp->KillDialog(this);
			return;
		}
		if (aValue == 1.0)
			SetChildrenMouseVisible(true);
	}
	else if (mFadeCurve.mRamp == CurvedVal::RAMP_CURVEDATA)
		SetChildrenMouseVisible(false);
}

// 0x769f00 (bejeweled3): Escape presses the No button, or the Yes button when there is none
void Bej3Dialog::KeyDown(KeyCode theKey)
{
	if (theKey != KEYCODE_ESCAPE || !mEscapeCloses)
		return;
	if (mNoButton != nullptr)
		mNoButton->mButtonListener->ButtonDepress(mNoButton->mId);
	else if (mYesButton != nullptr)
		mYesButton->mButtonListener->ButtonDepress(mYesButton->mId);
}

void Bej3Dialog::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (!mFadeCurve.IsDoingCurve() && mMouseDownAllowed)
		Dialog::MouseDown(x, y, theBtnNum, theClickCount);
}

// 0x769fd0 (bejeweled3): the click sound of a press, then the Dialog's
void Bej3Dialog::ButtonPress(int theId)
{
	if (!mClosing) {
		gApp->PlaySoundEx(SOUND_BUTTON_PRESS, 1.0, 0, 0.0);
		Dialog::ButtonPress(theId);
	}
}

// 0x769f60 (bejeweled3): the release sound, then the Dialog's, which tells the app; the result 0x3ea is kept
void Bej3Dialog::ButtonDepress(int theId)
{
	if (!mClosing) {
		gApp->PlaySoundEx(SOUND_BUTTON_RELEASE, 1.0, 0, 0.0);
		Dialog::ButtonDepress(theId);
	}
	if (theId == 0x3ea)
		mResult = 0x3ea;
}

int Bej3Dialog::GetPreferredHeight(int theWidth)
{
	return Dialog::GetPreferredHeight(theWidth) + Scaled(5);
}

void Bej3Dialog::StartClose()
{
	if (!mClosing) {
		mClosing = true;
		double aValue = mFadeCurve.GetOutVal();
		mFadeCurve.SetCurve("b+0,1,0.05,1,~###         ~#A5t");
		mFadeCurve.mOutMax = aValue;
	}
}

void Bej3Dialog::SetChildrenMouseVisible(bool theOn)
{
	for (Widget* aWidget : mWidgets)
		aWidget->mMouseVisible = theOn;
}

NewUserDialog::NewUserDialog(bool theIsRename, bool theWithButtons) :
	Bej3Dialog(theIsRename ? 2 : 1, true,
		theIsRename ? TextById(0x12e, "RENAME USER") : TextById(0x12f, "NEW USER"), "", "",
		theWithButtons ? BUTTONS_OK_CANCEL : BUTTONS_NONE)
{
	mEdit = new EditWidget(1, this);
	mEdit->SetFont(FONT_DIALOG_SMALL_TEXT);
	mEdit->SetColor(EditWidget::COLOR_BKG, Color(0, 0, 0, 0));
	mEdit->SetColor(EditWidget::COLOR_OUTLINE, Color(0, 0, 0, 0));
	mEdit->SetColor(EditWidget::COLOR_TEXT, Color(255, 255, 255, 255));
	mEdit->SetColor(EditWidget::COLOR_HILITE, Color(255, 255, 255, 255));
	mEdit->SetColor(EditWidget::COLOR_HILITE_TEXT, Color(0, 0, 0, 255));
	mEdit->mMaxPixels = Scaled(500);
	AddWidget(mEdit);
}

NewUserDialog::~NewUserDialog()
{
	delete mEdit;
}

// 0x682200 (bejeweled3): the label over the name's box, and the box's picture under it
void NewUserDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	g->SetFont(FONT_DIALOG_SMALL_TEXT);
	g->SetColor(Color(0, 0, 0, 255));
	std::string aLabel = mId == 2 ? TextById(0x130, "Enter your new name:") : TextById(0x131, "Enter your name:");
	Rect aLabelRect(Scaled(0x32), mEdit->mY - Scaled(0x50), mWidth - Scaled(100), mHeight);
	g->WriteWordWrapped(aLabelRect, aLabel, -1, 0);
	g->DrawImageBox(Rect(mWidth / 2 - Scaled(0x10e), mEdit->mY - Scaled(5), Scaled(0x21c), IMAGE_DIALOG_EDITBOX->GetHeight()),
		IMAGE_DIALOG_EDITBOX);
}

// 0x67b890 (bejeweled3): the box is centred, 210/1200 down, as wide as it may type and 60/1200 high
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	int aBoxWidth = mEdit->mMaxPixels;
	mEdit->Resize(theWidth / 2 - aBoxWidth / 2, Scaled(0xd2), aBoxWidth, Scaled(0x3c));
}

int NewUserDialog::GetPreferredHeight(int theWidth)
{
	return Scaled(0x1ae);
}

// 0x673b80 (bejeweled3): Enter in the box presses the dialog's OK (1000)
void NewUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	ButtonDepress(Dialog::ID_YES);
}

// 0x675db0 (bejeweled3): the first letter is capitalised once, and only letters, digits, spaces and what the font has
// are let in
bool NewUserDialog::AllowText(int theId, const SexyString& theText)
{
	if (theText.length() == 1 && !mCapitalised) {
		if (!mEdit->mString.empty())
			mEdit->mString[0] = (char)toupper((unsigned char)mEdit->mString[0]);
		mCapitalised = true;
	}
	for (char aChar : theText) {
		bool isPlain = (isalnum((unsigned char)aChar) != 0 && (unsigned char)aChar < 0x80) || aChar == ' ';
		if (!isPlain && (mLinesFont == nullptr || mLinesFont->CharWidth(aChar) <= 0))
			return false;
	}
	return true;
}

// 0x683600 (bejeweled3): the same asked of a game with no player; its title is string 0x1fd
WelcomeDialog::WelcomeDialog() : NewUserDialog(false, false)
{
	mDialogHeader = TextById(0x1fd, "WELCOME NEW USER");
}

}  // namespace Sexy
