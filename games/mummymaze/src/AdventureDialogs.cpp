// The dialogs of the adventure mode: the saved adventures to choose from (WinMM.exe 0x401000), the one to delete (0x412750)
// and the name of a new one (0x421db0).

#include "Button2003.h"
#include "Dialogs.h"
#include "MummyApp.h"
#include "GameText.h"
#include "Res.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

using namespace Sexy;

// the label of a slot: the name that the saved game carries, or NEW GAME
static SexyString SlotLabel(MMApp* theApp, int theSlot)
{
	SexyString aName;
	if (theApp->AdventureName(theSlot, aName))
		return aName;
	return GT(0x0048d1ac);
}

// 0x401000
AdventureDialog::AdventureDialog(MMApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_ADVENTURE_NEW, true, GT(0x0048d1d4), "",
		GT(0x0048d1f0), BUTTONS_FOOTER),
	mApp(theApp),
	mHasDelete(true)
{
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);
	bool aAllUsed = true;
	for (int i = 0; i < 5; i++)
	{
		mSlots[i] = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], i, this);
		mSlots[i]->SetFont(mApp->mFont1);
		mSlots[i]->mLabel = SlotLabel(mApp, i);
		if (!mApp->AdventureExists(i))
			aAllUsed = false;
	}
	mDeleteButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], 5, this);
	mDeleteButton->SetFont(mApp->mFont1);
	mDeleteButton->mLabel = GT(0x0048d1a0);
	mDeleteButton->SetColor(0, Color(0, 0, 0));
	// the Delete Game button is there only when every slot holds a game (the original clears its flag at each free slot)
	if (!aAllUsed)
	{
		mHasDelete = false;
		mDeleteButton->SetVisible(false);
	}
}

// 0x401620 (its deleting form 0x401600)
AdventureDialog::~AdventureDialog()
{
	for (int i = 0; i < 5; i++)
		delete mSlots[i];
	delete mDeleteButton;
}

// 0x4016d0
void AdventureDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->AddWidget(mSlots[i]);
	theWidgetManager->AddWidget(mDeleteButton);
}

// 0x401710
void AdventureDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(mSlots[i]);
	theWidgetManager->RemoveWidget(mDeleteButton);
}

// 0x401750
void AdventureDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	for (int i = 0; i < 5; i++)
		mSlots[i]->Resize(mX + mContentInsets.mLeft, mY + 0x4b + i * 0x2d, aWidth, 0x28);
	mDeleteButton->Resize(mX + mContentInsets.mLeft, mY + 0x133, aWidth, 0x28);
}

// 0x4016b0
int AdventureDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x190 + (mHasDelete ? 0x32 : 0);
}

// 0x412d80: every press of a button of the dialog clicks (the three dialogs share the one function)
void AdventureDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(SOUND_CLICK);
}

// 0x4017f0: a slot is played, the last button asks which game to delete, the footer button (Cancel) only closes
void AdventureDialog::ButtonDepress(int theId)
{
	if (theId < 5)
	{
		mApp->DoMap(theId, false);
		mApp->KillDialog(DIALOG_ADVENTURE_NEW);
		return;
	}
	if (theId == 5)
		mApp->DoDeleteDialog();
	mApp->KillDialog(DIALOG_ADVENTURE_NEW);
}

// 0x412750
DeleteDialog::DeleteDialog(MMApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_DELETE_ADVENTURE, true, GT(0x00491f28), "",
		GT(0x0048d1f0), BUTTONS_FOOTER),
	mApp(theApp)
{
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);
	for (int i = 0; i < 5; i++)
	{
		mSlots[i] = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], i, this);
		mSlots[i]->SetFont(mApp->mFont1);
		mSlots[i]->mLabel = SlotLabel(mApp, i);
	}
}

// 0x412c10 (its deleting form 0x412bf0)
DeleteDialog::~DeleteDialog()
{
	for (int i = 0; i < 5; i++)
		delete mSlots[i];
}

// 0x412ca0
void DeleteDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->AddWidget(mSlots[i]);
}

// 0x412cd0
void DeleteDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(mSlots[i]);
}

// 0x412d00
void DeleteDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	for (int i = 0; i < 5; i++)
		mSlots[i]->Resize(mX + mContentInsets.mLeft, mY + 0x4b + i * 0x2d, aWidth, 0x28);
}

// 0x412c90
int DeleteDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x190;
}

// 0x412d80
void DeleteDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->PlaySample(SOUND_CLICK);
}

// 0x412da0
void DeleteDialog::ButtonDepress(int theId)
{
	if (theId < 5)
		mApp->DeleteGame(theId);
	mApp->KillDialog(DIALOG_DELETE_ADVENTURE);
}

// 0x421db0
NameDialog::NameDialog(MMApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_NEW_ADVENTURE_NAME, true, GT(0x004949cc), "",
		GT(0x004949dc), BUTTONS_FOOTER),
	mApp(theApp)
{
	Use2003Buttons(this, mApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mFont1);
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFont1);
	mNameEdit->SetColors(kNameEditColors, 5);
	mNameEdit->mMaxChars = 0x14;
#ifdef __EMSCRIPTEN__
	SiteFillAdventureName(mNameEdit);	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
#endif
}

// 0x422090
NameDialog::~NameDialog()
{
	delete mNameEdit;
}

void NameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

void NameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// 0x422100
void NameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x30, mY + 0x70, mWidth - 0x60, 0x18);
}

int NameDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0xf5;
}

// 0x4222d0
void NameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0xa4, 0x2c, 0));
	g->DrawString(GT(0x004949f0), mNameEdit->mX - mX, (mNameEdit->mY - mY) - 0x13);
	DrawEditBox(g, mApp->mImages[IMAGE_EDITBOX], (mNameEdit->mX - mX) - 4, (mNameEdit->mY - mY) - 4, mNameEdit->mWidth + 8);
}

// 0x401c90: Enter in the box does nothing (RecordScoreDialog's 0x428450 answers OK; this dialog's listener slot is the
// empty function)
void NameDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
}
