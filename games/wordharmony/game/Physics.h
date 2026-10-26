#ifndef __WORDHARMONY_PHYSICS_H__
#define __WORDHARMONY_PHYSICS_H__

// The balls' physics: Verlet integration (each ball keeps its last position, and its velocity is the difference)
// with constraints that move the balls directly: the screen's edges, the obstacles, the balls pushing each other
// apart, the chains of a found word and the mouse's rope (games/wordharmony/docs/WORKING_NOTES.md, "The game code as
// read").

#include "Font.h"
#include "Graphics.h"
#include "Node.h"
#include <list>
#include <string>

class BallArray;
class Chain;

// The ball's states (+0xd8)
enum
{
	BALL_FREE = 0,
	BALL_PICKED = 1,
	BALL_PICKED_2 = 2,
	BALL_GONE = 3,
	BALL_STATE_4 = 4
};

// The ball's special flags (+0xac), named as a bubble's "special" attribute names them (0x41e280)
enum
{
	BALL_BOMB = 0x1,        // "bomb" (also set on the balls of the word BOMB)
	BALL_WILDCARD = 0x2,    // "wildcard"
	BALL_X2 = 0x4,          // "2x"
	BALL_X3 = 0x8,          // "3x"
	BALL_X4 = 0x10,         // "4x"
	BALL_BLOCK = 0x20,      // "block": cube02.png, heavy
	BALL_HEAVY = 0x40,      // "heavy": rock.png
	BALL_STONE = 0x80,      // "stone"
	BALL_BREAKABLE = 0x100  // "obstacle", and the level file's breakable obstacles: cube02.png, fixed
};

// A letter ball (0xfc bytes, constructor 0x40f200, vtable 0x481bd0). The ball array holds them in one array.
class Ball : public engine::Container
{
public:
	Ball();            // 0x40f200
	~Ball() override;  // 0x40efe0

	// 0x6c: 0x40ef30, its images and its letter in the middle
	void draw(engine::Graphics* g) override;

	// 0x40efa0: shows the lit image or the plain one
	void setOn(bool theOn);
	// 0x4111b0: in upper case
	void setLetter(char theLetter);

	engine::Node* mSelector;  // +0xa8: a Selector (its plain and lit images)
	unsigned int mSpecial;    // +0xac
	float mB0;                // +0xb0: 1 when spawned; picking sets it
	engine::Point mOld;       // +0xb8: the last position
	engine::Point mAcceleration;  // +0xc0: this step's
	float mRadius;            // +0xc8
	float mInverseMass;       // +0xcc
	float mDamping;           // +0xd0: this step's, lowered by each hit
	bool mFixed;              // +0xd4
	int mState;               // +0xd8
	engine::Point* mGravity;  // +0xdc: its own (owned), or null for the array's
	Chain* mChain;            // +0xe0
	bool mE4;                 // +0xe4
	int mRow;                 // +0xe8: its row group (the ball system's 0x415c20), or -1 (the original leaves both unset
	                          // until then; the port starts them at -1)
	int mColumn;              // +0xec: its column group, or -1
	char mLetter;             // +0xf0
	bool mLinked;             // +0xf1: pulled to the other linked balls it touches
};

// The balls (vtable 0x481bb0; the ball system's first base)
class BallArray
{
public:
	BallArray();           // 0x40ec20
	virtual ~BallArray();  // 0x40ec50
	virtual void step() = 0;  // slot 1: the constraints, once a physics step

	// 0x40ecd0
	Ball* getBall(unsigned int theIndex) { return &mBalls[theIndex]; }
	// 0x40ed10: each ball's acceleration for a step of theTime seconds
	void accumulate(float theTime);
	// 0x40ede0: moves the balls by their velocity and acceleration; theLastTime is the last step's
	void integrate(float theTime, float theLastTime);
	// 0x4111d0: the nearest free ball within 23 pixels, or -1
	unsigned int findBall(const engine::Point& thePoint);
	// 0x411300: how far apart two balls' edges are, along the farther axis
	float getGap(int theFirst, int theSecond);

	unsigned int mCount;     // +0x04
	int mBallsLeft;          // +0x08
	bool mC;                 // +0x0c
	engine::Point mGravity;  // +0x10
	float mFriction;         // +0x18
	Ball* mBalls;            // +0x1c
};

