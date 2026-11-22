// LevelObj: the rules of a level (0x439700..0x43f430 in the original; 0x150a4 bytes there). It owns the grid of
// ground cells and pipes (gFillGrid), the rope, the bugs' spawn table, the texts and effects of a level, and runs the
// level's states: the intro, the cells settling in, play, the stats after a win, the game over and the victory. A Board
// makes one; ObjBase::sLevel points to it while it lives, and the other classes reach it through the statics below.
//
// The class is complete. What it makes and calls but is not translated yet stands in Pending.h: the bugs (BugObj, one
// stand-in for every kind, and BugObj's list, which the stand-ins leave empty so the placing of bugs finds none), the
// pipes, the intro, the in-level menu, the stats and the victory scene.
#pragma once

#include "FillObj.h"
#include "ObjBase.h"
#include "Pending.h"

namespace Sexy
{

class DDImage;
class ObjString;
class TextFade;

const int LEVEL_SPAWN_KINDS = 11;          // the bug kinds the level places (Skeeter to Bee)
const int LEVEL_CELL_LIST_MAX = 0x500;     // the lists of cells: 40 columns by 32 rows
const int LEVEL_TEXT_COUNT = 4;            // the texts that take turns (the fifth is kept for one message)
const int LEVEL_PERSISTENT_TEXT = 4;       // the index of that fifth text
const int LEVEL_BUG_SCORE_COUNT = 16;      // the bug-score numbers that take turns
const int LEVEL_FIREWORK_COUNT = 12;
const int LEVEL_KIND_LIST_MAX = 32;        // the longest lists of bug kinds
const int LEVEL_MASK_COLUMNS = 20;         // the layout masks are half the grid wide and half as high
const int LEVEL_MASK_ROWS = 16;
const int LEVEL_PREVIEW_COLUMNS = 0x50;    // the rope-preview map's size
const int LEVEL_PREVIEW_ROWS = 0x40;
const int LEVEL_MORPH_IMAGES = 2;          // "images/morph01" and "images/morph_stats"
const int LEVEL_MORPH_SIZE = 29;           // the cells of a morph table, each way
const int LEVEL_CENTER_X = 620;            // the middle of the play area, in design pixels
const int LEVEL_CENTER_Y = 496;

// The values of the level's state (mState, +0x53c0)
enum LevelState
{
	LEVEL_STATE_INTRO = 0,                 // waits for the intro to end (at once when there is none)
	LEVEL_STATE_SETTLING = 1,              // the pipes settle into place
	LEVEL_STATE_PLAYING = 2,               // the monkey cuts the ground
	LEVEL_STATE_STATS_WAIT = 3,            // counts down to the stats
	LEVEL_STATE_CELEBRATE = 4,             // the level is won: counts down to the stats' picture
	LEVEL_STATE_STATS = 5,                 // the stats are shown until a click
	LEVEL_STATE_DONE = 6,                  // finished: GetResult says on to the map (or the end of the game)
	LEVEL_STATE_GAME_OVER = 7,             // "GAME OVER" is shown, then the level is left
	LEVEL_STATE_VICTORY = 8,               // the last level is won: the victory scene plays
	LEVEL_STATE_LEAVING = 9,               // the level is left for the main menu
};

// What a level ended with (GetResult)
enum LevelResult
{
	LEVEL_RESULT_NONE = 0,
	LEVEL_RESULT_WON = 1,                  // on to the map
	LEVEL_RESULT_QUIT = 2,                 // back to the main menu (the monkey quit from the in-level menu)
	LEVEL_RESULT_LOST = 3,                 // back to the main menu (game over, or the whole game won)
};

// What the in-level menu gives back (InGameMenu::GetResult)
enum InGameMenuResult
{
	IN_GAME_MENU_OPEN = 0,
	IN_GAME_MENU_RESUME = 2,
	IN_GAME_MENU_QUIT = 3,
};

// What a place of the grid is (FillGridCell::mKind); a value of 10 or more has had its neighbours checked
enum FillKind
{
	FILL_KIND_EMPTY = 0,                   // a place with no ground
	FILL_KIND_CORNER = 1,                  // a post: a corner where walls meet
	FILL_KIND_WALL_ACROSS = 2,             // a piece of wall that runs across
	FILL_KIND_WALL_DOWN = 3,               // a piece of wall that runs down
	FILL_KIND_CHECKED = 10,                // added once the place's neighbours are known
	FILL_KIND_INSIDE = -1,                 // ground with ground on all eight sides
};

// Where a bug is put (the class of a spawn table entry)
enum SpawnClass
{
	SPAWN_ON_WALLS = 0,                    // along the walls (FindWallCell)
	SPAWN_ON_EDGES = 1,                    // at the edge of the field (FindEdgeCell)
	SPAWN_IN_OPEN = 2,                     // in the open ground (FindOpenCell)
};

// One bug kind in the level's table (0x14 bytes in the original)
struct SpawnEntry
{
	int mCount;                            // +0x00: how many are to be put in this level
	int mMax;                              // +0x04: the most a level may have
	int mMinLevel;                         // +0x08: the first level that has this kind
	int mClass;                            // +0x0c: SpawnClass
	int mWeight;                           // +0x10: how likely the kind is among the random ones
};

// One cell of a morph table (0x1c bytes)
struct MorphCell
{
	float mX;
	float mY;
	int mUsed;
	float mRed;
	float mGreen;
	float mBlue;
	float mBrightness;
};

class Intro;
class InGameMenu;
class Stats;
class Victory;
class FireWorks;
class BonusCoin;
class BananaBonus;
class BugScore;
class MenuButton;

class LevelObj : public ObjBase
{
public:
	SpawnEntry mSpawn[LEVEL_SPAWN_KINDS];               // +0x280: the bug kinds' table (InitSpawnTable)
	int mPreviewMap[LEVEL_PREVIEW_COLUMNS][LEVEL_PREVIEW_ROWS]; // +0x3c0: the rope preview's map
	int mState;                                         // +0x53c0: LevelState
	ObjBase* mFillLayer;                                // +0x53c4: the cells' parent
	ObjBase* mObjectLayer;                              // +0x53c8: the pipes' and the rope's parent
	float mScreenCenterX;                               // +0x53cc: where the play area's middle is on the screen
	float mScreenCenterY;                               // +0x53d0
	int mCellOrder[LEVEL_CELL_LIST_MAX];                // +0x53d4: the places in a shuffled order
	int mOrderPos;                                      // +0x67d4: where the layout starts in the order
	int mWallCells[LEVEL_CELL_LIST_MAX];                // +0x67d8: the places along the walls (column + row * 40)
	int mOpenCells[LEVEL_CELL_LIST_MAX];                // +0x7bd8: the places in the open ground
	int mWallCellCount;                                 // +0x8fd8
	int mOpenCellCount;                                 // +0x8fdc
	int mEdgeCount;                                     // +0x8fe0
	int mCellCount;                                     // +0x8fe4: the ground cells of the level
	int mCellsCut;                                      // +0x8fe8: how many have gone
	int mUnused8fec;                                    // +0x8fec: nothing reads it
	int mFloodCount;                                    // +0x8ff0: the places the preview's flood fill reached
	float mCenterX;                                     // +0x8ff4: the middle of the play area (620)
	float mCenterY;                                     // +0x8ff8: (496)
	bool mGroundCleared;                                // +0x8ffc: the ground has been cleared by a cut
	bool mMenuButtonLive;                               // +0x8ffd: the in-level menu button takes the mouse
	bool mCellsReady;                                   // +0x8ffe: the pipes have settled once
	RopeObj* mRope;                                     // +0x9000
	DDImage* mMorphImage;                                // +0x9004: "images/morph01"
	DDImage* mMorphStatsImage;                            // +0x9008: "images/morph_stats"
	Stats* mStats;                                      // +0x900c: the stats shown after a win
	Victory* mVictory;                                  // +0x9010: the scene after the last level, if there is one
	int mFireBallCountdown;                             // +0x9018: frames until the monkey's fire balls start, or -1
	InGameMenu* mMenu;                                  // +0x901c: the in-level menu while it is open
	bool mQuitConfirmed;                                // +0x9024: the monkey quit from the menu
	MenuButton* mMenuButton;                            // +0x9028
	bool mFireworksOn;                                  // +0x902c
	int mFireworksDelay;                                // +0x9030: frames before the fireworks go on
	ObjBase* mYouWin;                                   // +0x9034: the "you win" picture
	BonusCoin* mBonusCoin;                              // +0x9038
	Intro* mIntro;                                      // +0x903c
	BananaBonus* mBananaBonus;                          // +0x9040
	FireWorks* mFireWorks[LEVEL_FIREWORK_COUNT];        // +0x9048
	TextFade* mTexts[LEVEL_TEXT_COUNT + 1];             // +0x9078: the texts that take turns, and the one that stays
	BugScore* mBugScores[LEVEL_BUG_SCORE_COUNT];        // +0x908c
	int mTextRing;                                      // +0x90cc: the text the next message uses
	int mBugScoreRing;                                  // +0x90d0
	ObjString* mBananasText;                            // +0x90d4
	ObjString* mScoreText;                              // +0x90d8 (the lives figure)
	ObjBase* mBananaPanel;                              // +0x90dc
	int mSpawnTimer;                                    // +0x90e0
	int mSpawnTimer2;                                   // +0x90e4
	int mGameOverTimer;                                 // +0x90e8: frames of "GAME OVER" left (-1 at the start)
	MorphCell mMorph[LEVEL_MORPH_IMAGES][LEVEL_MORPH_SIZE][LEVEL_MORPH_SIZE]; // +0x90ec
	int mMask[LEVEL_MASK_COLUMNS][LEVEL_MASK_ROWS];     // +0x148e4: the layout mask
	int mColumnOrder[FILL_GRID_COLUMNS];                // +0x14de4: the columns in a shuffled order
	int mRowOrder[FILL_GRID_ROWS];                      // +0x14e84: the rows in a shuffled order
	int mTimer;                                         // +0x14f24: counts frames in a state
	int mFrame;                                         // +0x14f30
	bool mPreviewShown;                                 // +0x14f34
	bool mPreviewSide2Valid;                            // +0x14f35
	bool mPreviewSide3Valid;                            // +0x14f36
	int mMorphWidth[LEVEL_MORPH_IMAGES];                // +0x14f38
	int mMorphHeight[LEVEL_MORPH_IMAGES];               // +0x14f40
	int mOpenKinds[LEVEL_KIND_LIST_MAX];                // +0x14f48: the bug kinds to put in the open
	int mOtherKinds[LEVEL_KIND_LIST_MAX];               // +0x14ff0: the others
	int mOpenKindCount;                                 // +0x15098
	int mOtherKindCount;                                // +0x1509c
	bool mVictoryShown;                                 // +0x150a0: the last level was won

