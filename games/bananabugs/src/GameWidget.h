// GameWidget: the game's one widget (0x42f230..0x42fd80 in the original). It runs the screens in turn (PopCap's
// logo, the loading bar while the pictures load, then the main menu, the levels and the map behind the doors of the
// Transition) and hands the mouse and the keys to the scene graph (ObjBase).
#pragma once

#include "Widget.h"

namespace Sexy
{

class Board;
class GameApp;
class LoadGame;
class MainMenu;
class MetaGame;
class PopCap;
class Transition;

// Which screen runs (GameWidget::mScreen)
enum GameScreen
{
	SCREEN_POPCAP = 0,
	SCREEN_LOADING = 1,
	SCREEN_TO_MENU = 2,      // the doors close on whatever was showing, and open on the main menu
	SCREEN_MENU = 3,
	SCREEN_TO_PLAY = 4,      // the doors close on the menu, and open on a level or the map
	SCREEN_LEVEL = 5,
	SCREEN_MAP = 7,
};

class GameWidget : public Widget
{
public:
	int mScreen;             // +0x8c: GameScreen
	Transition* mTransition; // +0x90
	Board* mBoard;           // +0x94
	MainMenu* mMainMenu;     // +0x98
	MetaGame* mMetaGame;     // +0x9c
	int mFrame;              // +0xa4: the scene's frame count
	int mInputLock;          // +0xa8: frames left during which the mouse is ignored
	GameApp* mApp;           // +0xac
	int mUnusedB0;           // +0xb0: set to 0 with the frame count; nothing reads it
	PopCap* mPopCap;         // +0xb4
	LoadGame* mLoadGame;     // +0xb8
	int mPreloadStep;        // +0xbc

	GameWidget(GameApp* theApp);
	~GameWidget();

	void Update() override;
	void Draw(Graphics* g) override;
	void KeyChar(SexyChar theChar) override;
	void KeyDown(KeyCode theKey) override;
	void MouseMove(int x, int y) override;
	void MouseDown(int x, int y, int theClickCount) override;
	using Widget::MouseDown;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseDrag(int x, int y) override;

	bool Preload(int theStep);
	void LockInput() { mInputLock = 45; } // 0x42fcb0
	void CentreCursor();
};

} // namespace Sexy
