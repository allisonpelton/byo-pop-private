// port: stand-ins for the parts of the original not translated yet, so the screens that are can run. Each piece moves
// to its own class's files as that class is translated, and this file goes once it is empty. Each stand-in names the
// original's address; one that does less than the original says so.
#pragma once

#include "AskWin.h"
#include "BBWindow.h"
#include "Bonus.h"
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

// The effect of a bonus on the monkey (0x40bfe0); the stand-in is an object that shows nothing
class BonusEffect : public ObjBase
{
public:
	void Play();                                // 0x40c040 (EBX the effect): starts its 18 pictures and shows it
};

// A bonus of the second half of the kinds, not translated yet: it holds its kind (Bonus::Setup) and nothing else, and
// when it is won it is put away at once, so play goes on. Each stand-in is named after the class at the constructor's
// address
class PendingBonus : public Bonus
{
public:
	void Start() override;                      // put away at once (the original's runs the bonus)
};

class BonusShotSpeed : public PendingBonus      // 0x410ca0 (0x2d8 bytes)
{
public:
	BonusShotSpeed();
};

class BonusFlyPaper : public PendingBonus       // 0x40c700 (0x2e8 bytes)
{
public:
	BonusFlyPaper();
};

class BonusZapper : public PendingBonus         // 0x4123e0 (0x2e4 bytes)
{
public:
	BonusZapper();
};

class BonusSwat : public PendingBonus           // 0x410e10 (0x300 bytes)
{
public:
	BonusSwat();
};

class BonusSpearCraft : public PendingBonus     // 0x410d60 (0x2d8 bytes)
{
public:
	BonusSpearCraft();
};

class BonusMagicArrows : public PendingBonus    // 0x40d740 (0x2d8 bytes)
{
public:
	BonusMagicArrows();
};

class BonusMissile : public PendingBonus        // 0x40e550 (0x318 bytes)
{
public:
	BonusMissile();
};

class BonusRain : public PendingBonus           // 0x410660 (0x3bc bytes)
{
public:
	BonusRain();
};

class BonusHurricane : public PendingBonus      // 0x40d090 (0x3c0 bytes)
{
public:
	BonusHurricane();
};

class BonusParrot : public PendingBonus         // 0x40eff0 (0x328 bytes)
{
public:
	BonusParrot();
};

class BonusTarzan : public PendingBonus         // 0x411c60 (0x32c bytes)
{
public:
	BonusTarzan();
};

class BonusZuma : public PendingBonus           // 0x412f20 (0x304 bytes)
{
public:
	BonusZuma();
};

class BonusMine : public PendingBonus           // 0x40d7c0 (0x380 bytes)
{
public:
	BonusMine();
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

} // namespace Sexy
