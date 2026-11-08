#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

// OptionsDialog.h: the options (Sexy::OptionsDialog, 0x5cae38-0x5cd63c). TODO: a stub until it is translated.

#include "PodDialog.h"

namespace Sexy
{

class OptionsDialog : public PodDialog
{
public:
	OptionsDialog(WinDMApp* theApp) : PodDialog(theApp, 2, true, "OPTIONS", "", "", 0) {}
	void FinishOptionVerify(bool theKeep) {}
};

}

#endif
