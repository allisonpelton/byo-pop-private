// NewUser: the window that asks for a new player's name (0x4504d0..0x4509f0 in the original). The name is typed into a
// box under two lines of text; Enter, a click on the box or the window's button takes it, and a name the profiles turn
// down (empty, taken, or no slot free) brings up a window saying so.
#pragma once

#include "BBWindow.h"

namespace Sexy
{

class Anim;
class AskWin;
class ObjString;

class NewUser : public BBWindow
{
public:
	AskWin* mAskWin;         // +0x354: the window saying the name was turned down, while it is open
	int mUnused358;          // +0x358: set to 0 by the constructor; nothing reads it
	int mButtonsWanted;      // +0x35c: the window's buttons, OK alone or OK and Cancel
	ObjString* mName;        // +0x360: the name typed so far
	Anim* mInputAnim;        // +0x364: images/menu/inputback
	ObjBase* mInputBox;      // +0x36c: the box behind the name; a click on it takes the name like the button

	NewUser(int theButtons); // 0x4504d0
	bool Update() override;                     // 0x450710
	bool MouseUp(ObjBase* theObj) override;     // 0x450860

	void Open(ObjBase* theParent);              // 0x4505e0

private:
	void BuildInputBox();    // 0x450920
};

} // namespace Sexy
