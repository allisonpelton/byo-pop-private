#ifndef __ASTROPOP_BRICKKINDS_H__
#define __ASTROPOP_BRICKKINDS_H__

// The four classes made from the plain brick (Brick.h). Each has its own vtables in the original; the members after
// the base keep the original's offsets in comments.

#include "Brick.h"

namespace Sexy
{

// The coloured brick that clears its colour (constructor 0x4073f0, sizeof 0x23c, vtables 0x55d6c0 / 0x55d618)
class KillAllBrick : public Brick
{
public:
	int				mField234;          // +0x234 255
	bool			mField238;          // +0x238

	KillAllBrick(Board* theBoard, EventBus* theBus, int theColumn);
	virtual void	OnDie() override;                                   // 0x404c60
	virtual void	UpdateState3() override;                            // 0x402750
	virtual void	SetType(int theType) override;                      // 0x404850
	virtual void	Draw(Graphics* g, bool theGlow) override;           // 0x404880
};

// Radial bomb (6), stone (7), steel (9), row nuke (0xb); the type is set by SetType (constructor 0x407480, sizeof
// 0x23c, vtables 0x55d7d8 / 0x55d730)
class SpecialBrick : public Brick
{
public:
	int				mBlastArg;          // +0x234 the number event 0x26 carries when it dies (1 radial, 0 stone, -1 steel, 19 row nuke)
	int				mStage;             // +0x238 steel: 2 whole, 1 cracked (SetType, OnSettle)

	SpecialBrick(Board* theBoard, EventBus* theBus, int theColumn);
	virtual void	OnSettle() override;                                // 0x403420
	virtual void	OnDie() override;                                   // 0x405140
	virtual void	StartMutatorCountdown() override;                   // 0x4063e0
	virtual void	SetType(int theType) override;                      // 0x4074c0
	virtual void	Draw(Graphics* g, bool theGlow) override;           // 0x404cb0
};

// The fuel cell (constructor 0x407bd0, sizeof 0x254, vtables 0x55d908 / 0x55d860)
class FuelCell : public Brick
{
public:
	int				mRow;               // +0x234 -1
	int				mColorPhase;        // +0x238 0 .. 5 the tint's walk round the colour wheel (6: not tinted)
	int				mColorStep;         // +0x23c 2 (4 once it has settled)
	bool			mFlag240;           // +0x240
	Color			mTint;              // +0x244 starts red

	FuelCell(Board* theBoard, EventBus* theBus);
	virtual void	OnSettle() override;                                // 0x403860
	virtual void	OnDie() override;                                   // 0x4056e0
	virtual bool	Update() override;                                  // 0x4036f0
	virtual void	SetType(int theType) override;                      // 0x402a90
	virtual void	Draw(Graphics* g, bool theGlow) override;           // 0x405340
	virtual void	DrawAt(Graphics* g, bool theGlow, float theX, float theY) override;  // 0x402b30
	virtual void	SetPlace(int theColumn, int theRow);                // 0x407ce0 slot 27
	virtual void	SetFlag240();                                       // 0x407d00 slot 28
	virtual int		GetColumn();                                        // 0x402a70 slot 29
	virtual int		GetRow();                                           // 0x402a80 slot 30
	virtual bool	GetFlag240();                                       // 0x407d10 slot 31
};

// Freeze (10), column nuke (0xd) and booster (0xe) (constructor 0x407d80, sizeof 0x24c, vtables 0x55da30 / 0x55d988)
class PowerBrick : public Brick
{
public:
	bool			mActive;            // +0x234 the death effect is playing (Update runs the effect's strips instead of the brick)
	bool			mBoosterOff;        // +0x235 event 0x2a: the booster is paused
	float			mNukeY;             // +0x238 the column nuke's flash is drawn at -mNukeY
	int				mBoosterTime;       // +0x23c updates of boost left
	int				mBoosterTime0;      // +0x240 the time the booster started with
	int				mBoosterY;          // +0x244 the row the booster's flames fly to
	int				mBoosterTick;       // +0x248 updates to the next whoosh

	PowerBrick(Board* theBoard, EventBus* theBus, int theColumn);
	virtual void	OnSpawn() override;                                 // 0x4039c0
	virtual void	OnSettle() override;                                // 0x403930
	virtual void	OnDie() override;                                   // 0x408340
	virtual void	OnEvent2A(int theArg) override;                     // 0x402d60
	virtual void	UpdateState0() override;                            // 0x402be0
	virtual bool	Update() override;                                  // 0x4038c0
	virtual void	SetType(int theType) override;                      // 0x407e80
	virtual void	Draw(Graphics* g, bool theGlow) override;           // 0x405720
};

}

#endif
