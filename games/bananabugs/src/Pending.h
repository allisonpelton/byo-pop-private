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

// The bonus under a cell (0x408ed0, 0x498 bytes; the class is not translated): it lies hidden under a cell of the ground
// and shows itself when the cell is cut (Uncover). The stand-in shows nothing and holds none of the 19 bonuses.
class BonusCoin : public ObjBase
{
public:
	FillObj* mCell;                             // +0x284: the cell that covers it
	void Place(int theColumn, int theRow);      // 0x409970 (EAX the row, the column pushed): puts the bonus at that place
	void Uncover();                             // 0x409bf0
	static void ResetCurrent();                 // 0x40af00: puts the coin that is showing back under its cell
};

// The effect of a bonus on the monkey (0x40bfe0); the stand-in is an object that shows nothing
class BonusEffect : public ObjBase
{
};

// The pictures PreloadBonusPictures (0x406dd0) loads, in the globals of the second half's bonuses (not translated yet)
extern Anim* gMissileAnim;    // 0x60e1ac
extern Anim* gParrotFlyAnim;  // 0x60e1b8
extern Anim* gParrotWalkAnim; // 0x60e1bc
extern Anim* gParrotPeckAnim; // 0x60e1c0
extern Anim* gDiscoBallAnim;  // 0x60e1c4
extern Anim* gMilkDropAnim;   // 0x60e1c8
extern Anim* gDropAnim;       // 0x60e1cc
extern Anim* gTarzanAnim;     // 0x60e1d0
extern Anim* gPaperAnim;      // 0x60e1d4
extern Anim* gZapperAnim;     // 0x60e1d8
extern Anim* gSwatterAnim;    // 0x60e1dc
extern Anim* gMineAnim;       // 0x60e1e0
extern Anim* gZumaAnim;       // 0x60e1e4
extern Anim* gZumaBallAnim;   // 0x60e1e8

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
void InitBonusTable();       // 0x40af90

} // namespace Sexy
