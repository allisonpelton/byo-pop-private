// The dialogs of the adventure mode: the saved adventures to choose from (WinMM.exe 0x401000), the one to delete (0x412750)
// and the name of a new one (0x421db0).

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
static SexyString SlotLabel(MummyApp* theApp, int theSlot)
{
	SexyString aName;
	if (theApp->AdventureName(theSlot, aName))
		return aName;
	return GT(0x0048d1ac);
}

// 0x401000
AdventureSlotsDialog::AdventureSlotsDialog(MummyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_ADVENTURE_NEW, true, GT(0x0048d1d4), "",
		GT(0x0048d1f0), BUTTONS_FOOTER),
	mApp(theApp),
	mHasDelete(true)
{
	SetButtonFont(mApp->mHeaderFont);
	SetHeaderFont(mApp->mFont1);
	SetLinesFont(mApp->mFont1);
	bool aAny = false;
	for (int i = 0; i < 5; i++)
	{
		mSlots[i] = new DialogButton(mApp->mImages[IMAGE_DBUTTON], i, this);
		mSlots[i]->SetFont(mApp->mFont1);
		mSlots[i]->mLabel = SlotLabel(mApp, i);
		if (mApp->AdventureExists(i))
			aAny = true;
	}
	mDeleteButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 5, this);
	mDeleteButton->SetFont(mApp->mFont1);
	mDeleteButton->mLabel = GT(0x0048d1a0);
	mDeleteButton->SetColor(0, Color(0, 0, 0));
	if (!aAny)
	{
		mHasDelete = false;
		mDeleteButton->SetVisible(false);
	}
}

AdventureSlotsDialog::~AdventureSlotsDialog()
{
	for (int i = 0; i < 5; i++)
		delete mSlots[i];
	delete mDeleteButton;
}

// 0x4016d0
void AdventureSlotsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->AddWidget(mSlots[i]);
	theWidgetManager->AddWidget(mDeleteButton);
}

// 0x401710
void AdventureSlotsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(mSlots[i]);
	theWidgetManager->RemoveWidget(mDeleteButton);
}

// 0x401750
void AdventureSlotsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	for (int i = 0; i < 5; i++)
		mSlots[i]->Resize(mX + mContentInsets.mLeft, mY + 0x4b + i * 0x2d, aWidth, 0x28);
	mDeleteButton->Resize(mX + mContentInsets.mLeft, mY + 0x133, aWidth, 0x28);
}

// 0x4016b0
int AdventureSlotsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x190 + (mHasDelete ? 0x32 : 0);
}

// 0x4017f0: a slot is played, the last button asks which game to delete, the footer button (Cancel) only closes
void AdventureSlotsDialog::ButtonDepress(int theId)
{
	if (theId < 5)
	{
		mApp->mTimesPlayed++;
		mApp->LoadAdventure(theId, false);
		mApp->KillDialog(DIALOG_ADVENTURE_NEW);
		return;
	}
	if (theId == 5)
		mApp->ShowDeleteAdventure();
	mApp->KillDialog(DIALOG_ADVENTURE_NEW);
}

// 0x412750
DeleteAdventureDialog::DeleteAdventureDialog(MummyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_DELETE_ADVENTURE, true, GT(0x00491f28), "",
		GT(0x0048d1f0), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mHeaderFont);
	SetHeaderFont(mApp->mFont1);
	SetLinesFont(mApp->mFont1);
	for (int i = 0; i < 5; i++)
	{
		mSlots[i] = new DialogButton(mApp->mImages[IMAGE_DBUTTON], i, this);
		mSlots[i]->SetFont(mApp->mFont1);
		mSlots[i]->mLabel = SlotLabel(mApp, i);
	}
}

DeleteAdventureDialog::~DeleteAdventureDialog()
{
	for (int i = 0; i < 5; i++)
		delete mSlots[i];
}

// 0x412ca0
void DeleteAdventureDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->AddWidget(mSlots[i]);
}

// 0x412cd0
void DeleteAdventureDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(mSlots[i]);
}

// 0x412d00
void DeleteAdventureDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	for (int i = 0; i < 5; i++)
		mSlots[i]->Resize(mX + mContentInsets.mLeft, mY + 0x4b + i * 0x2d, aWidth, 0x28);
}

// 0x412c90
int DeleteAdventureDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x190;
}

// 0x412da0
void DeleteAdventureDialog::ButtonDepress(int theId)
{
	if (theId < 5)
		mApp->DeleteAdventure(theId);
	mApp->KillDialog(DIALOG_DELETE_ADVENTURE);
}

// 0x421db0
NewAdventureDialog::NewAdventureDialog(MummyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_NEW_ADVENTURE_NAME, true, GT(0x004949cc), "",
		GT(0x004949dc), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mHeaderFont);
	SetHeaderFont(mApp->mFont1);
	SetLinesFont(mApp->mFont1);
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFont1);
	mNameEdit->SetColors(kNameEditColors, 5);
	mNameEdit->mMaxChars = 0x14;
#ifdef __EMSCRIPTEN__
	SiteFillAdventureName(mNameEdit);	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
#endif
}

NewAdventureDialog::~NewAdventureDialog()
{
	delete mNameEdit;
}

void NewAdventureDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

void NewAdventureDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// 0x422100
void NewAdventureDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x30, mY + 0x70, mWidth - 0x60, 0x18);
}

int NewAdventureDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0xf5;
}

// 0x4222d0
void NewAdventureDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0xa4, 0x2c, 0));
	g->DrawString(GT(0x004949f0), mNameEdit->mX - mX, (mNameEdit->mY - mY) - 0x13);
	DrawNameBox(g, mApp->mImages[IMAGE_EDITBOX], (mNameEdit->mX - mX) - 4, (mNameEdit->mY - mY) - 4, mNameEdit->mWidth + 8);
}

// Enter in the box starts the adventure (the footer button's answer)
void NewAdventureDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
	mApp->ButtonDepress(mId + 2000);
}