	LevelObj();                                         // 0x439700
	~LevelObj();                                        // 0x439ce0 (the body 0x439d00)
	bool Update() override;                             // 0x439f00

	int GetResult();                                    // 0x43a9b0

	// What the other classes call on the level sLevel points to
	static bool IsPlaying();                            // 0x43aad0: the level is in the state that takes play
	static void HideMenuButton();                       // 0x43aa30 (the stand-in's name was HideMessage)
	static void ShowGameOver();                         // 0x43aa60
	// 0x43aae0: theSlot -1 is the next of the four texts that take turns; theExtraHold goes to TextFade::SetExtraHold (the
	// original passes it in ECX: 0 at every call seen here)
	static void ShowMessage(const char* theText, float theX, float theY, int theDelay, float theScale, float theRed,
		float theGreen, float theBlue, int theSlot = -1, int theExtraHold = 0);
	static void HidePersistentMessage();                // 0x43ab70
	static void ShowBugScore(int theScore, float theX, float theY); // 0x43ab80
	static void ShowLives(int theLives);                // 0x43ac60
	static void ShowRopePreview(int theCellX, int theCellY, int theVertical, int theFrom, int theTo); // 0x43e060
	static void ClearRopePreview();                     // 0x43e020
	static void OnCellCut();                            // 0x43dc40: a cell of the ground has gone
	static void OnFillDone();                           // 0x43a990: the cut's last cell has gone
	static void CutGround(int theCellX, int theCellY, int theVertical, int theFrom, int theTo); // 0x43e3a0: the rope has reached its ends
	static bool HasIntro();                             // 0x4396b0: the first level of a new player
	static void CloseMenuForPause();                    // 0x43a4f0: the focus was lost (PauseWin::PauseForFocusLoss calls it)
	static void LinkBug(BugObj* theBug);               // 0x43c3a0 (ECX the bug): the bug is put last in the list of its place of the grid
	static void UnlinkBug(BugObj* theBug);             // 0x43c410 (EAX the bug): the bug is taken out of the list of its place
	// 0x43cf20: a cell of the morph table of that picture (0 "morph01", 1 "morph_stats") that the pop-out shows, picked at
	// random from where the scan starts and kept to the least used. theMiddleTones false takes the dark and light cells
	// (brightness up to 0.3 or from 0.7), true the ones between; none for a picture with no table
	static MorphCell* PickMorphCell(bool theMiddleTones, int theImage);
	static void ChoosePipeLook(PipeObj* thePipe);       // 0x43c0f0 (ESI the pipe; it never reads the level): the pipe's look from its place's kind

