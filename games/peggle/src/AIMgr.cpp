// AIMgr.cpp: see AIMgr.h.

#include "AIMgr.h"

#include "PhysObj.h"

using namespace Sexy;

// 0x454cb0
AIMgr::AIMgr(Board* theBoard)
{
	mUnknown40 = 0;
	mUnknown58 = 0;
	mUnknown80 = 0;
	mUnknown84 = 0;
	mUnknown88 = 0;
	mUnknown8C = 0;
	mBoard = theBoard;
	mCollisionMgr.reset(new CollisionMgr());
	mUnknown4C = false;
	mUnknown54 = 10;
	mUnknown60 = 30;
	Clear();
}

AIMgr::~AIMgr()
{
}

// 0x44b120
void AIMgr::Clear()
{
	mUnknown4D = false;
	mUnknown4E = false;
	mUnknown58 = 0;
	mUnknown3C = nullptr;
	mUnknown44 = nullptr;
	mUnknown48 = nullptr;
	mUnknown0C.clear();
	mUnknown18.clear();
	mUnknown24.clear();
	mUnknown30.clear();
	mCollisionMgr->Clear();
}
