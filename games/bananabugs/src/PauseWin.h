// PauseWin: the "Paused" window (0x454ef0..0x455aa0 in the original; 0x33c bytes there). The game opens it when the
// window loses the focus (GameWidget::Update calls PauseForFocusLoss with no other window open): a panel of seven by
// five tiles with one line of text and a picture, the first cel of "images/menu/ingame_buttons", that grows in. When
// it has grown, the game waits (IsBlocking) for a mouse release anywhere (GameWidget::MouseUp, ResumeOnRelease), which
// shrinks the window away; the window ends itself when the shrinking track ends.
#pragma once

#include "MenuObj.h"

namespace Sexy
{

class Anim;

class PauseWin : public MenuObj
{
public:
	ObjBase* mButton;        // +0x334: the picture
	bool mUnused338;         // +0x338: never set (the object comes from operator new)
	bool mUnused339;         // +0x339: set to false by the constructor; nothing reads it

	static PauseWin* sInstance; // 0x67018c: the window, while there is one
	static bool sWaiting;       // 0x670190: it has grown and waits for the release
	static bool sEnabled;       // 0x60e64b: set once the game has loaded (Enable); no pause before
	static Anim* sButtonAnim;   // 0x670188: "images/menu/ingame_buttons", kept by PreloadInGameButtons

	PauseWin();              // 0x454ef0
	~PauseWin();             // 0x455050 (body 0x455070)
	void Notify(ObjBase* theObj) override;      // 0x455080

	static void PauseForFocusLoss(); // 0x455920
	static bool IsBlocking();        // 0x455a10
	static void Enable() { sEnabled = true; }   // 0x455a30
	static void ResumeOnRelease();   // 0x455a40

private:
	void BuildTracks();      // 0x455570
	void BuildButton();      // 0x4550f0
};

void PreloadInGameButtons(); // 0x4558f0: the picture of the pause window's button (the in-level menu loads its own)

} // namespace Sexy
