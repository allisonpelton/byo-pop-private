// PhysObjFactory.cpp: see PhysObjFactory.h.

#include "PhysObjFactory.h"

#include "Ball.h"
#include "Board.h"
#include "Brick.h"
#include "Gun.h"
#include "Hole.h"
#include "Line.h"
#include "NotTranslated.h"
#include "PhysObj.h"
#include "Poly.h"
#include "ThunderballApp.h"

using namespace Sexy;

// 0x404ca0
PhysObj* Sexy::MakePhysObj(int theClass)
{
	switch (theClass)
	{
	case CLASS_PHYSOBJ:
		return new PhysObj();
	case CLASS_LINE:
		return new Line();
	case CLASS_POLY:
		return new Poly();
	case CLASS_SHOT_BALL:
		return new Ball(false);
	case CLASS_PEG:
		return new Ball(true);
	case CLASS_BRICK:
		return new Brick();
	case CLASS_GUN:
		return new Gun(GetBoard());
	case CLASS_HOLE:
		return new Hole();
	default:
		return new PhysObj();
	}
}

Board* Sexy::GetBoard()
{
	return static_cast<Board*>(gThunderballApp->mBoard); // the original's 0x404c80: the app's board, at +0x7b8
}
