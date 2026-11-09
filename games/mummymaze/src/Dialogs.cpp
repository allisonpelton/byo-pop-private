// The dialogs of Mummy Maze Deluxe that are classes of their own (WinMM.exe 0x427dc0 ...). The ones that only show
// lines of text and buttons are MummyApp::DoMessage calls (MummyFlow.cpp).

#include "Button2003.h"
#include "Dialogs.h"
#include "MummyApp.h"
#include "GameText.h"
#include "Res.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#endif

using namespace Sexy;

// the colours of the name box (0x49539c: background and outline are clear, the frame comes from the picture)
int Sexy::kNameEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {0xf0, 0xf0, 0xff, 0xff}, {0xff, 0xff, 0xff, 0xff}, {0, 0, 0, 0xff}
};

// 0x4282d0 (and 0x422150): the box of a name: the left end of the picture, a run of its middle and its right end
void Sexy::DrawNameBox(Graphics* g, Image* theBox, int theX, int theY, int theWidth)
{
	g->DrawImage(theBox, theX, theY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(theBox, theX + theWidth - 0x20, theY, Rect(0x40, 0, 0x20, 0x20));
	Graphics gc(*g);
	gc.ClipRect(theX + 0x20, theY, theWidth - 0x20, 0x20);
	int aTiles = (theWidth - 0x21) / 0x20;
	for (int i = 0; i < aTiles; i++)
		gc.DrawImage(theBox, theX + 0x20 + i * 0x20, theY, Rect(0x20, 0, 0x20, 0x20));
}

// 0x427dc0
RecordScoreDialog::RecordScoreDialog(MummyApp* theApp, const SexyString& theLines) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_RECORD_SCORE, true, GT(0x00494548),
		theLines, "", BUTTONS_OK_CANCEL),
	mApp(theApp)
{
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);
	SetColor(Dialog::COLOR_HEADER, Color(0x66, 0x5c, 0x38));
	SetColor(Dialog::COLOR_LINES, Color(0x2e, 0x14, 0));
	SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFont1);
	mNameEdit->SetColors(kNameEditColors, 5);
	mNameEdit->mMaxPixels = 0x68;
	mNameEdit->SetText(mApp->mLastScoreName);
#ifdef __EMSCRIPTEN__
	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
	Byopop::SiteType("mummymaze", mNameEdit, false);
#endif
}

// 0x428010 (its deleting form 0x427ff0)
RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameEdit;
}

// 0x428090
void RecordScoreDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

// 0x4280c0
void RecordScoreDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// 0x4280f0
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x30, mHeight - 0x78 + mY, mWidth - 0x60, 0x18);
}

// 0x428080
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0xf5;
}

// 0x428140: the dialog, the box of the name (0x4282d0: the picture's left end, a run of its middle and its right end), and
// the prompt above it
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	DrawNameBox(g, mApp->mImages[IMAGE_EDITBOX], (mNameEdit->mX - mX) - 4, (mNameEdit->mY - mY) - 4, mNameEdit->mWidth + 8);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0xa4, 0x2c, 0));
	g->DrawString(GT(0x004953ec), mNameEdit->mX - mX, (mNameEdit->mY - mY) - 0x13);
}

// 0x428450: Enter in the box answers the dialog's OK button
void RecordScoreDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
	mApp->ButtonDepress(mId + 2000);
}
