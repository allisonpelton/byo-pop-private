// port: stand-ins for the parts of the original not translated yet, so the screens that are can run. Each piece moves
// to its own class's files as that class is translated, and this file goes once it is empty. Each stand-in names the
// original's address; one that does less than the original says so.
#pragma once

#include "AskWin.h"
#include "BBWindow.h"
#include "MenuAnim.h"
#include "ObjBase.h"

namespace Sexy
{

class Anim;
class BBFont;
class LevelObj;

// The level being played (0x405770); a stand-in shows nothing and never ends
class Board : public ObjBase
{
public:
	int mLeaveTrack;           // +0x280: the track MainMenu's StartLeaving (0x440a60) starts, also on a board
	LevelObj* mLevel;          // +0x788

	Board();
	~Board();
	void StartLeaving() { StartTrack(mLeaveTrack); } // 0x440a60
};

// What a level ended with (LevelObj::GetResult); the names are to be checked as LevelObj is translated
enum LevelResult
{
	LEVEL_RESULT_NONE = 0,
	LEVEL_RESULT_WON = 1,      // on to the map
	LEVEL_RESULT_QUIT = 2,     // back to the main menu
	LEVEL_RESULT_LOST = 3,     // back to the main menu
};

// The rope the monkey shoots (0x461320; the level's rope is the object at +0x9000 of the LevelObj); the stand-in never
// exists, so Current is always nullptr and nothing calls the others
class RopeObj
{
public:
	static RopeObj* Current();                  // the level's rope, or nullptr when there is none
	bool IsAimReady();                          // 0x462620
	int GetState();                             // 0x462630: 1 while the rope runs
	void SetAim(int theVertical, int theCellX, int theCellY, int theFrom, int theTo); // 0x462360
	void Start(int theVertical, int theCellX, int theCellY, int theFrom, int theTo);  // 0x462270
	void ResetIfFinished();                     // 0x462600
	void HitBox(float theX, float theY, float theWidth, float theHeight);             // 0x462950
};

// The rules of a level (0x439700); the stand-in's never end, and the statics are those of the level sLevel points to,
// which the stand-in never makes: each does nothing and says there is no level
class LevelObj
{
public:
	int GetResult() { return LEVEL_RESULT_NONE; } // 0x43a9b0
	static bool IsPlaying();                    // 0x43aad0: the level is in the state that takes play (+0x53c0 is 2)
	static void HideMessage();                  // 0x43aa30
	static void ShowGameOver();                 // 0x43aa60
	static void ShowMessage(const char* theText, float theX, float theY, int theDelay, float theScale, float theRed,
		float theGreen, float theBlue);         // 0x43aae0
	static void ShowLives(int theLives);        // 0x43ac60
	static void ShowRopePreview(int theCellX, int theCellY, int theVertical, int theFrom, int theTo); // 0x43e060
	static void ClearRopePreview();             // 0x43e020
};

// The bugs (0x425410 and the kinds before it); the stand-in has none
class BugObj
{
public:
	static bool AnyNear(float theX, float theY, float theReach); // 0x426300
	static void OnShot();                       // 0x426060: tells the bugs a rope was shot
};

// One cell of the ground the monkey cuts; the stand-in has none
class TileObj : public ObjBase
{
public:
	bool mCut;                                  // +0x291: the tile has been cut away
};

// The ground the monkey cuts (0x42ac70); the stand-in is never ready
class FillObj
{
public:
	static bool IsReady();                      // 0x42ac70
	static ObjBase* GetWall(int theColumn, int theRow); // the grid at 0x602eb0, 40 columns of 32 rows of 8 objects: slot 2
	static TileObj* GetTile(int theColumn, int theRow); // slot 3
};

// The walk-through's hints (0x475610..0x475670); the stand-in shows none
class WalkThru
{
public:
	static void OnShoot();                      // 0x475610
	static void OnLoseLife();                   // 0x475630
	static void OnTurn();                       // 0x475670
};

// The bonuses (0x406cd0, 0x406d90); the stand-in has none running
class Bonus
{
public:
	static void Cancel();                       // 0x406cd0
	static void CancelIfDone();                 // 0x406d90
};

// The board of figures in a level (0x464320): the stand-in shows nothing. A Player's board is nullptr until a level
// gives it one, and these never read the object they are called on
class ScoreBoard
{
public:
	void ShowScore(int theScore);               // 0x4684a0
	void ShowPercent(int thePercent);           // 0x468500
	void ShowBananaIcon();                      // 0x465140: the next banana's icon appears
	void MarkLastIcon();                        // 0x4650f0: the last icon shown is marked with the id 0x43 and started
	void ActivateHud();                         // 0x4676e0
	void DeactivateHud();                       // 0x467750
};

// The effect of a bonus on the monkey (0x40bfe0); the stand-in is an object that shows nothing
class BonusEffect : public ObjBase
{
};

// A thing the monkey holds, a bonus on its way to be put down; the stand-in turns and lets go of nothing
class HeldObj : public ObjBase
{
public:
	virtual void Rotate();                      // slot 20 (+0x50)
	virtual bool Place();                       // slot 21 (+0x54): false when it may not go there
};

// The box of the bug the rope last asked about (0x6a6778..0x6a6784), written by the bugs (0x425410)
extern float gBugBoxX;
extern float gBugBoxY;
extern float gBugBoxWidth;
extern float gBugBoxHeight;

// The map between levels (0x448e10)
class MetaGame : public ObjBase
{
public:
	bool mWantsMenu;           // +0x27c
	bool mLeaving;             // +0x27d

	static bool sFirstVisit;   // 0x5ff082: set as a meta game starts, cleared by the main menu when it continues one

	MetaGame();
	~MetaGame();
	bool TakeLevelChosen();    // 0x449b90
	static void ClearFirstVisit() { sFirstVisit = false; } // 0x44f440
};

// The pause window (0x454ef0)
class PauseWin
{
public:
	static bool sEnabled;      // 0x60e64b
	static void PauseForFocusLoss(); // 0x455920
	static bool IsBlocking();  // 0x455a10
	static void ResumeOnRelease(); // 0x455a40
	static void Enable() { sEnabled = true; } // 0x455a30
};

// Whether the level being played takes the mouse (the byte at +0x8ffd of the LevelObj sLevel points to)
bool LevelAcceptsMouse();

// GameWidget's preloads (0x42f3b0): each class's pictures, loaded once and kept
void PreloadNetFly();        // 0x42ad40
void PreloadBananas();       // 0x405730
void PreloadWalkThru();      // 0x4756d0
void PreloadSlots();         // 0x46c710
void PreloadFly();           // 0x423980
void PreloadInGameButtons(); // 0x4558f0
void PreloadAnt();           // 0x420650
void PreloadMoth();          // 0x423d10
void PreloadBeetle();        // 0x421750
void PreloadCentipede();     // 0x422f10
void PreloadWasp();          // 0x420b30
void PreloadCaterpillar();   // 0x422220
void PreloadFlyPaths();      // 0x41aee0
void PreloadMothPaths();     // 0x41c470
void PreloadSkeeter();       // 0x425110
void PreloadBonusPictures(); // 0x406dd0
void InitBonusTable();       // 0x40af90
void PreloadSpinStars();     // 0x41f980
void PreloadRolyPoly();      // 0x424c80
void PreloadFireBall();      // 0x42b3f0
void PreloadPipes();         // 0x459360
void PreloadRope();          // 0x464250
void InitLevelTable();       // 0x43f3e0
void PreloadCockroach();     // 0x4242e0

} // namespace Sexy