	void ShowBananas(bool thePlaySound);                // 0x43ac00: the banana figure

private:
	void SetupCells();                                  // 0x439e30
	void StartLevel(int theDelay);                      // 0x43a560
	void EndLevel();                                    // 0x43a710
	void Leave(bool addScore, bool clearCells);         // 0x43ce80
	void ClearBugs();                                   // 0x43cdf0
	void ClearCells();                                  // 0x43ce30
	void InitSpawnTable();                              // 0x43c4a0
	void BuildMessages();                               // 0x43e530
	void BuildMenuButton();                             // 0x43e650
	void ShowOrHideCell(FillObj* theCell);              // 0x43c1d0: the cell is on the field when its place has ground
	bool IsGroundKind(int theColumn, int theRow);       // 0x43c200: the place has ground (kind 10 or more)
	void LinkPipes();                                   // 0x43d660
	bool AllPipesSettledA();                            // 0x43d5f0
	bool AllPipesSettledB();                            // 0x43d5b0
	void StartPlay();                                   // 0x43d4f0
	void ReloadLevel();                                 // 0x43d470
	void InitCells();                                   // 0x43c000
	void SpawnBugs();                                   // 0x43ccf0
	void SpawnBug(int theKind);                         // 0x43c600
	void FillKindLists();                               // 0x43cc60: (the original's name is not known)
	void PlayLevelMusic();                              // 0x439c42..0x439c67 and 0x43a2ff: the song of the world the level is in
	void PlaceBugs();                                   // 0x43b370: puts every bug made on a place of the field, a column's followers with it
	void GetKindBudgets(int* theOpenBudget, int* theOtherBudget); // 0x43c900 (ESI, EDI): the weights of bug kinds the level takes for the open ground and for the others
	BugObj* MakeGroupSegment(int theKind, int theIndex, int theGroupSize, BugObj* theHead, BugObj* thePrevious); // 0x43acc0 (EBX the previous)
	bool FindEdgeCell(BugObj* theBug, int* theColumn, int* theRow); // 0x43ae10 (the original's name is not known)
	bool FindOpenCell(BugObj* theBug, int* theColumn, int* theRow); // 0x43b170
	bool FindWallCell(BugObj* theBug, int* theColumn, int* theRow); // 0x43b2b0

