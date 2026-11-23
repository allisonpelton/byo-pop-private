// CollisionMgr.h: which pieces can hit a ball in play (the original's Sexy::CollisionMgr). It keeps every piece's box as two
// sorted lists of edges (one along x, one along y; a piece that moves is put back in order in each update), and a set of the
// pairs of pieces whose boxes meet when one of them is a ball in play. The collision tests themselves go through that set, in its
// order, so the order is part of how the game plays. The board has one; the bot (AIMgr) has another for the shots it tries.
// Only the filling and the emptying are translated so far; the update and the tests are not (the fields they use keep the
// interim names mUnknownNN, the offset in the original, until then).

#ifndef __COLLISIONMGR_H__
#define __COLLISIONMGR_H__

#include "SharedPtr.h"

#include <set>
#include <vector>

namespace Sexy
{

class PhysObj;
class Ball;
class SexyVector2;

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

// A ball in play and what it touched in this update: the list of them (CollisionMgr::mContacts) is what the logic reads to see
// the pegs that were hit
struct CollisionContact
{
	SharedPtr<PhysObj> mBall;
	SharedPtr<PhysObj> mOther;
};

class CollisionMgr
{
public:
	CollisionMgr(); // 0x44d870

	void Clear();				  // 0x446560
	void AddObj(PhysObj* theObj); // 0x44d9d0: finds the pieces it meets and puts its edges in the lists
	void RemoveObj(PhysObj* theObj); // 0x4465a0: the piece's edges and pairs are taken away
	void Update(bool theNoteContacts); // 0x45add0: puts the edges in order, and lets each ball in play meet what its box meets
	void BuildLocalPieces(Ball* theBall); // 0x454dc0: caches the pieces near the ball (the ones its box is not far from)
	bool UpdateOneBall(Ball* theBall, SexyVector2* theFirstContact); // 0x454f90: the ball goes for the rest of the update; true when it hit something that moves
	void SortEdges(std::vector<CollisionEdge>& theEdges); // 0x44b1e0: the edges are put in order by insertion, and the pairs follow

	std::vector<CollisionEdge> mEdgesX;				   // +0x04
	std::vector<CollisionEdge> mEdgesY;				   // +0x14
	std::set<CollisionPair, CollisionPairLess> mPairs; // +0x24
	int mUnknown30;									   // +0x30: Update's cursor into the pairs
	int mUnknown34;									   // +0x34
	std::vector<PhysObj*> mCandidates;				   // +0x38: the pieces the ball being looked at may hit in this update
	std::vector<CollisionContact> mContacts;		   // +0x48: what the balls touched in the update (when Update was asked to note it)
	float mRegionLeft;								   // +0x58: the box BuildLocalPieces looked in
	float mRegionRight;								   // +0x5c
	float mRegionTop;								   // +0x60
	float mRegionBottom;							   // +0x64
	std::vector<PhysObj*> mLocalPieces;				   // +0x68: the pieces whose boxes are in that region
	bool mUnknown00;								   // +0x00
	bool mUnknown78;								   // +0x78
};

// 0x650a6c and 0x650a70: while Update notes the contacts, this is the manager whose list they go in (none the rest of the time)
extern CollisionMgr* gContactRecorder;
// 0x45ad30: a ball in play touched another piece (the original calls it through a function pointer, set by Update)
void NoteContact(PhysObj* theFirst, PhysObj* theSecond);

} // namespace Sexy

#endif
