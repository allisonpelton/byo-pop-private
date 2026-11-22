// SlotMachineDialog.cpp: see SlotMachineDialog.h.

#include "SlotMachineDialog.h"

#include "Board.h"
#include "NotTranslated.h"

using namespace Sexy;

static const int SLOT_MACHINE_DIALOG_ID = 0x17;

// 0x4ab260
SlotMachineDialog::SlotMachineDialog(Board* theBoard)
	: ThunderDialog(SLOT_MACHINE_DIALOG_ID, false, "Lucky Spin!", "", "", BUTTONS_NONE)
{
	mUnknown188 = 0;
	mUnknown18C = 0;
	mBoard = theBoard;
	mApp = theBoard->mApp;
}

int SlotMachineDialog::GetPreferredHeight(int theWidth)
{
	NOT_TRANSLATED("SlotMachineDialog::GetPreferredHeight");
	return 0;
}

void SlotMachineDialog::Update()
{
	NOT_TRANSLATED("SlotMachineDialog::Update");
}

void SlotMachineDialog::Draw(Graphics* g)
{
	NOT_TRANSLATED_DRAWING("SlotMachineDialog::Draw");
}
