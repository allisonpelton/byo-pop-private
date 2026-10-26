// Sexy::Board (0x1d4 bytes, constructor 0x4168a0, destructor 0x416f40, vtable 0x7ab164; code 0x4161d0-0x41f0d0):
// the battle's grid of letter tiles and the buttons under it, a LuaWidget whose Lua object (gBoard, scripts/board)
// does the battle around it with the battle engine (gBattleEngine, the battle's rules in BattleEngine.lua) and the
// tile engine (gTileEngine, the tiles' letters and looks in TileEngine.lua). The app makes it in StartBattle, a
// mini-game's in StartMiniGame: a mini-game has no battle engine, its tiles lie in the grid from the start, and its
// script (gMiniGame) hears what the battle engine would.
#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>

#include "LuaWidgets.h"

namespace Sexy {

class FadeButton;
class GameApp;
class Graphics;
class Image;
class TooltipWidget;
class Tile;
class Widget;
class WidgetManager;

class Board : public LuaWidget {
public:
	enum {
		BUTTON_MENU = 0,
		BUTTON_ATTACK = 1,
		BUTTON_SCRAMBLE = 2,
	};

	std::vector<std::vector<bool>> mGridCells;  // +0xd0: [row][col], a cell that holds a tile
	bool mStarting = false;                     // +0xe0: Init's tiles still falling in (no clicks)
	bool mImmuneLook = false;                   // +0xe1: the attack button shows the enemy's immunity
	bool mLevelupOverlay = false;               // +0xe2: the battle engine's level-up overlay is up
	bool mEndingChapter = false;                // +0xe3: the battle engine is in BE_END_CHAPTER (and DoAltarScene)
	bool mShowTooltips = true;                  // +0xe4 (AllowTooltips)
	bool mFlagE5 = false;                       // +0xe5
	bool mScrambling = false;                   // +0xe6: the letters change one by one (no hilite)
	bool mIsMiniGame = false;                   // +0xe7
	bool mNoTilesWhilePaused = false;           // +0xe8: a mini-game's (InitMiniGame's last argument)
	bool mFlagE9 = false;                       // +0xe9: the button click that unpaused, to be ignored
	bool mScrambleWasEnabled = false;           // +0xea: Pause disabled an enabled scramble button
	bool mDrawArrowHintsAboveOverlay = false;   // +0xeb
	bool mZoomingIn = false;                    // +0xec
	bool mZooming = false;                      // +0xed: the zoom effect is on (DoZoom)
	bool mTMEnemyAttacking = false;             // +0xee (SetTMEnemyAttacking): the attack button's effect stays off
	bool mShowBeatdownMultiplier = false;       // +0xef
	int mGridX = 301;                           // +0xf0
	int mGridY = 309;                           // +0xf4
	int mScrambleStep = 0;                      // +0xf8: the letters change every mScrambleStep updates
	int mPauseCount = 0;                        // +0xfc
	int mPauseNoBannerCount = 0;                // +0x100: pauses without the "paused" banner
	int mUnpauseUpdateCnt = -1;                 // +0x104: the update of the click that unpaused
	int mRandSeed = 0;                          // +0x108: a Rand() of each submitted word
	int mField10C = 0;                          // +0x10c: the space bar's last pause (updates times 10)
	int mGridWidth = 4;                         // +0x110: columns
	int mGridHeight = 4;                        // +0x114: rows
	int mZoomX = 0;                             // +0x118 (SetZoomCoords)
	int mZoomY = 0;                             // +0x11c
	int mZoomHoldCount = 0;                     // +0x120: updates zoomed in
	int mPauseFade = 0;                         // +0x124: the pause's darkening, up to 140
	int mMuteCount = 0;                         // +0x128: the pauses that muted the app
	float mZoom = 0.0f;                         // +0x12c: 0 to 1
	std::string mString130;                     // +0x130: cleared with each tile selected
	std::string mString14C;                     // +0x14c
	// +0x168: the disabled attack button's. Nothing adds it to a widget manager, so the original never showed it.
	TooltipWidget* mTooltip = nullptr;
	GameApp* mApp;                              // +0x16c
	std::map<int, FadeButton*> mButtons;        // +0x170
	std::list<Tile*> mTiles;                    // +0x17c: the tiles in the grid
	std::list<Tile*> mSelected;                 // +0x188: the tiles of the word, in order
	std::list<Tile*> mNewTiles;                 // +0x194: tiles placed in the grid, still falling into it
	std::list<Tile*> mClones;                   // +0x1a0: copies of tiles (CloneTile)
	Tile* mHiliteTile = nullptr;                // +0x1ac: the tile under the mouse
	std::vector<std::string> mOverlayCallbacks; // +0x1b4: Lua functions that draw over the board
	std::vector<FadeButton*> mOverlayDisabled;  // +0x1c4: the buttons an overlay disabled
	bool mRestoringGrid = false;                // +0x1d0: Init sets the tiles' letters

