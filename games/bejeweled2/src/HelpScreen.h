#ifndef __HELPSCREEN_H__
#define __HELPSCREEN_H__

// HelpScreen.h: the help pages (Sexy::HelpScreen, 0x5c1c7a-0x5c267d). TODO: a stub until it is translated.

#include "Dialog.h"

namespace Sexy
{

class HelpScreen : public Dialog
{
public:
	bool mNeverShowAgain;
	HelpScreen() : Dialog(NULL, NULL, 40, true, "", "", "", 0), mNeverShowAgain(false) {}
};

}

#endif
