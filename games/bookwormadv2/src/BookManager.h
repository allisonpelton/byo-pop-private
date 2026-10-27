// Sexy::BookManager (0x100 bytes, constructor V1:0x429cf0, vtable 0x6f8b44): the game screen between the battles, a
// LuaWidget whose Lua object (gBookManager, scripts/bookmanager) is the adventure: the book, the map of its
// chapters, the battles and what happens after them. The C++ part keeps the screen's three buttons (back to the
// menu, continue, options; their presses go to the app), the buttons of the mini-game tutorials (theirs to the
// Lua object), and calls the scripts' overlay functions after its Lua Draw. The app makes it in InitGameScreen.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "LuaWidgets.h"

namespace Sexy {

class FadeButton;
class GameApp;

class BookManager : public LuaWidget {
public:
	enum { BUTTON_BACK, BUTTON_CONTINUE, BUTTON_OPTIONS };  // the scripts' ButtonId_*
	enum { STATE_NORMAL, STATE_BOOK_SLIDE_OUT, STATE_BOOK_SLIDE_IN, STATE_MAP_SLIDE_OUT, STATE_MAP_SLIDE_IN };

	GameApp* mApp;                                // +0xcc
	std::map<int, FadeButton*> mButtons;          // +0xd0, by BUTTON_*
	std::vector<std::string> mOverlayCallbacks;   // +0xdc: the scripts' functions called with the Graphics after Draw
	std::vector<FadeButton*> mMGButtons;          // +0xec
	int mState = STATE_NORMAL;                    // +0xfc

	explicit BookManager(GameApp* theApp);
	~BookManager() override;  // 0x421030

	const char* LuaGetRegisteredName() override { return "BookManager"; }
	static void RegisterLua();

	// The scripts' methods
	void PositionButton(int theId, int theX, int theY);             // V1:0x42a840: one of mButtons or mMGButtons
	void SetButtonState(int theId, bool theVisible, bool theEnabled);  // V1:0x42a930
	int GetButtonX(int theId);   // 0x421c40, -1 for no button
	int GetButtonY(int theId);   // 0x421cc0
	int GetButtonW(int theId);   // 0x421d40
	int GetButtonH(int theId);   // 0x421dc0
	void MakeMGButton(int theId);  // 0x421390
	void GiveKeyboardFocus();      // V1:0x42b460
	void RemoveMGButtons();        // V1:0x42a620
	void AddOverlayCallback(std::string theFunction);     // V1:0x42b270
	void DisableMGButtons();                              // 0x421510
	void RemoveOverlayCallback(std::string theFunction);  // V1:0x42b2d0
	// V1:0x42af80: hides the screen and its buttons and slides the treasure screen in, its buttons where these were
	void DoTreasureTransition();
	void DisableContinueButton();  // 0x421360
	int GetState() { return mState; }          // V1:0x429cb0
	void SetState(int theState) { mState = theState; }  // V1:0x429ca0
	// V1:0x42abf0
	bool IsContinueEnabled();

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x421140: makes the buttons, then the Lua Setup
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x421570: gBookManager = nil
	void Update() override;                // 0x4219b0
	void UpdateF(float theFrac) override;  // 0x421a30
	void Draw(Graphics* g) override;       // 0x4217d0
	void OrderInManagerChanged() override; // 0x4220b0
	void SetDisabled(bool isDisabled) override { Widget::SetDisabled(isDisabled); }  // 0x48e5c0
	void KeyDown(KeyCode theKey) override; // 0x421ab0: Enter presses continue
	void MouseMove(int x, int y) override; // 0x421b10
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x421bf0
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;    // 0x421b80

private:
	// Not drawn, updated or told of the mouse: hidden and disabled (and, for the mouse, not in STATE_NORMAL),
	// or behind a chapter's loading screen
	bool IsAsleep(bool theCheckState) const;
	FadeButton* FindButton(int theId);
};

}  // namespace Sexy
