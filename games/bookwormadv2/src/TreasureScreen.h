// Sexy::TreasureScreen (0xe4 bytes, constructor 0x4a4190, vtable 0x7b176c): the screen of the treasures won after
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
	~TreasureScreen() override;  // 0x4a4520: the Lua object's Destructor first

	const char* LuaGetRegisteredName() override { return "TreasureScreen"; }
	static void RegisterLua();

	// The scripts' methods
	void PositionButton(int theId, int theX, int theY);                // 0x4a4aa0
	void SetButtonState(int theId, bool theVisible, bool theEnabled);  // 0x4a4ad0
	int GetButtonX(int theId);  // 0x4a4db0, -1 for no button
	int GetButtonY(int theId);  // 0x4a4e40
	int GetButtonW(int theId);  // 0x4a4ed0
	int GetButtonH(int theId);  // 0x4a4f60
	void PreventRightClick(bool thePrevent) { mPreventRightClick = thePrevent; }  // 0x4a4180
	// 0x4a4ff0: back to the book, its buttons where these are, and the book's Lua DoChapterTransition
	void DoChapterTransition();
	// 0x4a5270: the arena's checkpoint: the buttons where the screen shows them in full, then the script's
	// StartFullyDisplayed(false)
	void StartFullyDisplayed();
	int GetState() { return mState; }                   // 0x4a4170
	void SetState(int theState) { mState = theState; }  // 0x4a4160

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x4a4600: makes the buttons, then Setup
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x4a4970: gTreasureScreen = nil
	void Update() override;                    // 0x4a4c50
	void Draw(Graphics* g) override;           // 0x4a4b70
	void DrawOverlay(Graphics* g) override;    // 0x4a4be0
	void KeyDown(KeyCode theKey) override;     // 0x4a51f0: Enter presses continue (only that)
	void MouseMove(int x, int y) override;     // 0x4a4cb0
	void MouseDrag(int x, int y) override;     // 0x4a4cf0
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x4a4d30

private:
	// Not drawn or updated: hidden and disabled (and, for the mouse, not in STATE_NORMAL), or behind a
	// chapter's loading screen
	bool IsAsleep(bool theCheckState) const;
	int GetButtonRect(int theId, int theField);
};

}  // namespace Sexy