	Board(GameApp* theApp, bool theIsMiniGame);
	~Board() override;

	const char* LuaGetRegisteredName() override { return "Board"; }
	static void RegisterLua();

	// 0x4179a0: a battle's grid, its engines' scripts, and the letters (the first game's, a chapter's, or the
	// saved grid's)
	void Init();
	// 0x418120: a mini-game's grid of theCols x theRows at (x, y), theXGap and theYGap pixels between its tiles,
	// its letters from theLetters (row by row) when not empty, else random ones; then the scripts' own
	// InitMiniGame(theName) (the mini-game's class), which makes the mini-game and loads its resources
	void InitMiniGame(const std::string& theName, int x, int y, int theXGap, int theYGap, int theCols, int theRows,
	                  const std::string& theLetters, bool theNoTilesWhilePaused);
	static void LoadScripts();  // 0x4185d0

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x41aba0
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x41ae30
	void Update() override;                                             // 0x41bac0
	void UpdateF(float theFrac) override;                               // 0x41c0d0
	void Draw(Graphics* g) override;                                    // 0x41b2f0
	void DrawOverlay(Graphics* g) override;                             // 0x41af90
	void OrderInManagerChanged() override;                              // 0x41ef10
	void Resize(int x, int y, int w, int h) override;                   // 0x41af60
	void GotFocus() override;                                           // 0x41aac0
	void LostFocus() override;                                          // 0x41ab30
	void KeyChar(SexyChar theChar) override;                            // 0x41d030
	void KeyDown(KeyCode theKey) override;                              // 0x41d020 (nothing)
	void MouseMove(int x, int y) override;                              // 0x41c7e0
	using LuaWidget::MouseDown;
	using LuaWidget::MouseUp;
	void MouseDown(int x, int y, int theClickCount) override;           // 0x41c430
	void MouseUp(int x, int y, int theLastDownButtonId) override;       // 0x41c1f0

	// ButtonListener (LuaWidgetTplBase's, at +0xb0): the board's buttons
	void ButtonPress(int theId, int theClickCount) override;  // 0x41c9a0
	void ButtonDepress(int theId) override;                   // 0x41ca80
	void ButtonMouseEnter(int theId) override;                // 0x41dda0
	void ButtonMouseLeave(int theId) override;                // 0x41de40

	int GetGridWidth() const { return mGridWidth; }
	int GetGridHeight() const { return mGridHeight; }
	bool IsMiniGame() const { return mIsMiniGame; }
	bool IsGridCell(int theCol, int theRow) const;   // 0x418dc0
	Tile* GridGetTile(int theCol, int theRow);       // 0x418e10
	// 0x418f00: the tiles around a cell (into theTiles); whether there are any
	bool GetAdjacentTiles(int theCol, int theRow, std::list<Tile*>& theTiles);
	// 0x4192f0: a new tile in a cell, above it to fall in (a battle's) or in it (a mini-game's)
	Tile* AddTile(Tile* theTile, int theCol, int theRow, int theXGap = 0, int theYGap = 0);
	// 0x419460: a tile to fall from one cell into another
	void MoveTile(Tile* theTile, int theOldCol, int theOldRow, int theNewCol, int theNewRow);
	// 0x419560: the tile of a cell out of the grid (to the tile engine)
	void RemoveTile(int theCol, int theRow);
	void RehupMouse();                               // 0x41ec10
	void TileDeleted(Tile* theTile);                 // 0x41ee90
	void AddClone(Tile* theTile) { mClones.push_back(theTile); }
	bool IsClonedTile(Tile* theTile);                // 0x4161f0

