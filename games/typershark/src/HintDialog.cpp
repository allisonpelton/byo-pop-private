#include "HintDialog.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "DialogButton.h"

using namespace Sexy;

// Not translated (folded forwarders and empty functions): 0x4382b0 (the button listener's ButtonDepress, slot 4: only
// Dialog::ButtonDepress, which Dialog2004 already is), 0x47b200 (the check box listener's one slot, empty)

// 0x439ef0: the footer button is the only one (mode 3). The check box pictures are the left and the right half of the
// checked image (the unchecked and the checked box), copied into two memory images.
HintDialog::HintDialog(WinWSApp* theApp, int theId, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, Image* theDialogImage, Image* theButtonImage, Image* theCheckedImage) :
	Dialog2004(theDialogImage, theButtonImage, theId, true, theDialogHeader, theDialogLines, theDialogFooter,
		Dialog::BUTTONS_FOOTER),
	mApp(theApp),
	mCheckedImage(NULL),
	mUncheckedImage(NULL),
	mField120(0),
	mField124(NULL)
{
	mTextAlign = -1;
	int aHeight = theCheckedImage->GetHeight();
	int aHalfWidth = theCheckedImage->GetWidth() >> 1;

	mCheckedImage = new MemoryImage(mApp);
	mCheckedImage->Create(aHalfWidth, aHeight);
	Graphics aCheckedGraphics(mCheckedImage);
	aCheckedGraphics.DrawImage(theCheckedImage, -aHalfWidth, 0);

	mUncheckedImage = new MemoryImage(mApp);
	mUncheckedImage->Create(aHalfWidth, aHeight);
	Graphics anUncheckedGraphics(mUncheckedImage);
	anUncheckedGraphics.DrawImage(theCheckedImage, 0, 0);

	mField124 = new Checkbox(mUncheckedImage, mCheckedImage, 0, this);
	mField124->Resize(mContentInsets.mLeft + 10 + mX, mYesButton->mY - mYesButton->mHeight,
		theCheckedImage->GetWidth() / 2, theCheckedImage->GetHeight());
	mField124->SetChecked(false, true);

	SetHeaderFont(mApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	SetButtonFont(mApp->mFontRockwell13);
}

// 0x4380e0 (body; the scalar deleting destructor is 0x438f40): the check box and its two pictures go
HintDialog::~HintDialog()
{
	delete mField124;
	delete mUncheckedImage;
	delete mCheckedImage;
}

// 0x4381c0 slot 1
void HintDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mField124);
}

// 0x4381f0 slot 2
void HintDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mField124);
}

// 0x438220 slot 13: the check box sits 20 pixels above the button's top edge less its height
void HintDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	Checkbox* aCheckbox = mField124;
	aCheckbox->Resize(mContentInsets.mLeft + 10 + mX, (mYesButton->mY - mYesButton->mHeight) - 0x14, aCheckbox->mWidth,
		aCheckbox->mHeight);
}

// 0x43bbe0 slot 16: the label of the check box, in the lines font and colour: in dialog coordinates 5 pixels right of
// the box and its baseline 8 above the box's bottom (the sums are in the original's grouping)
void HintDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	int aHeight = mField124->mHeight;
	int aY = mY;
	int aCheckY = mField124->mY;
	int aTextY = (aHeight - aY) + aCheckY - 8;
	g->SetFont(mLinesFont);
	g->SetColor(mColors[COLOR_LINES]);
	g->DrawString(GT(0x0052e868), (mField124->mWidth - mX) + mField124->mX + 5, aTextY);
}

// 0x438180 slot 20: enter, space and escape press the footer button (the app's ButtonDepress with the dialog id + 2000)
void HintDialog::KeyChar(SexyChar theChar)
{
	if ((theChar == '\n') || (theChar == '\r') || (theChar == ' ') || (theChar == '\x1b'))
		mApp->ButtonDepress(mId + ID_YES_2004);
}

// 0x438280 (ButtonListener slot 3): the click goes to the app, then the sample in mField120 plays
void HintDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(mField120);
}
