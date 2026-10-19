#include "MoneyDialog.h"

using namespace Sexy;

// 0x42e3d0
MoneyDialog::MoneyDialog(BookWormApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId,
	bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, int theButtonMode) :
	Dialog2004(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines,
		theDialogFooter, theButtonMode)
{
	mApp = theApp;
	mUnusedLightPhase = 0.0;    // port: never initialised in the original (and never read)
}

// 0x42e420 (body; scalar deleting dtor 0x42e450)
MoneyDialog::~MoneyDialog()
{
}

// 0x42e440 (identical-COMDAT folded: also HintDialog's Draw)
void MoneyDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
}

// 0x429290 (folded `jmp Dialog::Update`, shared with HintDialog and the other dialogs)
void MoneyDialog::Update()
{
	Dialog2004::Update();
}
