// Sexy::Board (0x1d4 bytes, constructor V1:0x4168a0, destructor 0x4100c0, vtable 0x6f86d4; code 0x40f780-V1:0x41f0d0):
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

	// V1:0x4179a0: a battle's grid, its engines' scripts, and the letters (the first game's, a chapter's, or the
	// saved grid's)
	void Init();
	// V1:0x418120: a mini-game's grid of theCols x theRows at (x, y), theXGap and theYGap pixels between its tiles,
	// its letters from theLetters (row by row) when not empty, else random ones; then the scripts' own
	// InitMiniGame(theName) (the mini-game's class), which makes the mini-game and loads its resources
	void InitMiniGame(const std::string& theName, int x, int y, int theXGap, int theYGap, int theCols, int theRows,
	                  const std::string& theLetters, bool theNoTilesWhilePaused);
	static void LoadScripts();  // V1:0x4185d0

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x415540
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x4157d0
	void Update() override;                                             // 0x416640
	void UpdateF(float theFrac) override;                               // 0x416c70
	void Draw(Graphics* g) override;                                    // 0x415c70
	void DrawOverlay(Graphics* g) override;                             // 0x415930
	void OrderInManagerChanged() override;                              // 0x41a010
	void Resize(int x, int y, int w, int h) override;                   // 0x415900
	void GotFocus() override;                                           // 0x415460
	void LostFocus() override;                                          // 0x4154d0
	void KeyChar(SexyChar theChar) override;                            // 0x417d70
	void KeyDown(KeyCode theKey) override;                              // 0x4094a0 (nothing)
	void MouseMove(int x, int y) override;                              // 0x4173a0
	using LuaWidget::MouseDown;
	using LuaWidget::MouseUp;
	void MouseDown(int x, int y, int theClickCount) override;           // 0x416ff0
	void MouseUp(int x, int y, int theLastDownButtonId) override;       // 0x416db0

	// ButtonListener (LuaWidgetTplBase's, at +0xb0): the board's buttons
	void ButtonPress(int theId, int theClickCount) override;  // 0x417560
	void ButtonDepress(int theId) override;                   // 0x417640
	void ButtonMouseEnter(int theId) override;                // 0x418920
	void ButtonMouseLeave(int theId) override;                // 0x4189c0

	int GetGridWidth() const { return mGridWidth; }
	int GetGridHeight() const { return mGridHeight; }
	bool IsMiniGame() const { return mIsMiniGame; }
	bool IsGridCell(int theCol, int theRow) const;   // V1:0x418dc0
	Tile* GridGetTile(int theCol, int theRow);       // V1:0x418e10
	// V1:0x418f00: the tiles around a cell (into theTiles); whether there are any
	bool GetAdjacentTiles(int theCol, int theRow, std::list<Tile*>& theTiles);
	// V1:0x4192f0: a new tile in a cell, above it to fall in (a battle's) or in it (a mini-game's)
	Tile* AddTile(Tile* theTile, int theCol, int theRow, int theXGap = 0, int theYGap = 0);
	// V1:0x419460: a tile to fall from one cell into another
	void MoveTile(Tile* theTile, int theOldCol, int theOldRow, int theNewCol, int theNewRow);
	// 0x4126b0 (the Lua method GridRemoveTile): the tile of a cell out of the grid (to the tile engine)
	void RemoveTile(int theCol, int theRow);
	// 0x412750 (the Lua method RemoveTile): the tile out of the board's list only; the script deletes it itself
	void RemoveTileFromList(Tile* theTile) { mTiles.remove(theTile); }
	void RehupMouse();                               // 0x419d60
	void TileDeleted(Tile* theTile);                 // 0x419f90
	void AddClone(Tile* theTile) { mClones.push_back(theTile); }
	bool IsClonedTile(Tile* theTile);                // V1:0x4161f0

	// the word
	bool SelectTile(Tile* theTile);                  // 0x412770
	void DeselectTile(Tile* theTile);                // 0x414260
	void DeselectAllTiles();                         // 0x414780
	// V1:0x418fb0: the word's tiles slide to their places above the grid (theTile, theSelected: a mini-game's news)
	void LayoutWord(Tile* theTile, bool theSelected);
	static std::string GetWord(const std::list<Tile*>& theTiles);  // V1:0x418c70
	// V1:0x41a4d0: a word of the list (and the easter egg of another PopCap game's name, unless theNoEgg)
	// theFlags, when given, gets the word's Lex flags (0 for what is not a word)
	bool IsValidWord(const std::string& theWord, bool theNoEgg, int* theFlags = nullptr);
	// 0x41a1a0, 0x41a1d0 and 0x41a200: what the word can be (the scripts ask; the word must be in capitals)
	bool IsNoun(const std::string& theWord);
	bool IsVerb(const std::string& theWord);
	bool IsAdjective(const std::string& theWord);
	bool ValidateWord(const std::string& theWord) { return IsValidWord(theWord, false); }  // V1:0x41a5b0
	void CheckIfWordStillValid();                    // V1:0x419840
	void UpdateHilite(int x, int y);                 // 0x411c30

	// the buttons
	void CreateButtons();                            // V1:0x4186e0
	void LayoutButtons();                            // 0x411b40
	void SetAttackDisabled(bool isDisabled, bool keepLook);  // 0x414ad0
	void SetAttackBtnPos(int x, int y);              // V1:0x4189b0
	void ShowImmuneAttack();                         // V1:0x41a3e0
	void SetScrambleDisabled(bool isDisabled);       // V1:0x41a1d0
	void SetScrambleVisible(bool isVisible);         // 0x414d00 (Volume 2's: the button is not drawn)
	void SetAttackVisible(bool isVisible);           // 0x414c30: the attack button is disabled and not drawn
	void ScrambleUseNormalForDisabled(bool on);      // V1:0x41a2d0
	bool Scramble();                                 // V1:0x41a620
	void SubmitWord();                               // V1:0x41cb90
	// 0x4181c0, a Lua method too: the tiles fall into the gaps and new ones fill the grid; with theCreateBonus the
	// battle engine hears of the new tiles (CreateBonusTiles), the word's submission says so, the fire tile's burn does not
	void AddNewTiles(bool theCreateBonus);
	void SetAttackImage(Image* theImage);            // V1:0x41de80 (ChangeAttackBtnImage)
	void EnableAttackBtnPAM(bool on);                // V1:0x41a020: the attack button's glow plays (or not)
	void DisableAttackPAM(bool isPlaying);           // V1:0x41ef30: the glow goes on or holds

	void Pause(bool thePause, bool theNoBanner);     // 0x4150a0
	void DoLevelUp(bool on);                         // V1:0x4162e0: the battle engine's level-up overlay
	void DoAltarScene(bool on) { mEndingChapter = on; }  // V1:0x413e90
	void SetTMEnemyAttacking(bool on) { mTMEnemyAttacking = on; }  // V1:0x416320
	bool GetTMEnemyAttacking() { return mTMEnemyAttacking; }       // 0x40f8d0
	bool IsScrambling() { return mScrambling; }                    // 0x415090
	void DisableButtons();                           // V1:0x41e780: for an overlay
	void RestoreButtons();                           // V1:0x41e900
	bool IsPaused() const { return mPauseCount > 0; }  // V1:0x416260
	void DoZoom(bool on);                            // V1:0x41ef60
	bool DoingZoomEffect() const { return mZooming; }  // V1:0x4162b0
	float GetZoomValue() const { return mZoom; }     // V1:0x4162c0
	void SetZoomCoords(int x, int y) { mZoomX = x; mZoomY = y; }  // V1:0x41f070
	void AddOverlayCallback(const std::string& theFunction);     // V1:0x41e9b0
	void RemoveOverlayCallback(const std::string& theFunction);  // V1:0x41ea20
	bool HasOverlays() const { return !mOverlayCallbacks.empty(); }  // V1:0x416270
	bool CursorIsOverGrid();                         // V1:0x41eba0
	void AllowTooltips(bool on);  // V1:0x41e750: off hides the app's tooltip under the mouse
	void DrawArrowHintsAboveOverlay(bool on) { mDrawArrowHintsAboveOverlay = on; }  // V1:0x416310
	void DisableAndHide();                           // V1:0x41f090
	int GetWordTileSize();                           // V1:0x4162d0

private:
	// the tutorial's arrows hear of a click (ArrowHintManager::Click), or hide while zooming (its mDontDraw)
	void ArrowHintsClick(int x, int y, int theClickCount);
	void ArrowHintsHide(bool isHidden);
};

extern Board* gBoard;  // 0x823b10

}  // namespace Sexy
