// MenuButton: a button of the menus (0x446850..0x446ac0 in the original). A two-cel picture, the second drawn while
// the mouse is on it; a click that reaches it is kept until taken, and it cannot be clicked again for 100 frames.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class MenuButton : public ObjBase
{
public:
	bool mClicked;           // +0x27c: clicked, and not taken yet
	int mClickCooldown;      // +0x280: frames until it may be clicked; 100 at the start and after each click
	bool mKeepCursor;        // +0x284: the game's pointer stays shown when the mouse leaves
	bool mHovered;           // +0x285

	MenuButton();            // 0x446850
	~MenuButton();           // 0x446930
	void Deactivate(ObjBase* theObj) override;                // 0x446960
	void Notify(ObjBase* theObj) override;                    // 0x42c290: does nothing
	bool Update() override;                                   // 0x446980
	bool MouseUp(ObjBase* theObj) override;                   // 0x4469b0
	void MouseEnter(ObjBase* theObj, bool isNew) override;    // 0x4469f0
	void MouseLeave(ObjBase* theObj) override;                // 0x446a30

	bool TakeClicked();      // 0x446a70
	void Reset();            // 0x446a80
	void SetKeepCursor(bool keep); // 0x446aa0
};

} // namespace Sexy
