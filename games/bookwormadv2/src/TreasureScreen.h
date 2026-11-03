// Sexy::TreasureScreen (0xe4 bytes, constructor V1:0x4a4190, vtable 0x6ffecc): the screen of the treasures won after
// a battle, a LuaWidget whose Lua object (gTreasureScreen, scripts/treasurescreen) does the screen. It slides in
// over the book (BookManager::DoTreasureTransition) with its own back, continue and options buttons where the
// book's were; their presses go to the app. The app makes it in InitGameScreen, hidden.
#pragma once

#include <map>

#include "LuaWidgets.h"

namespace Sexy {

class FadeButton;
class GameApp;

class TreasureScreen : public LuaWidget {
public:
	enum { BUTTON_BACK = 10, BUTTON_CONTINUE = 11, BUTTON_OPTIONS = 12 };  // the scripts' ButtonId_*
	enum { STATE_NORMAL, STATE_SLIDING_OUT, STATE_SLIDING_IN };

	std::map<int, FadeButton*> mButtons;  // +0xcc, by BUTTON_*
	GameApp* mApp;                        // +0xd8
	int mState = STATE_NORMAL;            // +0xdc
	bool mPreventRightClick = false;      // +0xe0: right clicks do not reach the Lua object

	explicit TreasureScreen(GameApp* theApp);
	~TreasureScreen() override;  // 0x48d8d0: the Lua object's Destructor first

	const char* LuaGetRegisteredName() override { return "TreasureScreen"; }
	static void RegisterLua();

	// The scripts' methods
	void PositionButton(int theId, int theX, int theY);                // V1:0x4a4aa0
	void SetButtonState(int theId, bool theVisible, bool theEnabled);  // 0x48de80
	int GetButtonX(int theId);  // 0x48e120, -1 for no button
	int GetButtonY(int theId);  // 0x48e1b0
	int GetButtonW(int theId);  // 0x48e240
	int GetButtonH(int theId);  // 0x48e2d0
	void PreventRightClick(bool thePrevent) { mPreventRightClick = thePrevent; }  // V1:0x4a4180
	// V1:0x4a4ff0: back to the book, its buttons where these are, and the book's Lua DoChapterTransition
	void DoChapterTransition();
	// V1:0x4a5270: the arena's checkpoint: the buttons where the screen shows them in full, then the script's
	// StartFullyDisplayed(false)
	void StartFullyDisplayed();
	int GetState() { return mState; }                   // V1:0x4a4170
	void SetState(int theState) { mState = theState; }  // V1:0x4a4160

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x48d9b0: makes the buttons, then Setup
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x48dd20: gTreasureScreen = nil
	void Update() override;                    // 0x48e000
	void Draw(Graphics* g) override;           // 0x48df20
	void DrawOverlay(Graphics* g) override;    // 0x48df90
	void KeyDown(KeyCode theKey) override;     // 0x48e560: Enter presses continue (only that)
	void MouseMove(int x, int y) override;     // 0x48e060
	void MouseDrag(int x, int y) override;     // 0x48e060
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x48e0a0

private:
	// Not drawn or updated: hidden and disabled (and, for the mouse, not in STATE_NORMAL), or behind a
	// chapter's loading screen
	bool IsAsleep(bool theCheckState) const;
	int GetButtonRect(int theId, int theField);
};

}  // namespace Sexy
