// Intro: the cartoon the first level of a player who has not seen the tutorial starts with (0x436460..0x4396f0 in the
// original; 0x460 bytes there). A cart of bananas rolls in, a swarm of flies (twenty, each waiting its own time) comes
// in and carries the bananas away one by one, the wheel too, and a monkey who walked beside the cart looks up, turns,
// shows his despair and ends looking at the idol that was the cart's. The flies are pictures of the intro alone: they
// are not the level's bugs (the intro never calls LevelObj; level one has no bugs of its own until the tutorial is
// done). The intro lets the level go on (IsReady) as the monkey starts looking at the idol, and ends itself (Finish)
// at a mouse press once the first 100 frames are over, or when the level is left.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

const int INTRO_BANANAS = 15;       // the bunch on the cart
const int INTRO_BUGS = 20;          // the flies: the first fifteen take a banana each, the sixteenth the wheel, the last
                                    //   four hover over the cart

// One of the flies. The original keeps these three in the object's base class (+0xe8, +0xec, +0xf0), which the port's
// ObjBase does not have room for.
class IntroBug : public ObjBase
{
public:
	int mNumber;                    // +0xe8: its place among the flies (and the banana it goes for)
	int mPhase;                     // +0xec: IntroBugPhase
	int mKind;                      // +0xf0: 0 the intro's own fly picture, 1 the skeeter's
};

class Intro : public ObjBase
{
public:
	ObjBase* mBananas[INTRO_BANANAS];   // +0x27c: the bunch, children of the cart
	IntroBug* mBugs[INTRO_BUGS];        // +0x2b8: the flies
	ObjBase* mCart;                     // +0x308: the cart (a guess at what the picture shows); everything rides on it
	ObjBase* mMan;                      // +0x30c: the monkey who walks beside the cart
	ObjBase* mWheel;                    // +0x310: the wheel
	ObjBase* mFlare;                    // +0x314: the burst of light at the end, a child of the monkey
	ObjBase* mTalk1;                    // +0x318: the first speech bubble
	ObjBase* mTalk2;                    // +0x31c: the second speech bubble
	AnimKey* mCartLeaveFrom;            // +0x320: the cart's leaving track starts where the cart is
	AnimKey* mCartLeaveTo;              // +0x324: and goes 1100 left and 300 up
	AnimKey* mWheelLeaveFrom;           // +0x328: the wheel's second track (nothing starts it)
	AnimKey* mWheelLeaveTo;             // +0x32c
	AnimKey* mFlyFrom[INTRO_BUGS];      // +0x330: each fly's flight: the first key (from where it is)
	AnimKey* mFlyTo[INTRO_BUGS];        // +0x380: the second (to where it goes); its end raises event 3
	int mStolen;                        // +0x420: bananas taken so far (also counts the wheel's step)
	int mReleased;                      // +0x428: how many flies have been let go for their banana
	int mReleaseTimer;                  // +0x42c: frames until the next one (670 at the start)
	int mSkipDelay;                     // +0x430: frames in which a mouse press does not end the intro (100)
	int mWheelRollTrack;                // +0x434
	int mCartRollTrack;                 // +0x438
	int mCartTiltTrack;                 // +0x43c: the cart tips back after the last banana is gone
	int mCartLeaveTrack;                // +0x440
	int mWheelLeaveTrack;               // +0x444: nothing starts it
	int mManWalkTrack;                  // +0x448
	int mManLookUpTrack;                // +0x44c
	int mManTurnTrack;                  // +0x450
	int mManDespairTrack;               // +0x454
	int mManIdolTrack;                  // +0x45c

	// The pictures (0x60e618..0x60e644), loaded again by each intro
	static Anim* sCartAnim;             // 0x60e618: images/intro/intro_cart
	static Anim* sWalkAnim;             // 0x60e61c: cartoon_walk
	static Anim* sLookUpAnim;           // 0x60e620: cartoon_lookup
	static Anim* sTurnAnim;             // 0x60e624: cartoon_turn
	static Anim* sDespairAnim;          // 0x60e628: cartoon_desperate
	static Anim* sIdolAnim;             // 0x60e62c: cartoon_lookidol
	static Anim* sWheelAnim;            // 0x60e630: intro_wheel
	static Anim* sBunchAnim;            // 0x60e634: intro_bunch
	static Anim* sFlyAnim;              // 0x60e638: intro_fly
	static Anim* sFlareAnim;            // 0x60e63c: images/flare
	static Anim* sTalk1Anim;            // 0x60e640: talk_01
	static Anim* sTalk2Anim;            // 0x60e644: talk_02

	static bool sReady;                 // 0x5ff083: the level may go on (true until an intro is made)
	static bool sFinished;              // 0x5ff089: Finish has run (true until an intro is made)

	Intro();                            // 0x436460
	~Intro() override;                  // 0x436580 (the body 0x4365a0: the base's)
	void Notify(ObjBase* theObj) override; // 0x4365b0
	bool Update() override;             // 0x436950

	static bool IsReady() { return sReady; }       // 0x4396e0
	static bool IsFinished() { return sFinished; } // 0x4396f0
	void Finish();                      // 0x436a20: the pieces that outlive it go, the level's song starts, the intro goes

private:
	void ReleaseLastBugs();             // 0x436ad0
	void BuildWheel();                  // 0x436b20
	void BuildCart();                   // 0x436dd0
	void BuildBananas();                // 0x437250
	void SteerBug(int theNumber, bool theMakeSound); // 0x4373e0: a fly's next flight (original's name unknown)
	void BuildBugs();                   // 0x437a30
	void BuildMan();                    // 0x437d80
	void BuildFlare();                  // 0x438a40
	void BuildTalk1();                  // 0x438d60
	void BuildTalk2();                  // 0x439150
	static void LoadPictures();         // 0x439550
};

} // namespace Sexy