// The constraints' base (vtable 0x481b20)
class Constraint
{
public:
	Constraint() : mActive(true) {}
	virtual ~Constraint() {}
	virtual void apply() = 0;  // slot 1

	bool mActive;  // +0x04
};

// 0x40e100 (0x2c bytes, vtable 0x481b28): moves a node (a ball, or in the editor an obstacle) part of the way to a
// target each step; with the grid on, straight onto the grid (the editor's drag)
class Follow : public Constraint
{
public:
	Follow(engine::Node* theNode, float theC, float theRate, float theBlend, float theGrid);
	void apply() override;  // 0x40e170

	engine::Node* mNode;    // +0x08
	float mC;               // +0x0c
	float mRate;            // +0x10
	float mBlend;           // +0x14
	float mLength;          // +0x18
	engine::Point mTarget;  // +0x1c
	float mGrid;            // +0x24
	bool mSnap;             // +0x28
};

// 0x40e220 (0x2c bytes, vtable 0x481b40): the mouse's rope; beyond its length it pulls the ball by the excess (at
// most theMaxStep a step) times theRate, and keeps theBlend of the ball's last position
class Rope : public Constraint
{
public:
	Rope(Ball* theBall, const engine::Point& theOffset, float theMaxStep, float theRate, float theBlend);
	void apply() override;  // 0x40e2a0

	Ball* mBall;            // +0x08
	float mMaxStep;         // +0x0c
	float mRate;            // +0x10
	float mBlend;           // +0x14
	float mLength;          // +0x18
	engine::Point mTarget;  // +0x1c
	engine::Point mOffset;  // +0x24
};

// How far apart two balls are kept (0x40e950's mode)
enum
{
	KEEP_AT_MOST = 0,
	KEEP_AT_LEAST = 1,
	KEEP_EXACTLY = 2
};

// A distance between two balls (vtable 0x481b90; made on the stack and applied at once): each moves by its inverse
// mass towards the squared distance theDistance2
class Distance : public Constraint
{
public:
	// 0x40e920
	Distance(Ball* theFirst, Ball* theSecond, float theDistance2, int theMode)
		: mFirst(theFirst), mSecond(theSecond), mDistance2(theDistance2), mMode(theMode) {}
	void apply() override;  // 0x40e950

	Ball* mFirst;      // +0x08
	Ball* mSecond;     // +0x0c
	float mDistance2;  // +0x10
	int mMode;         // +0x14
};

// 0x40ea90 (vtable 0x481ba0): pulls two balls to the squared distance theMin2 when their squared distance is in
// [theMin2, theMax2] (mode 0)
class Stick : public Constraint
{
public:
	Stick(Ball* theFirst, Ball* theSecond, float theMin2, float theMax2, int theMode);
	void apply() override;  // 0x40ead0

	Ball* mFirst;   // +0x08
	Ball* mSecond;  // +0x0c
	float mMin2;    // +0x10
	float mMax2;    // +0x14
	int mMode;      // +0x18
};

// 0x40e5d0 (0x3c bytes, vtable 0x481b64): the balls of a word kept together; with mAlong unset, each ball next to
// the next one in the list and the first one to the last; with it set, every ball of the array to the next one
class Chain : public Constraint
{
public:
	Chain(BallArray* theBalls, int theMode);  // 0x40e5d0
	~Chain() override;                        // 0x40e670
	void apply() override;                    // 0x40e3f0

	std::list<int> mIndices;  // +0x0c
	bool mAlong;              // +0x14
	std::string mWord;        // +0x18
	BallArray* mBalls;        // +0x34
	int mMode;                // +0x38
};

// 0x40e6c0 (vtable 0x481b78): every two balls in play pushed apart to 44 pixels; linked balls pulled together
class Collisions : public Constraint
{
public:
	Collisions(BallArray* theBalls) : mBalls(theBalls) {}
	void apply() override;  // 0x40e6e0

	BallArray* mBalls;  // +0x08
};

// 0x410ef0 (0x1c bytes, vtable 0x482198): keeps the balls inside the playfield
class Bounds : public Constraint
{
public:
	Bounds(BallArray* theBalls, float theSize);
	void apply() override;  // 0x410f60
	virtual void applyTo(unsigned int theIndex);  // slot 2: 0x410f90

	BallArray* mBalls;   // +0x08
	engine::Point mC;    // +0x0c
	float mBottom;       // +0x14
	float mRight;        // +0x18
};

#endif
