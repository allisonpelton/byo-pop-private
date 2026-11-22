// Hole.h: a bonus hole of a level (the original's Sexy::Hole, a PhysObj of class 8): a rectangle or an ellipse that the ball
// can fall into, scoring points and coming out at another piece after a delay. It has a position, a size and a score. The level
// editor's functions (the property hook, the outline) are left out.

#ifndef __HOLE_H__
#define __HOLE_H__

#include "PhysObj.h"

namespace Sexy
{

class Hole : public PhysObj
{
public:
	Hole();				// 0x4810e0
	~Hole() override {} // 0x4811e0

	void SyncState(DataSync& theSync) override;			   // 2, 0x481210
	int GetClass() const override { return CLASS_HOLE; }   // 3, 0x481140
	void SetPos(float theX, float theY) override;		   // 17, 0x475540
	void Translate(float theDeltaX, float theDeltaY) override; // 18, 0x4754f0
	void SetVelocity(float theDeltaX, float theDeltaY) override; // 19, 0x475600
	void Update() override;								   // 22, 0x47a930
	void Draw(Graphics* g) override;					   // 25, 0x47ce60
	float GetXPos() const override { return mX; }		   // 30, 0x47dfa0
	float GetYPos() const override { return mY; }		   // 31, 0x481150

	void SetSize(int theWidth, int theHeight); // the original's slot 34, 0x475580: the size, and the box round the position

	SharedPtr<PhysObj> mExitPiece; // +0xe4: the piece a ball that fell in comes out at (none: it comes out of the hole itself)
	int mOutDelay;				   // +0xe8 (hole_outdelay): updates the ball waits in the hole
	int mFlashTimer;			   // +0xec: updates the hole is lit for after a ball fell in
	int mScore;					   // +0xf0 (hole_score)
	float mX;					   // +0xf4: the middle of the hole
	float mY;					   // +0xf8
	float mVelocityX;			   // +0xfc: how far the mover moved the hole in the last update
	float mVelocityY;			   // +0x100
	int mWidth;					   // +0x104
	int mHeight;				   // +0x108
	bool mCircular;				   // +0x10c (hole_circular): an ellipse, not a rectangle
};

} // namespace Sexy

#endif
