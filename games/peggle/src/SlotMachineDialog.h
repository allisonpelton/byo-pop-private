// SlotMachineDialog.h: the "Lucky Spin!" dialog, a slot machine the player pulls after a level (the original's
// SlotMachineDialog.cpp). It is a dialog of id 0x17 that is not modal.
//
// Only the making of it is translated so far; every other function stops at NOT_TRANSLATED.

#ifndef __SLOTMACHINEDIALOG_H__
#define __SLOTMACHINEDIALOG_H__

#include "ThunderDialog.h"

namespace Sexy
{

class Board;
class Graphics;
class ThunderballApp;

class SlotMachineDialog : public ThunderDialog
{
public:
	explicit SlotMachineDialog(Board* theBoard); // 0x4ab260
	~SlotMachineDialog() override = default;

	int GetPreferredHeight(int theWidth) override; // 0x496130
	void Update() override;						   // 0x49d2c0
	void Draw(Graphics* g) override;			   // 0x496150

	ThunderballApp* mApp;	// +0x178
	Board* mBoard;			// +0x17c
	int mUnknown188;		// +0x188: 0
	int mUnknown18C;		// +0x18c: 0
};

} // namespace Sexy

#endif
