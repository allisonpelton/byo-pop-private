// Ball.h: a round piece (the original's Sexy::Ball): a round peg of a level (class 5, mKind 3, made by Ball(true)) or a ball in
// play (class 4, mKind 2, made by Ball(false): the cannon's ball, the bonus balls and the hat of a ball). It has a position, a speed and a radius, and what the logic and the
// physics keep on it. Only the loading is translated so far: the physics (Update, DoCollideUpdate, CheckCollision), the
// drawing and the hat's board calls are not, and the fields the physics reads still carry the interim names mUnknownNNN
// (the offset in the original) until it is.

#ifndef __BALL_H__
#define __BALL_H__

#include "PhysObj.h"
#include "SexyVector.h"

#include <vector>

namespace Sexy
{

class Ball : public PhysObj
{
public:
	Ball(bool theIsPeg = false); // 0x480de0: the ball that is shot when false, a round peg when true

	void SyncState(DataSync& theSync) override;					 // 2, 0x483410
	int GetClass() const override { return mKind != KIND_SHOT_BALL ? CLASS_PEG : CLASS_SHOT_BALL; } // 3, 0x4547a0
	void SetPos(float theX, float theY) override;				 // 17, 0x474e60
	void Translate(float theDeltaX, float theDeltaY) override;	 // 18, 0x474eb0
	void SetVelocity(float theDeltaX, float theDeltaY) override; // 19, 0x475600
	void Update() override;										 // 22, 0x47a450
	void UpdateSlowMo(float theSubFrame) override;				 // 23, 0x47a8d0
	void DrawShadow(Graphics* g) override;						 // 24, 0x4838f0
	void Draw(Graphics* g) override;							 // 25, 0x47b0c0
	void AddedToGame() override;								 // 27, 0x477070
	void GetCenter(float* theCenter) const override;			 // 29, 0x474e40
	float GetXPos() const override { return mX; }				 // 30, 0x47df90
	float GetYPos() const override { return mY; }				 // 31, 0x4547b0
	bool IsCollidableWith(const PhysObj* theOther) const override; // 32, 0x477a00

	bool CheckCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime, SexyVector2& theContact) const; // 0x475000: a ball at thePos going by theVel against this peg
	bool DoCollideUpdate(std::vector<PhysObj*>& theCandidates, SexyVector2* theFirstContact); // 0x47f160: moves for the rest of the update, hitting what it meets
	void DoNonCollideUpdate();							   // 0x475200
	void CollideWithBall(Ball* theOther);			   // 0x4775b0
	void BounceBalls(Ball* theOther);				   // 0x477110
	void BounceOffPeg(Ball* thePeg);				   // 0x477340
	void BounceOffWall(SexyVector2& theVelocity, PhysObj* theWall, const SexyVector2& theNormal, const SexyVector2& theContact,
					   const SexyVector2& theBounce, SexyVector2& theSurface); // 0x4775f0
	void SetHoleOut(PhysObj* thePiece, int theDelay); // 0x47a350: the ball comes out at the piece (at once, or after the delay)
	void SetAbsPos(float theX, float theY);		   // 0x474dd0: puts the ball there as if it had always been
	void SetHat(bool theHasHat, bool theMakeBall); // 0x480f50: gives the ball a hat, or takes it off

	int mUnknownE4;			  // +0xe4: saved by a ball in play
	int mBallId;			  // +0xe8: the number the piece was made with (saved from the version 0x18 on)
	float mX;				  // +0xec: the middle of the ball
	float mY;				  // +0xf0
	float mPreviousX;		  // +0xf4: the place at the start of the update (a ball in play saves it)
	float mPreviousY;		  // +0xf8
	float mVelocityX;		  // +0xfc: the speed, in pixels an update
	float mVelocityY;		  // +0x100
	float mUnknown104;		  // +0x104: the place when the piece was read, and where SetHat puts the hat's goal
	float mUnknown108;		  // +0x108
	float mTrail[6][2];		  // +0x10c: six places (x, y) the ball was at, the last (+0x134) the place at the end of the last update; saved by a ball in play (the middle four from the version 12 on)
	float mRadius;			  // +0x13c
	bool mIsPeg;			  // +0x140: the constructor's argument, saved
	bool mUnknown141;		  // +0x141: set while the ball is held (a peg, or waiting to come back): the collisions skip it
	bool mUnknown142;		  // +0x142
	bool mUnknown143;		  // +0x143
	bool mUnknown144;		  // +0x144
	bool mUnknown145;		  // +0x145
	bool mUnknown146;		  // +0x146
	int mUnknown148;		  // +0x148: 1000000 when it has none (saved when less)
	int mUnknown14C;		  // +0x14c
	int mUnknown150;		  // +0x150
	int mUnknown154;		  // +0x154
	int mUnknown158;		  // +0x158: -1000 when it has none
	SharedPtr<PhysObj> mUnknownPiece15C; // +0x15c
	bool mUnknown160;		  // +0x160
	bool mFireball;			  // +0x161: the fireball sound is playing for it (the sound is started again when it is read)
	bool mUnknown162;		  // +0x162
	bool mUnknown163;		  // +0x163: set by the constructor
	bool mUnknown164;		  // +0x164
	bool mUnknown165;		  // +0x165
	float mUnknown168;		  // +0x168
	float mUnknown16C;		  // +0x16c
	float mUnknown170;		  // +0x170
	int mUnknown174;		  // +0x174
	int mUnknown178;		  // +0x178
	int mUnknown17C;		  // +0x17c
	int mUnknown180;		  // +0x180: 10000 until read
	void* mFireballSound;	  // +0x184: the sound instance of the fireball (the sound manager's, once translated)
	SharedPtr<Ball> mHat;	  // +0x188: the ball that rides on this one, drawn as its hat
	bool mIsHat;			  // +0x18c: this ball is another's hat
	bool mHasHat;			  // +0x18d: it has a hat
};

} // namespace Sexy

#endif
