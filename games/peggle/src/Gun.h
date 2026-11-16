// Gun.h: the cannon of a level (the original's Sexy::Gun, a Poly of class 7): a triangle that turns to the aim, the ball it holds
// and the guide it draws. It belongs to a Board, which reads its aim from the player. Only the loading, the shape and the
// small helpers are translated so far; the aim, the shot and the guide are not (the fields they use keep the interim names
// mUnknownNNN, the offset in the original, until then). The level editor's functions are left out.

#ifndef __GUN_H__
#define __GUN_H__

#include "Ball.h"
#include "Poly.h"

namespace Sexy
{

class Board;

class Gun : public Poly
{
public:
	Gun(Board* theBoard); // 0x486a90

	void SyncState(DataSync& theSync) override;			   // 2, 0x4882c0
	int GetClass() const override { return CLASS_GUN; }	   // 3, 0x486c10
	void Update() override;								   // 22, 0x484120
	void UpdateSlowMo(float theSubFrame) override;		   // 23, 0x47df50
	void Draw(Graphics* g) override;					   // 25, 0x47f590

	void CalcPoints();						 // 0x483da0: puts the triangle where the aim is, and the ball at its muzzle
	void UpdateCommon();					 // 0x4752b0: notes the aim when it has been still for a while
	int GetMaxGuideLength() const;			 // 0x4754a0
	bool NeedDrawMouseClick() const;		 // 0x4754c0
	bool SetAngle(float theAngle, bool theAtOnce); // 0x4843a0
	SharedPtr<Ball> Fire(bool theKeepAim);	 // 0x484530: lets the ball go; the cannon holds none afterwards
	void UpdateBouncyGuide();				 // 0x482910

	Board* mBoard;			   // +0x148
	int mUpdateCount;		   // +0x14c: updates since the cannon was made (saved when not 0)
	float mAngle;			   // +0x150: the aim, in radians (the cannon turns towards mTargetAngle)
	float mTargetAngle;		   // +0x154
	float mBallX;			   // +0x158: where the ball sits, at the muzzle
	float mBallY;			   // +0x15c
	float mLaunchSpeed;		   // +0x160: the speed the ball is shot at (4)
	float mTipLength;		   // +0x164: how far the triangle's point is in front of its base
	float mHalfWidth;		   // +0x168: half the triangle's base
	float mBaseShift;		   // +0x16c: the triangle and the muzzle are this far from the cannon's place
	float mBarrelLength;	   // +0x170: from the triangle's base to the ball, less the ball's radius
	int mUnknown174;		   // +0x174
	int mUnknown178;		   // +0x178
	float mUnknown17C;		   // +0x17c: 1000
	bool mUnknown180;		   // +0x180: set by the constructor
	float mStillAngle;		   // +0x184: the aim as of the last update
	float mStableAngle;		   // +0x188: the aim that stayed for 30 updates
	int mStableTime;		   // +0x18c: the update count at that time
	int mStillCount;		   // +0x190: updates the aim has stayed
	bool mBouncyGuide;		   // +0x194: the guide shows the ball's bounces (saved)
	bool mUnknown195;		   // +0x195 (saved)
	int mUnknown198;		   // +0x198: -1
	int mUnknown19C;		   // +0x19c: -1
	bool mUnknown1A1;		   // +0x1a1 (saved)
	bool mUnknown1A2;		   // +0x1a2 (saved)
	int mUnknown1A4;		   // +0x1a4: three words that start as 0
	int mUnknown1A8;		   // +0x1a8
	int mUnknown1AC;		   // +0x1ac
	float mLastAngle;		   // +0x1b0: the aim of the last shot (saved when there was one)
	float mLastBallX;		   // +0x1b4
	float mLastBallY;		   // +0x1b8
	float mLastSpeed;		   // +0x1bc: 0 until a shot was made
	SharedPtr<Ball> mBall;	   // +0x1c0: the ball the cannon holds (saved)
	int mUnknown1C8;		   // +0x1c8: three words that start as 0
	int mUnknown1CC;		   // +0x1cc
	int mUnknown1D0;		   // +0x1d0
	int mUnknown1D8;		   // +0x1d8: the head of a list (made by 0x43f0d0) whose size is at 0x1dc
	int mUnknown1DC;		   // +0x1dc
};

} // namespace Sexy

#endif
