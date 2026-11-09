#ifndef __TYPERSHARK_WINGAMEDIALOG_H__
#define __TYPERSHARK_WINGAMEDIALOG_H__

// WinGameDialog.cpp: "CONGRATULATIONS!" (dialog 0x8d), shown by the map screen when the last level of the difficulty is
// won. It draws the user's totals for the game (score, sharks, piranhas, adjusted speed, accuracy), the medal for the
// number of completed games and the ten gems and treasures with the count of each the user has, in grey while there is
// none. The two buttons are "return home" (yes, id 0x8d + 2000) and "keep going" (no, id 0xc45).
// Original: class Sexy::WinGameDialog : public Dialog, vftable 0x52d94c (52 slots, slot 50 GetPreferredHeight 0x4379b0),
// ButtonListener vftable 0x52d930 (+0x58, all Dialog's), sizeof 0x120, constructor 0x43a4f0 (called only by the map screen's
// update, 0x45ad20, which sets the three counters right after it), destructor body 0x437990 (the deleting destructor is
// 0x438e70; the export names the body ~CreditsDialog). The original has no
// members to free, and its AddedToManager (0x4379c0), RemovedFromManager (0x4379d0) and Resize (0x4379e0) only call Dialog's.

#include "compat2004/Compat2004.h"

namespace Sexy
{

class WinWSApp;
class Graphics;

class WinGameDialog : public Dialog2004
{
public:
	WinWSApp*				mApp;					// +0x110 constructor argument
	int						mCharsTyped;			// +0x114 constructor 0; the map screen sets it from the app's +0x90c (the correct characters of the game)
	int						mTime;					// +0x118 constructor 0; from the app's +0x900 (the game's time in milliseconds)
	int						mMistakes;				// +0x11c constructor 0; from the app's +0x908 (the wrong keystrokes)

public:
	WinGameDialog(WinWSApp* theApp);														// 0x43a4f0
	virtual ~WinGameDialog() override;														// 0x437990 (deleting 0x438e70), slot 0

	virtual void			Draw(Graphics* g) override;										// 0x439390 slot 16
	virtual int				GetPreferredHeight(int theWidth) override;						// 0x4379b0 slot 50
};

}

#endif
