// UserSelect: the window that asks who is playing (0x4721e0..0x4732d0 in the original), opened from the menu's sign.
// Eight rows hold the profiles' names, the first empty row below them standing for a new player; a click picks a row,
// and the empty one opens the new player window. Its own two buttons rename and delete the player picked, and OK
// makes that player the current one.
#pragma once

#include "BBWindow.h"

namespace Sexy
{

class AskWin;
class NewUser;
class ObjString;

const int USERSELECT_ROWS = 8;
const int USERSELECT_BUTTONS = 2;
const int USERSELECT_NAME_SIZE = 0x20;

class UserSelect : public BBWindow
{
public:
	AskWin* mDeleteAsk;      // +0x350: the window asking whether to delete the player, while it is open
	AskWin* mAskWin;         // +0x354: a window telling the player what went wrong, while it is open
	NewUser* mNewUser;       // +0x358: the new player window, while it is open
	int mFreeRow;            // +0x35c: the first empty profile, the new player's row; -1 when all eight are in use
	int mSelected;           // +0x360: the row picked, or -1
	ObjBase* mRows[USERSELECT_ROWS];      // +0x364: three cels: plain, the new player's, picked
	ObjString* mNames[USERSELECT_ROWS];   // +0x384
	ObjString* mSelectedName;             // +0x3a4: the picked row's name
	ObjBase* mButtons[USERSELECT_BUTTONS]; // +0x3a8: rename, delete
	char mOldName[USERSELECT_NAME_SIZE];  // +0x3b0: the name as it was before renaming began
	bool mRenaming;          // +0x3d0: the picked name takes the keys

	UserSelect();            // 0x4721e0
	bool Update() override;  // 0x472340
	bool MouseUp(ObjBase* theObj) override;     // 0x472730

private:
	void Select(int theRow); // 0x472b50 (EAX the row)
	void Deselect();         // 0x472bc0
	void Refresh();          // 0x472c20
	void BuildRows();        // 0x472ca0
	void BuildButtons(float theHeight); // 0x472e70
	void Tell(const char* theFirstText, const char* theSecondText);
};

} // namespace Sexy
