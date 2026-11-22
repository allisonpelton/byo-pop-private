// AskWin: the game's question window (0x402920..0x403f70 in the original), a panel of two lines of text with a yes
// and a no button, one OK button, or yes, no and a cancel button, that grows in, takes the mouse, and shrinks away
// once a button is pressed. The caller asks TakeChoice each frame until it answers.
#pragma once

#include "MenuObj.h"

namespace Sexy
{

// What the window asks for (AskWin::SetText's mode)
enum AskMode
{
	ASK_MODE_YES_NO = 0,
	ASK_MODE_OK = 1,
	ASK_MODE_YES_NO_CANCEL = 2,
};

// The answers (AskWin::mChoice)
enum AskChoice
{
	ASK_CHOICE_NONE = -1,
	ASK_CHOICE_YES = 0,
	ASK_CHOICE_NO = 1,
	ASK_CHOICE_OTHER = 2,    // OK or cancel
};

const int ASK_MAX_BUTTONS = 4;
const int ASK_TEXT_SIZE = 0x51;

class AskWin : public MenuObj
{
public:
	ObjBase* mButtons[ASK_MAX_BUTTONS]; // +0x334: yes, no, cancel; the fourth is never used
	int mChoice;             // +0x344: AskChoice
	char mFirstText[ASK_TEXT_SIZE];  // +0x348
	char mSecondText[ASK_TEXT_SIZE]; // +0x399
	bool mClosed;            // +0x3ea: the close track ended
	bool mChoiceTaken;       // +0x3eb: TakeChoice has given the choice and started the close track
	int mButtonCount;        // +0x3ec
	float mSavedHoverMinZ;   // +0x3f0: the mouse's lowest depth before this window raised it

	static Anim* sYesNoAnim;   // 0x60e190: the yes and no buttons
	static Anim* sButtonsAnim; // 0x60e194: "usrsel_buttons", for the OK button
	static Anim* sCancelAnim;  // 0x60e198

	AskWin();                // 0x402920
	~AskWin();               // 0x4029b0
	void Notify(ObjBase* theObj) override;      // 0x402be0
	bool Update() override;                     // 0x402c10
	bool MouseUp(ObjBase* theObj) override;     // 0x402cc0

	void SetText(const char* theFirstText, int theMode, const char* theSecondText); // 0x4029f0
	int TakeChoice();        // 0x402d50
	bool IsClosed() { return mClosed; }         // 0x402d80
	void BuildYesNoButtons();                   // 0x402d90
	void BuildOkButton();                       // 0x403310
	void BuildCancelButton();                   // 0x4037a0
	void BuildWindowTracks();                   // 0x403c30
};

void PreloadAskWin(); // 0x403f10

} // namespace Sexy
