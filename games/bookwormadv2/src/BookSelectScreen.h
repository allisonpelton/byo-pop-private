// Sexy::BookSelectScreen (0x184 bytes, constructor 0x423e60, code 0x423e60-0x425630): the screen where the replay of a
// finished adventure begins, a LuaWidget whose Lua object (scripts/bookselectscreen, which draws the badges of the
// player's Hall of Fame through a BadgeManager) draws the badges and tooltips over what this class draws: the book
// page, the three books of Volume 2 (4, 5 and 6) as pictures to point at and pick, and under them the name and the
// description of the book the mouse is over (or the one picked) with a fade between one text and the next.
//
// The curtain's state 9 (GameApp::CurtainDone) makes it once the curtain has closed over the main menu, and state 10
// (the book chosen) or the back button takes it away (GameApp::RemoveBookSelectScreen). Its three buttons are
// FadeButtons that the app hears (id 0 back, 1 options, 2 begin, which stays disabled until a book is picked), so the
// original's own ButtonDepress for this screen (0x4254e0, which no button reaches) is not ported.
#pragma once

#include <string>
#include <vector>

#include "LuaWidgets.h"
#include "Rect.h"

namespace Sexy {

class FadeButton;
class GameApp;
class Image;

class BookSelectScreen : public LuaWidget {
public:
	enum { BUTTON_BACK = 0, BUTTON_OPTIONS = 1, BUTTON_BEGIN = 2 };

	// A book to pick (0x38 bytes plus its rectangle): where the mouse picks it, its picture (three cels: plain,
	// pointed at, picked), the picture's place, and the name and description that show while it is pointed at or picked
	struct Item {
		Rect mRect;
		Image* mImage = nullptr;
		int mId = 0;  // the book's number, 4 to 6; the same as the profile's CurrentBook
		int mX = 0;
		int mY = 0;
		std::string mTitle;
		std::string mText;
	};

	GameApp* mApp;                // +0xd4
	std::string mHeader;          // +0xd8: the shown name (or the instruction), drawn large over the page's foot
	std::string mText;            // +0xf4: the shown description, in the box below
	int mAlpha = 255;             // +0x110: how much of them shows
	std::string mPrevHeader;      // +0x114: what they replace, fading out
	std::string mPrevText;        // +0x130
	int mPrevAlpha = 0;           // +0x14c
	FadeButton* mBeginButton;     // +0x150, id 2
	FadeButton* mBackButton;      // +0x154, id 0
	FadeButton* mOptionsButton;   // +0x158, id 1
	int mHover = 0;               // +0x160: the book the mouse is over (0 for none)
	int mSelected = 0;            // +0x164: the book picked (0 for none)
	int mMouseX = 0;              // +0x168
	int mMouseY = 0;              // +0x16c
	std::vector<Item> mItems;     // +0x174
	bool mNeedsReplayDialog;      // +0x180: the welcome message of the first replay is still to come

	explicit BookSelectScreen(GameApp* theApp);
	~BookSelectScreen() override;  // 0x4242c0: its widgets (the buttons) go with it

	const char* LuaGetRegisteredName() override { return "BookSelectScreen"; }
	static void RegisterLua();  // the original registered the class in the constructor (0x425630), every time

	void AddedToManager(WidgetManager* theWidgetManager) override;  // 0x424450: the three buttons, hidden
	void Update() override;                                         // 0x424f00
	void Draw(Graphics* g) override;                                // 0x424640
	void DrawOverlay(Graphics* g) override;                         // 0x424c70
	void KeyChar(SexyChar theChar) override;                        // 0x424ea0: Enter begins with the book picked
	void MouseMove(int x, int y) override;                          // 0x424cd0
	void MouseUp(int x, int y, int theClickCount) override;         // 0x424d40: picks the book pointed at
};

}  // namespace Sexy