	// the word
	bool SelectTile(Tile* theTile);                  // 0x4195f0
	void DeselectTile(Tile* theTile);                // 0x4199a0
	void DeselectAllTiles();                         // 0x419e40
	// 0x418fb0: the word's tiles slide to their places above the grid (theTile, theSelected: a mini-game's news)
	void LayoutWord(Tile* theTile, bool theSelected);
	static std::string GetWord(const std::list<Tile*>& theTiles);  // 0x418c70
	// 0x41a4d0: a word of the list (and the easter egg of another PopCap game's name, unless theNoEgg)
	// theFlags, when given, gets the word's Lex flags (0 for what is not a word)
	bool IsValidWord(const std::string& theWord, bool theNoEgg, int* theFlags = nullptr);
	// 0x41a1a0, 0x41a1d0 and 0x41a200: what the word can be (the scripts ask; the word must be in capitals)
	bool IsNoun(const std::string& theWord);
	bool IsVerb(const std::string& theWord);
	bool IsAdjective(const std::string& theWord);
	bool ValidateWord(const std::string& theWord) { return IsValidWord(theWord, false); }  // 0x41a5b0
	void CheckIfWordStillValid();                    // 0x419840
	void UpdateHilite(int x, int y);                 // 0x418ad0

	// the buttons
	void CreateButtons();                            // 0x4186e0
	void LayoutButtons();                            // 0x4189e0
	void SetAttackDisabled(bool isDisabled, bool keepLook);  // 0x41a070
	void SetAttackBtnPos(int x, int y);              // 0x4189b0
	void ShowImmuneAttack();                         // 0x41a3e0
	void SetScrambleDisabled(bool isDisabled);       // 0x41a1d0
	void SetScrambleVisible(bool isVisible);         // 0x41a260
	void ScrambleUseNormalForDisabled(bool on);      // 0x41a2d0
	bool Scramble();                                 // 0x41a620
	void SubmitWord();                               // 0x41cb90
	// 0x41d3a0: the tiles fall into the word's gaps and new ones fill the grid
	void Refill();
	void SetAttackImage(Image* theImage);            // 0x41de80 (ChangeAttackBtnImage)
	void EnableAttackBtnPAM(bool on);                // 0x41a020: the attack button's glow plays (or not)
	void DisableAttackPAM(bool isPlaying);           // 0x41ef30: the glow goes on or holds

	void Pause(bool thePause, bool theNoBanner);     // 0x41a700
	void DoLevelUp(bool on);                         // 0x4162e0: the battle engine's level-up overlay
	void DoAltarScene(bool on) { mEndingChapter = on; }  // 0x413e90
	void SetTMEnemyAttacking(bool on) { mTMEnemyAttacking = on; }  // 0x416320
	void DisableButtons();                           // 0x41e780: for an overlay
	void RestoreButtons();                           // 0x41e900
	bool IsPaused() const { return mPauseCount > 0; }  // 0x416260
	void DoZoom(bool on);                            // 0x41ef60
	bool DoingZoomEffect() const { return mZooming; }  // 0x4162b0
	float GetZoomValue() const { return mZoom; }     // 0x4162c0
	void SetZoomCoords(int x, int y) { mZoomX = x; mZoomY = y; }  // 0x41f070
	void AddOverlayCallback(const std::string& theFunction);     // 0x41e9b0
	void RemoveOverlayCallback(const std::string& theFunction);  // 0x41ea20
	bool HasOverlays() const { return !mOverlayCallbacks.empty(); }  // 0x416270
	bool CursorIsOverGrid();                         // 0x41eba0
	void AllowTooltips(bool on);  // 0x41e750: off hides the app's tooltip under the mouse
	void DrawArrowHintsAboveOverlay(bool on) { mDrawArrowHintsAboveOverlay = on; }  // 0x416310
	void DisableAndHide();                           // 0x41f090
	int GetWordTileSize();                           // 0x4162d0

private:
	// the tutorial's arrows hear of a click (ArrowHintManager::Click), or hide while zooming (its mDontDraw)
	void ArrowHintsClick(int x, int y, int theClickCount);
	void ArrowHintsHide(bool isHidden);
};

extern Board* gBoard;  // 0x823b10

}  // namespace Sexy