	void PickBugKinds();                                // 0x43cb00: picks the bug kinds of the level
	void BuildHud();                                    // 0x43e6d0: the banana panel and the figures
	void BuildYouWin();                                 // 0x43edd0: the "you win" picture
	void BuildMorphTable();                             // 0x43d2b0: the two pictures' pixels as tables of cells
	void MakeLayoutFromPicture(int theLevel);           // 0x43be60
	void MakeRandomLayout();                            // 0x43bb50
	int FillMask(int theCenterColumn, int theCenterRow, int theHalfWidth, int theHalfHeight, int theValue); // 0x43b530
	bool MeasureMaskSquare(int* theWidth, int* theHeight); // 0x43ba70
	void HollowOut();                                   // 0x43b800
	void ClassifyCells();                               // 0x43b850
	void PlaceBonus();                                  // 0x43b750
	void StartWallPipes(int theCellX, int theCellY, int theVertical, int theFrom, int theTo); // 0x43d800
	void FloodPreview(int theX, int theY, int theSide); // 0x43dc80
	void BuildPreviewMap();                             // 0x43dd60
	void MarkPreviewLine(int theCellX, int theCellY, int theVertical, int theFrom, int theTo); // 0x43df60


	friend class RopeObj;
};

// Whether the level being played takes the mouse (mMenuButtonLive of the level sLevel points to)
bool LevelAcceptsMouse();

// 0x43f3e0: the game makes a cell and a pipe for every place of the grid as it starts
void InitLevelTable();

} // namespace Sexy
