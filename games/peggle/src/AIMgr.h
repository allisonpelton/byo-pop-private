// AIMgr.h: the bot's thinking (the original's Sexy::AIMgr): it tries shots on a copy of the board with a collision manager of
// its own, and keeps the best. The autoplay and the bot that tests the game both use it. Only the making and the emptying are
// translated so far (the fields keep the interim names mUnknownNN, the offset in the original, until the thinking is).

#ifndef __AIMGR_H__
#define __AIMGR_H__

#include "CollisionMgr.h"
#include "SharedPtr.h"

#include <list>
#include <memory>

namespace Sexy
{

class Board;
class PhysObj;

class AIMgr
{
public:
	explicit AIMgr(Board* theBoard); // 0x454cb0
	virtual ~AIMgr();				 // the original's deleting destructor

	void Clear(); // 0x44b120

	Board* mBoard;						   // +4
	std::unique_ptr<CollisionMgr> mCollisionMgr; // +8: the shots are tried against this one's lists
	std::list<int> mUnknown0C;			   // +0xc: four lists (their kinds of element are not known yet)
	std::list<int> mUnknown18;			   // +0x18
	std::list<int> mUnknown24;			   // +0x24
	std::list<int> mUnknown30;			   // +0x30
	SharedPtr<PhysObj> mUnknown3C;		   // +0x3c
	int mUnknown40;						   // +0x40
	SharedPtr<PhysObj> mUnknown44;		   // +0x44
	SharedPtr<PhysObj> mUnknown48;		   // +0x48
	bool mUnknown4C;					   // +0x4c
	bool mUnknown4D;					   // +0x4d
	bool mUnknown4E;					   // +0x4e
	int mUnknown54;						   // +0x54
	int mUnknown58;						   // +0x58
	int mUnknown60;						   // +0x60
	int mUnknown80;						   // +0x80
	int mUnknown84;						   // +0x84
	int mUnknown88;						   // +0x88
	int mUnknown8C;						   // +0x8c
};

} // namespace Sexy

#endif
