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

// The bonuses (0x406cd0, 0x406d90); the stand-in has none running
class Bonus
{
public:
	static void Cancel();                       // 0x406cd0
	static void CancelIfDone();                 // 0x406d90
	static void OnAllBugsDead();                // 0x406d40: the last bug in play is gone
	static void ClearAll();                     // 0x406c60: lets go of what is held and ends the bonuses (a guess at the name)
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

// GameWidget's preloads (0x42f3b0): each class's pictures, loaded once and kept
void PreloadNetFly();        // 0x42ad40
void PreloadSlots();         // 0x46c710
void PreloadBonusPictures(); // 0x406dd0
void InitBonusTable();       // 0x40af90

} // namespace Sexy
