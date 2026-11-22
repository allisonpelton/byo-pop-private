// BBWindow: a window with one or two buttons (0x4270e0..0x427b60 in the original). It opens by growing in from nothing
// and closes by shrinking, and while it is open nothing under it takes the mouse. Its buttons are cels of
// "images/menu/usrsel_buttons"; pressing either closes it.
#pragma once

#include "MenuObj.h"

namespace Sexy
{

enum BBWindowState
{
	BBWINDOW_NOT_OPEN = 0,
	BBWINDOW_OPEN = 1,
	BBWINDOW_CLOSED = 2,     // the close track ended
};

const int BBWINDOW_MAX_BUTTONS = 2;

class BBWindow : public MenuObj
{
public:
	int mOpenTrack;          // +0x334
	int mCloseTrack;         // +0x338
	int mState;              // +0x33c: BBWindowState
	ObjBase* mButtons[BBWINDOW_MAX_BUTTONS]; // +0x340
	int mButtonCount;        // +0x348
	float mSavedHoverMinZ;   // +0x34c: the mouse's lowest depth before this window raised it

	static int sOpenCount;   // 0x60e4dc: how many are alive

	BBWindow();              // 0x4270e0
	~BBWindow();             // 0x427120
	void Notify(ObjBase* theObj) override;      // 0x427230
	bool Update() override;                     // 0x427260
	bool MouseUp(ObjBase* theObj) override;     // 0x4272f0

	void Open(int theColumns, int theRows, int theButtons, bool deleteWhenClosed); // 0x427170
	int GetState() { return mState; }           // 0x427330
	void Close();                               // 0x427340
	void BuildTracks(bool deleteWhenClosed);    // 0x427350
	void BuildButtons(float theHeight);         // 0x4276c0
};

} // namespace Sexy
