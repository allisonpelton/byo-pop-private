// CollisionMgr.h: which pieces can hit a ball in play (the original's Sexy::CollisionMgr). It keeps every piece's box as two
// sorted lists of edges (one along x, one along y; a piece that moves is put back in order in each update), and a set of the
// pairs of pieces whose boxes meet when one of them is a ball in play. The collision tests themselves go through that set, in its
// order, so the order is part of how the game plays. The board has one; the bot (AIMgr) has another for the shots it tries.
// Only the filling and the emptying are translated so far; the update and the tests are not (the fields they use keep the
// interim names mUnknownNN, the offset in the original, until then).

#ifndef __COLLISIONMGR_H__
#define __COLLISIONMGR_H__

#include <set>
#include <vector>

namespace Sexy
{

class PhysObj;

// One end of a piece's box along an axis. mPosition points into the piece's box (its left or top for a start, its right or bottom
// for an end), so the lists see the piece move.
struct CollisionEdge
{
	PhysObj* mObject;
	const float* mPosition;
	bool mIsStart;
};

// Two pieces whose boxes meet. mFirst is the ball in play; when both are balls the one made first comes first.
struct CollisionPair
{
	PhysObj* mFirst;
	PhysObj* mSecond;
};

// The order of the set: by the first piece's number, and for the same first piece by the second one's kind and then number
struct CollisionPairLess
{
	bool operator()(const CollisionPair& theLeft, const CollisionPair& theRight) const;
};

class CollisionMgr
{
public:
	CollisionMgr(); // 0x44d870

	void Clear();				  // 0x446560
	void AddObj(PhysObj* theObj); // 0x44d9d0: finds the pieces it meets and puts its edges in the lists
	void Update(bool theDebug);	  // 0x45add0
	void UpdateOneBall();		  // 0x454f90

	std::vector<CollisionEdge> mEdgesX;				   // +0x04
	std::vector<CollisionEdge> mEdgesY;				   // +0x14
	std::set<CollisionPair, CollisionPairLess> mPairs; // +0x24
	int mUnknown30;									   // +0x30: Update's cursor into the pairs
	int mUnknown34;									   // +0x34
	int mUnknown3C;									   // +0x3c: three words that start as 0 (a list)
	int mUnknown40;									   // +0x40
	int mUnknown44;									   // +0x44
	int mUnknown4C;									   // +0x4c: three more
	int mUnknown50;									   // +0x50
	int mUnknown54;									   // +0x54
	int mUnknown6C;									   // +0x6c: and three more
	int mUnknown70;									   // +0x70
	int mUnknown74;									   // +0x74
	bool mUnknown00;								   // +0x00
	bool mUnknown78;								   // +0x78
};

} // namespace Sexy

#endif
