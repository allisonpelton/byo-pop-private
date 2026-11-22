// port: stand-ins for the parts of the original not translated yet, so the screens that are can run. Each piece moves
// to its own class's files as that class is translated, and this file goes once it is empty. Each stand-in names the
// original's address; one that does less than the original says so.
#pragma once

#include "AskWin.h"
#include "BBWindow.h"
#include "BugObj.h"
#include "FillObj.h"
#include "MenuAnim.h"
#include "ObjBase.h"
#include "PipeObj.h"
#include "RopeObj.h"

namespace Sexy
{

class Anim;
class BBFont;
class LevelObj;
class PipeObj;

// A bonus hidden under a cell of the ground (0x409b00..; the class is not translated)
class HiddenBonus : public ObjBase
{
public:
	FillObj* mCell;                             // +0x284: the cell that covers it
	void Uncover();                             // 0x409bf0
};

// The bolt of light that runs down a pipe (0x405fb0; 0x520 bytes in the original); the stand-in draws nothing
class BoltObj : public ObjBase
{
public:
	BoltObj(int theSegments);                   // 0x405fb0
	void Stop();                                // 0x406280
	void SetEnds(int theFromX, int theFromY, int theToX, int theToY); // 0x4062b0
	void UpdateBolt();                          // 0x4062e0
};

// The walk-through's hints (0x475610..0x475670); the stand-in shows none
class WalkThru
{
public:
	static void OnShoot();                      // 0x475610
	static void OnLoseLife();                   // 0x475630
	static void OnTurn();                       // 0x475670
	static void OnAreaFilled();                 // 0x475650
	static void Dismiss();                      // 0x4756b0
	static void OnBugZapped();                  // 0x475690: a bug was zapped
	static void ShowFirstStep();                // 0x4757d0 (the names of the three steps are guesses)
	static void ShowSecondStep();               // 0x4759e0
	static void ShowThirdStep();                // 0x475c40
};

// The bonuses (0x406cd0, 0x406d90); the stand-in has none running
class Bonus
{
public:
	static void Cancel();                       // 0x406cd0
	static void CancelIfDone();                 // 0x406d90
	static void OnAllBugsDead();                // 0x406d40: the last bug in play is gone
	static void ClearAll();                     // 0x406c60: lets go of what is held and ends the bonuses (a guess at the name)
};

// The board of figures in a level (0x464320): the stand-in shows nothing. A Player's board is nullptr until a level
// gives it one, and these never read the object they are called on
class ScoreBoard : public ObjBase
{
public:
	void ShowScore(int theScore);               // 0x4684a0
	void ShowPercent(int thePercent);           // 0x468500
	void ShowBananaIcon();                      // 0x465140: the next banana's icon appears
	void MarkLastIcon();                        // 0x4650f0: the last icon shown is marked with the id 0x43 and started
	void ActivateHud();                         // 0x4676e0
	void DeactivateHud();                       // 0x467750
	void LaunchFireBall(int theIndex);          // 0x4651a0: the monkey's fire ball of that number leaves the board
	void ShowLevel(int theLevel);               // 0x468550: the level's number ("%02d")
	void Show();                                // 0x465080
	void ClearBananaIcons();                    // 0x468780 (the original's name is not known)
	bool IsDone();                              // 0x468770 (the byte at +0x2ec): the board has stopped counting
};

// The things a level makes that are not translated yet; each is an object that shows nothing
class Intro : public ObjBase                    // 0x436460 (0x460 bytes): the first level's introduction
{
public:
	static bool IsReady();                      // 0x4396e0: the intro is ready for the level to go on (the stand-in: always)
	static bool IsFinished();                   // 0x4396f0
	void Finish();                              // 0x436a20
};

class InGameMenu : public ObjBase               // 0x435c00 (0x36c bytes, a BBWindow in the original)
{
public:
	int GetResult();                            // 0x436010: 2 for resume, 3 for quit (the stand-in resumes at once)
	void Close();                               // 0x427340
};

class Stats : public ObjBase                    // 0x46d2b0 (0x2ac bytes): the figures shown after a level
{
public:
	Stats(LevelObj* theLevel, int theFirst, int theSecond);
	bool IsReady();                             // 0x46da40
	void Continue();                            // 0x46d9e0
};

class Victory : public ObjBase                  // 0x473460 (0x2d0 bytes): the scene after the last level
{
public:
	void Show();                                // 0x473840
	bool IsDone();                              // 0x4738a0
};

// The bonus under a cell (0x408ed0, 0x498 bytes); the stand-in is a hidden bonus that shows nothing
class BonusCoin : public HiddenBonus
{
public:
	void Place(int theColumn, int theRow);      // 0x409970 (EAX the row, the column pushed): puts the bonus at that place
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

// The casino's game of slots (0x4687e0, 0x4e4 bytes), played from the base (MetaGame); the stand-in is over as soon as
// it starts, so the casino button pays nothing and ends at once
class MetaGame;
class Slots : public ObjBase
{
public:
	explicit Slots(MetaGame* theMetaGame);      // 0x4687e0
	void Start();                               // 0x468b10
	bool IsDone();                              // 0x469a30: true at once in the stand-in
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

// GameWidget's preloads (0x42f3b0): each class's pictures, loaded once and kept
void PreloadNetFly();        // 0x42ad40
void PreloadWalkThru();      // 0x4756d0
void PreloadSlots();         // 0x46c710
void PreloadInGameButtons(); // 0x4558f0
void PreloadBonusPictures(); // 0x406dd0
void InitBonusTable();       // 0x40af90
void PreloadSpinStars();     // 0x41f980
void PreloadRope();          // 0x464250

} // namespace Sexy
