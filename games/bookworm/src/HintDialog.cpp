#include "HintDialog.h"
#include "BookWormApp.h"
#include "Res.h"
#include "GameText.h"

#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include <map>

using namespace Sexy;

// 0x52f27c: label black, hilite white
static int gHintDisableButtonColors[2][3] =
{
	{0, 0, 0},
	{255, 255, 255}
};

// 0x534c38 (static init 0x42b480, atexit dtor 0x42b440): read by WasHintShown, never written
static std::map<int, bool> gHintShownMap;

// 0x42a820 (body; scalar deleting dtor 0x42a990)
HintDialog::~HintDialog()
{
	if (mDisableButton != NULL)
	{
		delete mDisableButton;
		mDisableButton = NULL;
	}
}

// 0x42a890 (slot 1)
void HintDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	if (mDisableButton != NULL)
		theWidgetManager->AddWidget(mDisableButton);
}

// 0x42a8c0 (slot 2)
void HintDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	if (mDisableButton != NULL)
		theWidgetManager->RemoveWidget(mDisableButton);
}

// 0x42a8f0 (slot 50): room for the second button, its height (at least 32) + 4. Needs a yes button (every
// caller passes a footer).
int HintDialog::GetPreferredHeight(int theWidth)
{
	int aButtonHeight = mYesButton->mHeight;
	if (aButtonHeight < 33)
		aButtonHeight = 32;
	return Dialog2004::GetPreferredHeight(theWidth) + 4 + aButtonHeight;
}

// 0x42a920 (ButtonListener +0x58 slot 2, ButtonPress(id, clickCount)): every button clicks here. The footer
// button (2015) clicks through the app as well, so it plays "buttonclick" twice; the disable button once.
void HintDialog::ButtonPress(int theId, int theClickCount)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x42a950 (ButtonListener +0x58 slot 4)
void HintDialog::ButtonDepress(int theId)
{
	// 2015 goes to the app: KillDialog(15)
	Dialog2004::ButtonDepress(theId);

	// The disable-hints button: off for the session (saved with the user's settings later, not here).
	// The app's own branch for id 26 is unreachable: this button's listener is the dialog.
	if (theId == BUTTON_HINT_DISABLE)
	{
		mApp->mShowHints = false;
		mApp->KillDialog(DIALOG_HINT);
	}
}

// 0x42af50. The creator (BookWormApp::DoHintDialog) sizes it to the screen, then to
// (233, 110, 400, GetPreferredHeight(400)).
HintDialog::HintDialog(BookWormApp* theApp, int theHintId, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_HINT, true,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode),
	mApp(theApp),
	mDisableButton(NULL),
	mHintId(theHintId)
{
	mHasAlpha = true;

	SetHeaderFont(theApp->mFonts[FONT_LOCKERGNOMESMALLCOLOR]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12BOLD]);
	SetButtonFont(mApp->mFonts[FONT_VERDANA12BOLD]);
	Dialog2004::SetColor(COLOR_HEADER, Color(255, 255, 255));
	Dialog2004::SetColor(COLOR_LINES, Color(255, 255, 255));
	Dialog2004::SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	if (mYesButton != NULL)
		mYesButton->SetColor(DialogButton::COLOR_LABEL_HILITE, Color(255, 255, 255));
	if (mNoButton != NULL)
		mNoButton->SetColor(DialogButton::COLOR_LABEL_HILITE, Color(255, 255, 255));

	mDisableButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTON], BUTTON_HINT_DISABLE, this);
	mDisableButton->mLabel = GameText(GT_HINT_DISABLE);
	mDisableButton->mDoFinger = true;
	mDisableButton->SetColors(gHintDisableButtonColors, 2);
	mDisableButton->SetFont(mYesButton->mFont);    // the yes button's font (needs a yes button)
}

// 0x42b160 (slot 13). Screen coordinates. For (233, 110, 400, H): continue (269, 110 + H - 94, 328, 31),
// disable (269, 110 + H - 59, 328, 31).
void HintDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// The header is upper-cased on every call
	mDialogHeader = Upper(mDialogHeader);

	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	if (mDisableButton != NULL)
	{
		// The footer button narrows by 12 px each side and moves up 4 px; the disable button takes that
		// place and the footer button moves above it, 4 px apart
		mYesButton->Resize(mYesButton->mX + 12, mYesButton->mY - 4, mYesButton->mWidth - 24, mYesButton->mHeight);
		mDisableButton->Resize(mYesButton->mX, mYesButton->mY, mYesButton->mWidth, mYesButton->mHeight);
		mYesButton->Move(mYesButton->mX, mYesButton->mY - mDisableButton->mHeight - 4);
	}
}

// 0x42e440 (slot 16; MoneyDialog's, folded): header (upper case, LockergnomeSmallColor) and lines; the footer
// is the button
void HintDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
}

// 0x429290 (slot 17; folded jmp Dialog::Update)
void HintDialog::Update()
{
	Dialog2004::Update();
}

// 0x42b3e0 (cdecl static). Looks up key 0 whatever the id (NAMES D2) and nothing ever inserts into the
// map, so it always returns false: a hint shows whenever hints are on.
bool HintDialog::WasHintShown(int theHintId)
{
	std::map<int, bool>::iterator anItr = gHintShownMap.find(0);
	if (anItr != gHintShownMap.end())
		return anItr->second;
	return false;
}
